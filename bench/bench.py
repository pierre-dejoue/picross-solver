import argparse
import csv
import io
import json
import random
import statistics
import subprocess
import sys
import time
from pathlib import Path

from src.cache_puzzles import PUZZLE_DIR, get_puzzles

DEFAULT_SOLVER = Path(__file__).resolve().parent.parent / "build" / "bin" / "picross_solver_cli"

def run(solver: Path, puzzle_path: Path, timeout: float) -> tuple[str, float | None]:
    cmd = [str(solver), "--validation", "--quiet", str(puzzle_path)]
    start = time.perf_counter()
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return "timeout", None
    elapsed = time.perf_counter() - start

    reader = csv.DictReader(io.StringIO(proc.stdout))
    row = next(iter(reader), None)
    if row is None:
        return "error", None
    
    valid = row.get("Valid", "")
    if valid not in ("OK", "MULT", "ZERO"):
        return "error", None

    return "ok", elapsed

def cmd_timing(args: argparse.Namespace) -> None:
    solver = DEFAULT_SOLVER
    if not solver.exists():
        raise SystemExit(f"solver binary not found: {solver}\n")

    puzzles = get_puzzles()
    if args.source is not None:
        puzzles = [puzzle for puzzle in puzzles if puzzle["source"] == args.source]

    n = min(args.num_puzzles, len(puzzles))
    if n < args.num_puzzles:
        print(f"only {n} puzzles are available but {args.num_puzzles} were requested", file=sys.stderr)
    puzzles = random.Random(args.seed).sample(puzzles, n)

    report = []
    for puzzle in puzzles:
        puzzle_path = PUZZLE_DIR / f"{puzzle['sha']}.non"
        times: list[float] = []
        counts = {"ok": 0, "error": 0, "timeout": 0}

        for _ in range(args.trials):
            status, elapsed = run(solver, puzzle_path, args.timeout)
            counts[status] += 1
            if elapsed is not None:
                times.append(elapsed)

        entry = {
            "name": puzzle["name"],
            "sha": puzzle["sha"],
            "trials": args.trials,
            **counts,
        }

        if len(times) != 0:
            entry["min_s"] = min(times)
            entry["max_s"] = max(times)
            entry["median_s"] = statistics.median(times)
            entry["mean_s"] = statistics.mean(times)
            entry["stdev_s"] = statistics.stdev(times)
        report.append(entry)
    
    print(json.dumps(report))

def main() -> None:
    parser = argparse.ArgumentParser(prog="bench.py")
    subparsers = parser.add_subparsers(dest="command", required=True)

    timing_parser = subparsers.add_parser("timing", help="Run the full solver on each puzzle")
    timing_parser.add_argument("--trials", type=int, default=5, help="trials per puzzle (default: 5)")
    timing_parser.add_argument("--timeout", type=float, default=10.0, help="solve timeout in seconds (default: 10)")
    timing_parser.add_argument("--num-puzzles", type=int, default=10, help="number of puzzles randomly selected to solve (default: 10)")
    timing_parser.add_argument("--seed", type=int, default=None, help="seed for --num-puzzles (default: None)")
    timing_parser.add_argument("--source", type=str, default=None, choices=["survey", "tournament"], help="source to select puzzles from (default: all)")
    timing_parser.set_defaults(func=cmd_timing)

    args = parser.parse_args()
    args.func(args)

if __name__ == "__main__":
    main()
