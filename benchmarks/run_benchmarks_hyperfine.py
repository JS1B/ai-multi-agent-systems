#!/usr/bin/env python3
import json
import os
import sys
import datetime
import subprocess
import time # For timing the direct run
import tempfile # For hyperfine's temporary JSON output
from dataclasses import dataclass, field
from concurrent.futures import ProcessPoolExecutor, as_completed, CancelledError
from typing import Dict, Any, List, Tuple, Optional, Union

# For keyboard listener (optional, can be adapted from original)
import threading
import termios
import tty

from rich.live import Live
from rich.table import Table
from rich.progress import Progress, SpinnerColumn, BarColumn, TextColumn, TimeElapsedColumn, TimeRemainingColumn
from rich.console import Group, Console
from rich.panel import Panel
from rich.text import Text

# Global shutdown flag
SHUTDOWN_REQUESTED = False
# Store original terminal settings
ORIGINAL_TERMIOS_SETTINGS: Optional[list] = None
MAX_WORKERS_COUNT: Optional[int] = None

BENCHMARK_CONFIG_FILE = "benchmarks.json"
CPP_EXECUTABLE_PATH = "../searchclient_cpp/searchclient"
DEFAULT_TIMEOUT_SECONDS_DIRECT_RUN = 60 # Timeout for the single direct run to get app metrics
DEFAULT_HYPERFINE_TIMEOUT_SECONDS = 600 # Timeout for the entire hyperfine command
DEFAULT_HYPERFINE_RUNS = 5 # Default runs for hyperfine, can be overridden in config
DEFAULT_HYPERFINE_WARMUP = 1 # Default warmup for hyperfine, can be overridden in config

BENCHMARK_CONFIG_REQUIRED_KEYS = ["levels_dir", "output_dir", "cases"]

@dataclass
class CaseResultHyperfine:
    level: str
    strategy: str
    solution: List[str] = field(default_factory=list)
    app_metrics: Dict[str, float] = field(default_factory=dict) # Metrics from parse_cpp_output
    hyperfine_results: Dict[str, Any] = field(default_factory=dict) # Parsed JSON from hyperfine for this specific strategy command
    status: str = "Unknown"
    error_message: Optional[str] = None
    direct_run_duration_seconds: Optional[float] = None
    # hyperfine_command_executed: Optional[str] = None # This will be per-level now

    def to_dict(self) -> Dict[str, Any]:
        return self.__dict__

@dataclass
class BenchmarkResultHyperfine:
    timestamp: str
    hyperfine_version: Optional[str]
    hyperfine_options_used: Dict[str, Union[str, int, float, None]] # Adjusted type
    level_run_info: List[Dict[str, Any]] = field(default_factory=list) # To store per-level hyperfine command and global errors
    cases: List[CaseResultHyperfine] = field(default_factory=list) # List of results, one per strategy

    def to_json(self) -> str:
        return json.dumps(self, default=lambda o: o.to_dict() if hasattr(o, 'to_dict') else o.__dict__, indent=4)


def parse_cpp_output(output_str: str) -> Tuple[Dict[str, float], List[str]]:
    lines = output_str.strip().split('\n')
    app_metrics: Dict[str, float] = {}
    solution_lines: List[str] = []
    benchmark_data_lines = [line[1:] for line in lines if line.strip().startswith('#')]
    solution_lines = [line for line in lines if not line.strip().startswith('#') and line.strip()]

    if not benchmark_data_lines:
        return app_metrics, solution_lines
    if len(benchmark_data_lines) < 2: # if there's one, it must be header only
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
        print(f"Error: Missing required keys in {BENCHMARK_CONFIG_FILE}: {', '.join(BENCHMARK_CONFIG_REQUIRED_KEYS)}", file=sys.stderr)
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
    if not os.access(path, os.X_OK):
        print(f"Error: C++ executable at {path} is not executable.", file=sys.stderr)
        try:
            os.chmod(path, 0o755)
            print(f"Attempted to make {path} executable. Please re-run if this was the issue.", file=sys.stderr)
            return os.access(path, os.X_OK)
        except OSError as e:
            print(f"Could not make {path} executable: {e}", file=sys.stderr)
            return False
    return True

def get_hyperfine_version() -> Optional[str]:
    try:
        result = subprocess.run(["hyperfine", "--version"], capture_output=True, text=True, check=True)
        return result.stdout.strip()
    except (subprocess.CalledProcessError, FileNotFoundError) as e:
        print(f"Error getting hyperfine version: {e}. Is hyperfine installed and in PATH?", file=sys.stderr)
        return None

# New function to run benchmarks for a single level, encompassing multiple strategies
def run_benchmarks_for_level_process(
    executable_path: str,
    level_path_full: str,
    level_path_relative: str, # For reporting
    strategy_args_list: List[str],
    hyperfine_runs: int,
    hyperfine_warmup: int,
    direct_run_timeout: int,
    hyperfine_cmd_timeout: int,
    hyperfine_common_options: List[str] # e.g. [--show-output]
) -> Tuple[str, Optional[str], List[CaseResultHyperfine]]: # Returns (level_id, hyperfine_command_str, list_of_case_results)

    level_id = os.path.basename(level_path_relative)
    case_results_for_level: List[CaseResultHyperfine] = []
    direct_run_data_map: Dict[str, Dict[str, Any]] = {} # strategy_args -> {metrics, solution, duration, error, status}

    if not os.path.isfile(level_path_full):
        # This error applies to the whole level, so all strategies fail.
        error_msg = f"Level file not found: {level_path_full}"
        for strategy_args in strategy_args_list:
            case_results_for_level.append(CaseResultHyperfine(
                level=level_id, strategy=strategy_args, status="LevelNotFound", error_message=error_msg
            ))
        return level_id, None, case_results_for_level

    # 1. Direct runs for each strategy to get app metrics and solution
    for strategy_args in strategy_args_list:
        current_direct_data: Dict[str, Any] = {"status": "Unknown_DirectRun"}
        direct_command = [executable_path] + strategy_args.split()
        start_time_direct_run = time.perf_counter()
        
        try:
            with open(level_path_full, 'r') as level_file_handle:
                direct_run_process = subprocess.run(
                    direct_command, stdin=level_file_handle, capture_output=True, text=True,
                    timeout=direct_run_timeout, check=False
                )
            current_direct_data["duration"] = time.perf_counter() - start_time_direct_run

            if direct_run_process.returncode != 0:
                current_direct_data["status"] = "AppError_DirectRun"
                current_direct_data["error"] = (
                    f"Direct run failed with code {direct_run_process.returncode}. "
                    f"Stderr: {direct_run_process.stderr.strip()}. Stdout: {direct_run_process.stdout.strip()}"
                )
                if direct_run_process.stdout: # Attempt to parse solution even on error
                    _, current_direct_data["solution"] = parse_cpp_output(direct_run_process.stdout)
            else:
                try:
                    current_direct_data["metrics"], current_direct_data["solution"] = parse_cpp_output(direct_run_process.stdout)
                    current_direct_data["status"] = "Success_DirectRun"
                except ValueError as e:
                    current_direct_data["status"] = "AppParseError_DirectRun"
                    current_direct_data["error"] = f"Failed to parse C++ app output: {e}. Stdout:\\n{direct_run_process.stdout}"
                    if not current_direct_data.get("solution") and direct_run_process.stdout:
                        current_direct_data["solution"] = [line for line in direct_run_process.stdout.strip().split('\n') if not line.strip().startswith('#') and line.strip()]
        
        except subprocess.TimeoutExpired as e:
            current_direct_data["duration"] = time.perf_counter() - start_time_direct_run
            current_direct_data["status"] = "AppTimeout_DirectRun"
            current_direct_data["error"] = f"Direct run command timed out. Stderr: {e.stderr.strip() if e.stderr else 'N/A'}"
        except Exception as e:
            current_direct_data["duration"] = time.perf_counter() - start_time_direct_run
            current_direct_data["status"] = "AppException_DirectRun"
            current_direct_data["error"] = f"Unexpected error during direct run: {e}"
        
        direct_run_data_map[strategy_args] = current_direct_data

    # 2. Hyperfine run for timing metrics (all strategies for this level together)
    hyperfine_final_status_for_level = "HyperfineRun_NotAttempted"
    hyperfine_error_for_level : Optional[str] = None
    hyperfine_command_str_executed : Optional[str] = None

    # Check if any direct run was a critical failure (e.g. timeout) that should prevent hyperfine
    # For now, let's assume we always try hyperfine unless level not found.
    # Specific strategy errors from direct run will be combined with hyperfine results later.

    tmp_json_fd, tmp_json_path = tempfile.mkstemp(suffix=".json", text=True)
    os.close(tmp_json_fd)

    # Construct hyperfine command: hyperfine [options] --export-json <tmp> --input <level> "cmd1" "cmd2" ...
    hyperfine_commands_to_run = [f'"{executable_path} {s}"' for s in strategy_args_list] # Quotes for safety
    
    base_hyperfine_cmd = ["hyperfine"]
    if hyperfine_runs is not None: base_hyperfine_cmd.extend(["--runs", str(hyperfine_runs)])
    if hyperfine_warmup is not None: base_hyperfine_cmd.extend(["--warmup", str(hyperfine_warmup)])

    hyperfine_command_list = base_hyperfine_cmd + hyperfine_common_options + [
        "--export-json", tmp_json_path,
        "--input", level_path_full
    ] + [cmd_str for cmd_str in hyperfine_commands_to_run] # Add individual strategy commands

    hyperfine_command_str_executed = " ".join(hyperfine_command_list)

    hyperfine_parsed_results = None
    try:
        hyperfine_process = subprocess.run(
            hyperfine_command_list, capture_output=True, text=True,
            timeout=hyperfine_cmd_timeout, check=False
        )

        if hyperfine_process.returncode != 0:
            hyperfine_final_status_for_level = "HyperfineError"
            hyperfine_error_for_level = (
                f"Hyperfine command failed with code {hyperfine_process.returncode}.\n"
                f"Stderr: {hyperfine_process.stderr.strip()}\nStdout: {hyperfine_process.stdout.strip()}"
            )
        
        if os.path.exists(tmp_json_path) and os.path.getsize(tmp_json_path) > 0:
            try:
                with open(tmp_json_path, 'r') as hf_json_file:
                    hyperfine_parsed_results = json.load(hf_json_file)
                if not (hyperfine_parsed_results and "results" in hyperfine_parsed_results and hyperfine_parsed_results["results"]):
                    if hyperfine_final_status_for_level == "HyperfineRun_NotAttempted": hyperfine_final_status_for_level = "HyperfineEmptyResults" # if not already error
                    err_msg = "Hyperfine ran but produced empty or malformed JSON results."
                    hyperfine_error_for_level = (hyperfine_error_for_level + "\n---\n" if hyperfine_error_for_level else "") + err_msg
                elif hyperfine_final_status_for_level == "HyperfineRun_NotAttempted": # No prior error and got results
                     hyperfine_final_status_for_level = "HyperfineSuccess"

            except json.JSONDecodeError as e:
                if hyperfine_final_status_for_level == "HyperfineRun_NotAttempted": hyperfine_final_status_for_level = "HyperfineJsonParseError"
                err_msg = f"Failed to parse hyperfine JSON output: {e}"
                hyperfine_error_for_level = (hyperfine_error_for_level + "\n---\n" if hyperfine_error_for_level else "") + err_msg
        elif hyperfine_final_status_for_level == "HyperfineRun_NotAttempted": # No JSON file and no prior error
            hyperfine_final_status_for_level = "HyperfineNoOutput"
            hyperfine_error_for_level = (hyperfine_error_for_level + "\n---\n" if hyperfine_error_for_level else "") + "Hyperfine did not produce an output JSON file."

    except subprocess.TimeoutExpired as e:
        hyperfine_final_status_for_level = "HyperfineTimeout"
        hyperfine_error_for_level = f"Hyperfine command timed out. Stderr: {e.stderr.strip() if e.stderr else 'N/A'}"
    except Exception as e:
        hyperfine_final_status_for_level = "HyperfineException"
        hyperfine_error_for_level = f"Unexpected error during hyperfine run: {e}"
    finally:
        if os.path.exists(tmp_json_path):
            os.remove(tmp_json_path)

    # 3. Combine direct run data with hyperfine data for each strategy
    # And build the CaseResultHyperfine list
    
    # Create a mapping from the command string used in hyperfine to strategy_args
    # Hyperfine command string: f'"{executable_path} {s}"'
    # Strategy args: s
    hyperfine_cmd_to_strat_args: Dict[str, str] = {}
    for s_args in strategy_args_list:
        # Need to reconstruct the exact command string as hyperfine sees it.
        # Hyperfine might see it without the outer quotes if run through shell internally,
        # or with quotes if it parses them. Assuming it's the inner part for now.
        # The `command` field in hyperfine JSON is usually what was passed.
        hyperfine_cmd_to_strat_args[f"{executable_path} {s_args}"] = s_args


    if hyperfine_parsed_results and "results" in hyperfine_parsed_results:
        for hf_result_item in hyperfine_parsed_results["results"]:
            # hf_result_item["command"] is like "/path/to/executable arg1 arg2"
            # We need to map this back to our original strategy_args_str
            original_strategy_args = None
            # Try to match hf_result_item["command"] to one of the constructed command strings
            # This matching needs to be robust if hyperfine modifies the command string.
            # For now, assuming direct match after stripping quotes if any.
            
            processed_hf_command = hf_result_item["command"].strip('\'"') # Strip potential outer quotes

            if processed_hf_command in hyperfine_cmd_to_strat_args:
                original_strategy_args = hyperfine_cmd_to_strat_args[processed_hf_command]
            else: # Fallback: try to extract args part
                if processed_hf_command.startswith(executable_path + " "):
                    original_strategy_args = processed_hf_command[len(executable_path)+1:]
                
            if not original_strategy_args or original_strategy_args not in direct_run_data_map:
                # Could not map hyperfine result back to a strategy, or no direct run data
                # This indicates an issue. Log it or create a partial error result.
                case_results_for_level.append(CaseResultHyperfine(
                    level=level_id, strategy=hf_result_item["command"], # Use raw command from hf
                    status="HyperfineResult_Unmapped",
                    error_message=f"Could not map hyperfine result command '{hf_result_item['command']}' back to an original strategy.",
                    hyperfine_results=hf_result_item
                ))
                continue

            direct_data = direct_run_data_map[original_strategy_args]
            
            final_status = "Unknown"
            combined_error_msg = direct_data.get("error")

            # Determine final status based on direct run and hyperfine status for this strategy
            if "Success" in direct_data["status"]: # Direct run was OK for this strategy
                final_status = "Success" # Base success on direct run
                # If hyperfine itself had issues at the level, it's a general note, not per strategy success
            else: # Direct run had an issue
                final_status = direct_data["status"] # e.g. AppParseError_DirectRun, AppTimeout_DirectRun
            
            # If the overall hyperfine run for the level failed, this strategy might not have timed data
            # The hf_result_item itself is from a successful parse of hyperfine JSON for *this* command
            # So, individual strategy timing should be there if it ran.
            # Add level-wide hyperfine error if it occurred and no specific strategy error.
            if hyperfine_error_for_level and not combined_error_msg :
                 combined_error_msg = f"(Level Hyperfine Issue: {hyperfine_error_for_level})"
            elif hyperfine_error_for_level and combined_error_msg:
                 combined_error_msg += f" (Level Hyperfine Issue: {hyperfine_error_for_level})"


            case_results_for_level.append(CaseResultHyperfine(
                level=level_id,
                strategy=original_strategy_args,
                solution=direct_data.get("solution", []),
                app_metrics=direct_data.get("metrics", {}),
                hyperfine_results=hf_result_item, # This is per-command from hyperfine
                status=final_status,
                error_message=combined_error_msg,
                direct_run_duration_seconds=direct_data.get("duration")
            ))
            # Mark this strategy as processed from hyperfine results
            if original_strategy_args in direct_run_data_map:
                 direct_run_data_map[original_strategy_args]["processed_by_hyperfine"] = True
    
    # Add results for strategies that had direct runs but were not in hyperfine output (e.g. if hyperfine failed before running them)
    for strategy_args, direct_data in direct_run_data_map.items():
        if not direct_data.get("processed_by_hyperfine"):
            error_msg = direct_data.get("error","")
            if hyperfine_error_for_level:
                 error_msg = (error_msg + "\n---\n" if error_msg else "") + f"Hyperfine run for level failed: {hyperfine_error_for_level}"
            if not error_msg: error_msg = "Strategy not found in hyperfine output; Hyperfine run may have failed."
            
            status = direct_data.get("status", "DirectRunOnly_NoHyperfineData")
            if "Success" not in status and hyperfine_error_for_level: # If direct run also failed, and hyperfine failed
                status = direct_data.get("status") + "_And_HyperfineFailed"


            case_results_for_level.append(CaseResultHyperfine(
                level=level_id,
                strategy=strategy_args,
                solution=direct_data.get("solution", []),
                app_metrics=direct_data.get("metrics", {}),
                status=status,
                error_message=error_msg,
                direct_run_duration_seconds=direct_data.get("duration")
            ))

    # If case_results_for_level is empty AND there was a general hyperfine error, create a placeholder
    if not case_results_for_level and hyperfine_error_for_level:
        for strategy_args in strategy_args_list: # Create error entry for each strategy expected
            case_results_for_level.append(CaseResultHyperfine(
                level=level_id, strategy=strategy_args,
                status=hyperfine_final_status_for_level, # e.g. HyperfineError, HyperfineTimeout
                error_message=hyperfine_error_for_level
            ))
            
    level_run_summary = {
        "level_id": level_id,
        "hyperfine_command": hyperfine_command_str_executed,
        "hyperfine_run_status": hyperfine_final_status_for_level,
        "hyperfine_run_error": hyperfine_error_for_level
    }

    return level_id, level_run_summary, case_results_for_level


def save_benchmark_results(results: BenchmarkResultHyperfine, output_dir: str, console_for_print: Console) -> bool:
    now_time = datetime.datetime.now()
    timestamp_str = now_time.strftime("%Y%m%d_%H%M%S")
    output_filename = f"hyperfine_run_{timestamp_str}.json"
    output_filepath = os.path.join(output_dir, output_filename)
    try:
        with open(output_filepath, 'w') as f:
            f.write(results.to_json())
        console_for_print.print(f"[green]Benchmark results saved to:[/green] {output_filepath}")
        return True
    except IOError as e:
        console_for_print.print(f"[red]Error: Could not write results to {output_filepath}:[/red] {e}", file=sys.stderr)
        return False

def create_benchmark_live_progress() -> Progress:
    return Progress(
        SpinnerColumn(), TextColumn("[progress.description]{task.description}"), BarColumn(),
        TextColumn("[progress.percentage]{task.percentage:>3.0f}%"), TimeRemainingColumn(), TimeElapsedColumn(), expand=True
    )

def keyboard_listener():
    global SHUTDOWN_REQUESTED, ORIGINAL_TERMIOS_SETTINGS
    import select 
    fd = sys.stdin.fileno()
    ORIGINAL_TERMIOS_SETTINGS = termios.tcgetattr(fd)
    try:
        tty.setcbreak(fd)
        sys.stderr.write("\nPress 'q' to request shutdown of benchmarks...\n")
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
        if ORIGINAL_TERMIOS_SETTINGS:
            termios.tcsetattr(fd, termios.TCSADRAIN, ORIGINAL_TERMIOS_SETTINGS)

# Updated display function for active level benchmarks
def _update_active_levels_display(panel_to_update: Panel, 
                                  active_level_runs: Dict[Any, Dict[str, Any]], # Future or str -> {level_id, strategies_str}
                                  total_levels: int, 
                                  shutting_down: bool):
    
    num_active = 0
    processed_futures = 0 # Count how many actual futures are being processed

    # Iterate to count actual futures vs initial string placeholders
    for ft_key in active_level_runs.keys():
        if not isinstance(ft_key, str): # It's a Future object
            num_active +=1
        # We can't easily count 'processed' from this map alone if it contains initial placeholders
        # So, we rely on total_levels and current num_active (actual futures)

    num_processed_display = total_levels - num_active # This might be initially misleading if temp_initial_display_map is large
                                                   # Let's adjust title based on whether we have actual futures
    
    has_actual_futures = any(not isinstance(k, str) for k in active_level_runs.keys())

    if shutting_down and num_active == 0:
        table_title = f"All Levels Processed - Shutdown Complete"
    elif shutting_down:
        table_title = f"Shutting Down... ({num_active} levels remaining / {total_levels} total)"
    elif not has_actual_futures and total_levels > 0: # No actual futures submitted yet, but tasks exist
        table_title = f"Preparing to process {total_levels} levels..."
        num_processed_display = 0 # Correct display for this state
    else: # Actively processing or all submitted
        table_title = f"Processing Levels ({total_levels - num_active}/{total_levels} done, {num_active} active)"
        # The 'done' count here is based on total_levels minus currently active *actual* futures.

    new_table = Table(title=table_title, expand=True, show_header=True, show_edge=True, padding=(0,1))
    new_table.add_column("Level ID", style="dim cyan", max_width=35, overflow="fold")
    new_table.add_column("Strategies", style="magenta", max_width=45, overflow="fold")
    new_table.add_column("Status", justify="center", style="bold", max_width=15)
    
    for future_key, task_info in active_level_runs.items():
        level_id_display = task_info.get("level_id", "Unknown Level")
        strategies_display = task_info.get("strategies_str", "N/A")
        
        status_text = "Pending..."
        status_style = "grey50"

        # Check if future_key is an actual Future object before calling methods on it
        if not isinstance(future_key, str): # It's a Future object
            if future_key.running():
                status_text = "Running..."
                status_style = "bold yellow"
            elif future_key.cancelled():
                status_text = "Cancelled"
                status_style = "dark_orange"
            # elif future_key.done(): # Could add this if we want to show "Completed" briefly before removal
            #     status_text = "Done"
            #     status_style = "green"
            
            if shutting_down and not future_key.done():
                 if status_text == "Running...": status_text = "Running (SD)"
                 elif status_text == "Pending...": status_text = "Pending (SD)" # Should not happen if it's a real future
        else: # It's a string key from the initial placeholder map
            status_text = "Queued..." # Or "Initializing..." for the very first display
            status_style = "dim blue" # A different style for queued/initial items
            if shutting_down: # If shutting down and these were never processed
                status_text = "Not Started (SD)"

        new_table.add_row(
            Text(level_id_display),
            Text(strategies_display),
            Text(status_text, style=status_style)
        )

    if not active_level_runs and not (shutting_down and num_active == 0):
        if total_levels > 0 : # if there are tasks but none started
             new_table.add_row(Text("Waiting for level benchmarks to start...", overflow="fold"), "", "")
        else: # if no tasks were even configured
             new_table.add_row(Text("No levels to benchmark.", overflow="fold"), "", "")


    renderables_for_panel = []
    if MAX_WORKERS_COUNT is not None:
         info_texts = [Text(f"Parallel Level Benchmarks (Workers): {MAX_WORKERS_COUNT}", style="bold", justify="center"), Text(" ")]
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
        console.print("Info: Not a TTY or unsupported OS for 'q' to quit. Manual Ctrl+C needed to interrupt.", file=sys.stderr)

    config = load_benchmark_config()
    if config is None: return 1

    levels_dir = config["levels_dir"]
    output_dir = config["output_dir"]
    cases_config = config["cases"] # This is a list of case configs
    
    hyperfine_runs_config = config.get("hyperfine_runs", DEFAULT_HYPERFINE_RUNS)
    hyperfine_warmup_config = config.get("hyperfine_warmup", DEFAULT_HYPERFINE_WARMUP)
    direct_run_timeout_config = config.get("direct_run_timeout_seconds", DEFAULT_TIMEOUT_SECONDS_DIRECT_RUN)
    hyperfine_cmd_timeout_config = config.get("hyperfine_command_timeout_seconds", DEFAULT_HYPERFINE_TIMEOUT_SECONDS)
    # Get common hyperfine options if specified (e.g. --show-output)
    hyperfine_global_options_str = config.get("hyperfine_common_options", "") # e.g. "--show-output --ignore-failure"
    hyperfine_common_options_list = hyperfine_global_options_str.split()


    MAX_WORKERS_COUNT = config.get("max_workers", os.cpu_count() or 4)

    if not ensure_output_directory(output_dir): return 1
    if not check_executable_exists(CPP_EXECUTABLE_PATH): return 1
    
    hyperfine_version_str = get_hyperfine_version()
    if hyperfine_version_str is None:
        console.print("[red]Hyperfine not found or version could not be determined. Please install hyperfine.[/red]")
        return 1
    console.print(f"Using: {hyperfine_version_str}")

    now_time = datetime.datetime.now()
    all_results_collector = BenchmarkResultHyperfine(
        timestamp=now_time.isoformat(),
        hyperfine_version=hyperfine_version_str,
        hyperfine_options_used={
            "runs": hyperfine_runs_config, "warmup": hyperfine_warmup_config,
            "direct_run_timeout": direct_run_timeout_config,
            "hyperfine_cmd_timeout": hyperfine_cmd_timeout_config,
            "common_options_str": hyperfine_global_options_str
        },
        cases=[],
        level_run_info=[]
    )

    # Prepare tasks: group strategies by level
    level_tasks_map: Dict[str, Dict[str, Any]] = {} # level_relative_path -> {full_path, strategies_list}
    for case_cfg in cases_config:
        level_relative = case_cfg.get("input")
        strategy_arg_strings_for_case = case_cfg.get("strategies", [])

        if not level_relative or not strategy_arg_strings_for_case:
            console.print(f"[yellow]Warning: Skipping case due to missing 'input' or 'strategies' in config: {case_cfg}[/yellow]", file=sys.stderr)
            continue
        
        level_full = os.path.join(levels_dir, level_relative)
        if not os.path.isfile(level_full):
            console.print(f"[yellow]Warning: Level file not found for '{level_relative}', skipping.[/yellow]", file=sys.stderr)
            continue

        if level_relative not in level_tasks_map:
            level_tasks_map[level_relative] = {
                "level_path_full": level_full,
                "level_path_relative": level_relative,
                "strategies": []
            }
        level_tasks_map[level_relative]["strategies"].extend(strategy_arg_strings_for_case)

    # Remove duplicates from strategies list for each level
    tasks_to_run_configs_final: List[Dict[str, Any]] = []
    for level_rel_path, data in level_tasks_map.items():
        unique_strategies = sorted(list(set(data["strategies"]))) # Sort for consistent hyperfine command order
        if not unique_strategies:
            console.print(f"[yellow]Warning: No strategies for level '{level_rel_path}' after processing. Skipping.[/yellow]", file=sys.stderr)
            continue
        tasks_to_run_configs_final.append({
            "level_path_full": data["level_path_full"],
            "level_path_relative": level_rel_path,
            "strategy_args_list": unique_strategies,
            "id": level_rel_path # Use level path as unique ID for the level task
        })

    if not tasks_to_run_configs_final:
        console.print("[red]No valid benchmark tasks found to run after grouping by level.[/red]", file=sys.stderr)
        return 1

    console.print(f"Starting hyperfine benchmarks for {len(tasks_to_run_configs_final)} levels. Results will be saved in {output_dir}")
    console.print(f"Hyperfine options: runs={hyperfine_runs_config}, warmup={hyperfine_warmup_config}, common='{hyperfine_global_options_str}'")
    console.print(f"Max parallel level benchmarks: {MAX_WORKERS_COUNT}")

    overall_progress = create_benchmark_live_progress()
    active_levels_panel = Panel(Text("Initializing level benchmarks..."), title="Active Level Benchmarks", border_style="blue", expand=True)
    layout = Group(overall_progress, active_levels_panel)
    
    # For tracking active futures and their associated level info for the display
    future_to_level_task_map: Dict[Any, Dict[str, Any]] = {} 

    try:
        with Live(layout, console=console, refresh_per_second=2, vertical_overflow="visible") as live:
            progress_task_id = overall_progress.add_task("[cyan]Overall Progress (Levels)", total=len(tasks_to_run_configs_final))
            
            # Initial display update - show all tasks as pending initially
            temp_initial_display_map = {}
            for idx, task_config in enumerate(tasks_to_run_configs_final):
                 # Create a dummy future object or use a unique ID for initial display
                 # This is just to populate the display before actual futures exist.
                 temp_initial_display_map[f"initial_{idx}"] = {
                     "level_id": os.path.basename(task_config["level_path_relative"]),
                     "strategies_str": ", ".join(task_config["strategy_args_list"])
                 }
            _update_active_levels_display(active_levels_panel, temp_initial_display_map, len(tasks_to_run_configs_final), SHUTDOWN_REQUESTED)
            live.refresh()


            with ProcessPoolExecutor(max_workers=MAX_WORKERS_COUNT) as executor:
                for task_config in tasks_to_run_configs_final:
                    if SHUTDOWN_REQUESTED:
                        # live.console.print("[yellow]Shutdown requested. No more level benchmarks will be submitted.[/yellow]")
                        # Consider adding this message to a status area within the Live display if essential.
                        break
                    
                    future = executor.submit(
                        run_benchmarks_for_level_process, # New function
                        CPP_EXECUTABLE_PATH,
                        task_config["level_path_full"],
                        task_config["level_path_relative"],
                        task_config["strategy_args_list"],
                        hyperfine_runs_config,
                        hyperfine_warmup_config,
                        direct_run_timeout_config,
                        hyperfine_cmd_timeout_config,
                        hyperfine_common_options_list
                    )
                    future_to_level_task_map[future] = {
                        "level_id": os.path.basename(task_config["level_path_relative"]),
                        "strategies_str": ", ".join(task_config["strategy_args_list"]),
                        "original_config": task_config # Keep original for logging if needed
                    }
                    # Update display as tasks are submitted
                    _update_active_levels_display(active_levels_panel, future_to_level_task_map, len(tasks_to_run_configs_final), SHUTDOWN_REQUESTED)
                    live.refresh()

                for future in as_completed(future_to_level_task_map):
                    task_info_for_future = future_to_level_task_map.pop(future) # Remove from active map
                    level_id_completed = task_info_for_future["level_id"]
                    
                    try:
                        # Function now returns: level_id, level_run_summary_dict, List[CaseResultHyperfine]
                        level_id_from_worker, level_run_summary_dict, strategy_results_list = future.result()
                        
                        all_results_collector.level_run_info.append(level_run_summary_dict)
                        all_results_collector.cases.extend(strategy_results_list)
                        
                        # Log summary for the completed level
                        status_color = "green"
                        final_level_status = level_run_summary_dict.get("hyperfine_run_status", "Unknown")
                        if "Error" in final_level_status or "Timeout" in final_level_status or "Fail" in final_level_status or "Exception" in final_level_status or "NotFound" in final_level_status :
                            status_color = "red"
                        elif "Unknown" in final_level_status or "EmptyResults" in final_level_status or "NoOutput" in final_level_status:
                            status_color = "yellow"
                        
                        log_msg = (
                            f"[{status_color}]Level Completed:[/{status_color}] ID: {level_id_completed}, "
                            f"Hyperfine Status: {final_level_status}, Strategies Processed: {len(strategy_results_list)}"
                        )
                        if level_run_summary_dict.get("hyperfine_run_error"):
                            log_msg += f" (Global Hyperfine Error for Level)"
                        # Individual strategy errors are in CaseResultHyperfine objects and would be too verbose here.

                    except CancelledError:
                        # live.console.print(f"[yellow]Level benchmark {level_id_completed} was cancelled.[/yellow]")
                        # Log to a file or incorporate into a less disruptive part of the UI if needed.
                        all_results_collector.level_run_info.append({
                            "level_id": level_id_completed, "hyperfine_command": "N/A - Cancelled",
                            "hyperfine_run_status": "Cancelled", 
                            "hyperfine_run_error": "Task cancelled by user or shutdown."})
                    except Exception as exc:
                        # live.console.print(f"[bold red]Exception for level benchmark {level_id_completed} during future.result(): {exc}[/bold red]")
                        # Log to a file or incorporate into a less disruptive part of the UI if needed.
                        all_results_collector.level_run_info.append({
                            "level_id": level_id_completed, "hyperfine_command": "N/A - Exception",
                            "hyperfine_run_status": "FutureException", 
                            "hyperfine_run_error": f"Unhandled exception processing future: {str(exc)}"})
                    finally:
                        overall_progress.update(progress_task_id, advance=1)
                        _update_active_levels_display(active_levels_panel, future_to_level_task_map, len(tasks_to_run_configs_final), SHUTDOWN_REQUESTED)
                        live.refresh() # Refresh after each completion

                if SHUTDOWN_REQUESTED:
                     live.console.print("[bold yellow]Benchmark run interrupted. Finalizing...[/bold yellow]")
                
                _update_active_levels_display(active_levels_panel, {}, len(tasks_to_run_configs_final), True)
                live.refresh()

    except KeyboardInterrupt: 
        SHUTDOWN_REQUESTED = True
        console.print("\n[bold red]KeyboardInterrupt (Ctrl+C) caught. Requesting shutdown immediately.[/bold red]")
    finally:
        if listener_thread and listener_thread.is_alive():
            SHUTDOWN_REQUESTED = True 
            listener_thread.join(timeout=1) 
        if ORIGINAL_TERMIOS_SETTINGS and sys.stdin.isatty() and os.name != 'nt':
            try:
                termios.tcsetattr(sys.stdin.fileno(), termios.TCSADRAIN, ORIGINAL_TERMIOS_SETTINGS)
                sys.stderr.write("\nTerminal settings restored.\n")
                sys.stderr.flush()
            except Exception as e:
                sys.stderr.write(f"\nError restoring terminal settings: {e}\n")
                sys.stderr.flush()

        if not all_results_collector.cases and not all_results_collector.level_run_info :
            console.print("[yellow]No benchmark results were recorded.[/yellow]")
        else:
            all_results_collector.cases.sort(key=lambda c: (c.level, c.strategy))
            all_results_collector.level_run_info.sort(key=lambda lri: lri.get("level_id", ""))
            save_benchmark_results(all_results_collector, output_dir, console)

    console.print("[bold blue]Hyperfine benchmark script finished.[/bold blue]")
    return 0 if not SHUTDOWN_REQUESTED else 1

if __name__ == "__main__":
    exit_code = main()
    sys.exit(exit_code)
