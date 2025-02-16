import os
from argparse import ArgumentParser
from collections import defaultdict


BUILD_SCRIPT = {
    "linux": "./build_searchclient.sh",
    "windows": ".\\build_searchclient.ps1",
}

def exc_template_factory():
    return {
        "level": list[str],
        "strategy": list[str],
        "heuristic": list[str],
        "timeout": 3600,
        "display": "", # "-g -s 150"
    }

exc_dict = defaultdict(exc_template_factory)

def run_command(command: str):
    print()
    print(command)
    os.system(command)

def form_command_and_run(exc: dict, output_dir: str):
    for lvl in exc["level"]:
        if not os.path.exists(f"levels/{lvl}.lvl"):
            print(f"Level {lvl} does not exist, skipping")
            continue

        for strat in exc["strategy"]:
            for heur in exc["heuristic"]:
                output_file: str = os.path.join(output_dir, f"{lvl}_{strat}_{heur}.txt")
                command = f"java -jar server.jar -l levels/{lvl}.lvl -c \"java -Xmx8g -jar searchclient_java/searchclient.jar -benchmark -heur {heur} -{strat}\" -t {exc['timeout']} {exc['display']} -o {output_file}"
                run_command(command)

def main():
    exc_3 = exc_dict["exc_3"]
    exc_3["level"] = ["MAPF00", "MAPF01", "MAPF02", "MAPF02C", "MAPF03", "MAPF03C", "MAPFslidingpuzzle", "MAPFreorder2", "BFSFriendly"]
    exc_3["strategy"] = ["bfs", "dfs"]

    exc_42 = exc_dict["exc_42"]
    exc_42["level"] = [ "MAPF00", "MAPF01", "MAPF02", "MAPF02C", "MAPF03", "MAPF03C", "MAPFslidingpuzzle", "MAPFreorder2", "BFSFriendly"]
    exc_42["strategy"] = ["greedy", "astar"]
    exc_42["heuristic"] = ["goalcount"]
    
    exc_43 = exc_dict["exc_43"]
    exc_43["level"] = ["MAPF00", "MAPF01", "MAPF02", "MAPF02C", "MAPF03", "MAPF03C", "MAPFslidingpuzzle", "MAPFreorder2", "BFSFriendly"]
    exc_43["strategy"] = ["greedy", "astar"]
    exc_43["heuristic"] = ["goalcount"]

    exc_61 = exc_dict["exc_61"]
    exc_61["level"] = ["SAFirefly", "SACrunch"]
    exc_61["strategy"] = ["greedy", "bfs", "dfs"]
    exc_61["heuristic"] = ["goalcount"]
    exc_61["timeout"] = 100

    exc_62 = exc_dict["exc_62"]
    exc_62["level"] = ["MAPF00", "MAPF01", "MAPF02", "MAPF02C", "MAPF03", "MAPF03C", "MAPFslidingpuzzle", "MAPFreorder2", "BFSFriendly"]
    exc_62["strategy"] = ["greedy", "astar"]
    exc_62["heuristic"] = ["boxcustom"]
    exc_62["timeout"] = 100

    parser = ArgumentParser()
    parser.add_argument("--exercise", type=str, default="all")
    parser.add_argument("--output", type=str, default="results")
    args = parser.parse_args()

    if not os.path.exists(args.output):
        os.makedirs(args.output)

    if os.name == "nt":
        run_command(BUILD_SCRIPT["windows"])
    else:
        run_command(BUILD_SCRIPT["linux"])

    if args.exercise == "3":
        form_command_and_run(exc_3, args.output)
    elif args.exercise == "42":
        form_command_and_run(exc_42, args.output)
    elif args.exercise == "43":
        form_command_and_run(exc_43, args.output)
    elif args.exercise == "61":
        form_command_and_run(exc_61, args.output)
    elif args.exercise == "62":
        form_command_and_run(exc_62, args.output)
    elif args.exercise == "all":
        form_command_and_run(exc_3, args.output)
        form_command_and_run(exc_42, args.output)
        form_command_and_run(exc_43, args.output)
        form_command_and_run(exc_61, args.output)
        form_command_and_run(exc_62, args.output)


if __name__ == "__main__":
    main()
