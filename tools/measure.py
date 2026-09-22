"""Measure reproducible headless eight- and twelve-character expedition replays."""

from __future__ import annotations

import argparse
import json
import os
import platform
import re
import statistics
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run_tool(tool: Path, *arguments: str) -> tuple[dict, dict]:
    result = subprocess.run(
        [str(tool), *arguments, "--profile"],
        cwd=ROOT,
        capture_output=True,
        input="",
        text=True,
        check=True,
    )
    match = re.search(r"^ODR_PROFILE (\{.*\})$", result.stderr, re.MULTILINE)
    if match is None:
        raise RuntimeError("headless tool did not emit profile data")
    return json.loads(result.stdout), json.loads(match.group(1))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repeats", type=int, default=3)
    parser.add_argument("--preset", choices=("release", "dev"), default="release")
    args = parser.parse_args()
    if not 1 <= args.repeats <= 20:
        raise SystemExit("repeats must be between one and twenty")
    build = ROOT / "build" / args.preset
    tool = build / "odr_tool"
    test = build / "odr_tests"
    if not tool.exists() or not test.exists():
        raise SystemExit(f"build the {args.preset} preset before measuring")
    output: dict = {"machine": platform.platform(), "repeats": args.repeats, "parties": {}}
    for size in (8, 12):
        samples: list[dict] = []
        scenario = (
            ROOT
            / "game"
            / "scenarios"
            / ("blacksmith_mine.jsonl" if size == 8 else "blacksmith_mine_12.jsonl")
        )
        with tempfile.TemporaryDirectory(dir=build) as directory:
            save = Path(directory) / "session.json"
            for _ in range(args.repeats):
                state, profile = run_tool(tool, "--script", str(scenario), "--save", str(save))
                if state["phase"] != "city" or state["equipment"]["tier"] != 1:
                    raise RuntimeError("scenario did not complete the equipment upgrade")
                loaded, load_profile = run_tool(tool, "--load", str(save))
                if loaded != state:
                    raise RuntimeError("loaded state differs from the completed run")
                profile["load_ms"] = load_profile["load_ms"]
                samples.append(profile)
        keys = (
            "generation_ms",
            "save_ms",
            "load_ms",
            "elapsed_ms",
            "save_bytes",
            "peak_rss_bytes",
            "commands",
            "battle_commands",
            "max_battle_round",
        )
        output["parties"][str(size)] = {
            key: statistics.median(sample[key] for sample in samples) for key in keys
        }
    result = subprocess.run(
        [str(test)],
        cwd=ROOT,
        env={**os.environ, "ODR_MEASURE": "1"},
        capture_output=True,
        text=True,
        check=True,
    )
    for line in result.stdout.splitlines():
        if not line.startswith("ODR_BATTLE ") or "generated=0" not in line:
            continue
        fields = dict(part.split("=", 1) for part in line.split()[1:])
        size = fields.pop("party")
        fields.pop("generated")
        output["parties"][size]["scripted_battle"] = {
            key: float(value) if key == "elapsed_ms" else int(value)
            for key, value in fields.items()
        }
    destination = ROOT / "build" / "measurements.json"
    destination.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(output, indent=2))


if __name__ == "__main__":
    main()
