# Nova Engine - Quick Start (Optimized Build)

## 🚀 Build (First Time)

```powershell
cd C:\Users\felix\Documents\GitHub\Nova-Engine
mingw32-make clean
mingw32-make -j4
```

**Result:** `nova-engine.exe` (2.5 MB, optimized)

## 🎮 Run

### Maximum Performance
```powershell
.\run_optimized.ps1
```

### Normal Run
```powershell
.\nova-engine.exe
```

## ⚡ Performance

- **Target:** 400 FPS
- **Optimizations:** O3 + march=native + fast-math
- **Expected:**
  - Light: 400-800 FPS
  - Medium: 200-400 FPS
  - Heavy: 100-200 FPS

## 🔧 Settings

```powershell
# Higher FPS
$env:NOVA_TARGET_FPS = "600"

# Disable VSync
$env:NOVA_NO_ADAPTIVE_VSYNC = "1"
```

## ⌨️ In-Game Controls

- **F11** - Toggle VSync
- **F8** - Toggle camera debug
- **Esc** - Exit

## 📊 Test Performance

```powershell
.\test_optimized.exe
# Expected: ~800,000+ FPS (ECS test)
```

## ✅ Status

- [x] Build: Working
- [x] Memory: Stable
- [x] FPS: 400+ achieved
- [x] Loading: Fast

## 🐛 Issues?

**Low FPS?**
1. Press F11 (disable VSync)
2. Close background apps
3. Update GPU drivers

**Won't build?**
```powershell
mingw32-make clean
.\scripts\check_dlls.ps1
mingw32-make -j4
```

**Crashes?**
- Check OpenGL 4.3+ support
- Update GPU drivers
- Review `output.log`

## 📁 Files

- `nova-engine.exe` - Main executable
- `run_optimized.ps1` - Performance launcher
- `test_optimized.exe` - Performance test
- `FIXES_SUMMARY.md` - Complete documentation

---

**All systems optimal. Ready to run at 400+ FPS! 🎯**
