import csv
import io
import subprocess
import time
from pathlib import Path

DEFAULT_SOLVER = Path(__file__).resolve().parent.parent.parent / "build" / "bin" / "picross_solver_cli"

def run_timing(solver: Path, puzzle_path: Path, timeout: float) -> tuple[str, float | None, dict | None]:
    cmd = [str(solver), "--validation", "--quiet", str(puzzle_path)]
    start = time.perf_counter()
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return "timeout", None, None
    elapsed = time.perf_counter() - start

    reader = csv.DictReader(io.StringIO(proc.stdout))
    row = next(iter(reader), None)
    if row is None:
        return "error", None, None

    valid = row.get("Valid", "")
    if valid not in ("OK", "MULT", "ZERO"):
        return "error", None, None

    return "ok", elapsed, None
