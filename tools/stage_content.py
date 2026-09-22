"""Validate versioned game data and stage it for Unreal's NonUFS packaging."""

from __future__ import annotations

import argparse
import json
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "game" / "content"
DESTINATION = ROOT / "Unreal" / "Content" / "Data"
TEMPLATES = ("mine.json", "quarry.json")
FILES = (*TEMPLATES, "visuals.json")
SCENARIO_DIR = ROOT / "game" / "scenarios"
SCENARIO_FILES = (
    "blacksmith_mine.jsonl",
    "blacksmith_mine_12.jsonl",
    "blacksmith_mine.expected.json",
)


def validate() -> None:
    visuals = json.loads((SOURCE / "visuals.json").read_text(encoding="utf-8"))
    if visuals.get("schema_version") != 1:
        raise ValueError("unsupported visual binding schema version")
    required = {"entry", "exit", "clue", "hidden_loot", "main_trigger", "ore", "secret_trigger"}
    for name in TEMPLATES:
        definition = json.loads((SOURCE / name).read_text(encoding="utf-8"))
        if definition.get("schema_version") != 2 or not definition.get("template_id"):
            raise ValueError(f"unsupported or unidentified adventure template: {name}")
        if not required.issubset(definition.get("mine", {})):
            raise ValueError(f"adventure objective bindings are incomplete: {name}")
        if not definition.get("defaults") or not definition.get("encounters"):
            raise ValueError(f"adventure defaults or encounters are missing: {name}")
    for visual_id, binding in visuals.items():
        if visual_id == "schema_version":
            continue
        if not binding.get("fallback_mesh") or not binding.get("animation_role"):
            raise ValueError(f"visual binding {visual_id} lacks fallback or animation role")
    expected = json.loads((SCENARIO_DIR / "blacksmith_mine.expected.json").read_text())
    if (
        expected.get("schema_version") != 2
        or expected.get("phase") != "city"
        or expected.get("equipment", {}).get("tier") != 1
    ):
        raise ValueError("expected scenario state does not include the completed upgrade")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--check", action="store_true", help="inspect staged bytes without changing them"
    )
    args = parser.parse_args()
    validate()
    if args.check:
        for filename in FILES:
            if (
                not (DESTINATION / filename).exists()
                or (SOURCE / filename).read_bytes() != (DESTINATION / filename).read_bytes()
            ):
                raise SystemExit(f"staged content is missing or stale: {filename}")
        for filename in SCENARIO_FILES:
            if (
                not (DESTINATION / filename).exists()
                or (SCENARIO_DIR / filename).read_bytes() != (DESTINATION / filename).read_bytes()
            ):
                raise SystemExit(f"staged scenario is missing or stale: {filename}")
        print("content staging is current")
        return
    DESTINATION.mkdir(parents=True, exist_ok=True)
    for filename in FILES:
        shutil.copyfile(SOURCE / filename, DESTINATION / filename)
    for filename in SCENARIO_FILES:
        shutil.copyfile(SCENARIO_DIR / filename, DESTINATION / filename)
    print("staged game content for Unreal")


if __name__ == "__main__":
    main()
