# Ship Module Asset Library

This directory stores modular art assets for the Star-Engine spaceship system. Assets are organized by module type so hulls, wings, engine exhausts, and interiors can be mixed and matched when building ships.

```text
ship_modules/
├── hulls/
│   ├── sources/   # Authoring files (Blender, high-poly meshes, texture PSDs)
│   └── exports/   # Game-ready exports (glTF/OBJ, baked textures)
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
    ├── materials/ # Shared material/texture libraries referenced by modules
    └── previews/  # Generated thumbnails for tooling and documentation
```

Use the `ship_art_manifest.json` at the root of this folder to register each module. The build pipeline validates the manifest and packages assets into the runtime structure expected by the engine.
