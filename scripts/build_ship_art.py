#!/usr/bin/env python3
"""Build pipeline for modular spaceship art assets.

Reads the `ship_art_manifest.json` definition, validates referenced files,
then assembles a runtime-friendly asset bundle beneath the chosen output
folder. The script is intentionally lightweight so artists can run it locally
without additional dependencies.
"""

from __future__ import annotations

import argparse
import json
import shutil
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, List, Optional

MODULE_TYPES = {"hull", "wing", "exhaust", "interior"}


@dataclass
class LodEntry:
    level: int
    mesh: Path
    collision: Optional[Path]


@dataclass
class ModuleAsset:
    module_id: str
    module_type: str
    display_name: str
    source: Dict
    lods: List[LodEntry]
    materials: List[Path]
    thumbnails: List[Path]
    extra_files: List[Path]


class ValidationError(Exception):
    pass


def read_manifest(manifest_path: Path) -> Dict:
    try:
        with manifest_path.open("r", encoding="utf-8") as handle:
            data = json.load(handle)
    except FileNotFoundError as exc:
        raise ValidationError(f"Manifest not found: {manifest_path}") from exc
    except json.JSONDecodeError as exc:
        raise ValidationError(f"Manifest JSON is invalid: {exc}") from exc

    if not isinstance(data, dict):
        raise ValidationError("Manifest root must be an object")

    version = data.get("version")
    if version != 1:
        raise ValidationError(
            "Manifest version 1 expected. Update the pipeline if schema changes."
        )

    modules = data.get("modules")
    if not isinstance(modules, list) or not modules:
        raise ValidationError("Manifest requires a non-empty 'modules' array")

    return data


def ensure_relative_path(base_dir: Path, relative_str: str, field: str, module_id: str) -> Path:
    if not isinstance(relative_str, str) or not relative_str:
        raise ValidationError(f"Module '{module_id}' has invalid {field}: expected string path")

    relative_path = Path(relative_str)
    resolved = (base_dir / relative_path).resolve()
    try:
        base_dir.resolve()
    except FileNotFoundError:
        # parent directories may not exist yet, insist on their presence separately
        pass

    if not resolved.exists():
        raise ValidationError(
            f"Module '{module_id}' references missing file for {field}: {relative_path}"
        )

    try:
        resolved.relative_to(base_dir.resolve())
    except ValueError as exc:
        raise ValidationError(
            f"Module '{module_id}' {field} must stay within {base_dir}. Got: {relative_path}"
        ) from exc

    return resolved


def parse_module(defn: Dict, assets_dir: Path) -> ModuleAsset:
    if not isinstance(defn, dict):
        raise ValidationError("Each module entry must be an object")

    module_id = defn.get("id")
    if not isinstance(module_id, str) or not module_id:
        raise ValidationError("Module missing string 'id'")

    module_type = defn.get("type")
    if module_type not in MODULE_TYPES:
        raise ValidationError(
            f"Module '{module_id}' has unsupported type '{module_type}'."
            f" Allowed: {sorted(MODULE_TYPES)}"
        )

    display_name = defn.get("displayName", module_id)

    lod_raw = defn.get("lods")
    if not isinstance(lod_raw, list) or not lod_raw:
        raise ValidationError(f"Module '{module_id}' requires a non-empty 'lods' array")

    lods: List[LodEntry] = []
    seen_levels: set[int] = set()
    for idx, lod in enumerate(lod_raw):
        if not isinstance(lod, dict):
            raise ValidationError(f"Module '{module_id}' LOD #{idx} must be an object")
        level = lod.get("level")
        if not isinstance(level, int) or level < 0:
            raise ValidationError(f"Module '{module_id}' has invalid LOD level: {level}")
        if level in seen_levels:
            raise ValidationError(f"Module '{module_id}' reuses LOD level {level}")
        seen_levels.add(level)

        mesh_path = ensure_relative_path(assets_dir, lod.get("mesh"), "mesh", module_id)
        collision_path = None
        if lod.get("collision"):
            collision_path = ensure_relative_path(
                assets_dir, lod["collision"], "collision", module_id
            )
        lods.append(LodEntry(level=level, mesh=mesh_path, collision=collision_path))

    materials: List[Path] = []
    for material in defn.get("materials", []):
        materials.append(ensure_relative_path(assets_dir, material, "materials", module_id))

    thumbnails: List[Path] = []
    thumb_field = defn.get("thumbnails")
    if isinstance(thumb_field, list):
        for thumb in thumb_field:
            thumbnails.append(ensure_relative_path(assets_dir, thumb, "thumbnail", module_id))
    elif isinstance(defn.get("thumbnail"), str):
        thumbnails.append(ensure_relative_path(assets_dir, defn["thumbnail"], "thumbnail", module_id))

    extra_files: List[Path] = []
    for path in defn.get("extraFiles", []):
        extra_files.append(ensure_relative_path(assets_dir, path, "extraFiles", module_id))

    return ModuleAsset(
        module_id=module_id,
        module_type=module_type,
        display_name=display_name,
        source=defn,
        lods=lods,
        materials=materials,
        thumbnails=thumbnails,
        extra_files=extra_files,
    )


def copy_into_bundle(files: Iterable[Path], bundle_root: Path, target_subdir: Path, dry_run: bool) -> List[str]:
    copied: List[str] = []
    for src in files:
        rel_target = target_subdir / src.name
        dest = bundle_root / rel_target
        if not dry_run:
            dest.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(src, dest)
        copied.append(str(rel_target.as_posix()))
    return copied


def build_bundle(assets_dir: Path, manifest: Dict, output_dir: Path, dry_run: bool) -> Dict:
    modules_out: List[Dict] = []
    errors: List[str] = []

    for module_def in manifest["modules"]:
        try:
            module = parse_module(module_def, assets_dir)
        except ValidationError as exc:
            errors.append(str(exc))
            continue

        module_target = Path("modules") / module.module_type / module.module_id
        lod_outputs: List[Dict] = []
        for lod in module.lods:
            files_to_copy = [lod.mesh]
            if lod.collision:
                files_to_copy.append(lod.collision)
            copied = copy_into_bundle(files_to_copy, output_dir, module_target / f"lod_{lod.level}", dry_run)
            lod_entry = {
                "level": lod.level,
                "mesh": copied[0]
            }
            if lod.collision:
                lod_entry["collision"] = copied[1]
            lod_outputs.append(lod_entry)

        materials_out = copy_into_bundle(
            module.materials,
            output_dir,
            module_target / "materials",
            dry_run,
        )

        thumbs_out = copy_into_bundle(
            module.thumbnails,
            output_dir,
            module_target / "thumbnails",
            dry_run,
        )

        extras_out = copy_into_bundle(
            module.extra_files,
            output_dir,
            module_target / "extras",
            dry_run,
        )

        sanitized = {
            "id": module.module_id,
            "type": module.module_type,
            "displayName": module.display_name,
            "lods": lod_outputs,
        }

        for optional_key in (
            "description",
            "sockets",
            "interiorAnchor",
            "metrics",
            "compatibleSockets",
            "mirrorOf",
            "tags",
            "dependencies",
            "vfxHooks",
        ):
            if optional_key in module.source:
                sanitized[optional_key] = module.source[optional_key]

        if materials_out:
            sanitized["materials"] = materials_out
        if thumbs_out:
            sanitized["thumbnails"] = thumbs_out
        if extras_out:
            sanitized["extraFiles"] = extras_out

        modules_out.append(sanitized)

    if errors:
        joined = "\n".join(errors)
        raise ValidationError(f"Build failed with {len(errors)} error(s):\n{joined}")

    compiled = {
        "version": manifest["version"],
        "generated": True,
        "modules": modules_out,
    }

    if not dry_run:
        output_dir.mkdir(parents=True, exist_ok=True)
        manifest_out = output_dir / "ship_art_manifest.compiled.json"
        with manifest_out.open("w", encoding="utf-8") as handle:
            json.dump(compiled, handle, indent=2, sort_keys=False)
            handle.write("\n")

    return compiled


def parse_args(argv: Optional[List[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Build the modular ship art bundle")
    parser.add_argument(
        "--assets-dir",
        type=Path,
        default=Path("assets/ship_modules"),
        help="Root directory containing the raw ship module assets",
    )
    parser.add_argument(
        "--manifest",
        type=Path,
        default=None,
        help="Optional manifest path (defaults to assets-dir/ship_art_manifest.json)",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("build/ship_art"),
        help="Directory where the packaged assets should be written",
    )
    parser.add_argument(
        "--dry-run",
        action="store_true",
        help="Validate manifest and report actions without copying files",
    )
    return parser.parse_args(argv)


def main(argv: Optional[List[str]] = None) -> int:
    args = parse_args(argv)
    assets_dir = args.assets_dir
    manifest_path = args.manifest or (assets_dir / "ship_art_manifest.json")

    try:
        manifest = read_manifest(manifest_path)
        compiled = build_bundle(assets_dir, manifest, args.output_dir, args.dry_run)
    except ValidationError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    print(f"Packaged {len(compiled['modules'])} module(s) into {args.output_dir}")
    if args.dry_run:
        print("Dry-run complete. No files were written.")
    else:
        manifest_out = args.output_dir / "ship_art_manifest.compiled.json"
        print(f"Wrote compiled manifest: {manifest_out}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
