import json
import subprocess
from pathlib import Path

DEFAULT_PHASE_DRIVER = Path(__file__).resolve().parent.parent.parent / "build" / "bin" / "phase_driver"
COMPLETED_STATUSES = {"OK", "NOT_LINE_SOLVABLE", "CONTRADICTORY_GRID"}

def run_phases(driver: Path, puzzle_path: Path, timeout: float) -> tuple[str, float | None, dict | None]:
    try:
        proc = subprocess.run([str(driver), str(puzzle_path), "2"], capture_output=True, text=True, timeout=timeout)
    except subprocess.TimeoutExpired:
        return "timeout", None, None

    try:
        grid = json.loads(proc.stdout)
    except json.JSONDecodeError:
        return "error", None, None

    if grid["status"] not in COMPLETED_STATUSES:
        return "error", None, None

    return "ok", grid["wall_time_s"], grid["phases_s"]
