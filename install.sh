#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
#  OverclockCTL — Linux GUI Installer
#  Requires: zenity (for GUI dialogs)  |  fallback: terminal mode
#  Usage:    bash install.sh
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

APP_NAME="OverclockCTL"
APP_BIN="overclockctl"
APP_VERSION="1.0.0"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ── GUI abstraction (zenity → yad → terminal fallback) ────────────────────────
HAS_GUI=false
GUI_TOOL=""

if command -v zenity &>/dev/null && [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
    HAS_GUI=true; GUI_TOOL="zenity"
elif command -v yad &>/dev/null && [ -n "${DISPLAY:-}${WAYLAND_DISPLAY:-}" ]; then
    HAS_GUI=true; GUI_TOOL="yad"
fi

# Pipe FD for zenity progress
PROGRESS_PID=""
PROGRESS_PIPE=""

gui_info() {
    # $1 = title, $2 = message
    if $HAS_GUI; then
        zenity --info --title="$1" --text="$2" --width=420 2>/dev/null || true
    else
        echo "[INFO] $2"
    fi
}

gui_error() {
    if $HAS_GUI; then
        zenity --error --title="$1" --text="$2" --width=420 2>/dev/null || true
    else
        echo "[ERROR] $2" >&2
    fi
    exit 1
}

gui_question() {
    # returns 0 = Yes, 1 = No
    if $HAS_GUI; then
        zenity --question --title="$1" --text="$2" --width=420 2>/dev/null
    else
        read -r -p "$2 [y/N] " ans
        [[ "${ans,,}" == "y" ]]
    fi
}

gui_choose_dir() {
    # $1 = default path  →  echoes chosen path
    if $HAS_GUI; then
        zenity --file-selection \
               --directory \
               --title="Choose install directory" \
               --filename="$1/" \
               2>/dev/null || echo "$1"
    else
        read -r -p "Install directory [$1]: " ans
        echo "${ans:-$1}"
    fi
}

gui_checklist() {
    # Returns newline-separated selected items
    if $HAS_GUI; then
        zenity --list \
               --checklist \
               --title="$APP_NAME — Installation Options" \
               --text="Select optional features:" \
               --column="" --column="Option" --column="Description" \
               --width=560 --height=300 \
               TRUE  "desktop"  "Create Desktop shortcut (.desktop file)" \
               TRUE  "path"     "Add to PATH via /usr/local/bin symlink" \
               FALSE "autostart" "Launch on login (XDG autostart)" \
               --separator=$'\n' 2>/dev/null || true
    else
        echo "desktop"
        echo "path"
    fi
}

# ── Open a zenity progress window, write to its stdin via a named pipe ─────────
start_progress() {
    # $1 = window title
    if $HAS_GUI; then
        PROGRESS_PIPE=$(mktemp -u /tmp/oc_progress_XXXXXX)
        mkfifo "$PROGRESS_PIPE"
        zenity --progress \
               --title="$1" \
               --text="Starting…" \
               --percentage=0 \
               --width=540 \
               --auto-close \
               --no-cancel \
               < "$PROGRESS_PIPE" &
        PROGRESS_PID=$!
        # Open pipe for writing (keep it open so zenity doesn't exit)
        exec 9>"$PROGRESS_PIPE"
    fi
}

progress() {
    # $1 = 0-100  $2 = message
    local pct="$1"
    local msg="$2"
    if $HAS_GUI && [ -n "$PROGRESS_PID" ]; then
        echo "$pct"      >&9
        echo "# $msg"    >&9
    fi
    echo "  [${pct}%] $msg"
}

finish_progress() {
    if $HAS_GUI && [ -n "$PROGRESS_PID" ]; then
        echo "100"        >&9
        echo "# Done!"    >&9
        exec 9>&-
        wait "$PROGRESS_PID" 2>/dev/null || true
        rm -f "$PROGRESS_PIPE"
        PROGRESS_PID=""
    fi
}

# ── Privilege helper ──────────────────────────────────────────────────────────
run_privileged() {
    # Run a command with root. Use pkexec if available, else sudo.
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif command -v pkexec &>/dev/null; then
        pkexec "$@"
    else
        sudo "$@"
    fi
}

# ── Pacman package installer ──────────────────────────────────────────────────
ensure_packages() {
    local missing=()
    local pkg_map=(
        "gtkmm-4.0:gtkmm-4.0"
        "libadwaita-1:libadwaita"
        "vulkan:vulkan-icd-loader"
    )

    for pair in "${pkg_map[@]}"; do
        local pc="${pair%%:*}"
        local pkg="${pair##*:}"
        if ! pkg-config --exists "$pc" 2>/dev/null; then
            missing+=("$pkg")
        fi
    done

    if ! command -v glslc &>/dev/null && ! command -v glslangValidator &>/dev/null; then
        missing+=("shaderc")
    fi

    if [ ${#missing[@]} -eq 0 ]; then
        progress 15 "All dependencies satisfied ✔"
        return 0
    fi

    progress 10 "Installing missing packages: ${missing[*]}"

    local pm_cmd="pacman -S --noconfirm --needed ${missing[*]}"
    if $HAS_GUI; then
        local pkg_list
        pkg_list=$(printf '%s\n' "${missing[@]}")
        zenity --question \
               --title="Missing Dependencies" \
               --text="The following packages need to be installed:\n\n<tt>${pkg_list}</tt>\n\nInstall them now?" \
               --width=440 2>/dev/null || { gui_error "Aborted" "Cannot continue without required packages."; }
    fi

    run_privileged pacman -S --noconfirm --needed "${missing[@]}" 2>&1 | \
        while IFS= read -r line; do progress 12 "$line"; done

    progress 15 "Dependencies installed ✔"
}

# ─────────────────────────────────────────────────────────────────────────────
#  MAIN
# ─────────────────────────────────────────────────────────────────────────────

# Welcome dialog
if $HAS_GUI; then
    zenity --info \
           --title="$APP_NAME $APP_VERSION Installer" \
           --text="<big><b>⚡ Welcome to $APP_NAME $APP_VERSION</b></big>\n\nGTK4 CPU/GPU overclocking, monitoring and benchmarking.\n\nThis installer will:\n  • Check and install dependencies\n  • Build the application\n  • Install to your chosen directory\n  • Create a desktop shortcut\n  • Register <tt>overclockctl</tt> in your PATH\n\nClick OK to continue." \
           --width=460 2>/dev/null
fi

# ── Choose install directory ──────────────────────────────────────────────────
DEFAULT_DIR="$HOME/.local/bin"
if [ "$(id -u)" -eq 0 ]; then DEFAULT_DIR="/usr/local/bin"; fi

INSTALL_DIR=$(gui_choose_dir "$DEFAULT_DIR")
INSTALL_DIR="${INSTALL_DIR%/}"   # strip trailing slash

if [ -z "$INSTALL_DIR" ]; then
    gui_error "Cancelled" "No install directory selected. Aborting."
fi

# ── Choose options ────────────────────────────────────────────────────────────
OPTIONS=$(gui_checklist)
DO_DESKTOP=false;   echo "$OPTIONS" | grep -q "desktop"   && DO_DESKTOP=true
DO_PATH=false;      echo "$OPTIONS" | grep -q "path"      && DO_PATH=true
DO_AUTOSTART=false; echo "$OPTIONS" | grep -q "autostart" && DO_AUTOSTART=true

# ── Open progress window ──────────────────────────────────────────────────────
start_progress "Installing $APP_NAME $APP_VERSION"

progress 5  "Checking dependencies…"
ensure_packages

# ── Build ─────────────────────────────────────────────────────────────────────
progress 20 "Compiling GLSL compute shader…"
cd "$SCRIPT_DIR"
if command -v glslc &>/dev/null; then
    glslc -fshader-stage=compute shaders/benchmark.comp \
          -o shaders/benchmark.spv 2>&1 || true
elif command -v glslangValidator &>/dev/null; then
    glslangValidator -V shaders/benchmark.comp \
                     -o shaders/benchmark.spv 2>&1 || true
else
    progress 22 "  ⚠ No GLSL compiler found — Vulkan benchmark will be skipped"
fi
progress 25 "Shader compiled ✔"

progress 30 "Building $APP_NAME (this may take a minute)…"
CORES=$(nproc)
# Stream make output to progress messages
make -j"$CORES" 2>&1 | while IFS= read -r line; do
    # Pick a file compiled and bump progress slightly
    progress 35 "$line"
done
progress 65 "Build complete ✔"

# ── Install binary ────────────────────────────────────────────────────────────
progress 68 "Creating install directory: $INSTALL_DIR"
mkdir -p "$INSTALL_DIR" 2>/dev/null || run_privileged mkdir -p "$INSTALL_DIR"

progress 72 "Installing binary → $INSTALL_DIR/$APP_BIN"
if cp "$APP_BIN" "$INSTALL_DIR/$APP_BIN" 2>/dev/null; then
    chmod 755 "$INSTALL_DIR/$APP_BIN"
else
    run_privileged cp "$APP_BIN" "$INSTALL_DIR/$APP_BIN"
    run_privileged chmod 755 "$INSTALL_DIR/$APP_BIN"
fi

# ── Install shader data ───────────────────────────────────────────────────────
SHADER_DIR="/usr/share/overclockctl"
progress 75 "Installing shader data → $SHADER_DIR"
run_privileged mkdir -p "$SHADER_DIR"
run_privileged cp shaders/benchmark.spv "$SHADER_DIR/benchmark.spv" 2>/dev/null || true

# ── PATH symlink ──────────────────────────────────────────────────────────────
if $DO_PATH; then
    progress 78 "Creating PATH symlink → /usr/local/bin/$APP_BIN"
    run_privileged ln -sf "$INSTALL_DIR/$APP_BIN" "/usr/local/bin/$APP_BIN"
    progress 80 "  ✔ 'overclockctl' now available system-wide"
fi

# ── Desktop shortcut ──────────────────────────────────────────────────────────
if $DO_DESKTOP; then
    progress 83 "Creating desktop shortcut…"

    DESKTOP_DIR="$HOME/.local/share/applications"
    mkdir -p "$DESKTOP_DIR"

    cat > "$DESKTOP_DIR/overclockctl.desktop" <<DESKTOP
[Desktop Entry]
Version=1.0
Type=Application
Name=OverclockCTL
GenericName=Overclocking Tool
Comment=CPU/GPU Overclocking, Monitoring and Benchmarking
Exec=pkexec $INSTALL_DIR/$APP_BIN
Icon=overclockctl
Categories=System;Monitor;HardwareSettings;
Keywords=overclock;cpu;gpu;benchmark;monitor;fan;
Terminal=false
StartupNotify=true
StartupWMClass=overclockctl
DESKTOP
    chmod 644 "$DESKTOP_DIR/overclockctl.desktop"

    # Also place on actual Desktop folder if it exists
    REAL_DESKTOP="$HOME/Desktop"
    if [ -d "$REAL_DESKTOP" ]; then
        cp "$DESKTOP_DIR/overclockctl.desktop" "$REAL_DESKTOP/overclockctl.desktop"
        chmod +x "$REAL_DESKTOP/overclockctl.desktop"
    fi

    # Refresh desktop DB
    update-desktop-database "$DESKTOP_DIR" 2>/dev/null || true

    progress 87 "Desktop shortcut created ✔"
fi

# ── XDG autostart ─────────────────────────────────────────────────────────────
if $DO_AUTOSTART; then
    progress 90 "Setting up autostart…"
    AUTOSTART_DIR="$HOME/.config/autostart"
    mkdir -p "$AUTOSTART_DIR"
    cp "$HOME/.local/share/applications/overclockctl.desktop" \
       "$AUTOSTART_DIR/overclockctl.desktop"
    progress 92 "Autostart entry created ✔"
fi

# ── Write uninstaller ─────────────────────────────────────────────────────────
progress 94 "Writing uninstaller → $INSTALL_DIR/uninstall-overclockctl.sh"
cat > "$INSTALL_DIR/uninstall-overclockctl.sh" <<UNINSTALL
#!/usr/bin/env bash
echo "Uninstalling $APP_NAME..."
rm -f "$INSTALL_DIR/$APP_BIN"
rm -f "$INSTALL_DIR/uninstall-overclockctl.sh"
rm -f "/usr/local/bin/$APP_BIN"
rm -f "$HOME/.local/share/applications/overclockctl.desktop"
rm -f "$HOME/Desktop/overclockctl.desktop"
rm -f "$HOME/.config/autostart/overclockctl.desktop"
sudo rm -rf /usr/share/overclockctl
echo "$APP_NAME has been uninstalled."
UNINSTALL
chmod +x "$INSTALL_DIR/uninstall-overclockctl.sh"

# ── Verify ────────────────────────────────────────────────────────────────────
progress 97 "Verifying installation…"
if [ -x "$INSTALL_DIR/$APP_BIN" ]; then
    progress 99 "✔ $INSTALL_DIR/$APP_BIN — OK"
else
    finish_progress
    gui_error "Verification Failed" "Binary not found at $INSTALL_DIR/$APP_BIN"
fi

finish_progress

# ── Done dialog ───────────────────────────────────────────────────────────────
LAUNCH_CMD="$INSTALL_DIR/$APP_BIN"
$DO_PATH && LAUNCH_CMD="overclockctl"

if $HAS_GUI; then
    if zenity --question \
              --title="✅ Installation Complete" \
              --text="<big><b>$APP_NAME has been installed!</b></big>\n\nInstalled to:  <tt>$INSTALL_DIR/$APP_BIN</tt>\n\nFrom any terminal, type:\n  <tt>overclockctl</tt>\n\nFor full overclocking control, run as root:\n  <tt>sudo overclockctl</tt>\n\n<b>Launch $APP_NAME now?</b>" \
              --ok-label="Launch" \
              --cancel-label="Close" \
              --width=460 2>/dev/null; then
        # Launch with pkexec so OC features work immediately
        pkexec "$INSTALL_DIR/$APP_BIN" &
    fi
else
    echo ""
    echo "════════════════════════════════════════"
    echo "  ✅ $APP_NAME installed successfully!"
    echo "  Binary: $INSTALL_DIR/$APP_BIN"
    echo "  Run:    overclockctl"
    echo "  OC:     sudo overclockctl"
    echo "════════════════════════════════════════"
fi
