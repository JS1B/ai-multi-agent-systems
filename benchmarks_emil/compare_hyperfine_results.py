#!/usr/bin/env python3
import json
import argparse
import os
import sys
from dataclasses import dataclass, field
from typing import Dict, Any, List, Tuple, Optional, Union
from collections import defaultdict
import datetime # Added for timestamp parsing
import re # Added for filename parsing

from rich.console import Console
from rich.table import Table
from rich.text import Text

# Key for the app_metric that represents the number of steps/solution length
# This will be lowercased by the parser in run_benchmarks_hyperfine.py
# if the C++ header is "SolutionLength"
APP_METRIC_FOR_STEPS = "solutionlength"

# --- Dataclass definitions (copied from run_benchmarks_hyperfine.py for compatibility) ---
@dataclass
class CaseResultHyperfine:
    level: str
    strategy: str
    solution: List[str] = field(default_factory=list)
    app_metrics: Dict[str, float] = field(default_factory=dict)
    hyperfine_results: Dict[str, Any] = field(default_factory=dict) # Parsed JSON from hyperfine
    status: str = "Unknown"
    error_message: Optional[str] = None
    direct_run_duration_seconds: Optional[float] = None

    # Helper to create from dict, as it might be loaded from JSON
    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'CaseResultHyperfine':
        # Handle cases where hyperfine_results might be missing or None
        hf_results = data.get('hyperfine_results')
        if hf_results is None:
            hf_results = {}
        
        return cls(
            level=data.get('level', 'UnknownLevel'),
            strategy=data.get('strategy', 'UnknownStrategy'),
            solution=data.get('solution', []),
            app_metrics=data.get('app_metrics', {}),
            hyperfine_results=hf_results,
            status=data.get('status', 'Unknown'),
            error_message=data.get('error_message'),
            direct_run_duration_seconds=data.get('direct_run_duration_seconds')
        )

@dataclass
class BenchmarkResultHyperfine:
    timestamp: str
    hyperfine_version: Optional[str]
    hyperfine_options_used: Dict[str, Union[str, int, float, None]]
    level_run_info: List[Dict[str, Any]] = field(default_factory=list)
    cases: List[CaseResultHyperfine] = field(default_factory=list)

    @classmethod
    def from_dict(cls, data: Dict[str, Any]) -> 'BenchmarkResultHyperfine':
        return cls(
            timestamp=data.get('timestamp', 'N/A'),
            hyperfine_version=data.get('hyperfine_version'),
            hyperfine_options_used=data.get('hyperfine_options_used', {}),
            level_run_info=data.get('level_run_info', []),
            cases=[CaseResultHyperfine.from_dict(c) for c in data.get('cases', [])]
        )
# --- End Dataclass definitions ---

def find_latest_benchmark_file(directory_path: str, console: Console) -> Optional[str]:
    """Finds the most recent benchmark JSON file in a directory based on filename pattern."""
    latest_file_path = None
    
    filename_pattern = re.compile(r"hyperfine_run_(\d{8}_\d{6})\.json")

    try:
        if not os.path.isdir(directory_path):
            console.print(f"[bold red]Error: Directory not found: {directory_path}[/bold red]")
            return None

        console.print(f"[blue]Searching for latest benchmark file in: {os.path.abspath(directory_path)}[/blue]")
        found_files_with_timestamps: List[Tuple[datetime.datetime, str]] = []
        
        for filename in os.listdir(directory_path):
            match = filename_pattern.fullmatch(filename)
            if match:
                timestamp_str = match.group(1)
                try:
                    timestamp = datetime.datetime.strptime(timestamp_str, "%Y%m%d_%H%M%S")
                    full_path = os.path.join(directory_path, filename)
                    found_files_with_timestamps.append((timestamp, full_path))
                except ValueError:
                    console.print(f"[yellow]Warning: Found file '{filename}' with invalid timestamp format. Skipping.[/yellow]")
        
        if not found_files_with_timestamps:
            console.print(f"[yellow]No benchmark files matching pattern 'hyperfine_run_YYYYMMDD_HHMMSS.json' found in {directory_path}.[/yellow]")
            return None

        # Sort by timestamp, newest first
        found_files_with_timestamps.sort(key=lambda x: x[0], reverse=True)
        
        latest_file_path = found_files_with_timestamps[0][1]
        console.print(f"[green]Found latest benchmark file: {os.path.basename(latest_file_path)} (Timestamp: {found_files_with_timestamps[0][0].strftime('%Y-%m-%d %H:%M:%S')})[/green]")
        return latest_file_path

    except OSError as e:
        console.print(f"[bold red]Error accessing directory {directory_path}: {e}[/bold red]")
    except Exception as e:
        console.print(f"[bold red]An unexpected error occurred while searching for latest file: {e}[/bold red]")
    return None

def load_benchmark_file(file_path: str, console: Console) -> Optional[BenchmarkResultHyperfine]:
    """Loads a single benchmark JSON file."""
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
        return BenchmarkResultHyperfine.from_dict(data)
    except FileNotFoundError:
        console.print(f"[bold red]Error: File not found: {file_path}[/bold red]")
    except json.JSONDecodeError:
        console.print(f"[bold red]Error: Could not parse JSON from {file_path}[/bold red]")
    except Exception as e:
        console.print(f"[bold red]Error loading {file_path}: {e}[/bold red]")
    return None

def display_single_file_results(benchmark_data: BenchmarkResultHyperfine, console: Console, file_name: str):
    """Displays results from a single benchmark file."""
    console.print(f"[bold cyan]Benchmark Results from: {file_name}[/bold cyan]")
    console.print(f"Timestamp: {benchmark_data.timestamp}, Hyperfine Version: {benchmark_data.hyperfine_version or 'N/A'}")

    # Group cases by level
    results_by_level: Dict[str, List[CaseResultHyperfine]] = defaultdict(list)
    for case in benchmark_data.cases:
        results_by_level[case.level].append(case)

    if not results_by_level:
        console.print("[yellow]No benchmark cases found in this file.[/yellow]")
        return

    for level_name, cases_in_level in sorted(results_by_level.items()):
        table = Table(title=f"Level: {level_name}", show_lines=True)
        table.add_column("Strategy", style="dim cyan", overflow="fold")
        table.add_column("Status", style="yellow")
        table.add_column(f"Steps ({APP_METRIC_FOR_STEPS})", justify="right")
        table.add_column("Mean (s)", justify="right")
        table.add_column("Median (s)", justify="right")
        table.add_column("StdDev (s)", justify="right")
        table.add_column("Min (s)", justify="right")
        table.add_column("Max (s)", justify="right")

        cases_in_level.sort(key=lambda c: c.strategy)

        if not cases_in_level:
            console.print(Text(f"Level: {level_name} - No cases found.", style="yellow"))
            continue

        successful_runs_exist = any("Success" in case.status for case in cases_in_level if case.hyperfine_results)
        all_cases_problematic = all("Error" in case.status or "Timeout" in case.status or "Fail" in case.status or not successful_runs_exist for case in cases_in_level)

        if not successful_runs_exist and all_cases_problematic:
            # Attempt to see if a common timeout status is prevalent
            is_timeout_level = any("Timeout" in case.status for case in cases_in_level)
            if is_timeout_level:
                 console.print(Text(f"Level: {level_name} - No successful results; potential timeouts occurred during benchmark run.", style="bold orange_red1"))
            else:
                 console.print(Text(f"Level: {level_name} - No successful results; errors or no data reported.", style="bold red"))
            continue # Skip table rendering for this level

        best_mean_time = float('inf')
        best_stddev = float('inf')
        best_steps = float('inf')
        winner_mean_time_strats: List[str] = []
        winner_stddev_strats: List[str] = []
        winner_steps_strats: List[str] = []

        for case in cases_in_level:
            hf_res = case.hyperfine_results
            steps_val = case.app_metrics.get(APP_METRIC_FOR_STEPS)
            current_steps = float(steps_val) if steps_val is not None and isinstance(steps_val, (int, float)) else float('inf')
            steps_str = f"{int(steps_val)}" if steps_val is not None and not isinstance(steps_val, str) else str(steps_val) if steps_val is not None else "N/A"

            current_mean_time = float('inf')
            current_stddev = float('inf')

            row_to_add = [
                case.strategy,
                Text("Success", style="green") if "Success" in case.status else Text(case.status, style="yellow" if "Unknown" not in case.status else "dim"),
                steps_str
            ]

            if "Success" in case.status and hf_res and isinstance(hf_res, dict):
                current_mean_time = hf_res.get('mean', float('inf'))
                current_stddev = hf_res.get('stddev', float('inf'))
                row_to_add.extend([
                    f"{current_mean_time:.4f}" if current_mean_time != float('inf') else "N/A",
                    f"{hf_res.get('median', float('nan')):.4f}" if hf_res.get('median') is not None else "N/A",
                    f"{current_stddev:.4f}" if current_stddev != float('inf') else "N/A",
                    f"{hf_res.get('min', float('nan')):.4f}" if hf_res.get('min') is not None else "N/A",
                    f"{hf_res.get('max', float('nan')):.4f}" if hf_res.get('max') is not None else "N/A",
                ])
                if "Success" not in case.status or not hf_res or not isinstance(hf_res, dict):
                     row_to_add[1] = Text(case.status, style="red" if "Error" in case.status or "Fail" in case.status else "yellow")

            else:
                 row_to_add[1] = Text(case.status, style="red" if "Error" in case.status or "Fail" in case.status else "yellow")
                 row_to_add.extend(["N/A"] * 5) # Mean, Median, StdDev, Min, Max
            
            table.add_row(*row_to_add)

            # Update winners - only consider successful runs for min/best values
            if "Success" in case.status and hf_res and isinstance(hf_res, dict):
                if current_mean_time < best_mean_time:
                    best_mean_time = current_mean_time
                    winner_mean_time_strats = [case.strategy]
                elif current_mean_time == best_mean_time:
                    winner_mean_time_strats.append(case.strategy)

                if current_stddev < best_stddev:
                    best_stddev = current_stddev
                    winner_stddev_strats = [case.strategy]
                elif current_stddev == best_stddev:
                    winner_stddev_strats.append(case.strategy)
                
                if current_steps < best_steps:
                    best_steps = current_steps
                    winner_steps_strats = [case.strategy]
                elif current_steps == best_steps:
                    winner_steps_strats.append(case.strategy)
        
        # Add new style footer with separate rows for each metric's best
        if successful_runs_exist: # Only add footer if there was at least one successful run to evaluate
            def format_winner_strategies(strategies: List[str]) -> str:
                if not strategies:
                    return "N/A"
                # Revert to showing all strategies
                return ', '.join(strategies)

            # Footer for Best Mean Time
            if winner_mean_time_strats:
                best_mean_time_str = f"{best_mean_time:.4f}s" if best_mean_time != float('inf') else "N/A"
                mean_winners_text = format_winner_strategies(winner_mean_time_strats)
                table.add_row(
                    Text("Fastest (Mean Time)", style="bold white"), 
                    "", # Status
                    "", # Steps
                    Text(best_mean_time_str, style="bold green"), # Mean
                    Text(mean_winners_text, style="dim green"), # Median (used for strategy list)
                    "", # StdDev
                    "", "" # Min, Max
                )
            
            # Footer for Fewest Steps
            if winner_steps_strats:
                best_steps_str = f"{int(best_steps)}" if best_steps != float('inf') else "N/A"
                steps_winners_text = format_winner_strategies(winner_steps_strats)
                table.add_row(
                    Text("Fewest Steps", style="bold white"),
                    "", # Status
                    Text(best_steps_str, style="bold green"), # Steps
                    "", # Mean
                    Text(steps_winners_text, style="dim green"), # Median (used for strategy list)
                    "", # StdDev
                    "", "" # Min, Max
                )

            # Footer for Lowest StdDev
            if winner_stddev_strats:
                best_stddev_str = f"{best_stddev:.4f}s" if best_stddev != float('inf') else "N/A"
                stddev_winners_text = format_winner_strategies(winner_stddev_strats)
                table.add_row(
                    Text("Lowest StdDev", style="bold white"),
                    "", # Status
                    "", # Steps
                    "", # Mean
                    Text(stddev_winners_text, style="dim green"), # Median (used for strategy list)
                    Text(best_stddev_str, style="bold green"), # StdDev
                    "", "" # Min, Max
                )
        # --- End of new footer logic ---

        console.print(table)

def display_multi_file_comparison(benchmark_files_data: List[Tuple[str, BenchmarkResultHyperfine]], console: Console):
    """Displays a comparison of results from multiple benchmark files."""
    console.print(f"[bold cyan]Comparing {len(benchmark_files_data)} Benchmark Runs[/bold cyan]")

    # Aggregate data: level -> strategy -> list of metrics_per_file
    # metrics_per_file = {"mean": float, "stddev": float, "status": str, "error": str | None}
    aggregated_data: Dict[str, Dict[str, List[Optional[Dict[str, Any]]]]] = defaultdict(lambda: defaultdict(lambda: [None] * len(benchmark_files_data)))

    all_levels = set()
    all_strategies_per_level: Dict[str, set] = defaultdict(set)

    for i, (file_name, bench_data) in enumerate(benchmark_files_data):
        for case in bench_data.cases:
            all_levels.add(case.level)
            all_strategies_per_level[case.level].add(case.strategy)
            
            metrics = None
            if case.hyperfine_results and isinstance(case.hyperfine_results, dict): # Ensure it's a dict
                metrics = {
                    "mean": case.hyperfine_results.get("mean"),
                    "stddev": case.hyperfine_results.get("stddev"),
                    "steps": case.app_metrics.get(APP_METRIC_FOR_STEPS),
                    "status": case.status,
                    "error": case.error_message
                }
            else: # Handle missing or malformed hyperfine_results
                 metrics = {
                    "mean": None,
                    "stddev": None,
                    "steps": case.app_metrics.get(APP_METRIC_FOR_STEPS), # Still try to get steps
                    "status": case.status or "DataMissing",
                    "error": case.error_message or "Hyperfine data missing"
                 }
            aggregated_data[case.level][case.strategy][i] = metrics

    if not aggregated_data:
        console.print("[yellow]No benchmark data found to compare across files.[/yellow]")
        return

    for level_name in sorted(list(all_levels)):
        table = Table(title=f"Level: {level_name} - Comparison", show_lines=True)
        table.add_column("Strategy", style="dim cyan", overflow="fold")

        for i, (file_name, _) in enumerate(benchmark_files_data):
            short_file_name = os.path.basename(file_name)
            table.add_column(f"{short_file_name}\nMean (s)", justify="right")
            table.add_column(f"{short_file_name}\nStdDev (s)", justify="right")
            table.add_column(f"{short_file_name}\nSteps ({APP_METRIC_FOR_STEPS[:6]}...)", justify="right") # Abbreviate long key in header
        
        if len(benchmark_files_data) == 2: # Add diff column if exactly two files
             table.add_column("Mean Diff %\n(F2-F1)/F1", justify="right", style="bold")


        strategies_for_level = sorted(list(all_strategies_per_level[level_name]))
        for strategy_name in strategies_for_level:
            row_data = [strategy_name]
            means = [] # To store mean values for diff calculation
            stddevs = [] # Not used for diff, but for finding best
            step_counts = [] # For finding best

            # First pass: collect all data for the row to find bests
            raw_metrics_for_row: List[Optional[Dict[str, Any]]] = []
            for i in range(len(benchmark_files_data)):
                metrics = aggregated_data[level_name][strategy_name][i]
                raw_metrics_for_row.append(metrics)
                if metrics and "Success" in metrics["status"] and metrics["mean"] is not None:
                    means.append(metrics["mean"])
                    stddevs.append(metrics.get("stddev"))
                    steps_val = metrics.get("steps")
                    step_counts.append(float(steps_val) if steps_val is not None and isinstance(steps_val, (int, float)) else float('inf'))
                else:
                    means.append(None)
                    stddevs.append(None)
                    step_counts.append(float('inf'))
            
            best_mean_idx = -1
            min_mean = float('inf')
            if any(m is not None for m in means):
                valid_means_with_indices = [(m, idx) for idx, m in enumerate(means) if m is not None]
                if valid_means_with_indices:
                    min_mean, best_mean_idx = min(valid_means_with_indices, key=lambda x: x[0])

            best_steps_idx = -1
            min_steps = float('inf')
            if any(s != float('inf') for s in step_counts):
                valid_steps_with_indices = [(s, idx) for idx, s in enumerate(step_counts) if s != float('inf')]
                if valid_steps_with_indices:
                     min_steps, best_steps_idx = min(valid_steps_with_indices, key=lambda x: x[0])

            # Second pass: build row data with highlighting
            for i in range(len(benchmark_files_data)):
                metrics = raw_metrics_for_row[i]
                if metrics and "Success" in metrics["status"] and metrics["mean"] is not None:
                    mean_str = f"{metrics['mean']:.4f}"
                    stddev_str = f"{metrics['stddev']:.4f}" if metrics['stddev'] is not None else "N/A"
                    steps_val = metrics.get("steps")
                    steps_str = f"{int(steps_val)}" if steps_val is not None and not isinstance(steps_val, str) else str(steps_val) if steps_val is not None else "N/A"
                    
                    mean_text = Text(mean_str)
                    steps_text = Text(steps_str)

                    if i == best_mean_idx:
                        mean_text.stylize("bold green")
                    if i == best_steps_idx:
                        steps_text.stylize("bold green")
                        
                    row_data.append(mean_text)
                    row_data.append(stddev_str) # No special styling for stddev for now
                    row_data.append(steps_text)
                elif metrics: # Data exists but not successful or mean is None
                    status_text = Text(metrics['status'], style="yellow")
                    if "Error" in metrics['status'] or "Fail" in metrics['status']:
                        status_text = Text(metrics['status'], style="red")

                    row_data.append(status_text) 
                    row_data.append("") # StdDev placeholder
                    steps_val = metrics.get("steps") # Still try to show steps
                    steps_str = f"{int(steps_val)}" if steps_val is not None and not isinstance(steps_val, str) else str(steps_val) if steps_val is not None else "N/A"
                    row_data.append(steps_str) # Steps for non-success cases
                else: # No data for this file/strategy
                    row_data.append(Text("N/A", style="dim"))
                    row_data.append(Text("N/A", style="dim"))
                    row_data.append(Text("N/A", style="dim")) # For steps
            
            if len(benchmark_files_data) == 2 and means[0] is not None and means[1] is not None and means[0] > 1e-9:
                diff_percent = ((means[1] - means[0]) / means[0]) * 100
                diff_color = "green" if diff_percent < 0 else "red" if diff_percent > 0 else "white"
                row_data.append(Text(f"{diff_percent:+.2f}%", style=diff_color))
            elif len(benchmark_files_data) == 2:
                 row_data.append("N/A")


            table.add_row(*row_data)
        console.print(table)

def main():
    parser = argparse.ArgumentParser(
        description="Compare hyperfine benchmark results from JSON files.\n"
                    "Can also automatically pick the latest result from a specified directory.",
        formatter_class=argparse.RawTextHelpFormatter # Allows newlines in help text
    )
    parser.add_argument(
        "json_files",
        nargs='*', # Changed from '+' to '*' to allow zero if --latest-in is used
        help="Paths to one or more benchmark JSON files to compare.\n"
             "If --latest-in is also used, these files will be added to the comparison."
    )
    parser.add_argument(
        "--latest-in",
        metavar="DIRECTORY",
        help="Directory to search for the latest benchmark file (named like 'hyperfine_run_YYYYMMDD_HHMMSS.json').\\n"
             "The user mentioned 'benchmark_results' as a typical directory for this."
    )
    parser.add_argument(
        "-o", "--output-file",
        metavar="FILE_PATH",
        help="Path to save the comparison output. If not provided, output goes to console."
    )
    args = parser.parse_args()
    # console = Console()

    # if args.output_file:
    #     try:
    #         # Prepare a console that records its output to a file
    #         output_fh = open(args.output_file, "wt", encoding="utf-8")
    #         file_console = Console(file=output_fh, record=True, width=120 if not args.output_file.endswith(".html") else None) # Rich handles HTML width
    #         console_to_use = file_console
    #         # Set a flag to indicate we need to close the file later
    #         # We can't use a with statement here easily because console_to_use needs to be passed around.
    #         # Instead, we'll close it explicitly in a finally block.
    #     except IOError as e:
    #         console.print(f"[bold red]Error opening output file {args.output_file}: {e}[/bold red]")
    #         sys.exit(1)
    # else:
    #     console_to_use = console

    # --- Determine console to use (standard or file-based) ---
    console_to_use: Console
    output_fh = None # File handle, if we're writing to a non-HTML file

    if args.output_file and not args.output_file.endswith(".html"):
        try:
            output_fh = open(args.output_file, "wt", encoding="utf-8")
            console_to_use = Console(file=output_fh, width=120) # For text files
        except IOError as e:
            # Fallback to standard console if file opening fails
            standard_console = Console()
            standard_console.print(f"[bold red]Error opening output file {args.output_file}: {e}[/bold red]")
            standard_console.print("[yellow]Will output to standard console instead.[/yellow]")
            console_to_use = standard_console
    elif args.output_file and args.output_file.endswith(".html"):
        console_to_use = Console(record=True, width=180) # For HTML, record and save later, wider width
    else:
        console_to_use = Console() # Default console
    # --- End console determination ---

    try:
        if args.latest_in:
            latest_file_from_dir = find_latest_benchmark_file(args.latest_in, console_to_use)
            if latest_file_from_dir:
                files_to_process = [latest_file_from_dir]
            else:
                console_to_use.print(f"[bold red]Failed to find a suitable file in '{args.latest_in}' and no other files were specified. Exiting.[/bold red]")
                sys.exit(1)
        else:
            files_to_process = args.json_files

        if not files_to_process:
            console_to_use.print("[bold red]No benchmark files to process after checking all sources. Exiting.[/bold red]")
            sys.exit(1)
        
        console_to_use.print(f"[blue]Files to be processed ({len(files_to_process)}):[/blue]")
        for f_path in files_to_process:
            console_to_use.print(f"  - {os.path.abspath(f_path)}")


        loaded_benchmarks: List[Tuple[str, BenchmarkResultHyperfine]] = []
        for file_path in files_to_process: # Use the consolidated list
            benchmark_data = load_benchmark_file(file_path, console_to_use) # Pass the potentially file-bound console
            if benchmark_data:
                loaded_benchmarks.append((file_path, benchmark_data))

        if not loaded_benchmarks:
            console_to_use.print("[bold red]No benchmark files were successfully loaded. Exiting.[/bold red]")
            sys.exit(1)

        if len(loaded_benchmarks) == 1:
            file_path, benchmark_data = loaded_benchmarks[0]
            display_single_file_results(benchmark_data, console_to_use, os.path.basename(file_path))
        else:
            display_multi_file_comparison(loaded_benchmarks, console_to_use)
        
        if args.output_file and args.output_file.endswith(".html") and isinstance(console_to_use, Console) and console_to_use.record:
            try:
                console_to_use.save_html(args.output_file)
                console_to_use.print(f"[green]Output successfully saved to {args.output_file}[/green]")
            except Exception as e:
                # If saving HTML fails, print error to a temporary standard console
                temp_console = Console()
                temp_console.print(f"[bold red]Error saving HTML to {args.output_file}: {e}[/bold red]")
        elif output_fh:
            output_fh.close()
            # For non-HTML files, print a success message to the standard console if it wasn't the one used for file output
            if console_to_use.file != sys.stdout:
                 standard_console_for_message = Console()
                 standard_console_for_message.print(f"[green]Output successfully saved to {args.output_file}[/green]")

    except Exception as e:
        console_to_use.print(f"[bold red]Error: {e}[/bold red]")
        sys.exit(1)

if __name__ == "__main__":
    main() 