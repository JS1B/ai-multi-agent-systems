import json
import os
import sys
import datetime
import subprocess
from concurrent.futures import ProcessPoolExecutor
from dataclasses import dataclass

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
BENCHMARK_CONFIG_REQUIRED_KEYS = ["levels_dir", "output_dir", "cases", "timeout_s"]

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
        solution_length = int(output_str.split("Actions used: ")[1].split(".")[0])
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
    
def print_better_benchmark_results(cases: list[dict], results: BenchmarkResult):
    print("\nBetter benchmark summary:")
    
    for case in cases:
        previous_best_case_result = case.get("best_found_solution_metrics", None)
        if not previous_best_case_result:
            print(f"Warning: No best found solution metrics for case {case.get('input')}", file=sys.stderr)
            continue
        
        results_for_case = [case_result for case_result in results.cases if case.get("input") in case_result.level]
        # Filter out timedout results
        results_for_case = [result for result in results_for_case if result.solution_length != 0]
        if not results_for_case:
            print(f"Warning: No results for case {case.get('input')}", file=sys.stderr)
            continue
        
        
        # TODO: Rethink scoring
        new_best_result_for_case = min(results_for_case, key=lambda x: (x.solution_length, x.metrics["time[s]"]))
        if new_best_result_for_case.solution_length > previous_best_case_result["length"]:
            continue
        if new_best_result_for_case.solution_length == previous_best_case_result["length"] and new_best_result_for_case.metrics["time[s]"] >= previous_best_case_result["time_s"]:
            continue

        print(f"{case.get('input'):^24}")
        print(f"{"Previous best":<15}\t{"New best":<15}")
        print(f"{previous_best_case_result['strategy']:<15}\t{new_best_result_for_case.strategy:<15}")
        print(f"{previous_best_case_result['length']:<15}\t{new_best_result_for_case.solution_length:<15}")
        print(f"{previous_best_case_result['time_s']:<15}\t{new_best_result_for_case.metrics['time[s]']:<15}")
        print(f"{previous_best_case_result['memory_mb']:<15}\t{new_best_result_for_case.metrics['alloc[mb]']:<15}")
        print()

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

    tasks_to_run_commands = []
    for case in cases:
        level_relative_path = case.get("input")
        strategies = case.get("strategies", [])

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
            client_command_str = f"{CLIENT_EXECUTABLE_PATH} {strategy}"
            command = [JAVA, "-jar", SERVER_JAR, 
                       "-c", client_command_str, 
                       "-l", level_full_path,
                       "-t", str(int(config["timeout_s"]))
                       ]
            tasks_to_run_commands.append(
                {
                    "command_list": command,
                    "level_name": level_relative_path,
                    "strategy": strategy,
                    "timeout_s": config["timeout_s"]
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
    print_better_benchmark_results(cases, all_results)



if __name__ == "__main__":
    main()

