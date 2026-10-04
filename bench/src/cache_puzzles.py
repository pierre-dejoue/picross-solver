import hashlib
import json
import re
import urllib.request
import tarfile
from pathlib import Path
from typing import TypedDict, Callable

class RawPuzzle(TypedDict):
    name: str
    sha: str
    source: str

class Puzzle(RawPuzzle):
    width: int
    height: int

CACHE_DIR = Path(__file__).resolve().parent.parent / "cache"
RAW_DIR = CACHE_DIR / "raw"
PUZZLE_DIR = CACHE_DIR / "puzzles"
PUZZLE_INDEX = CACHE_DIR / "puzzles.json"

SAMPLE_SIMPSON_URL = "https://webpbn.com/survey/puzzles/sample-simpson.tgz"
RAND30_URL = "https://webpbn.com/survey/rand30.tgz"
TOURNAMENT_URL = "https://raw.githubusercontent.com/zxkyjimmy/NonogramRecord/master"
TOURNAMENTS = ["icga2016", "icga2017", "icga2018", "icga2019", "icga2020", "icga2021", "icga2022", "icga2023", "taai2011", "taai2013", "taai2014", "taai2015", "taai2016", "taai2017", "taai2018", "taai2019", "taai2020", "taai2021", "taai2022", "tcga2012", "tcga2013", "tcga2014", "tcga2015", "tcga2016", "tcga2017", "tcga2019", "tcga2020", "tcga2021", "tcga2022", "tcga2023"]

def save_non(non: bytes) -> str:
    PUZZLE_DIR.mkdir(parents=True, exist_ok=True)
    sha = hashlib.sha256(non).hexdigest()
    path = PUZZLE_DIR / f"{sha}.non"
    if not path.exists():
        path.write_bytes(non)
    return sha

def fetch_raw(url: str) -> tuple[bytes, str]:
    RAW_DIR.mkdir(parents=True, exist_ok=True)
    req = urllib.request.Request(url)
    with urllib.request.urlopen(req) as resp:
        data = resp.read()
        sha = hashlib.sha256(data).hexdigest()
        path = RAW_DIR / sha
        if not path.exists():
            path.write_bytes(data)
        return data, sha

def clues_to_non(rows: list[str], cols: list[str]) -> bytes:
    width = len(cols)
    height = len(rows)
    
    lines = [f"width {width}", f"height {height}", "", "rows"]
    lines += rows
    lines += ["", "columns"]
    lines += cols
        
    return ("\n".join(lines) + "\n").encode()

def fetch_archived_puzzles(url: str) -> list[tuple[str, bytes]]:
    puzzles = []
    _, tar_sha = fetch_raw(url)
    with tarfile.open(RAW_DIR / tar_sha) as tf:
        for member in tf.getmembers():
            if member.isfile():
                file = tf.extractfile(member)
                if file is not None:
                    puzzles.append((member.name, file.read()))

    return puzzles

def simpson_to_non(name: str, puzzle: bytes) -> bytes | None:
    if name.endswith(".non"):
        return puzzle
    else:
        return None

def runs(line: str) -> list[str]:
    res = [str(len(run)) for run in line.split("0") if run]
    if not res:
        return ["0"]
    
    return res

def rand30_to_non(name: str, puzzle: bytes) -> bytes | None:
    grid = puzzle.decode().split()
    width = len(grid[0])

    raw_cols = ["".join(row[i] for row in grid) for i in range(width)]

    rows = [",".join(runs(row)) for row in grid]
    cols = [",".join(runs(col)) for col in raw_cols]
    
    return clues_to_non(rows, cols)

def cache_archived_puzzles(url: str, to_non: Callable[[str, bytes], bytes | None], source: str) -> list[RawPuzzle]:
    puzzles: list[RawPuzzle] = []
    puzzle_data = fetch_archived_puzzles(url)
    for name, puzzle in puzzle_data:
        non = to_non(name, puzzle)
        if non is not None:
            sha = save_non(non)
            puzzles.append({ "name": Path(name).name, "sha": sha, "source": source })
    
    return puzzles

def parse_tournament(data: str) -> list[tuple[str, bytes]]:
    puzzles = []
    lines = data.splitlines()
    for i, line in enumerate(lines):
        if line.startswith("$"):
            block = lines[i + 1 : i + 1 + 2 * 25]
            clues = [",".join(l.split()) if len(l.split()) != 0 else "0" for l in block]
            cols, rows = clues[:25], clues[25:]
            puzzles.append((line[1:], clues_to_non(rows, cols)))
    return puzzles

def cache_tournament(name: str) -> list[RawPuzzle]:
    puzzles: list[RawPuzzle] = []
    data = fetch_raw(f"{TOURNAMENT_URL}/question/{name}.txt")[0].decode()
    for id, non in parse_tournament(data):
        sha = save_non(non)
        puzzles.append({ "name": f"{name}#{id}", "sha": sha, "source": "tournament" })

    return puzzles

def cache_tournament_puzzles() -> list[RawPuzzle]:
    puzzles: list[RawPuzzle] = []
    for tournament in TOURNAMENTS:
        puzzles += cache_tournament(tournament)

    return puzzles

def cache_puzzles() -> list[RawPuzzle]:
    puzzles = cache_archived_puzzles(SAMPLE_SIMPSON_URL, simpson_to_non, "sample_simpson")
    puzzles += cache_archived_puzzles(RAND30_URL, rand30_to_non, "rand30")
    puzzles += cache_tournament_puzzles()
    return puzzles

def read_non_dimensions(path: Path) -> tuple[int, int]:
    text = path.read_text()
    width = re.search(r"^width\s+(\d+)", text, re.MULTILINE)
    height = re.search(r"^height\s+(\d+)", text, re.MULTILINE)
    if width is None or height is None:
        return 0, 0
    
    return int(width.group(1)), int(height.group(1))

def add_dimensions(puzzles: list[RawPuzzle]) -> list[Puzzle]:
    enriched: list[Puzzle] = []
    for puzzle in puzzles:
        width, height = read_non_dimensions(PUZZLE_DIR / f"{puzzle['sha']}.non")
        enriched.append({ **puzzle, "width": width, "height": height })

    return enriched

def load_puzzles_index() -> list[Puzzle] | None:
    if not PUZZLE_INDEX.exists():
        return None
    with PUZZLE_INDEX.open() as f:
        return json.load(f)

def save_puzzles_index(puzzles: list[Puzzle]) -> None:
    CACHE_DIR.mkdir(parents=True, exist_ok=True)
    with PUZZLE_INDEX.open("w") as f:
        json.dump(puzzles, f)

def get_puzzles() -> list[Puzzle]:
    cached = load_puzzles_index()
    if cached is not None:
        return cached

    puzzles = add_dimensions(cache_puzzles())
    save_puzzles_index(puzzles)
    return puzzles
