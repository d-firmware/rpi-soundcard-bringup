#!/usr/bin/env bash
# =============================================================================
# setup.sh — Full bringup script for PCM5102A DAC on Raspberry Pi 4B
#
# Usage:  sudo ./setup.sh
#
# What this script does:
#   1. Checks prerequisites (dtc, kernel headers)
#   2. Compiles the Device Tree Overlay (.dts → .dtbo)
#   3. Installs the overlay to /boot/overlays/
#   4. Updates /boot/config.txt to load the overlay at boot
#   5. Installs ALSA configuration
#   6. Disables onboard audio (optional — avoids card index conflicts)
#   7. Prompts for reboot
#
# After reboot, verify with:
#   aplay -l
#   ./scripts/test_playback.sh
# =============================================================================

set -euo pipefail

# ── Colour helpers ────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; NC='\033[0m'
info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; exit 1; }

# ── Must run as root ──────────────────────────────────────────────────────────
[[ $EUID -ne 0 ]] && error "Run as root: sudo $0"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
DTS_FILE="$REPO_ROOT/dts/pcm5102a-overlay.dts"
DTBO_FILE="pcm5102a-overlay.dtbo"
OVERLAY_DEST="/boot/overlays/$DTBO_FILE"
CONFIG_FILE="/boot/config.txt"
ASOUND_CONF="$REPO_ROOT/alsa/asound.conf"

echo ""
echo "============================================================"
echo "  PCM5102A Soundcard Bringup — Raspberry Pi 4B"
echo "============================================================"
echo ""

# ── Step 1: Check prerequisites ───────────────────────────────────────────────
info "Checking prerequisites..."

if ! command -v dtc &>/dev/null; then
    warn "'dtc' not found. Installing device-tree-compiler..."
    apt-get install -y device-tree-compiler
fi
success "dtc found: $(dtc --version | head -1)"

# ── Step 2: Compile Device Tree Overlay ──────────────────────────────────────
info "Compiling Device Tree Overlay..."
dtc -@ -I dts -O dtb \
    -o "/tmp/$DTBO_FILE" \
    "$DTS_FILE" 2>&1 | grep -v "Warning (unit_address_vs_reg)" || true

[[ -f "/tmp/$DTBO_FILE" ]] || error "DT compilation failed. Check $DTS_FILE"
success "Overlay compiled → /tmp/$DTBO_FILE"

# ── Step 3: Install overlay ───────────────────────────────────────────────────
info "Installing overlay to $OVERLAY_DEST..."
cp "/tmp/$DTBO_FILE" "$OVERLAY_DEST"
chmod 644 "$OVERLAY_DEST"
success "Overlay installed."

# ── Step 4: Update /boot/config.txt ──────────────────────────────────────────
info "Updating $CONFIG_FILE..."

# Disable onboard audio to avoid card index conflict
if grep -q "^dtparam=audio=on" "$CONFIG_FILE"; then
    sed -i 's/^dtparam=audio=on/# dtparam=audio=on  # disabled for PCM5102A/' "$CONFIG_FILE"
    warn "Onboard audio disabled (was dtparam=audio=on)"
fi

# Add overlay if not already present
OVERLAY_ENTRY="dtoverlay=pcm5102a-overlay"
if grep -q "^$OVERLAY_ENTRY" "$CONFIG_FILE"; then
    warn "Overlay entry already in $CONFIG_FILE — skipping."
else
    echo "" >> "$CONFIG_FILE"
    echo "# PCM5102A I2S DAC overlay" >> "$CONFIG_FILE"
    echo "$OVERLAY_ENTRY" >> "$CONFIG_FILE"
    success "Added '$OVERLAY_ENTRY' to $CONFIG_FILE"
fi

# Enable I2S
I2S_ENTRY="dtparam=i2s=on"
if grep -q "^$I2S_ENTRY" "$CONFIG_FILE"; then
    warn "I2S already enabled — skipping."
else
    echo "$I2S_ENTRY" >> "$CONFIG_FILE"
    success "I2S enabled in $CONFIG_FILE"
fi

# ── Step 5: Install ALSA config ───────────────────────────────────────────────
info "Installing ALSA configuration to /etc/asound.conf..."
if [[ -f /etc/asound.conf ]]; then
    cp /etc/asound.conf /etc/asound.conf.bak
    warn "Backed up existing asound.conf → /etc/asound.conf.bak"
fi
cp "$ASOUND_CONF" /etc/asound.conf
success "ALSA config installed."

# ── Step 6: Add user to audio group ──────────────────────────────────────────
REAL_USER="${SUDO_USER:-pi}"
if id "$REAL_USER" &>/dev/null; then
    usermod -aG audio "$REAL_USER"
    success "User '$REAL_USER' added to 'audio' group."
fi

# ── Done ──────────────────────────────────────────────────────────────────────
echo ""
echo "============================================================"
success "Setup complete!"
echo ""
echo "  Next steps:"
echo "  1. Reboot: sudo reboot"
echo "  2. After reboot, verify card: aplay -l"
echo "  3. Run playback test: ./scripts/test_playback.sh"
echo "============================================================"
echo ""

read -rp "Reboot now? [y/N] " choice
[[ "${choice,,}" == "y" ]] && reboot || warn "Remember to reboot before testing."
