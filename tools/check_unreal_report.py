"""Fail CI unless Unreal exported a passing blacksmith/mine automation result."""

from __future__ import annotations

import json
import sys
from pathlib import Path


def main() -> None:
    path = Path(sys.argv[1] if len(sys.argv) > 1 else "build/reports/unreal/index.json")
    report = json.loads(path.read_text(encoding="utf-8-sig"))
    tests = [
        test for test in report.get("tests", []) if test.get("fullTestPath") == "ODR.BlacksmithMine"
    ]
    if report.get("failed") != 0 or len(tests) != 1 or tests[0].get("state") != "Success":
        raise SystemExit("Unreal blacksmith/mine automation report did not pass")
    print("Unreal blacksmith/mine automation passed")


if __name__ == "__main__":
    main()
