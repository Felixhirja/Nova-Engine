# Scripts

This directory contains helper scripts for building, testing, and running Nova-Engine locally.

## Available scripts

- `run_tests.sh`: Builds and executes the engine's automated test suite using the existing Makefile targets.
- `run_engine.bat`: Launches the prebuilt Windows executable with the expected working-directory layout.
- `build_ship_art.py`: Validates the ship art manifest and assembles distributable bundles for modular spaceship assets.
- `package_svg_fonts.py`: Copies the font files referenced by `assets/ui/fonts/fonts.manifest` into a distributable folder for HUD SVG rendering.

Feel free to add additional scripts for common workflows as the project evolves.
