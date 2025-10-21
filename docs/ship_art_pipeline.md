# Ship Art Pipeline

Last updated: 2025-10-09

## Goals

- Encourage modular authoring so hulls, wings, engine exhausts, and interiors can be recombined at runtime.
- Provide a predictable directory structure and metadata schema that tooling can validate.
- Automate packaging into a runtime-friendly bundle that the engine can ingest alongside ship assembly data.

## Directory Layout

Authoring assets live under `assets/ship_modules`. Files are grouped by module type and separated into high-quality sources vs. engine-ready exports.

```text
ship_modules/
├── hulls/
│   ├── sources/   # Blender scenes, high-poly sculpts, texture PSDs
│   └── exports/   # Runtime meshes (glTF/OBJ) and baked textures
├── wings/
│   ├── sources/
│   └── exports/
├── exhausts/
│   ├── sources/
│   └── exports/
├── interiors/
│   ├── sources/
│   └── exports/
└── shared/
    ├── materials/ # Shared material libraries referenced by modules
    └── previews/  # Optional thumbnails for shipyard UIs
```

All new modules must be registered in `assets/ship_modules/ship_art_manifest.json` so the build script can locate their exports.

## Authoring Workflow

1. **Block out the module** in your DCC tool (Blender recommended). Align the pivot with the module's logical attachment origin.
2. **Name sockets** using empties or locators in the DCC scene so placement vectors are easy to transfer into the manifest.
3. **Create LODs** (at minimum LOD 0) and bake textures/material libraries into the `exports` subfolder. Use glTF 2.0 (`.gltf/.glb`) or Wavefront (`.obj`) for mesh data. Store shared textures or materials under `shared/materials` and reference them from the manifest.
4. **Update the manifest**: add a new module entry describing the exported files, socket bindings, dimensions, and any dependencies. See [Manifest Schema](#manifest-schema) for details.
5. **Run the pipeline** to validate the manifest and assemble the runtime bundle:

   ```powershell
   python scripts\build_ship_art.py
   ```

   Use `--dry-run` when you want validation without copying files.

6. **Confirm output** under `build/ship_art`. This bundle contains a compiled manifest (`ship_art_manifest.compiled.json`) and the per-module directory layout consumed by the engine.

## Manifest Schema

The manifest is a JSON document with the following top-level shape:

```json
{
  "version": 1,
  "modules": [ { /* module definition */ } ]
}
```

Required module fields:

- `id` *(string)* – unique identifier used by ship assembly data.
- `type` *(string)* – one of `hull`, `wing`, `exhaust`, `interior`.
- `displayName` *(string)* – user-facing name for editors/shipyard UI.
- `lods` *(array)* – list of objects containing:
  - `level` *(int)* – 0 for highest detail, increasing for lower detail.
  - `mesh` *(string)* – relative path to exported mesh.
  - `collision` *(string, optional)* – relative path to simplified collision mesh.

Common optional fields:

- `description` – free-form text for documentation.
- `materials` – array of relative paths to shared material JSON files.
- `sockets` – array describing attachment sockets (id, category, position, orientation).
- `compatibleSockets` – for wings/exhausts/interiors, enumerates socket ids they can attach to.
- `mirrorOf` – id of the opposite-side module (helpful for mirroring wings).
- `metrics` – dimensional information (length, span, volume) in meters.
- `dependencies` – other module ids that shipyard editors should co-select.
- `vfxHooks` – named anchors for engine-driven particle or light effects.
- `thumbnails` or `thumbnail` – relative path(s) to preview images in `shared/previews`.
- `extraFiles` – additional resources to copy into the bundle (animations, decals, etc.).

The build script validates paths, checks for duplicate LOD levels, and rewrites file references so runtime systems can load from the packaged bundle.

## LOD and Naming Recommendations

- Use suffixes `_lod0`, `_lod1`, etc. for exported meshes to mirror manifest levels.
- Bake collision proxies separately when hull topology is complex (`_col` suffix).
- Mirrors pair names such as `wing_port` and `wing_starboard` so the assembler can infer symmetrical relationships.
- Store reusable textures/materials in `shared/materials` and reference them from multiple modules to minimize duplication.

## Running the Pipeline

| Action | Command |
| --- | --- |
| Validate manifest only | `python scripts\build_ship_art.py --dry-run` |
| Build bundle with defaults | `python scripts\build_ship_art.py` |
| Custom output directory | `python scripts\build_ship_art.py --output-dir build\ship_art_debug` |

The script prints the number of packaged modules and writes the compiled manifest location on success. Any validation errors are emitted to stderr with actionable messages.

## Integration Notes

- `ShipAssembly` consumers can read `build/ship_art/ship_art_manifest.compiled.json` to align art modules with gameplay stats.
- Socket ids and categories should match the values used in `ShipAssembly` to guarantee compatibility checks work in editors and runtime validation.
- The pipeline keeps module directories isolated, making it straightforward to stream or hot-reload individual assemblies without rebuilding the entire bundle.

## Next Steps

- Extend the build script to generate simplified collision volumes automatically from tagged meshes.
- Add thumbnail generation by invoking Blender in headless mode to render module turntables.
- Wire the compiled manifest into the editor so artists can preview combinations directly in the shipyard UI.
