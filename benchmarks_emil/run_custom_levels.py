#!/usr/bin/env python3
import os
import sys
import subprocess
import argparse
import time # Added for keyboard listener and refresh
import threading # Added for keyboard listener
import termios # Added for keyboard listener
import tty # Added for keyboard listener
from typing import List, Dict, Tuple, Any, Optional
from concurrent.futures import ProcessPoolExecutor, as_completed, CancelledError # Added

from rich.live import Live
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn, TimeElapsedColumn, TimeRemainingColumn
from rich.console import Group, Console
from rich.panel import Panel
from rich.text import Text
from rich.style import Style

# Global shutdown flag
SHUTDOWN_REQUESTED = False
# Store original terminal settings
ORIGINAL_TERMIOS_SETTINGS: Optional[list] = None
MAX_WORKERS_COUNT: Optional[int] = None # Added

def parse_cpp_output(output_str: str) -> Tuple[Dict[str, float], List[str]]:
    lines = output_str.strip().split('\n')
    app_metrics: Dict[str, float] = {}
    solution_lines: List[str] = []
    benchmark_data_lines = [line[1:] for line in lines if line.strip().startswith('#')]
    solution_lines = [line for line in lines if not line.strip().startswith('#') and line.strip()]

    if not benchmark_data_lines:
        return app_metrics, solution_lines
    if len(benchmark_data_lines) < 2:
         raise ValueError("Output must contain at least two benchmark lines (header and data) if any '#' lines are present.")

    header_line = benchmark_data_lines[0]
    data_line = benchmark_data_lines[-1]
    header_parts = [h.strip().lower() for h in header_line.split(',')]
    data_parts = [d.strip() for d in data_line.split(',')]

    if len(header_parts) != len(data_parts):
        raise ValueError(f"Mismatched number of header ({len(header_parts)}) and data ({len(data_parts)}) fields in app output.")
    try:
        for i in range(len(header_parts)):
            app_metrics[header_parts[i]] = float(data_parts[i])
    except (ValueError, IndexError) as e:
        raise ValueError(f"Failed to parse app metrics from data line '{data_line}': {e}") from e
    return app_metrics, solution_lines

DEFAULT_STRATEGIES = [
    # Best A* (Comprehensive SCP)
    "-s astar -hc scp:pdb_all,sic,mds",
    # Best WA* (Comprehensive SCP)
    "-s wastar -hc scp:pdb_all,sic,mds",
    # Best Greedy (Comprehensive SCP)
    "-s greedy -hc scp:pdb_all,sic,mds",
    # A* with Simple Manhattan Distance (Baseline)
    "-s astar -hc mds",
]
CPP_EXECUTABLE_PATH = "../searchclient_cpp/searchclient"
DEFAULT_LEVELS_SUBDIR = "comp"

def run_level_strategy(executable_path: str, level_path: str, strategy_args: str, timeout_seconds: int = 30) -> Dict[str, Any]:
    command = [executable_path] + strategy_args.split()
    result_info = {
        "level_file_name": os.path.basename(level_path), # Changed from "level" for clarity
        "strategy": strategy_args,
        "status": "Unknown",
        "app_metrics": {},
        "solution": [],
        "error_message": None,
        "stdout": "",
        "stderr": ""
    }

    try:
        with open(level_path, 'r') as level_file_handle:
            process = subprocess.run(
                command,
                stdin=level_file_handle,
                capture_output=True,
                text=True,
                timeout=timeout_seconds,
                check=False
            )
        result_info["stdout"] = process.stdout
        result_info["stderr"] = process.stderr

        if process.returncode != 0:
            result_info["status"] = "AppError"
            result_info["error_message"] = (
                f"App failed with code {process.returncode}. "
                f"Stderr: {process.stderr.strip()}"
            )
            if process.stdout:
                 try:
                     _, result_info["solution"] = parse_cpp_output(process.stdout)
                 except ValueError:
                     result_info["solution"] = [line for line in process.stdout.strip().split('\n') if not line.strip().startswith('#') and line.strip()]
        else:
            try:
                metrics, solution = parse_cpp_output(process.stdout)
                result_info["app_metrics"] = metrics
                result_info["solution"] = solution
                result_info["status"] = "Success"
                if not solution and not metrics:
                     result_info["status"] = "Success_NoSolution_NoMetrics"
                elif not solution:
                     result_info["status"] = "Success_NoSolution"
            except ValueError as e:
                result_info["status"] = "AppParseError"
                result_info["error_message"] = f"Failed to parse C++ app output: {e}. Stdout:\n{process.stdout}"
                if process.stdout:
                    result_info["solution"] = [line for line in process.stdout.strip().split('\n') if not line.strip().startswith('#') and line.strip()]

    except subprocess.TimeoutExpired as e:
        result_info["status"] = "AppTimeout"
        result_info["error_message"] = f"Command timed out. Stderr: {e.stderr.strip() if e.stderr else 'N/A'}"
        result_info["stderr"] = e.stderr.strip() if e.stderr else ''
    except Exception as e:
        result_info["status"] = "ScriptException"
        result_info["error_message"] = f"Unexpected error during run: {e}"

    return result_info

# New function to be run by ProcessPoolExecutor for each level
def run_all_strategies_for_level_process(
    executable_path: str, 
    level_full_path: str, 
    level_file_name: str, # For reporting
    strategies_list: List[str], 
    timeout_per_strategy: int
) -> Tuple[str, List[Dict[str, Any]], Optional[str]]: # Returns (level_file_name, list_of_strategy_results, optional_level_error)
    
    if SHUTDOWN_REQUESTED:
        return level_file_name, [], "Skipped due to shutdown request before start."

    level_results: List[Dict[str, Any]] = []
    
    if not os.path.isfile(level_full_path):
        # This error applies to the whole level
        return level_file_name, [], f"Level file not found: {level_full_path}"

    for strategy_args in strategies_list:
        if SHUTDOWN_REQUESTED:
            # If shutdown happens mid-strategies for a level, we report what we have and a note
            # For simplicity, we'll just stop processing more strategies for this level.
            # A more complex handling might add a special error status to remaining strategies.
            break 
        result = run_level_strategy(executable_path, level_full_path, strategy_args, timeout_per_strategy)
        level_results.append(result)
    
    return level_file_name, level_results, None

def keyboard_listener():
    global SHUTDOWN_REQUESTED, ORIGINAL_TERMIOS_SETTINGS
    import select
    fd = sys.stdin.fileno()
    ORIGINAL_TERMIOS_SETTINGS = termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        sys.stderr.write("\nPress 'q' to request shutdown of runs...\n")
        sys.stderr.flush()
        while not SHUTDOWN_REQUESTED:
            if sys.stdin in select.select([sys.stdin], [], [], 0.1)[0]:
                char = sys.stdin.read(1)
                if char.lower() == 'q':
                    sys.stderr.write("\n'q' pressed. Requesting shutdown...\n")
                    sys.stderr.flush()
                    SHUTDOWN_REQUESTED = True
                    break
            time.sleep(0.05)
    except termios.error:
        sys.stderr.write("\nWarning: Not a TTY, 'q' to quit functionality disabled.\n")
        sys.stderr.flush()
    except ImportError:
        sys.stderr.write("\nWarning: 'select' module not available. 'q' to quit functionality might be impaired.\n")
        sys.stderr.flush()
    finally:
        if ORIGINAL_TERMIOS_SETTINGS: # Check if it was set
            termios.tcsetattr(fd, termios.TCSADRAIN, ORIGINAL_TERMIOS_SETTINGS)

def create_overall_progress_bar() -> Progress:
    return Progress(
        SpinnerColumn(), TextColumn("[progress.description]{task.description}"), BarColumn(),
        TextColumn("[progress.percentage]{task.percentage:>3.0f}%"), TimeRemainingColumn(), TimeElapsedColumn(), expand=True
    )

# Adapted from run_benchmarks_hyperfine.py
def _update_active_levels_display(panel_to_update: Panel, 
                                  active_level_futures: Dict[Any, Dict[str, Any]], 
                                  total_levels: int, 
                                  shutting_down: bool):
    num_active = 0
    for ft_key in active_level_futures.keys():
        if not isinstance(ft_key, str): # It's a Future object
            num_active +=1
    
    has_actual_futures = any(not isinstance(k, str) for k in active_level_futures.keys())

    if shutting_down and num_active == 0:
        table_title = f"All Levels Processed - Shutdown Complete"
    elif shutting_down:
        table_title = f"Shutting Down... ({num_active} levels remaining / {total_levels} total)"
    elif not has_actual_futures and total_levels > 0: 
        table_title = f"Preparing to process {total_levels} levels..."
    else:
        table_title = f"Processing Levels ({total_levels - num_active}/{total_levels} done, {num_active} active)"

    new_table = Table(title=table_title, expand=True, show_header=True, show_edge=True, padding=(0,1))
    new_table.add_column("Level File", style="dim cyan", max_width=40, overflow="fold")
    new_table.add_column("Strategies", style="magenta", max_width=50, overflow="fold") # Can show # of strategies
    new_table.add_column("Status", justify="center", style="bold", max_width=15)
    
    for future_key, task_info in active_level_futures.items():
        level_id_display = task_info.get("level_file_name", "Unknown Level")
        strategies_display = task_info.get("strategies_str", "N/A")
        status_text = "Pending..."
        status_style = "grey50"

        if not isinstance(future_key, str): # It's a Future object
            if future_key.running():
                status_text = "Running..."
                status_style = "bold yellow"
            elif future_key.cancelled():
                status_text = "Cancelled"
                status_style = "dark_orange"
            if shutting_down and not future_key.done():
                 if status_text == "Running...": status_text = "Running (SD)"
                 elif status_text == "Pending...": status_text = "Pending (SD)"
        else: 
            status_text = "Queued..."
            status_style = "dim blue"
            if shutting_down: status_text = "Not Started (SD)"

        new_table.add_row(Text(level_id_display), Text(strategies_display), Text(status_text, style=status_style))

    if not active_level_futures and not (shutting_down and num_active == 0):
        if total_levels > 0 : new_table.add_row(Text("Waiting for level runs to start...", overflow="fold"), "", "")
        else: new_table.add_row(Text("No levels to run.", overflow="fold"), "", "")

    renderables_for_panel = []
    if MAX_WORKERS_COUNT is not None:
         info_texts = [Text(f"Parallel Level Runs (Workers): {MAX_WORKERS_COUNT}", style="bold", justify="center"), Text(" ")]
         renderables_for_panel.extend(info_texts)
    renderables_for_panel.append(new_table)
    
    panel_to_update.renderable = Group(*renderables_for_panel)

def main():
    global SHUTDOWN_REQUESTED, ORIGINAL_TERMIOS_SETTINGS, MAX_WORKERS_COUNT
    console = Console()

    listener_thread = None
    if sys.stdin.isatty() and os.name != 'nt':
        try:
            listener_thread = threading.Thread(target=keyboard_listener, daemon=True)
            listener_thread.start()
        except Exception as e:
            console.print(f"[yellow]Warning: Could not start keyboard listener: {e}[/yellow]")
    else:
        console.print("[dim]Info: Not a TTY or unsupported OS for 'q' to quit. Manual Ctrl+C needed.[/dim]", file=sys.stderr)

    parser = argparse.ArgumentParser(description=f"Run C++ search client on .lvl files in parallel.")
    parser.add_argument(
        "levels_subdir", nargs="?", default=DEFAULT_LEVELS_SUBDIR,
        help=f"Subdirectory within '../levels/' (default: {DEFAULT_LEVELS_SUBDIR})."
    )
    parser.add_argument(
        "--strategies", nargs="+", default=DEFAULT_STRATEGIES,
        help="List of strategies. E.g., \"-s bfs\" \"-s astar -hc sic\"."
    )
    parser.add_argument(
        "--executable", default=CPP_EXECUTABLE_PATH,
        help=f"Path to C++ search client (default: {CPP_EXECUTABLE_PATH})."
    )
    parser.add_argument(
        "--levels_base_dir", default="../levels",
        help="Base directory for levels (default: ../levels)."
    )
    parser.add_argument(
        "--timeout", type=int, default=180,
        help="Timeout in seconds per strategy run (default: 180)."
    )
    parser.add_argument(
        "--max_workers", type=int, default=os.cpu_count() or 4, # Default to CPU count or 4
        help="Maximum number of parallel level processes (default: CPU count or 4)."
    )

    args = parser.parse_args()
    MAX_WORKERS_COUNT = args.max_workers
    
    script_dir = os.path.dirname(os.path.abspath(__file__))
    if not os.path.isabs(args.executable):
        args.executable = os.path.join(script_dir, args.executable)
    args.executable = os.path.normpath(args.executable)

    if not os.path.isabs(args.levels_base_dir):
        args.levels_base_dir = os.path.join(script_dir, args.levels_base_dir)
    args.levels_base_dir = os.path.normpath(args.levels_base_dir)

    levels_target_dir = os.path.join(args.levels_base_dir, args.levels_subdir)
    
    if not os.path.isdir(levels_target_dir):
        console.print(f"[bold red]Error: Levels directory not found: {levels_target_dir}[/bold red]")
        sys.exit(1)
    if not os.path.isfile(args.executable):
        console.print(f"[bold red]Error: Executable not found: {args.executable}[/bold red]")
        sys.exit(1)
    if not os.access(args.executable, os.X_OK):
        console.print(f"[yellow]Warning: Executable at {args.executable} is not executable. Trying to chmod...[/yellow]")
        try:
            os.chmod(args.executable, 0o755)
            if not os.access(args.executable, os.X_OK):
                console.print(f"[bold red]Failed to make {args.executable} executable.[/bold red]")
                sys.exit(1)
            console.print(f"[green]Made {args.executable} executable.[/green]")
        except OSError as e:
            console.print(f"[bold red]Could not make {args.executable} executable: {e}[/bold red]")
            sys.exit(1)

    lvl_files_configs = []
    for f_name in sorted([f for f in os.listdir(levels_target_dir) if f.endswith(".lvl") and os.path.isfile(os.path.join(levels_target_dir, f))]):
        lvl_files_configs.append({
            "level_path_full": os.path.join(levels_target_dir, f_name),
            "level_file_name": f_name,
        })

    if not lvl_files_configs:
        console.print(f"[yellow]No .lvl files found in {levels_target_dir}[/yellow]")
        sys.exit(0)

    console.print(f"Found [bold blue]{len(lvl_files_configs)}[/bold blue] .lvl files in [cyan]{levels_target_dir}[/cyan].")
    console.print(f"Using strategies: [magenta]{', '.join(args.strategies)}[/magenta]")
    console.print(f"Using executable: [cyan]{args.executable}[/cyan]")
    console.print(f"Timeout per strategy run: [blue]{args.timeout}s[/blue]")
    console.print(f"Max parallel level workers: [blue]{MAX_WORKERS_COUNT}[/blue]")
    
    overall_progress = create_overall_progress_bar()
    active_levels_panel = Panel(Text("Initializing parallel runs..."), title="Active Level Runs", border_style="blue", expand=True)
    results_log_table = Table(title="Strategy Run Log", show_header=True, show_edge=True, box=None, expand=True)
    results_log_table.add_column("Level", style="dim cyan", max_width=25, overflow="fold")
    results_log_table.add_column("Strategy", style="magenta", max_width=30, overflow="fold")
    results_log_table.add_column("Status", justify="center", style="bold", max_width=15)
    results_log_table.add_column("Metrics", style="green", max_width=40, overflow="fold")
    results_log_table.add_column("Error", style="red", overflow="fold")

    layout = Group(overall_progress, active_levels_panel, results_log_table)
    all_collected_strategy_results = [] # To store all individual strategy results
    future_to_level_map: Dict[Any, Dict[str, Any]] = {} # To map futures back to level info for display

    try:
        with Live(layout, console=console, refresh_per_second=2, vertical_overflow="crop") as live:
            overall_task_id = overall_progress.add_task("[cyan]Overall Levels Progress", total=len(lvl_files_configs))
            
            # Initial display with queued tasks
            temp_initial_display_map = {}
            for idx, lvl_config in enumerate(lvl_files_configs):
                 temp_initial_display_map[f"initial_{idx}"] = {
                     "level_file_name": lvl_config["level_file_name"],
                     "strategies_str": f"{len(args.strategies)} strategies"
                 }
            _update_active_levels_display(active_levels_panel, temp_initial_display_map, len(lvl_files_configs), SHUTDOWN_REQUESTED)
            live.refresh()

            with ProcessPoolExecutor(max_workers=MAX_WORKERS_COUNT) as executor:
                for lvl_conf in lvl_files_configs:
                    if SHUTDOWN_REQUESTED:
                        break
                    future = executor.submit(
                        run_all_strategies_for_level_process,
                        args.executable,
                        lvl_conf["level_path_full"],
                        lvl_conf["level_file_name"],
                        args.strategies, # Pass the list of strategy strings
                        args.timeout
                    )
                    future_to_level_map[future] = {
                        "level_file_name": lvl_conf["level_file_name"],
                        "strategies_str": f"{len(args.strategies)} strategies",
                        "original_config": lvl_conf
                    }
                    _update_active_levels_display(active_levels_panel, future_to_level_map, len(lvl_files_configs), SHUTDOWN_REQUESTED)
                    live.refresh()
                
                for future in as_completed(future_to_level_map):
                    task_info_for_future = future_to_level_map.pop(future) # Remove from active map
                    level_id_completed = task_info_for_future["level_file_name"]
                    
                    try:
                        level_file_name, strategy_results_list, level_error = future.result()
                        
                        if level_error:
                            # Log a single error for the whole level if file not found or skipped early
                            results_log_table.add_row(
                                Text(level_file_name, overflow="fold"), 
                                Text("N/A", overflow="fold"), 
                                Text("❌ LevelError", style=Style(color="red")),
                                Text("N/A", overflow="fold"), 
                                Text(level_error, overflow="fold")
                            )
                        else:
                            all_collected_strategy_results.extend(strategy_results_list)
                            for run_result in strategy_results_list:
                                status_emoji = "❓"
                                status_style = Style()
                                if run_result["status"] == "Success": status_emoji = "✅"; status_style = Style(color="green")
                                elif "Success_NoSolution" in run_result["status"]: status_emoji = "⚠️"; status_style = Style(color="yellow")
                                elif run_result["status"] in ["AppError", "AppTimeout", "AppParseError", "ScriptException"]: status_emoji = "❌"; status_style = Style(color="red")
                                
                                metrics_str = ", ".join([f"{k}: {v}" for k,v in run_result["app_metrics"].items()])
                                error_msg_display = run_result['error_message'] if run_result['error_message'] else ""
                                results_log_table.add_row(
                                    Text(run_result["level_file_name"], overflow="fold"), 
                                    Text(run_result["strategy"], overflow="fold"), 
                                    Text(f"{status_emoji} {run_result['status']}", style=status_style),
                                    Text(metrics_str if metrics_str else "N/A", overflow="fold"), 
                                    Text(error_msg_display, overflow="fold")
                                )
                    except CancelledError:
                        results_log_table.add_row(Text(level_id_completed), Text("N/A"), Text("❌ Cancelled", style=Style(color="dark_orange")), Text("N/A"), Text("Task cancelled by user or shutdown."))
                    except Exception as exc:
                        results_log_table.add_row(Text(level_id_completed), Text("N/A"), Text("❌ FutureException", style=Style(color="red")), Text("N/A"), Text(f"Unhandled exception from process: {exc}"))
                    finally:
                        overall_progress.update(overall_task_id, advance=1)
                        _update_active_levels_display(active_levels_panel, future_to_level_map, len(lvl_files_configs), SHUTDOWN_REQUESTED)
                        live.refresh()

                if SHUTDOWN_REQUESTED:
                     live.console.print("[bold yellow]Run interrupted. Finalizing...[/bold yellow]")
                _update_active_levels_display(active_levels_panel, {}, len(lvl_files_configs), True)
                live.refresh()
                live.console.print("\n[bold underline]Summary of Runs[/bold underline]")

    except KeyboardInterrupt: 
        SHUTDOWN_REQUESTED = True
        console.print("\n[bold red]KeyboardInterrupt (Ctrl+C) caught. Requesting shutdown immediately.[/bold red]")
        # Executor shutdown is handled by its context manager exiting
    finally:
        if listener_thread and listener_thread.is_alive():
            SHUTDOWN_REQUESTED = True # Ensure flag is set for listener thread to see
            listener_thread.join(timeout=1)
        # Restore terminal settings (already in keyboard_listener's finally, but good to have a failsafe)
        if ORIGINAL_TERMIOS_SETTINGS and sys.stdin.isatty() and os.name != 'nt':
            try:
                termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, ORIGINAL_TERMIOS_SETTINGS)
                sys.stderr.write("\nTerminal settings restored.\n")
                sys.stderr.flush()
            except Exception as e:
                sys.stderr.write(f"\nError restoring terminal settings on exit: {e}\n")
                sys.stderr.flush()

    console.print(results_log_table) # Print final table
    console.print("\n" + "=" * 30)
    console.print("[bold blue]Run script finished.[/bold blue]")
    
    successful_runs = sum(1 for r in all_collected_strategy_results if "Success" in r["status"])
    failed_runs = len(all_collected_strategy_results) - successful_runs
    console.print(f"Total strategy runs attempted: [bold]{len(all_collected_strategy_results)}[/bold], Successful: [green]{successful_runs}[/green], Failed/Errors: [red]{failed_runs}[/red]")

    sys.exit(0 if not SHUTDOWN_REQUESTED else 1)

if __name__ == "__main__":
    main() 