#!/usr/bin/env python3
"""Package HUD font assets for distribution.

The SVG loader expects fonts declared in ``assets/ui/fonts/fonts.manifest``.
This helper copies the referenced font files into an output directory so they
can be shipped alongside prebuilt binaries.
"""

from __future__ import annotations

import argparse
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, List


@dataclass
class ManifestEntry:
    family: str
    font_path: Path


def parse_manifest(path: Path) -> List[ManifestEntry]:
    if not path.exists():
        raise FileNotFoundError(f"manifest not found: {path}")

    entries: List[ManifestEntry] = []
    with path.open("r", encoding="utf-8") as handle:
        for raw_line in handle:
            line = raw_line.strip()
            if not line or line.startswith("#"):
                continue
            delimiter = "=" if "=" in line else ":"
            if delimiter not in line:
                print(f"warning: skipping malformed line: {raw_line.rstrip()}", file=sys.stderr)
                continue
            left, right = line.split(delimiter, 1)
            families = [alias.strip(" \"'\t").lower() for alias in left.split(",")]
            target = right.strip().strip("\"'")
            if not target:
                continue
            font_path = (path.parent / target).resolve()
            for family in families:
                if family == "default":
                    # default entry acts as fallback only; skip copying
                    continue
                if not font_path.exists():
                    print(f"warning: font file missing for '{family}': {font_path}", file=sys.stderr)
                    continue
                entries.append(ManifestEntry(family=family, font_path=font_path))
    if not entries:
        raise RuntimeError("manifest did not contain any valid font entries")
    return entries


def package_fonts(manifest: Path, output_dir: Path, dry_run: bool) -> None:
    entries = parse_manifest(manifest)
    copied: Dict[Path, Path] = {}

    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)

    for entry in entries:
        dest = output_dir / entry.font_path.name
        if dry_run:
            print(f"copy {entry.font_path} -> {dest}")
        else:
            shutil.copy2(entry.font_path, dest)
        copied[entry.font_path] = dest

    print(f"Packaged {len(copied)} font file(s) into {output_dir}")
    if dry_run:
        print("Dry-run: no files were copied.")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Copy SVG font assets to a build directory")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=Path("assets/ui/fonts/fonts.manifest"),
        help="Path to the font manifest file",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("build/ui/fonts"),
        help="Destination directory for packaged fonts",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate and report actions without copying files",
    )
    return parser


def main(argv: List[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        package_fonts(args.manifest, args.output_dir, args.dry_run)
    except Exception as exc:  # pylint: disable=broad-except
        print(f"error: {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
