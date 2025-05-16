import json
import os
import sys
import datetime
import subprocess
import time # For timing individual benchmarks
from dataclasses import dataclass, field
from concurrent.futures import ProcessPoolExecutor, as_completed, CancelledError
from typing import Dict, Any, List, Tuple, Optional # For type hints

# For keyboard listener
import threading
import termios
import tty

from rich.live import Live
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn, TimeElapsedColumn, TimeRemainingColumn, TaskID
from rich.console import Group, Console
from rich.panel import Panel
from rich.text import Text

# Global shutdown flag
SHUTDOWN_REQUESTED = False
# Store original terminal settings
ORIGINAL_TERMIOS_SETTINGS: Optional[list] = None

# TODO: Add process pool executor from concurrent.futures with tqdm progress bar
# Global variable to store max_workers, set in main
MAX_WORKERS_COUNT: Optional[int] = None

BENCHMARK_CONFIG_FILE = "benchmarks.json"
CPP_EXECUTABLE_PATH = "../searchclient_cpp/searchclient" 
DEFAULT_TIMEOUT_SECONDS = 600 # 5 minutes per case

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

def keyboard_listener():
    global SHUTDOWN_REQUESTED, ORIGINAL_TERMIOS_SETTINGS
    # Need to import select here as it's unix-specific with tty
    # and only relevant if this function is actually called (i.e. is a tty)
    import select

    fd = sys.stdin.fileno()
    ORIGINAL_TERMIOS_SETTINGS = termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        # Print to stderr to avoid interfering with Rich Live display on stdout
        print("\nPress 'q' to request shutdown of benchmarks...", flush=True, file=sys.stderr)
        while not SHUTDOWN_REQUESTED: 
            if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                char = sys.stdin.read(1)
                if char.lower() == 'q':
                    print("\n'q' pressed. Requesting shutdown...", flush=True, file=sys.stderr)
                    SHUTDOWN_REQUESTED = True
                    break
            time.sleep(0.1) 
    except termios.error:
        print("\nWarning: Not a TTY, 'q' to quit functionality disabled.", flush=True, file=sys.stderr)
    except ImportError:
        print("\nWarning: 'select' module not available. 'q' to quit functionality might be impaired on this system.", flush=True, file=sys.stderr)
    finally:
        if ORIGINAL_TERMIOS_SETTINGS:
            termios.tcsetattr(fd, termios.TCSADRAIN, ORIGINAL_TERMIOS_SETTINGS)

def _update_active_tasks_display(panel_to_update: Panel, current_active_tasks: Dict[Any, Dict[str, Any]], all_submitted_count: int, is_initial_call: bool, shutting_down: bool = False):
    title_status = "Submitted" if is_initial_call else ("Shutting Down..." if shutting_down and not current_active_tasks else "Processing")
    if shutting_down and not current_active_tasks and not is_initial_call:
        table_title = f"All Tasks Processed - Shutdown Complete"
    elif is_initial_call:
        table_title = f"Preparing Benchmark Cases ({all_submitted_count})"
    else:
        active_count = len(current_active_tasks)
        table_title = f"{title_status} Benchmark Cases ({active_count}/{all_submitted_count})"
        if shutting_down:
            table_title += " - Shutdown Requested"

    info_texts = []
    if not is_initial_call and MAX_WORKERS_COUNT is not None:
        num_actually_running = 0
        # current_active_tasks is Dict[Future, Dict[str, Any]] when not is_initial_call
        for future_obj in current_active_tasks.keys():
            if future_obj.running():
                num_actually_running += 1
        info_texts.append(Text(f"Tasks Actively Running: {num_actually_running} / {MAX_WORKERS_COUNT}", style="bold", justify="center"))
        info_texts.append(Text(" ")) # Spacer

    new_table = Table(title=table_title, expand=True, show_header=True, show_edge=True, padding=(0,1))
    new_table.add_column("Level", style="dim cyan", max_width=30, overflow="fold")
    new_table.add_column("Strategy", style="magenta", max_width=25, overflow="fold")
    new_table.add_column("Status", justify="center", style="bold")

    if is_initial_call:
        # current_active_tasks is Dict[int, Dict[str, Any]] (index -> task_config)
        status_text = "Queued"
        status_style = "dim blue" # Changed for initial state
        # Sort by original submission order (using index from enumerate)
        # The dict is {i: task_config}, so values are task_configs
        sorted_task_configs = [task_data for task_data in current_active_tasks.values()] 
        # Assuming current_active_tasks was {i: task ...} which implies it's already prepared this way.

        for task_config in sorted_task_configs:
            new_table.add_row(
                Text(os.path.basename(task_config["level_path_relative"])),
                Text(task_config["strategy_args"]),
                Text(status_text, style=status_style)
            )
    else:
        # current_active_tasks is Dict[Future, Dict[str, Any]] (future -> task_config)
        tasks_to_display = []
        for future_obj, task_cfg in current_active_tasks.items():
            tasks_to_display.append((future_obj, task_cfg))

        # Sort by task ID for consistent display order
        tasks_to_display.sort(key=lambda x: x[1].get('id', ''))

        for future_obj, task_cfg in tasks_to_display:
            current_status_text = ""
            current_status_style = ""

            if future_obj.cancelled():
                current_status_text = "Cancelled"
                current_status_style = "dark_orange"
            elif future_obj.running():
                current_status_text = "Running..."
                current_status_style = "bold yellow" # Changed style
            elif future_obj.done():
                current_status_text = "Completing..." # Task done in worker, pending result sync
                current_status_style = "light_green"
            else: # Not cancelled, not running, not done => Pending in queue
                current_status_text = "Pending..."
                current_status_style = "grey50"
            
            if shutting_down and not future_obj.done():
                if current_status_text == "Running...":
                    current_status_text = "Running (SD)"
                    current_status_style = "orange_red1"
                elif current_status_text == "Pending...":
                    current_status_text = "Pending (SD)"
                    # Keep grey50 or change?
                # If "Cancelled" or "Completing...", no (SD) needed as they are already in a terminal state for this future

            new_table.add_row(
                Text(os.path.basename(task_cfg["level_path_relative"])),
                Text(task_cfg["strategy_args"]),
                Text(current_status_text, style=current_status_style)
            )
            
    renderables_for_panel = []
    if info_texts:
        renderables_for_panel.extend(info_texts)
    renderables_for_panel.append(new_table)
    
    panel_to_update.renderable = Group(*renderables_for_panel)


def main():
    global SHUTDOWN_REQUESTED, ORIGINAL_TERMIOS_SETTINGS, MAX_WORKERS_COUNT

    listener_thread = None
    if sys.stdin.isatty():
        listener_thread = threading.Thread(target=keyboard_listener, daemon=True)
        listener_thread.start()
    else:
        # Print to stderr as this is before Rich console might be fully set up for Live
        print("Info: Not a TTY, 'q' to quit functionality disabled.", flush=True, file=sys.stderr)

    config = load_benchmark_config()
    if config is None:
        return

    levels_dir = config["levels_dir"]
    output_dir = config["output_dir"]
    cases_config = config["cases"]
    # Allow timeout to be configurable in benchmarks.json in the future, for now use global default
    case_timeout_seconds = config.get("timeout_seconds_per_case", DEFAULT_TIMEOUT_SECONDS)

    MAX_WORKERS_COUNT = os.cpu_count() or 4 # Default to 4 if os.cpu_count() is None

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
    active_tasks_panel = Panel(Text("Initializing..."), title="Active Benchmark Cases", border_style="blue", expand=True)
    
    layout = Group(overall_progress, active_tasks_panel)

    console = Console() # Create a console for printing messages after Live exits

    # Store futures to be able to cancel them
    future_to_task_map: Dict[Any, Dict[str, Any]] = {}

    try:
        # Reverted screen and transient, kept vertical_overflow and console
        with Live(layout, console=console, refresh_per_second=4, vertical_overflow="visible", screen=True, transient=True) as live:
            task_id_progress = overall_progress.add_task("[cyan]Overall Progress", total=len(tasks_to_run_configs))
            
            # Prepare initial display for active tasks
            _update_active_tasks_display(active_tasks_panel, \
                                         {i: task for i, task in enumerate(tasks_to_run_configs)}, \
                                         len(tasks_to_run_configs), \
                                         is_initial_call=True)
            live.refresh() # Initial display before starting pool

            with ProcessPoolExecutor(max_workers=MAX_WORKERS_COUNT) as executor:
                active_tasks_runtime_map: Dict[Any, Dict[str, Any]] = {} # future -> task_config

                for task_config in tasks_to_run_configs:
                    if SHUTDOWN_REQUESTED:
                        live.console.print("[yellow]Shutdown requested. No more tasks will be submitted.[/yellow]")
                        break 
                    future = executor.submit(run_single_benchmark_case_process, CPP_EXECUTABLE_PATH, task_config["level_path_full"], task_config["strategy_args"], case_timeout_seconds)
                    future_to_task_map[future] = task_config
                    active_tasks_runtime_map[future] = task_config # Add to runtime map

                # Update active tasks display to "Running" for those submitted
                # This will initially show all submitted tasks as "Pending..." or "Running..."
                # We don't have individual future status from the pool directly here
                _update_active_tasks_display(active_tasks_panel, active_tasks_runtime_map, len(tasks_to_run_configs), is_initial_call=False)
                live.refresh()

                for future in as_completed(future_to_task_map):
                    if SHUTDOWN_REQUESTED and not future.done(): # Check if we need to cancel
                        if future.cancel():
                             live.console.print(f"[yellow]Cancelled task: {future_to_task_map[future]['id']}[/yellow]")
                        # If cancel() returns False, it might be too late (running or finished)
                    
                    task_config = future_to_task_map[future]
                    level_name = os.path.basename(task_config["level_path_relative"])
                    strategy_name = task_config["strategy_args"]
                    
                    # Remove from active tasks map as it's completing
                    if future in active_tasks_runtime_map:
                        del active_tasks_runtime_map[future]

                    try:
                        status, data, duration = future.result()
                        
                        case_status = "Unknown"
                        error_msg = None
                        metrics_data = {}
                        solution_data = []

                        if status == "success" and data is not None:
                            try:
                                metrics_data = parse_cpp_output(data)
                                # Assuming solution lines do not start with '#'
                                solution_data = [line for line in data.strip().split('\n') if not line.strip().startswith('#')]
                                case_status = "Success"
                                live.console.print(f"[green]Completed:[/green] Level: {level_name}, Strategy: {strategy_name}, Duration: {duration:.2f}s, Status: {case_status}")
                            except ValueError as e:
                                case_status = "ParseError"
                                error_msg = f"Failed to parse output: {e}"
                                live.console.print(f"[red]Parse Error:[/red] Level: {level_name}, Strategy: {strategy_name}, Duration: {duration:.2f}s, Error: {error_msg}")
                        
                        elif status == "timeout":
                            case_status = "Timeout"
                            error_msg = data
                            live.console.print(f"[red]Timeout:[/red] Level: {level_name}, Strategy: {strategy_name}, Duration: {duration:.2f}s")
                        elif status == "cancelled": # This would be if future.cancel() worked and it raised CancelledError
                            case_status = "Cancelled"
                            error_msg = "Task was cancelled due to shutdown request."
                            live.console.print(f"[yellow]Cancelled:[/yellow] Level: {level_name}, Strategy: {strategy_name}")
                        else: # file_not_found, called_process_error, os_error, other_error
                            case_status = status.replace('_', ' ').title() # e.g. "Called Process Error"
                            error_msg = data
                            live.console.print(f"[red]{case_status}:[/red] Level: {level_name}, Strategy: {strategy_name}, Duration: {duration:.2f}s, Error: {error_msg}")

                        all_results.cases.append(CaseResult(
                            level=level_name,
                            strategy=strategy_name,
                            solution=solution_data,
                            metrics=metrics_data,
                            status=case_status,
                            error_message=error_msg,
                            duration_seconds=duration
                        ))
                    except CancelledError:
                        live.console.print(f"[yellow]Task cancelled (as_completed): {task_config['id']}[/yellow]")
                        all_results.cases.append(CaseResult(
                            level=level_name,
                            strategy=strategy_name,
                            status="Cancelled",
                            error_message="Task cancelled due to shutdown request."
                        ))
                    except Exception as exc:
                        live.console.print(f"[bold red]Exception for task {task_config['id']}: {exc}[/bold red]")
                        all_results.cases.append(CaseResult(
                            level=level_name,
                            strategy=strategy_name,
                            status="UnhandledException",
                            error_message=str(exc)
                        ))
                    finally:
                        overall_progress.update(task_id_progress, advance=1)
                        # Update the table of active tasks
                        _update_active_tasks_display(active_tasks_panel, active_tasks_runtime_map, \
                                                     len(tasks_to_run_configs), is_initial_call=False, \
                                                     shutting_down=SHUTDOWN_REQUESTED)
                        live.refresh()
                
                if SHUTDOWN_REQUESTED:
                    live.console.print("[bold yellow]Benchmark run interrupted by user. Shutting down worker processes...[/bold yellow]")
                    # Attempt to cancel any futures that might not have been processed by as_completed yet
                    # This is mostly for futures that were submitted but not yet started or picked by as_completed
                    for fut in future_to_task_map.keys():
                        if not fut.done():
                            fut.cancel()
                
                # Shutdown the executor
                # For Python 3.9+, cancel_futures=True can be used.
                # For older versions, this just waits for running tasks or previously cancelled ones.
                if sys.version_info >= (3, 9):
                    executor.shutdown(wait=True, cancel_futures=True)
                else:
                    executor.shutdown(wait=True) # For older Pythons, doesn't have cancel_futures
                
                live.console.print("[bold green]All benchmark tasks finished or cancelled.[/bold green]")
                _update_active_tasks_display(active_tasks_panel, {}, len(tasks_to_run_configs), is_initial_call=False, shutting_down=True) # Clear active tasks
                live.refresh()


    except KeyboardInterrupt: # Catches Ctrl+C
        console.print("\n[bold red]KeyboardInterrupt caught (Ctrl+C). Shutting down abruptly.[/bold red]")
        SHUTDOWN_REQUESTED = True 
        # Executor shutdown will be attempted by the finally block of the ProcessPoolExecutor context manager
        # or if we add an explicit shutdown here.
        # The 'with ProcessPoolExecutor() as executor:' handles shutdown on exit/exception.
    finally:
        if listener_thread and listener_thread.is_alive():
            SHUTDOWN_REQUESTED = True 
            listener_thread.join(timeout=1)

        if ORIGINAL_TERMIOS_SETTINGS and sys.stdin.isatty():
            termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, ORIGINAL_TERMIOS_SETTINGS)
            # Print to stderr as the Live console is gone
            print("\nTerminal settings restored.", flush=True, file=sys.stderr)
        
        if not all_results.cases:
            console.print("[yellow]No benchmark results were recorded.[/yellow]")
        else:
            save_benchmark_results(all_results, output_dir)

    console.print("[bold blue]Benchmark script finished.[/bold blue]")


if __name__ == "__main__":
    main()

