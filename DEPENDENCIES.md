# Dependency and Runtime Requirements

This document tracks the third-party packages, runtime DLLs, and supporting
utilities needed to build and run **Nova-Engine** across supported platforms.
It complements the high-level setup instructions in `README.md`.

## Runtime DLL checklist

The engine ships with a Windows toolchain and relies on the following runtime
libraries. Use the `scripts/check_dlls.ps1` helper before every build or launch
on Windows to ensure the DLLs are present in the project root:

| DLL | Purpose | Required | Supplied by |
| --- | --- | :---: | --- |
| `glfw3.dll` | GLFW windowing/runtime | ✅ | `mingw-w64-x86_64-glfw`
| `libfreeglut.dll` | OpenGL utility helpers | ✅ | `mingw-w64-x86_64-freeglut`
| `libgcc_s_seh-1.dll` | MinGW runtime support | ✅ | `mingw-w64-x86_64-gcc`
| `libstdc++-6.dll` | C++ standard library | ✅ | `mingw-w64-x86_64-gcc`
| `libwinpthread-1.dll` | POSIX threading layer | ✅ | `mingw-w64-x86_64-gcc`
| `xinput1_4.dll` | Optional XInput gamepad support | ⚪️ | Windows SDK / DirectX redistributables

Running the script copies any missing DLLs from your MSYS2 installation to the
project directory and reports unresolved dependencies.

```powershell
pwsh -File scripts/check_dlls.ps1
```

You can override discovery paths when necessary:

```powershell
pwsh -File scripts/check_dlls.ps1 -ProjectRoot C:\path\to\Nova-Engine -MsysBinDir C:\msys64\mingw64\bin
```

Set the `MSYS2_MINGW64_BIN` environment variable to avoid passing
`-MsysBinDir` explicitly.

## MSYS2 / MinGW packages

Install the following packages from an MSYS2 MinGW 64-bit shell. Listed versions
reflect the latest tested release; newer patch releases are expected to work.

| Package | Version | Purpose |
| --- | --- | --- |
| `mingw-w64-x86_64-gcc` | 13.2.0 | C++17 compiler, libstdc++, libgcc, libwinpthread |
| `mingw-w64-x86_64-glfw` | 3.4.0 | Windowing, input |
| `mingw-w64-x86_64-freeglut` | 3.4.0 | GLUT utility library |
| `mingw-w64-x86_64-glu` | 9.0.2 | OpenGL Utility Library |
| `mingw-w64-x86_64-pkg-config` | 0.29.2 | Optional, aids native discovery |

Update the package database and install everything in one go:

```bash
pacman -Syu \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-glfw \
  mingw-w64-x86_64-freeglut \
  mingw-w64-x86_64-glu \
  mingw-w64-x86_64-pkg-config
```

## Troubleshooting

- **DLL still missing after running the checker** – Confirm that `MSYS2_MINGW64_BIN`
  points to the `mingw64\bin` directory in your MSYS2 installation. Reinstall the
  package that owns the DLL (`pacman -S mingw-w64-x86_64-<package>`).
- **Architecture mismatch errors** – Ensure you are using the 64-bit (`mingw64`)
  shell. Mixing 32-bit binaries introduces unresolved references at runtime.
- **Unsigned DLL warnings from Windows SmartScreen** – The DLLs copied from MSYS2
  are unsigned. When distributing builds, replace them with official releases or
  sign them yourself to avoid prompts.
- **XInput not detected** – `xinput1_4.dll` ships with modern Windows versions.
  On Windows 7, install the DirectX End-User Runtime to obtain the legacy
  `xinput1_3.dll` and update the loader accordingly.

## Alternative download sources

When MSYS2 is unavailable, you can obtain compatible binaries from:

- [GLFW official downloads](https://www.glfw.org/download.html)
- [FreeGLUT releases](https://www.transmissionzero.co.uk/software/freeglut-devel/)
- [MinGW-w64 standalone builds](https://winlibs.com/)
- [Microsoft DirectX redistributables](https://www.microsoft.com/en-us/download/details.aspx?id=35)

Copy the DLLs into the repository root and re-run `scripts/check_dlls.ps1` to
verify the environment before executing the engine.

## Evaluating dependency managers

We are assessing C++ package managers to reduce manual DLL tracking:

- **vcpkg** – Provides curated ports for GLFW, GLM, OpenAL, and other common
  graphics dependencies. Good Visual Studio and CMake integration, but MinGW
  support is community maintained.
- **Conan** – Flexible build system agnostic package manager with binary caching.
  Supports custom profiles for MSYS2 and cross-compilation scenarios.

### Next steps

1. Prototype a `vcpkg` integration targeting Windows + MSYS2 to confirm MinGW
   compatibility and evaluate binary size impact.
2. Create a Conan profile mirroring the existing toolchain and test fetching
   GLFW and future audio libraries.
3. Compare workflow friction (bootstrap time, cache sizes, CI integration) and
   adopt the solution that best balances portability and maintenance overhead.
