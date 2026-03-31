#!/usr/bin/env python3
import argparse
import json
import time
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def count_lines(path: Path) -> int:
    return sum(1 for _ in path.read_text(encoding="utf-8").splitlines() if _.strip())


def main() -> int:
    parser = argparse.ArgumentParser(description="Measure codegen size from assembly outputs")
    parser.add_argument(
        "--assembly-dir",
        default="docs/assembly",
        help="Directory containing *_with.asm and *_without.asm",
    )
    parser.add_argument(
        "--out",
        help="Output JSON path (default: tmp/bench/codegen-size-<timestamp>.json)",
    )
    parser.add_argument(
        "--jsonl",
        help="Optional JSONL path for per-entry append (default: tmp/bench/codegen-size.jsonl)",
    )
    args = parser.parse_args()

    root = repo_root()
    assembly_dir = (root / args.assembly_dir).resolve()
    if not assembly_dir.exists():
        print(f"assembly dir not found: {assembly_dir}")
        return 2

    run_id = time.strftime("%Y%m%d-%H%M%S")
    out_path = Path(args.out) if args.out else root / "tmp" / "bench" / f"codegen-size-{run_id}.json"
    jsonl_path = Path(args.jsonl) if args.jsonl else root / "tmp" / "bench" / "codegen-size.jsonl"
    out_path.parent.mkdir(parents=True, exist_ok=True)
    jsonl_path.parent.mkdir(parents=True, exist_ok=True)

    entries = []

    for with_path in assembly_dir.rglob("*_with.asm"):
        without_path = Path(str(with_path).replace("_with.asm", "_without.asm"))
        if not without_path.exists():
            continue
        with_lines = count_lines(with_path)
        without_lines = count_lines(without_path)
        delta = with_lines - without_lines
        percent = (delta / without_lines * 100.0) if without_lines else None

        record = {
            "name": with_path.stem.replace("_with", ""),
            "with": str(with_path.relative_to(root)),
            "without": str(without_path.relative_to(root)),
            "with_lines": with_lines,
            "without_lines": without_lines,
            "delta_lines": delta,
            "delta_percent": percent,
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
        }
        entries.append(record)

        with jsonl_path.open("a", encoding="utf-8") as handle:
            handle.write(json.dumps(record, ensure_ascii=False) + "\n")

    payload = {
        "run_id": run_id,
        "assembly_dir": str(assembly_dir.relative_to(root)) if assembly_dir.is_relative_to(root) else str(assembly_dir),
        "entries": entries,
    }
    out_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    print(f"Saved: {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
