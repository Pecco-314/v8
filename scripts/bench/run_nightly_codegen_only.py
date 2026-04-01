#!/usr/bin/env python3
import subprocess
import sys
from pathlib import Path


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def main() -> int:
    root = repo_root()
    runner = root / "scripts" / "bench" / "run_nightly.py"
    command = [
        sys.executable,
        str(runner),
        "--dimensions",
        "binary_codegen",
        "--results-jsonl",
        "tmp/bench/nightly-codegen-results.jsonl",
        "--run-log",
        "tmp/bench/nightly-codegen.log",
        *sys.argv[1:],
    ]
    result = subprocess.run(command)
    return result.returncode


if __name__ == "__main__":
    raise SystemExit(main())
