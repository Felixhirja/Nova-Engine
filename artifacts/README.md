# Artifacts

This directory collects generated assets and diagnostic data that are useful for
reference but not required to build the engine.

## Directory layout

- `artifacts/screenshots/` – captured images that illustrate different engine views and HUD states.
- `artifacts/captures/` – raw capture dumps (such as `.xwd` files and render snapshots) used for debugging.
- `artifacts/diagnostics/` – log files, profiler output, build transcripts, and other investigative reports.

Screenshots, capture dumps, and diagnostics used to live in separate top-level
folders. Relocating them underneath `artifacts/` keeps the repository root
focused on the source code while still making historical data easy to find.
