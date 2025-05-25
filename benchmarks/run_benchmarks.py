import json
import os
import sys
import datetime
import subprocess
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass
from enum import Enum
from tqdm import tqdm

try:
    from psutil import cpu_count
    WORKERS_COUNT = max(cpu_count(logical=False) - 1, 1)
except ImportError:
    WORKERS_COUNT = max(os.cpu_count() - 1, 1)
    print("Warning: psutil is not installed. Using os.cpu_count() instead (threads, not cores).", file=sys.stderr)


JAVA = "java"
SERVER_JAR = "../server.jar"
CLIENT_EXECUTABLE_PATH = "../searchclient_cpp/searchclient" 

BENCHMARK_CONFIG_FILE = "benchmarks.json"
BENCHMARK_CONFIG_REQUIRED_KEYS = ["levels_dir", "output_dir", "cases", "strategies", "skip_best_found_strategy"]

class bcolors(Enum):
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

    @staticmethod
    def colorize(text: str, color: 'bcolors') -> str:
        return f"{color.value}{text}{bcolors.ENDC.value}"

@dataclass
class CaseResult:
    level: str
    strategy: str
    solution_length: int
    metrics: dict[str, float]
    
@dataclass
class BenchmarkResult:
    timestamp: str
    cases: list[CaseResult]
    
    def to_json(self) -> str:
        return json.dumps(self, default=lambda o: o.__dict__, indent=4)


def parse_server_output(output_str: str) -> tuple[dict, int]:
    lines = output_str.strip().split('\n')
    if len(lines) < 2:
        raise ValueError("Output must contain at least a header and a data line.")
    
    COMMENTS_PREFIX = "[client][message]"
    benchmarks_lines = [line[len(COMMENTS_PREFIX):] for line in lines if line.strip().startswith(COMMENTS_PREFIX)]
    if len(benchmarks_lines) < 2:
        raise ValueError("Output must contain at least two benchmark lines.")

    header_line = benchmarks_lines[0]
    data_line = benchmarks_lines[-1]

    header_parts = [h.strip().lower() for h in header_line.split(',')]
    data_parts = [d.strip() for d in data_line.split(',')]

    if len(header_parts) != len(data_parts):
         raise ValueError(f"Mismatched number of header ({len(header_parts)}) and data ({len(data_parts)}) fields.")

    metrics = {}
    try:
        for i in range(len(header_parts)):
            metrics[header_parts[i]] = float(data_parts[i])
    except (ValueError, IndexError) as e:
        # Catch potential errors during data conversion or indexing
        raise ValueError(f"Failed to parse metrics from data line '{data_line}': {e}") from e

    try:
        solution_length = int(output_str.split("Actions used: ")[1].split(".")[0].replace(",", ""))
    except (ValueError, IndexError) as e:
        raise ValueError(f"Failed to parse solution length from output: {e}") from e

    return metrics, solution_length


def load_benchmark_config()-> dict | None:
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
    if not os.path.exists(path):
        print(f"Error: Executable not found at {path}", file=sys.stderr)
        return False
    return True

def run_single_benchmark_case(commands: dict) -> CaseResult:
    command_list = commands["command_list"]
    level_name = commands["level_name"]
    strategy = commands["strategy"]
    timeout_s = commands["timeout_s"]
    
    case_result = CaseResult(
        level=level_name,
        solution_length=0,
        strategy=strategy,
        metrics={}
    )

    try:
        process = subprocess.Popen(
            command_list, 
            stdout=subprocess.PIPE, 
            stderr=subprocess.PIPE, 
            text=True,
        )
        stdout, stderr = process.communicate(timeout=timeout_s + 1)
    except subprocess.TimeoutExpired:
        process.kill()  # Shouldnt be needed
        stdout, stderr = process.communicate()
    
    returncode = process.returncode
    
    if returncode != 0:
        case_result.metrics = {
            "error": f"Process returned non-zero code {returncode}"
        }
        return case_result
    
    try:
        metrics, solution_length = parse_server_output(stdout)
        case_result.metrics = metrics
        case_result.solution_length = solution_length
    except Exception as e:
        case_result.metrics = {
            "error": f"Failed to parse server output: {e}"
        }
    
    return case_result

def save_benchmark_results(results: BenchmarkResult, output_dir: str) -> bool:
    now_time = datetime.datetime.now()
    timestamp_str = now_time.strftime("%Y%m%d_%H%M%S")
    output_filename = f"run_{timestamp_str}.json"
    output_filepath = os.path.join(output_dir, output_filename)

    try:
        with open(output_filepath, 'w') as f:
            f.write(results.to_json())
        print(f"Benchmark results saved to: {output_filepath}")
        return True
    except IOError as e:
        print(f"Error: Could not write results to {output_filepath}: {e}", file=sys.stderr)
        return False



def print_comparative_benchmark_results(cases: list[dict], results: BenchmarkResult):
    def _format_new_metric_with_change(
        prev_val,
        new_val,
        str_width: int,
        unit: str = "",
        precision: int = 0,
        gray_area_percentage: float = 5.0
    ):
        new_val_str = f"{new_val:.{precision}f}{unit}" if isinstance(new_val, (int, float)) else "N/A"
        perc_change_str_colored = ""

        if not isinstance(prev_val, (int, float)) or not isinstance(new_val, (int, float)):
            return new_val_str
        
        abs_change = new_val - prev_val
        perc_change = 0.0
        color_to_use = bcolors.OKCYAN

        if prev_val != 0:
            perc_change = (abs_change / abs(prev_val)) * 100
        elif new_val != 0:
            perc_change = float('inf') if new_val > 0 else float('-inf')

        perc_sign = "+" if perc_change > 0 else ""
        if perc_change == 0: perc_sign = ""
        perc_change_display_str = f"{perc_sign}{perc_change:.1f}%"

        # Determine color (lower is better)
        if abs_change == 0:
            color_to_use = bcolors.OKCYAN # Same
        elif abs(perc_change) <= gray_area_percentage and prev_val != 0:
            color_to_use = bcolors.WARNING # Neutral (gray area)
        elif abs_change < 0: # Better
            color_to_use = bcolors.OKGREEN
        else: # Worse
            color_to_use = bcolors.FAIL
        
        perc_change_str_colored = f" ({bcolors.colorize(perc_change_display_str, color_to_use)})".ljust(20)
        return f"{new_val_str:<{str_width}}{perc_change_str_colored}"

    HEADER_SEPARATOR = "=" * 156
    
    print(bcolors.colorize("\nComparative Benchmark Summary:", bcolors.HEADER))
    print(bcolors.colorize(HEADER_SEPARATOR, bcolors.HEADER))

    GRAY_AREA_PERCENTAGE = 5.0

    for case_config in cases:
        level_name = case_config.get("input")
        level_name_str = level_name[:-4].split("/")[-1][:15]
        if not level_name:
            print(bcolors.colorize("Warning: Skipping config case (missing 'input').", bcolors.WARNING), file=sys.stderr)
            continue

        previous_best_data = case_config.get("best_found_solution_metrics")
        if not previous_best_data:
            print(f"{level_name_str:<15} | {bcolors.colorize('Prev:', bcolors.BOLD)} No previous best data to compare.", file=sys.stderr)
            continue

        prev_strategy = previous_best_data.get("strategy", "N/A")[:12]
        prev_length = previous_best_data.get("length")
        prev_time_s = previous_best_data.get("time_s")
        prev_memory_mb = previous_best_data.get("memory_mb")

        valid_new_results_for_level = [
            res for res in results.cases
            if res.level == level_name and
               res.solution_length > 0 and
               not res.metrics.get("error")
        ]

        if not valid_new_results_for_level:
            prev_part = f"{bcolors.colorize('Prev:', bcolors.BOLD)} S:{prev_strategy:<12} L:{prev_length:<4} T:{prev_time_s:<9} M:{prev_memory_mb:<7}"
            print(f"{level_name_str:<15} | {prev_part} | {bcolors.colorize('New:', bcolors.BOLD)} No successful new results.")
            continue

        new_best_run_result = min(
            valid_new_results_for_level,
            key=lambda r: (
                r.solution_length,
                r.metrics.get("time[s]", float('inf')),
                r.metrics.get("alloc[mb]", float('inf'))
            )
        )

        new_strategy = new_best_run_result.strategy[:10]
        new_length = new_best_run_result.solution_length
        new_time_s = new_best_run_result.metrics.get("time[s]")
        new_memory_mb = new_best_run_result.metrics.get("alloc[mb]")

        # Format previous values
        prev_len_str = f"{prev_length}"
        prev_time_str = f"{prev_time_s:.3f}"
        prev_mem_str = f"{prev_memory_mb:.1f}"

        # Format new values with percentage changes
        new_len_fmt = _format_new_metric_with_change(prev_length, new_length, 4, precision=0, gray_area_percentage=GRAY_AREA_PERCENTAGE)
        new_time_fmt = _format_new_metric_with_change(prev_time_s, new_time_s, 7, precision=3, gray_area_percentage=GRAY_AREA_PERCENTAGE)
        new_mem_fmt = _format_new_metric_with_change(prev_memory_mb, new_memory_mb, 5, precision=1, gray_area_percentage=GRAY_AREA_PERCENTAGE)

        # Determine Overall Status
        overall_status_text = "N/A"
        overall_color = bcolors.OKBLUE

        if isinstance(prev_length, int) and isinstance(new_length, int):
            if new_length < prev_length:
                overall_status_text = "BETTER"
                overall_color = bcolors.OKGREEN
            elif new_length > prev_length:
                overall_status_text = "WORSE"
                overall_color = bcolors.FAIL
            else: # Lengths are the same, check time
                if isinstance(prev_time_s, float) and isinstance(new_time_s, float):
                    time_abs_change = new_time_s - prev_time_s
                    time_perc_change = 0.0
                    if prev_time_s != 0:
                        time_perc_change = (time_abs_change / abs(prev_time_s)) * 100
                    elif new_time_s != 0:
                        time_perc_change = float('inf') if new_time_s > 0 else float('-inf')
                    
                    if abs(time_perc_change) <= GRAY_AREA_PERCENTAGE and prev_time_s != 0:
                        overall_status_text = "NEUTRAL" # (time in gray area)
                        overall_color = bcolors.WARNING
                    elif time_abs_change == 0:
                        overall_status_text = "SAME" # (time identical)
                        overall_color = bcolors.OKCYAN
                    elif time_abs_change < 0:
                        overall_status_text = "BETTER" # (time improved)
                        overall_color = bcolors.OKGREEN
                    else:
                        overall_status_text = "WORSE" # (time worsened)
                        overall_color = bcolors.FAIL
                else: # Lengths same, but time data missing for comparison
                    overall_status_text = "SAME (L)" # Length same, time N/A
                    overall_color = bcolors.OKCYAN
        else: # Length data missing for comparison
            overall_status_text = "INCONCLUSIVE"
            overall_color = bcolors.OKBLUE
        
        overall_status_display = bcolors.colorize(f"[{overall_status_text}]", overall_color)

        prev_part = f"{bcolors.colorize('Prev:', bcolors.BOLD)} S:{prev_strategy:<12} L:{prev_len_str:<4} T:{prev_time_str:<9} M:{prev_mem_str:<7}"
        new_part = f"{bcolors.colorize('New:', bcolors.BOLD)} S:{new_strategy:<14} L:{new_len_fmt} T:{new_time_fmt:<17} M:{new_mem_fmt:<15} {overall_status_display}"  

        print(f"{level_name_str:<15} | {prev_part} | {new_part}")


    print(bcolors.colorize(HEADER_SEPARATOR, bcolors.HEADER))

def main():
    config = load_benchmark_config()
    if config is None:
        return

    levels_dir = config["levels_dir"]
    output_dir = config["output_dir"]
    cases = config["cases"]

    if not ensure_output_directory(output_dir):
        return

    if not check_executable_exists(SERVER_JAR):
        return

    if not check_executable_exists(CLIENT_EXECUTABLE_PATH):
        return

    now_time = datetime.datetime.now()
    all_results = BenchmarkResult(
        timestamp=now_time.isoformat(),
        cases=[]
    )

    print(f"Starting benchmarks with {WORKERS_COUNT} workers...")

    strategies = config.get("strategies", [])
    skip_best_found_strategy = config.get("skip_best_found_strategy")
    tasks_to_run_commands = []
    for case in cases:
        level_relative_path = case.get("input")
        timeout_s = case.get("timeout_s")

        if not level_relative_path:
            print(f"Warning: Skipping case due to missing 'input' level path in config.", file=sys.stderr)
            continue

        level_full_path = os.path.join(levels_dir, level_relative_path)

        if not os.path.isfile(level_full_path):
            print(f"Warning: Skipping level '{level_relative_path}' as it does not exist.", file=sys.stderr)
            continue

        if not strategies:
             print(f"Warning: Skipping level '{level_relative_path}' as no strategies are specified.", file=sys.stderr)
             continue

        for strategy in strategies:
            if skip_best_found_strategy and strategy == case.get("best_found_solution_metrics", {}).get("strategy"):
                continue
            
            client_command_str = f"{CLIENT_EXECUTABLE_PATH} {strategy}"
            command = [JAVA, "-jar", SERVER_JAR, 
                       "-c", client_command_str, 
                       "-l", level_full_path,
                       "-t", str(timeout_s)
                       ]
            tasks_to_run_commands.append(
                {
                    "command_list": command,
                    "level_name": level_relative_path,
                    "strategy": strategy,
                    "timeout_s": timeout_s
                }
            )
            
    if not tasks_to_run_commands:
        print("No tasks to run.", file=sys.stderr)
        return
    
    with ProcessPoolExecutor(max_workers=WORKERS_COUNT) as executor:
        case_results = list(tqdm(
            executor.map(run_single_benchmark_case, tasks_to_run_commands), 
            total=len(tasks_to_run_commands), 
            desc="Running benchmarks", 
            unit="case",
            miniters=1,
            dynamic_ncols=True,
            maxinterval=2.0,
            mininterval=0.5
        ))
    
    all_results.cases.extend(case_results)
    
    if not all_results.cases: # Only save if there are results
        print("No benchmark results were collected.", file=sys.stderr)
    
    save_benchmark_results(all_results, output_dir)
    print_comparative_benchmark_results(cases, all_results)



if __name__ == "__main__":
    main()

