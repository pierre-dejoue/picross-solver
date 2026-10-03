import argparse
import json
import random
import statistics
import sys
from pathlib import Path

from src.cache_puzzles import PUZZLE_DIR, get_puzzles
from src.run_timing import DEFAULT_SOLVER, run_timing
from src.run_phases import DEFAULT_PHASE_DRIVER, run_phases

def select_puzzles(args: argparse.Namespace) -> list:
    puzzles = get_puzzles()
    if args.source is not None:
        puzzles = [puzzle for puzzle in puzzles if puzzle["source"] == args.source]

    n = min(args.num_puzzles, len(puzzles))
    if n < args.num_puzzles:
        print(f"only {n} puzzles are available but {args.num_puzzles} were requested", file=sys.stderr)
    return random.Random(args.seed).sample(puzzles, n)

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

    puzzles = select_puzzles(args)
    report = run_benchmark(run, binary, puzzles, args.trials, args.timeout)

    print(json.dumps(report))

def add_common_args(parser: argparse.ArgumentParser) -> None:
    parser.add_argument("--trials", type=int, default=5, help="trials per puzzle (default: 5)")
    parser.add_argument("--timeout", type=float, default=10.0, help="solve timeout in seconds (default: 10)")
    parser.add_argument("--num-puzzles", type=int, default=10, help="number of puzzles randomly selected to solve (default: 10)")
    parser.add_argument("--seed", type=int, default=None, help="seed for --num-puzzles (default: None)")
    parser.add_argument("--source", type=str, default=None, choices=["survey", "tournament"], help="source to select puzzles from (default: all)")

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
