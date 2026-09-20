"""Check local Markdown targets; external links are handled by source review."""

from __future__ import annotations

import re
from pathlib import Path
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]
LINK = re.compile(r"!?\[[^\]]*\]\((<[^>]+>|[^)]+)\)")


def main() -> None:
    failures: list[str] = []
    pages = [ROOT / "README.md", *sorted((ROOT / "docs").rglob("*.md"))]
    for page in pages:
        if page.name.startswith("._"):
            continue
        contents = page.read_text(encoding="utf-8")
        for match in LINK.finditer(contents):
            raw = match.group(1).strip("<>").split(" ", 1)[0]
            if raw.startswith("#"):
                continue
            parsed = urlsplit(raw)
            if parsed.scheme or raw.startswith("//"):
                continue
            destination = (page.parent / unquote(parsed.path)).resolve()
            if not destination.is_relative_to(ROOT) or not destination.exists():
                line = contents.count("\n", 0, match.start()) + 1
                failures.append(f"{page.relative_to(ROOT)}:{line}: {raw}")
    if failures:
        raise SystemExit("Broken local links:\n" + "\n".join(failures))
    print("local documentation links resolve")


if __name__ == "__main__":
    main()
