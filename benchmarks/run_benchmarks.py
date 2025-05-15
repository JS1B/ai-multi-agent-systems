import json
import os
import sys
import datetime
import subprocess
import time # For timing individual benchmarks
from dataclasses import dataclass, field
from concurrent.futures import ProcessPoolExecutor, as_completed
from typing import Dict, Any, List, Tuple, Optional # For type hints

from rich.live import Live
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn, TimeElapsedColumn, TimeRemainingColumn, TaskID
from rich.console import Group, Console
from rich.panel import Panel
from rich.text import Text

# TODO: Add process pool executor from concurrent.futures with tqdm progress bar

BENCHMARK_CONFIG_FILE = "benchmarks.json"
CPP_EXECUTABLE_PATH = "../searchclient_cpp/searchclient" 
DEFAULT_TIMEOUT_SECONDS = 300 # 5 minutes per case

BENCHMARK_CONFIG_REQUIRED_KEYS = ["levels_dir", "output_dir", "cases"]

@dataclass
class CaseResult:
    level: str
    strategy: str
    solution: List[str] = field(default_factory=list) # Keep as list, but default to empty
    metrics: Dict[str, float] = field(default_factory=dict)
    status: str = "Unknown" 
    error_message: Optional[str] = None
    duration_seconds: Optional[float] = None # Add duration
    
@dataclass
class BenchmarkResult:
    timestamp: str
    cases: List[CaseResult]
    
    def to_json(self) -> str:
        return json.dumps(self, default=lambda o: o.__dict__, indent=4)


def parse_cpp_output(output_str: str) -> Dict[str, float]:
    lines = output_str.strip().split('\n')
    if len(lines) < 2:
        raise ValueError("Output must contain at least a header and a data line.")
    
    benchmarks_lines = [line[1:] for line in lines if line.strip().startswith('#')]
    if len(benchmarks_lines) < 2:
        raise ValueError("Output must contain at least two benchmark lines.")

    header_line = benchmarks_lines[0]
    data_line = benchmarks_lines[-1]

    header_parts = [h.strip().lower() for h in header_line.split(',')]
    data_parts = [d.strip() for d in data_line.split(',')]

    if len(header_parts) != len(data_parts):
         raise ValueError(f"Mismatched number of header ({len(header_parts)}) and data ({len(data_parts)}) fields.")

    metrics: Dict[str, float] = {}
    try:
        for i in range(len(header_parts)):
             metrics[header_parts[i]] = float(data_parts[i])
    except (ValueError, IndexError) as e:
        # Catch potential errors during data conversion or indexing
        raise ValueError(f"Failed to parse metrics from data line '{data_line}': {e}") from e

    return metrics

def load_benchmark_config()-> Optional[Dict[str, Any]]:
    try:
        with open(BENCHMARK_CONFIG_FILE, 'r') as f:
            config = json.load(f)
    except FileNotFoundError:
        print(f"Error: Benchmark configuration file not found at {BENCHMARK_CONFIG_FILE}", file=sys.stderr)
        return None
    except json.JSONDecodeError:
        print(f"Error: Could not parse JSON from {BENCHMARK_CONFIG_FILE}", file=sys.stderr)
        return None
    
    if not all(key in config for key in BENCHMARK_CONFIG_REQUIRED_KEYS):
        print(f"Error: Missing required keys in {BENCHMARK_CONFIG_FILE}", file=sys.stderr)
        return None
    
    return config

def ensure_output_directory(path: str) -> bool:
    try:
        os.makedirs(path, exist_ok=True)
        return True
    except OSError as e:
        print(f"Error: Could not create output directory {path}: {e}", file=sys.stderr)
        return False
    
def check_executable_exists(path: str) -> bool:
    if not os.path.isfile(path):
        print(f"Error: C++ executable not found at {path}", file=sys.stderr)
        return False
    return True

# Return type: tuple[status_str, data_or_error_msg_str | None, duration_float]
def run_single_benchmark_case_process(executable_path: str, level_path: str, strategy_args_str: str, timeout_seconds: int) -> Tuple[str, Optional[str], float]:
    start_time = time.perf_counter()
    duration = 0.0
    try:
        if not os.path.isfile(level_path):
            duration = time.perf_counter() - start_time
            return "file_not_found", f"Level file not found: {level_path}", duration

        strategy_args = strategy_args_str.split()
        command = [executable_path] + strategy_args

        with open(level_path, 'r') as level_file:
            result = subprocess.run(
                command,
                stdin=level_file,
                capture_output=True,
                text=True,
                check=True,
                timeout=timeout_seconds
            )
        duration = time.perf_counter() - start_time
        return "success", result.stdout, duration
    
    except subprocess.TimeoutExpired as e:
        duration = time.perf_counter() - start_time
        error_message = (
            f"Command '{' '.join(e.cmd)}' timed out after {timeout_seconds} seconds. "
            f"Level: {os.path.basename(level_path)}. Stderr: {e.stderr if e.stderr else 'N/A'}"
        )
        return "timeout", error_message, duration
    
    except subprocess.CalledProcessError as e:
        duration = time.perf_counter() - start_time
        error_message = (
            f"Command '{' '.join(e.cmd)}' failed with code {e.returncode}. "
            f"Level: {os.path.basename(level_path)}. Stderr:\n{e.stderr}"
        )
        return "called_process_error", error_message, duration
    
    except OSError as e:
        duration = time.perf_counter() - start_time
        return "os_error", f"OS Error for level {os.path.basename(level_path)} with args '{strategy_args_str}': {e}", duration
    
    except Exception as e: # Catch any other unexpected error
        duration = time.perf_counter() - start_time
        return "other_error", f"An unexpected error occurred: {str(e)}", duration

def save_benchmark_results(results: BenchmarkResult, output_dir: str) -> bool:
    now_time = datetime.datetime.now()
    timestamp_str = now_time.strftime("%Y%m%d_%H%M%S")
    output_filename = f"run_{timestamp_str}.json"
    output_filepath = os.path.join(output_dir, output_filename)

    try:
        with open(output_filepath, 'w') as f:
            f.write(results.to_json())
        # Print after Live context has exited
        Console().print(f"[green]Benchmark results saved to:[/green] {output_filepath}") 
        return True
    except IOError as e:
        Console().print(f"[red]Error: Could not write results to {output_filepath}:[/red] {e}", file=sys.stderr)
        return False

def create_benchmark_live_progress() -> Progress:
    return Progress(
        SpinnerColumn(), 
        TextColumn("[progress.description]{task.description}"), 
        BarColumn(), 
        TextColumn("[progress.percentage]{task.percentage:>3.0f}%"), 
        TimeRemainingColumn(), 
        TimeElapsedColumn(), 
        expand=True
    )

def _update_active_tasks_display(panel_to_update: Panel, current_active_tasks: Dict[Any, Dict[str, Any]], all_submitted_count: int, is_initial_call: bool):
    title_status = "Submitted" if is_initial_call else "Running"
    table_title = f"{title_status} Benchmark Cases ({len(current_active_tasks)}/{all_submitted_count})"
    new_table = Table(title=table_title, expand=True, show_header=True, show_edge=True, padding=(0,1))
    new_table.add_column("Level", style="dim cyan", max_width=30, overflow="fold")
    new_table.add_column("Strategy", style="magenta", max_width=25, overflow="fold")
    new_table.add_column("Status", justify="center", style="bold")

    status_text = "Pending..." if is_initial_call else "Running..."
    status_style = "grey50" if is_initial_call else "yellow"
    
    sorted_active_tasks = list(current_active_tasks.values()) # Get task_configs
    
    for task_config in sorted_active_tasks:
        new_table.add_row(
            Text(os.path.basename(task_config["level_path_relative"])),
            Text(task_config["strategy_args"]),
            Text(status_text, style=status_style)
        )
    panel_to_update.renderable = new_table


def main():
    config = load_benchmark_config()
    if config is None:
        return

    levels_dir = config["levels_dir"]
    output_dir = config["output_dir"]
    cases_config = config["cases"]
    # Allow timeout to be configurable in benchmarks.json in the future, for now use global default
    case_timeout_seconds = config.get("timeout_seconds_per_case", DEFAULT_TIMEOUT_SECONDS)

    if not ensure_output_directory(output_dir):
        return

    if not check_executable_exists(CPP_EXECUTABLE_PATH):
        return

    now_time = datetime.datetime.now()
    all_results = BenchmarkResult(
        timestamp=now_time.isoformat(),
        cases=[]
    )

    print(f"Starting benchmarks (timeout per case: {case_timeout_seconds}s). Results will be saved in {output_dir}")

    tasks_to_run_configs: List[Dict[str, Any]] = []
    for case_cfg in cases_config:
        level_relative_path = case_cfg.get("input")
        strategy_arg_strings = case_cfg.get("strategies", []) 

        if not level_relative_path:
            print(f"Warning: Skipping case due to missing 'input' level path in config.", file=sys.stderr)
            continue

        level_full_path = os.path.join(levels_dir, level_relative_path)

        if not os.path.isfile(level_full_path):
            print(f"Warning: Skipping level '{level_relative_path}' as file not found at '{level_full_path}'.", file=sys.stderr)
            continue
            
        if not strategy_arg_strings:
             print(f"Warning: Skipping level '{level_relative_path}' as no strategy argument strings are specified.", file=sys.stderr)
             continue

        for strat_args in strategy_arg_strings:
            tasks_to_run_configs.append({
                "level_path_full": level_full_path,
                "level_path_relative": level_relative_path,
                "strategy_args": strat_args,
                "id": f"{os.path.basename(level_relative_path)}-{strat_args}" # Unique ID for the task
            })

    if not tasks_to_run_configs:
        print("No valid benchmark tasks found to run.", file=sys.stderr)
        return

    overall_progress = create_benchmark_live_progress()
    # Create an initial empty table for the panel that _update_active_tasks_display will populate
    initial_active_tasks_table = Table(expand=True, show_header=True, show_edge=True, padding=(0,1))
    initial_active_tasks_table.add_column("Level", style="dim cyan", max_width=30, overflow="fold")
    initial_active_tasks_table.add_column("Strategy", style="magenta", max_width=25, overflow="fold")
    initial_active_tasks_table.add_column("Status", justify="center", style="bold")
    
    progress_panel = Panel(overall_progress, title="[b]Benchmark Progress[/b]", border_style="green")
    active_tasks_panel = Panel(initial_active_tasks_table, title="[b]Active Tasks[/b]", border_style="blue") # Title here might be overridden by table title
    layout = Group(progress_panel, active_tasks_panel)
    
    overall_task_id: Optional[TaskID] = overall_progress.add_task("Overall Progress", total=len(tasks_to_run_configs))

    active_futures: Dict[Any, Dict[str, Any]] = {} # Maps future to task_config

    Console().print(f"[bold green]Starting benchmarks[/bold green] (timeout per case: {case_timeout_seconds}s). Results will be saved in {output_dir}")

    try:
        with Live(layout, refresh_per_second=2, screen=True, transient=True) as live:
            with ProcessPoolExecutor() as executor:
                # Submit all tasks and initially populate the active_tasks_table
                initial_active_tasks_table.rows.clear() # Ensure it's empty before populating
                for task_config in tasks_to_run_configs:
                    future = executor.submit(run_single_benchmark_case_process, CPP_EXECUTABLE_PATH, task_config["level_path_full"], task_config["strategy_args"], case_timeout_seconds)
                    active_futures[future] = task_config
                    initial_active_tasks_table.add_row(
                        Text(os.path.basename(task_config["level_path_relative"])),
                        Text(task_config["strategy_args"]),
                        Text("Pending...", style="grey50")
                    )
                
                # Initial display of all tasks as pending/running
                _update_active_tasks_display(active_tasks_panel, active_futures, len(tasks_to_run_configs), is_initial_call=True)
            
                completed_count = 0
                for future in as_completed(active_futures):
                    task_config = active_futures.pop(future) # Remove from active as it's now completed
                    completed_count += 1
                    level_rel_path = task_config["level_path_relative"]
                    strat_args = task_config["strategy_args"]
                    
                    current_case_result = CaseResult(
                        level=level_rel_path,
                        strategy=strat_args,
                        solution=["WIP"], # TODO: Parse actual solution if needed
                    )
                    raw_output_for_parsing_error = None # Store raw output if parsing fails

                    try:
                        status_code, result_data, duration = future.result()
                        current_case_result.duration_seconds = duration

                        if status_code == "success":
                            try:
                                current_case_result.metrics = parse_cpp_output(result_data)
                                current_case_result.status = "Success"
                                details_text = Text(json.dumps(current_case_result.metrics), style="green")
                            except (ValueError, IndexError) as e_parse: # Parsing errors
                                current_case_result.status = "Failed (Parse Err)"
                                current_case_result.error_message = str(e_parse)
                                raw_output_for_parsing_error = result_data
                                details_text = Text(str(e_parse) + (f"\nOutput: {raw_output_for_parsing_error[:200]}..." if raw_output_for_parsing_error else ""), style="red")
                        elif status_code == "file_not_found":
                            current_case_result.status = "Skipped (File NF)"
                            current_case_result.error_message = result_data
                            details_text = Text(result_data if result_data else "File not found", style="yellow")
                        elif status_code == "timeout":
                            current_case_result.status = "Timeout"
                            current_case_result.error_message = result_data
                            details_text = Text(result_data if result_data else "Timeout occurred", style="orange_red1")
                        elif status_code == "called_process_error":
                            current_case_result.status = "Failed (Exit Code)"
                            current_case_result.error_message = result_data
                            details_text = Text(result_data if result_data else "Subprocess error", style="red")
                        elif status_code == "os_error":
                            current_case_result.status = "Failed (OS Error)"
                            current_case_result.error_message = result_data
                            details_text = Text(result_data if result_data else "OS error", style="red")
                        else: # "other_error" or any unexpected status_code
                            current_case_result.status = f"Failed ({status_code})"
                            current_case_result.error_message = result_data
                            details_text = Text(result_data if result_data else "Unknown error from worker", style="bold red")
                    
                    except Exception as e_future: # For truly unexpected errors from future.result() itself
                        current_case_result.status = "Failed (Future Error)"
                        current_case_result.error_message = str(e_future)
                        details_text = Text(str(e_future), style="bold red")
                        # duration might not be set if future.result() failed catastrophically
                        if current_case_result.duration_seconds is None:
                            current_case_result.duration_seconds = 0.0 # Or some indicator like -1
                    
                    all_results.cases.append(current_case_result)
                    
                    # Add row to table
                    status_style = "green" if current_case_result.status == "Success" else \
                                   "yellow" if "Skipped" in current_case_result.status else \
                                   "orange_red1" if current_case_result.status == "Timeout" else "red"
                    time_str = f"{current_case_result.duration_seconds:.2f}" if current_case_result.duration_seconds is not None else "N/A"
                    
                    results_table = Table(title="Active Benchmark Cases", expand=True)
                    results_table.add_column("Level", style="dim cyan", max_width=30, overflow="fold")
                    results_table.add_column("Strategy", style="magenta", max_width=25, overflow="fold")
                    results_table.add_column("Status", justify="center", style="bold")
                    results_table.add_column("Details", max_width=50, overflow="fold") # Adjusted width

                    results_table.add_row(
                        Text(os.path.basename(level_rel_path)), 
                        Text(strat_args), 
                        Text(current_case_result.status, style=status_style), 
                        details_text
                    )
                    initial_active_tasks_table.renderable = results_table # Update the table in the layout
                    overall_progress.update(overall_task_id, completed=completed_count)
                    # live.update(layout) # Not strictly needed, Live refreshes periodically

                    _update_active_tasks_display(active_tasks_panel, active_futures, len(tasks_to_run_configs), is_initial_call=False)

    except KeyboardInterrupt:
        # Access live console safely
        console_to_print_interrupt = Console()
        if 'live' in locals() and live.is_started:
            console_to_print_interrupt = live.console
        console_to_print_interrupt.print("\n[bold yellow]Benchmark run interrupted by user.[/bold yellow]")
        # Perform a more graceful shutdown of the executor
        # Note: executor.shutdown(wait=False, cancel_futures=True) might be available in newer Python/concurrent.futures
        # but cancel_futures is Python 3.9+. For broader compatibility, just letting it exit.
        # The ProcessPoolExecutor context manager handles shutdown.
    finally:
        # Ensure the Live display is stopped, especially if an unhandled exception occurred inside the Live context
        # This is mostly handled by `transient=True` and the context manager, but can be explicit if needed.
        pass # Live display stops automatically on exiting the 'with' block or if transient=True

    if all_results.cases:
        save_benchmark_results(all_results, output_dir)
    else:
        Console().print("[red]\nNo benchmark results were successfully collected.[/red]", file=sys.stderr)


if __name__ == "__main__":
    main()

