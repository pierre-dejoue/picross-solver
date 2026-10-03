from src.cache_puzzles import get_puzzles

def main() -> None:
    puzzles = get_puzzles()
    print(len(puzzles))

if __name__ == "__main__":
    main()
