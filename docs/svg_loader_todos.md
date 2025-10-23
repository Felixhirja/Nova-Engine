# SVG Loader TODOs

This document summarizes the current limitations of `src/SVGSurfaceLoader.cpp`, prioritized tasks to improve fidelity for HUD assets, and implementation notes / quick tests.

## Current status (Oct 23, 2025)

- The loader supports basic shapes (rect, circle, ellipse, polygon, polyline, line) and path commands M/L/H/V/C/S/Q/T/Z/A (arc converted to cubic Bézier segments).
- Supports inline `style` attributes and basic `.class { ... }` style blocks for fill, fill-opacity, and opacity.
- Parses solid color fills (hex, rgb, rgba, basic named colors).
- Added (recent): basic stroke parsing (`stroke`, `stroke-width`) and a simple stroke rasterization pass.
- Added (recent): minimal `<defs>`/`<use>` support for resolving `<path id="...">` and basic `<use href="#id" x="..." y="..." />` with offset.

## Known limitations

1. Gradients: `linearGradient` and `radialGradient` are not supported. Elements using `fill="url(#...)"` will be rendered with a solid fallback or not rendered as intended.
2. Filters: `feGaussianBlur`, `feColorMatrix`, `feMerge` and other filters are ignored. Glow and blur effects will not appear.
3. Strokes: basic stroke is implemented, but joins, caps, dashed patterns, and stroke alignment (inside/center/outside) are not handled.
4. Text: `<text>` elements are not rendered. Text in HUD SVGs must be rasterized or drawn with engine text overlays.
5. `<use>` resolution: only references to `<path id=...>` collected from the same document are supported, and only x/y offsets are applied; transforms are not resolved.
6. Performance: stroke rasterization checks every pixel against every segment; this is slow for large images.

## Prioritized TODO list

High priority

- [ ] Add support for gradients (linear and radial). Many HUDs depend on subtle gradients for panel depth. Approach: parse `\<linearGradient\>`/`\<radialGradient\>`, evaluate color stops, and apply per-scanline interpolation during rasterization or generate a temporary gradient texture and composite.

- [ ] Improve `\<use\>` handling and `\<defs\>`: support referencing not only `\<path\>` but `\<g\>`/`\<rect\>`/`circle`, and apply transform attributes on both the referenced element and the `\<use\>` itself.


Medium priority

- [ ] Implement feGaussianBlur and basic filter chain handling (feMerge). Use an intermediate surface and separable Gaussian blur for performance.

- [ ] Implement strokes more accurately: support stroke-linejoin, stroke-linecap, miter limits, and dashed strokes.

- [ ] Implement text fallback: either rasterize text using a font rasterizer library or map SVG `\<text\>` to the engine TextRenderer (preferred for HUDs because the engine already renders text).


Low priority

- [x] Proper arc support for `A` commands (convert to cubic segments instead of straight line fallback).

- [ ] Optimize rasterization: bounding-box clipping, segment spatial partitioning (tile grid), and early-out checks to reduce per-pixel work.

- [ ] Consider replacing software rasterizer with a native SVG rendering library (librsvg, Skia, Cairo) for correctness if build-time dependencies are acceptable.

## Implementation notes and small tests

- Gradients: store gradient definitions from `<defs>`, parse `gradientTransform`, and on rasterization compute a parameter `t` (0..1) per pixel and interpolate between color stops. For radial gradients compute radial `t` using distance from focal/center.
- Filters (blur): rasterize shape to an intermediate RGBA buffer then run a separable Gaussian blur on that buffer; then composite back using alpha.
- Stroke performance: before running distance checks, compute a conservative expanded bounding box for each subpath (path bbox expanded by stroke width) and only evaluate pixels inside those boxes.

Quick test cases (add to `artifacts/diagnostics`):

- `svg_tests/gradient_test.svg` — simple rectangle with linear and radial gradients and stops at multiple offsets.

- `svg_tests/filter_blur_test.svg` — object with gaussian blur to verify filter pipeline.

- `svg_tests/use_test.svg` — defines a `\<path id="tick"\>` and uses `\<use\>` multiple times with offsets and transforms.

## Suggested workflow

1. For immediate visual parity, export HUD SVGs to PNGs at desired resolutions and use the PNG assets in-engine.
2. For long-term fidelity, implement gradient + improved defs/use support first, then tackle filters.
3. Add unit tests that compare produced PNGs of small test SVGs against reference images (pixel tolerant threshold) to avoid regressions.

## Files touched during investigation

- `assets/ui/spaceship_hud.svg` — main HUD vector asset used for testing.

- `src/SVGSurfaceLoader.cpp` — loader implementation (recently extended with stroke and minimal use/defs support).

- `artifacts/svg_preview/spaceship_hud_preview.html` — quick browser preview of the HUD.

## Next small tasks (pick one to start)

- [ ] Implement gradient fills (recommended next step)

- [ ] Implement feGaussianBlur (if you prefer glow fidelity first)

- [ ] Improve `\<use\>` transform handling and support `\<g\>` reference

- [ ] Add export script and example pre-rasterized PNGs

If you'd like, I can start with the gradient implementation now and make small, tested commits. If you prefer a different priority, tell me which item to tackle first.


