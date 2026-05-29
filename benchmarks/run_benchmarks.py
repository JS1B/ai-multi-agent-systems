import argparse
import datetime as dt
import json
import os
import re
import statistics
import subprocess
import sys
import time
from concurrent.futures import ProcessPoolExecutor
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import Any

from tqdm import tqdm

try:
    from psutil import cpu_count
except ImportError:
    cpu_count = None


SCRIPT_DIR = Path(__file__).resolve().parent
WORKSPACE_DIR = SCRIPT_DIR.parent
DEFAULT_CLIENT = WORKSPACE_DIR / "searchclient_cpp" / "searchclient"
DEFAULT_SERVER = WORKSPACE_DIR / "server.jar"
DEFAULT_OUTPUT_DIR = SCRIPT_DIR / "output"
DEFAULT_SUITE_DIR = SCRIPT_DIR / "suites"
DEFAULT_PLANNER = "current-cbs"


@dataclass
class Case:
    level: str
    timeout_s: int


@dataclass
class CaseResult:
    suite: str
    level: str
    planner: str
    run: int
    solved: bool
    timeout: bool
    wall_time_ms: int
    generated_states: int | None
    expanded_states: int | None
    frontier: int | None
    peak_rss_mb: int | None
    actions_used: int | None
    server_solve_time_s: float | None
    failure_reason: str
    returncode: int | None
    command: list[str]


@dataclass
class BenchmarkSummary:
    suite: str
    planner: str
    total_runs: int
    solved_runs: int
    timeout_runs: int
    median_wall_time_ms_solved: int | None
    top_generated_states: list[dict[str, Any]]
    top_peak_rss_mb: list[dict[str, Any]]
    failure_reasons: dict[str, int]


@dataclass
class BenchmarkReport:
    metadata: dict[str, Any]
    summary: BenchmarkSummary
    cases: list[CaseResult]


def default_jobs_count() -> int:
    if cpu_count is not None:
        physical_cores = cpu_count(logical=False)
        if physical_cores:
            return max(physical_cores - 1, 1)
    return max((os.cpu_count() or 2) - 1, 1)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run repeatable MAvis redesign baselines.")
    parser.add_argument("--suite", default="warmup_smoke", help="Suite name under benchmarks/suites, or a JSON path.")
    parser.add_argument("--client", default=str(DEFAULT_CLIENT), help="Path to the searchclient executable.")
    parser.add_argument("--server", default=str(DEFAULT_SERVER), help="Path to server.jar.")
    parser.add_argument("--java", default="java", help="Java executable.")
    parser.add_argument("--planner", default=DEFAULT_PLANNER, help="Planner label stored in output metadata.")
    parser.add_argument(
        "--pass-planner-arg",
        action="store_true",
        help="Pass --planner to the client. Disabled by default because current client ignores argv.",
    )
    parser.add_argument("--runs", type=int, default=1, help="Number of runs per case.")
    parser.add_argument("--jobs", type=int, default=default_jobs_count(), help="Parallel worker count.")
    parser.add_argument("--timeout-s", type=int, help="Override timeout for every case.")
    parser.add_argument("--output-dir", default=str(DEFAULT_OUTPUT_DIR), help="Directory for JSON results.")
    parser.add_argument("--no-progress", action="store_true", help="Disable tqdm progress output.")
    return parser.parse_args()


def resolve_suite_path(suite: str) -> Path:
    candidate = Path(suite)
    if candidate.suffix == ".json" or candidate.exists():
        return candidate if candidate.is_absolute() else (Path.cwd() / candidate).resolve()
    return DEFAULT_SUITE_DIR / f"{suite}.json"


def load_suite(path: Path, timeout_override: int | None) -> tuple[dict[str, Any], list[Case]]:
    with path.open() as handle:
        suite = json.load(handle)

    levels_dir = Path(suite.get("levels_dir", "../levels"))
    if not levels_dir.is_absolute():
        levels_dir = (SCRIPT_DIR / levels_dir).resolve()
    suite["levels_dir_resolved"] = str(levels_dir)

    default_timeout = int(suite.get("default_timeout_s", 10))
    cases = []
    for raw_case in suite.get("cases", []):
        level = raw_case.get("level") or raw_case.get("input")
        if not level:
            raise ValueError(f"Suite case is missing a level path: {raw_case}")
        timeout_s = int(timeout_override or raw_case.get("timeout_s", default_timeout))
        level_path = levels_dir / level
        if not level_path.is_file():
            raise FileNotFoundError(f"Level not found: {level_path}")
        cases.append(Case(level=level, timeout_s=timeout_s))

    if not cases:
        raise ValueError(f"Suite {path} has no cases.")
    return suite, cases


def parse_int_after(pattern: str, text: str) -> int | None:
    match = re.search(pattern, text)
    if not match:
        return None
    return int(match.group(1).replace(",", ""))


def parse_float_after(pattern: str, text: str) -> float | None:
    match = re.search(pattern, text)
    if not match:
        return None
    return float(match.group(1))


def normalize_metric_name(name: str) -> str:
    normalized = name.strip().lower().replace(" ", "_")
    return {
        "generated": "generated_states",
        "expanded": "expanded_states",
        "alloc[mb]": "peak_rss_mb",
        "peak_rss[mb]": "peak_rss_mb",
        "peak_rss_mb": "peak_rss_mb",
    }.get(normalized, normalized)


def parse_client_metrics(stdout: str) -> dict[str, int]:
    metric_lines = []
    prefix = "[client][message]"
    for raw_line in stdout.splitlines():
        line = raw_line.strip()
        if line.startswith(prefix):
            metric_lines.append(line[len(prefix) :].strip())
    if len(metric_lines) < 2:
        return {}

    header = [normalize_metric_name(part) for part in metric_lines[0].split(",")]
    values = [part.strip() for part in metric_lines[-1].split(",")]
    if len(header) != len(values):
        return {}

    metrics = {}
    for key, value in zip(header, values):
        try:
            metrics[key] = int(float(value.replace(",", "")))
        except ValueError:
            continue
    return metrics


def failure_reason(returncode: int | None, timed_out: bool, solved: bool, stdout: str, stderr: str) -> str:
    combined = f"{stdout}\n{stderr}"
    if timed_out:
        return "timeout"
    if returncode not in (0, None):
        return f"process_exit_{returncode}"
    if solved:
        return ""
    if "Maximum memory usage exceeded" in combined:
        return "memory_cap"
    if "Unable to solve level" in combined:
        return "unsolved"
    if "Level solved: No" in combined:
        return "invalid_or_unsolved"
    return "unknown"


def run_case(task: dict[str, Any]) -> CaseResult:
    suite_name = task["suite_name"]
    levels_dir = Path(task["levels_dir"])
    level = task["level"]
    timeout_s = task["timeout_s"]
    planner = task["planner"]
    run = task["run"]
    command = [
        task["java"],
        "-jar",
        task["server"],
        "-c",
        task["client_command"],
        "-l",
        str(levels_dir / level),
        "-t",
        str(timeout_s),
    ]

    stdout = ""
    stderr = ""
    timed_out = False
    returncode = None
    started_at = time.perf_counter()
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
    try:
        stdout, stderr = process.communicate(timeout=timeout_s + 2)
        returncode = process.returncode
    except subprocess.TimeoutExpired:
        timed_out = True
        process.kill()
        stdout, stderr = process.communicate()
        returncode = process.returncode
    wall_time_ms = int((time.perf_counter() - started_at) * 1000)

    solved = "Level solved: Yes" in stdout
    server_timed_out = "Client timed out" in stdout
    metrics = parse_client_metrics(stdout)
    reason = failure_reason(returncode, timed_out or server_timed_out, solved, stdout, stderr)

    return CaseResult(
        suite=suite_name,
        level=level,
        planner=planner,
        run=run,
        solved=solved,
        timeout=timed_out or server_timed_out,
        wall_time_ms=wall_time_ms,
        generated_states=metrics.get("generated_states"),
        expanded_states=metrics.get("expanded_states"),
        frontier=metrics.get("frontier"),
        peak_rss_mb=metrics.get("peak_rss_mb"),
        actions_used=parse_int_after(r"Actions used: ([\d,]+)\.", stdout),
        server_solve_time_s=parse_float_after(r"Time to solve: ([\d.]+) seconds\.", stdout),
        failure_reason=reason,
        returncode=returncode,
        command=command,
    )


def build_tasks(args: argparse.Namespace, suite: dict[str, Any], cases: list[Case]) -> list[dict[str, Any]]:
    client_command = args.client
    if args.pass_planner_arg:
        client_command = f"{args.client} {args.planner}"

    tasks = []
    for case in cases:
        for run in range(1, args.runs + 1):
            tasks.append(
                {
                    "suite_name": suite["name"],
                    "levels_dir": suite["levels_dir_resolved"],
                    "level": case.level,
                    "timeout_s": case.timeout_s,
                    "planner": args.planner,
                    "run": run,
                    "java": args.java,
                    "server": args.server,
                    "client_command": client_command,
                }
            )
    return tasks


def top_metric(results: list[CaseResult], metric: str) -> list[dict[str, Any]]:
    ranked = [
        {
            "level": result.level,
            "run": result.run,
            metric: getattr(result, metric),
        }
        for result in results
        if getattr(result, metric) is not None
    ]
    ranked.sort(key=lambda item: item[metric], reverse=True)
    return ranked[:10]


def summarize(suite_name: str, planner: str, results: list[CaseResult]) -> BenchmarkSummary:
    solved_wall_times = [result.wall_time_ms for result in results if result.solved]
    failure_counts: dict[str, int] = {}
    for result in results:
        if result.failure_reason:
            failure_counts[result.failure_reason] = failure_counts.get(result.failure_reason, 0) + 1

    return BenchmarkSummary(
        suite=suite_name,
        planner=planner,
        total_runs=len(results),
        solved_runs=sum(1 for result in results if result.solved),
        timeout_runs=sum(1 for result in results if result.timeout),
        median_wall_time_ms_solved=int(statistics.median(solved_wall_times)) if solved_wall_times else None,
        top_generated_states=top_metric(results, "generated_states"),
        top_peak_rss_mb=top_metric(results, "peak_rss_mb"),
        failure_reasons=failure_counts,
    )


def write_report(args: argparse.Namespace, suite: dict[str, Any], results: list[CaseResult]) -> Path:
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    timestamp_value = dt.datetime.now(dt.UTC).replace(microsecond=0)
    timestamp = timestamp_value.isoformat()
    safe_timestamp = timestamp_value.strftime("%Y%m%dT%H%M%SZ")
    output_path = output_dir / f"{suite['name']}_{args.planner}_{safe_timestamp}.json"
    summary = summarize(suite["name"], args.planner, results)
    report = BenchmarkReport(
        metadata={
            "timestamp": timestamp,
            "suite_description": suite.get("description", ""),
            "levels_dir": suite["levels_dir_resolved"],
            "client": args.client,
            "server": args.server,
            "java": args.java,
            "runs": args.runs,
            "jobs": args.jobs,
            "pass_planner_arg": args.pass_planner_arg,
        },
        summary=summary,
        cases=results,
    )
    with output_path.open("w") as handle:
        json.dump(
            {
                "metadata": report.metadata,
                "summary": asdict(report.summary),
                "cases": [asdict(result) for result in report.cases],
            },
            handle,
            indent=2,
        )
        handle.write("\n")
    return output_path


def print_summary(summary: BenchmarkSummary, output_path: Path) -> None:
    print("\nBenchmark summary")
    print("=================")
    print(f"suite: {summary.suite}")
    print(f"planner: {summary.planner}")
    print(f"runs: {summary.total_runs}")
    print(f"solved: {summary.solved_runs}/{summary.total_runs}")
    print(f"timeouts: {summary.timeout_runs}")
    print(f"median wall time solved: {summary.median_wall_time_ms_solved} ms")
    print(f"failure reasons: {summary.failure_reasons or {}}")
    print(f"output: {output_path}")


def main() -> int:
    args = parse_args()
    suite_path = resolve_suite_path(args.suite)
    client = Path(args.client)
    server = Path(args.server)
    if not suite_path.is_file():
        print(f"Suite not found: {suite_path}", file=sys.stderr)
        return 2
    if not client.is_file():
        print(f"Client executable not found: {client}", file=sys.stderr)
        return 2
    if not server.is_file():
        print(f"Server jar not found: {server}", file=sys.stderr)
        return 2
    if args.runs < 1:
        print("--runs must be at least 1", file=sys.stderr)
        return 2
    if args.jobs < 1:
        print("--jobs must be at least 1", file=sys.stderr)
        return 2

    suite, cases = load_suite(suite_path, args.timeout_s)
    tasks = build_tasks(args, suite, cases)
    print(
        f"Running suite '{suite['name']}' with {len(cases)} cases, {args.runs} run(s), "
        f"{args.jobs} worker(s), planner '{args.planner}'."
    )

    if args.jobs == 1:
        iterator = map(run_case, tasks)
        results = list(tqdm(iterator, total=len(tasks), disable=args.no_progress, unit="case"))
    else:
        with ProcessPoolExecutor(max_workers=args.jobs) as executor:
            results = list(tqdm(executor.map(run_case, tasks), total=len(tasks), disable=args.no_progress, unit="case"))

    output_path = write_report(args, suite, results)
    print_summary(summarize(suite["name"], args.planner, results), output_path)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
