# OverclockCTL

Native GTK4 CPU/GPU overclocking, monitoring and benchmarking — Linux C++20.

---

## Install

### Linux — `install.sh`

A single self-contained script with a full GUI (zenity dialogs + progress bar):

```bash
bash install.sh
```

**What it does:**
1. Shows a welcome dialog
2. Lets you pick the install directory with a native folder picker
3. Lets you choose options (Desktop shortcut, PATH symlink, autostart)
4. Checks and installs missing pacman packages
5. Compiles the GLSL shader and builds the app
6. Installs the binary and registers `overclockctl` in your PATH
7. Creates a `.desktop` launcher so it appears in your app menu
8. Offers to launch immediately when done

After install, just type in any terminal:
```bash
overclockctl          # monitoring + benchmark
sudo overclockctl     # full OC control
```

Requires: `zenity` for GUI (comes with GNOME). Falls back to terminal if not available.

---

### Windows — `OverclockCTL-Setup.exe`

Build the `.exe` installer with NSIS:

```bash
sudo pacman -S nsis         # Arch Linux
makensis installer/windows/installer.nsi
# → installer/windows/OverclockCTL-Setup.exe
```

Or download NSIS on Windows: https://nsis.sourceforge.io/Download

**Installer wizard includes:**
- Welcome screen with description
- License agreement (MIT)
- Component selector: Core, Start Menu shortcuts, Desktop shortcut
- Directory picker with size estimate
- Animated progress bar with per-file detail
- Automatically adds install dir to system PATH so `overclockctl` works in CMD/PowerShell/Terminal
- Finish page with "Launch now" button
- Full uninstaller in Add/Remove Programs

---

## Manual Build

```bash
# Install deps (Arch Linux)
sudo pacman -S gtkmm-4.0 libadwaita vulkan-icd-loader vulkan-headers shaderc

# Build
make

# Install manually
sudo make install
```

---

## Features

| Tab | What it does |
|-----|-------------|
| ⚙ CPU | Freq sliders, governor selector, per-core bars |
| 🖥 GPU | Intel i915: max/min/boost freq sliders |
| 📊 Monitor | All hwmon temps + fan PWM control |
| 🏁 Benchmark | CPU Mandelbrot FP64 + Vulkan compute, results comparison |

---

## Project Structure

```
overclockctl/
├── install.sh                     ← Linux GUI installer (.sh)
├── installer/
│   ├── windows/installer.nsi      ← Windows NSIS script (→ .exe)
│   └── assets/                    ← icon.ico, sidebar.bmp
├── Makefile
├── LICENSE.txt
├── shaders/benchmark.comp
└── src/
    ├── main.cpp
    ├── benchmark/   CpuBenchmark, VulkanBenchmark
    ├── hardware/    CpuController, IntelGpuController, HwMonitor
    ├── ui/          MainWindow, CpuPage, GpuPage, MonitorPage, BenchmarkPage
    └── utils/       SysfsHelper, Platform.h
```
