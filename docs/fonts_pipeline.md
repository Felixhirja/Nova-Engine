# HUD Font Pipeline

This document summarizes how vector HUD assets load their fonts and how to
package those fonts for builds.

## Overview

- HUD SVGs use `<text>` elements extensively (e.g. `assets/ui/player_hud.svg`).
- `src/SVGSurfaceLoader.cpp` now renders text via FreeType when `USE_FREETYPE`
  is available during the build.
- Font discovery happens through the manifest at
  `assets/ui/fonts/fonts.manifest`. Each entry maps one or more CSS
  `font-family` names to a `.ttf`/`.otf` file.
- The SVG loader searches for that manifest relative to the SVG (current
  directory, `fonts/` subdirectory, parent `fonts/`, or the path specified by
  the `NOVA_SVG_FONT_MANIFEST` environment variable).

## Adding or updating fonts

1. Download the desired font files (ensure they are licensed for redistribution).
2. Copy them into `assets/ui/fonts/` alongside `fonts.manifest`.
3. Edit the manifest so the family names listed in SVGs resolve to the new
   files. Example:

   ```text
   Orbitron = Orbitron-Regular.ttf
   Share Tech Mono = ShareTechMono-Regular.ttf
   default = Share Tech Mono
   ```

4. Run the packaging script to mirror fonts into the build output directory:

   ```powershell
   python scripts/package_svg_fonts.py --output-dir build/ui/fonts
   ```

   Use `--dry-run` to verify the manifest without copying files.

5. Commit the updated manifest and any newly added fonts (confirm licenses
   allow checked-in distribution). The script emits warnings if a referenced
   file cannot be found.

## Build integration

- The Makefile auto-enables FreeType support when `pkg-config freetype2` is
  available (Linux/macOS) or when the MSYS2 include/lib paths are present on
  Windows. If FreeType cannot be located the build proceeds, but SVG text
  rendering falls back to silent omission and the loader logs warnings at
  runtime.
- Packaged builds should include the `build/ui/fonts/` directory produced by
  `package_svg_fonts.py` so shipped binaries find the required `.ttf` files.
- Developers can override the manifest location by setting the
  `NOVA_SVG_FONT_MANIFEST` environment variable to an absolute path.
