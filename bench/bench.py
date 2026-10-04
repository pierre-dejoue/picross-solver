import argparse
import json
import random
import statistics
import sys
from pathlib import Path

from src.cache_puzzles import PUZZLE_DIR, get_puzzles
from src.run_timing import DEFAULT_SOLVER, run_timing
from src.run_phases import DEFAULT_PHASE_DRIVER, run_phases

SOURCES = ("sample_simpson", "rand30", "tournament")

def select_puzzles(args: argparse.Namespace) -> list:
    puzzles = get_puzzles()
    if args.source is not None:
        puzzles = [puzzle for puzzle in puzzles if puzzle["source"] == args.source]

    groups = {source: [puzzle for puzzle in puzzles if puzzle["source"] == source] for source in ([args.source] if args.source is not None else SOURCES)}

    rng = random.Random(args.seed)
    n = args.num_puzzles
    base_share = n // len(groups)
    shares = {group: min(len(puzzles), base_share) for group, puzzles in groups.items()}

    leftover = n - sum(shares.values())
    growable = [group for group in groups if shares[group] < len(groups[group])]
    i = 0
    while leftover > 0 and len(growable) > 0:
        group = growable[i % len(growable)]
        shares[group] += 1
        leftover -= 1
        if shares[group] >= len(groups[group]):
            growable.remove(group)
        else:
            i += 1

    if leftover > 0:
        total = sum(len(puzzles) for puzzles in groups.values())
        print(f"only {total} puzzles are available but {n} were requested", file=sys.stderr)

    selected = []
    for group, puzzles in groups.items():
        selected += rng.sample(puzzles, shares[group])
    return selected

def select_puzzles_from_config(config: dict) -> list:
    puzzles = get_puzzles()
    sha_map = {puzzle["sha"]: puzzle for puzzle in puzzles}
    source_map: dict[str, list] = {}
    for puzzle in puzzles:
        if puzzle["source"] not in source_map:
            source_map[puzzle["source"]] = [puzzle]
        else:
            source_map[puzzle["source"]].append(puzzle)

    rng = random.Random(config.get("seed"))
    selected_shas: set[str] = set()
    selected_puzzles = []

    for source, count in config.get("sources", {}).items():
        pool = source_map.get(source, [])
        n = min(count, len(pool))
        if n < count:
            print(f"only {n} puzzles available in source '{source}' but {count } were requested", file=sys.stderr)
        for puzzle in rng.sample(pool, n):
            if puzzle["sha"] not in selected_shas:
                selected_shas.add(puzzle["sha"])
                selected_puzzles.append(puzzle)

    for sha in config.get("include", []):
        if sha in selected_shas:
            continue
        puzzle = sha_map.get(sha)
        if puzzle is None:
            print(f"unknown puzzle {sha}", file=sys.stderr)
            continue
        selected_shas.add(sha)
        selected_puzzles.append(puzzle)

    return selected_puzzles

def run_benchmark(run_fn, binary: Path, puzzles: list, trials: int, timeout: float) -> list[dict]:
    report = []
    for puzzle in puzzles:
        puzzle_path = PUZZLE_DIR / f"{puzzle['sha']}.non"
        times: list[float] = []
        phase_times: dict[str, list[float]] = {}
        counts = {"ok": 0, "error": 0, "timeout": 0}

        for _ in range(trials):
            status, elapsed, phases = run_fn(binary, puzzle_path, timeout)
            counts[status] += 1
            if elapsed is not None:
                times.append(elapsed)
            if phases is not None:
                for phase, seconds in phases.items():
                    if not phase in phase_times:
                        phase_times[phase] = []
                    phase_times[phase].append(seconds)

        entry = {
            "name": puzzle["name"],
            "sha": puzzle["sha"],
            "trials": trials,
            **counts,
        }

        if len(times) != 0:
            entry["min_s"] = min(times)
            entry["max_s"] = max(times)
            entry["median_s"] = statistics.median(times)
            entry["mean_s"] = statistics.mean(times)
            entry["stdev_s"] = statistics.stdev(times) if len(times) > 1 else 0.0

        if len(phase_times) != 0:
            entry["phases"] = {
                phase: {
                    "min_s": min(values),
                    "max_s": max(values),
                    "median_s": statistics.median(values),
                    "mean_s": statistics.mean(values),
                    "stdev_s": statistics.stdev(values) if len(values) > 1 else 0.0,
                } for phase, values in phase_times.items()
            }

        report.append(entry)

    return report

def cmd_timing(args: argparse.Namespace) -> None:
    binary = args.binary
    if not binary.exists():
        raise SystemExit(f"binary not found: {binary}\n")

    run = run_phases if args.command == "phases" else run_timing

    if args.config is not None:
        with open(args.config) as f:
            config = json.load(f)
            puzzles = select_puzzles_from_config(config)
            trials = config.get("trials", args.trials)
            timeout = config.get("timeout", args.timeout)
    else:
        puzzles = select_puzzles(args)
        trials = args.trials
        timeout = args.timeout

    report = run_benchmark(run, binary, puzzles, trials, timeout)

    print(json.dumps(report))

def add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--trials", type=int, default=5, help="trials per puzzle (default: 5)")
    parser.add_argument("--timeout", type=float, default=10.0, help="solve timeout in seconds (default: 10)")
    parser.add_argument("--num-puzzles", type=int, default=10, help="number of puzzles randomly selected to solve (default: 10)")
    parser.add_argument("--seed", type=int, default=None, help="seed for --num-puzzles (default: None)")
    parser.add_argument("--source", type=str, default=None, choices=list(SOURCES), help="source to select puzzles from (default: all)")
    parser.add_argument("--config", type=str, default=None, help="path to JSON config file: {\"seed\":, \"trials\":, \"timeout\":,\n\"sources\": {name: count,}, \"include\": [sha,]}")

def main() -> None:
    parser = argparse.ArgumentParser(prog="bench.py")
    subparsers = parser.add_subparsers(dest="command", required=True)

    timing_parser = subparsers.add_parser("timing", help="Run the full solver on each puzzle")
    add_common_args(timing_parser)
    timing_parser.set_defaults(func=cmd_timing, binary=DEFAULT_SOLVER)

    phases_parser = subparsers.add_parser("phases", help="Time each phase of solving on each puzzle")
    add_common_args(phases_parser)
    phases_parser.set_defaults(func=cmd_timing, binary=DEFAULT_PHASE_DRIVER)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
