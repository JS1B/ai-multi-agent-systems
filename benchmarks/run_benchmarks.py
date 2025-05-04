import json
import os
import sys
import datetime
import subprocess
from dataclasses import dataclass

# TODO: Add process pool executor from concurrent.futures with tqdm progress bar

BENCHMARK_CONFIG_FILE = "benchmarks.json"
CPP_EXECUTABLE_PATH = "../searchclient_cpp/searchclient" 

BENCHMARK_CONFIG_REQUIRED_KEYS = ["levels_dir", "output_dir", "cases"]

@dataclass
class CaseResult:
    level: str
    strategy: str
    solution: list[str]
    metrics: dict[str, float]
    
@dataclass
class BenchmarkResult:
    timestamp: str
    cases: list[CaseResult]
    
    def to_json(self) -> str:
        return json.dumps(self, default=lambda o: o.__dict__, indent=4)


def parse_cpp_output(output_str: str) -> dict:
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

    metrics = {}
    try:
        for i in range(len(header_parts)):
             metrics[header_parts[i]] = float(data_parts[i])
    except (ValueError, IndexError) as e:
        # Catch potential errors during data conversion or indexing
        raise ValueError(f"Failed to parse metrics from data line '{data_line}': {e}") from e

    return metrics

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
    if not os.path.isfile(path):
        print(f"Error: C++ executable not found at {path}", file=sys.stderr)
        return False
    return True

def run_single_benchmark_case(executable_path: str, level_path: str, strategy: str) -> str:
    if not os.path.isfile(level_path):
        raise FileNotFoundError(f"Level file not found: {level_path}")

    command = [executable_path, strategy]

    try:
        with open(level_path, 'r') as level_file:
            result = subprocess.run(
                command,
                stdin=level_file,
                capture_output=True,
                text=True,
                check=True
            )
        return result.stdout
    except subprocess.CalledProcessError as e:
        print(f"Error executing command: {' '.join(e.cmd)}", file=sys.stderr)
        print(f"Return code: {e.returncode}", file=sys.stderr)
        print(f"Stderr:\n{e.stderr}", file=sys.stderr)
        raise # Re-raise the exception after printing details
    except OSError as e:
        print(f"OS Error during subprocess execution: {e}", file=sys.stderr)
        raise # Re-raise other OS errors

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

def main():
    config = load_benchmark_config()
    if config is None:
        return

    levels_dir = config["levels_dir"]
    output_dir = config["output_dir"]
    cases = config["cases"]

    if not ensure_output_directory(output_dir):
        return

    if not check_executable_exists(CPP_EXECUTABLE_PATH):
        return

    now_time = datetime.datetime.now()
    all_results = BenchmarkResult(
        timestamp=now_time.isoformat(),
        cases=[]
    )

    print(f"Starting benchmarks. Results will be saved in {output_dir}")

    for case in cases:
        level_relative_path = case.get("input")
        strategies = case.get("strategies", [])

        if not level_relative_path:
            print(f"Warning: Skipping case due to missing 'input' level path in config.", file=sys.stderr)
            continue

        level_full_path = os.path.join(levels_dir, level_relative_path)

        if not strategies:
             print(f"Warning: Skipping level '{level_relative_path}' as no strategies are specified.", file=sys.stderr)
             continue

        for strategy in strategies:
            print(f"Running benchmark for level: {level_relative_path}, strategy: {strategy}...", end=' ')
            try:
                output_str = run_single_benchmark_case(
                    CPP_EXECUTABLE_PATH,
                    level_full_path,
                    strategy
                )
                metrics = parse_cpp_output(output_str)
                case_result = CaseResult(
                    level=level_relative_path,
                    strategy=strategy,
                    solution=["WIP"], # TODO
                    metrics=metrics
                )
                all_results.cases.append(case_result)
                print("Success.")
            except FileNotFoundError:
                 print(f"Skipping: Level file not found at {level_full_path}", file=sys.stderr)
                 break # Skip other strategies for this level if file is missing
            except (subprocess.CalledProcessError, OSError, ValueError, IndexError) as e:
                # Specific errors from subprocess or parsing are caught and reported
                print(f"Failed: {e}", file=sys.stderr)
                # Continue to the next strategy/case

    if all_results.cases: # Only save if there are results
        save_benchmark_results(all_results, output_dir)
    else:
        print("No benchmark results were collected.", file=sys.stderr)


if __name__ == "__main__":
    main()

