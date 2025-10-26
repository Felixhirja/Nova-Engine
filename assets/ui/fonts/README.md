# SVG Font Assets

The SVG HUD assets depend on custom fonts. This directory stores the
TrueType/OpenType files that the software SVG rasterizer (FreeType) uses when
rendering `<text>` elements.

## Required fonts

The default HUD artwork references the following families:

- `Share Tech Mono`
- `Orbitron`
- `Rajdhani`

Download open-licensed versions of these fonts (Google Fonts provides SIL OFL
licensed variants) and copy the `.ttf` files into this directory. Update
[`fonts.manifest`](fonts.manifest) if you change filenames or introduce new
families.

The manifest format is simple:

```text
Family Name = Relative/Path/To/FontFile.ttf
```

You can list multiple aliases for the same font separated by commas, and use
the `default = family name` entry to define a fallback when an SVG requests an
unknown family.

After updating fonts or the manifest, run the packaging script to copy them
into the build output:

```powershell
python scripts/package_svg_fonts.py --output-dir build/ui/fonts
```

The FreeType integration will warn at runtime if a requested font cannot be
resolved.
