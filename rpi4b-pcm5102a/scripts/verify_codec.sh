#!/usr/bin/env bash
# =============================================================================
# verify_codec.sh — Codec and ASoC driver health check
#
# Checks kernel messages, ALSA proc entries, and DAI link status
# to confirm the PCM5102A is properly brought up by the ASoC framework.
# =============================================================================

set -uo pipefail

CYAN='\033[0;36m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
RED='\033[0;31m'; NC='\033[0m'
section() { echo -e "\n${CYAN}══ $* ══${NC}"; }
ok()      { echo -e "  ${GREEN}✔${NC}  $*"; }
warn()    { echo -e "  ${YELLOW}⚠${NC}  $*"; }
bad()     { echo -e "  ${RED}✘${NC}  $*"; }

echo ""
echo "  PCM5102A / ASoC Driver Health Check"
echo "  $(date)"
echo ""

# ── 1. Overlay loaded? ────────────────────────────────────────────────────────
section "Device Tree Overlay"
if dtoverlay -l 2>/dev/null | grep -q "pcm5102a"; then
    ok "pcm5102a overlay is active"
else
    bad "pcm5102a overlay NOT found in dtoverlay -l"
    warn "Run: sudo dtoverlay pcm5102a-overlay  (or check /boot/config.txt)"
fi

# ── 2. Kernel messages ────────────────────────────────────────────────────────
section "Kernel Log (dmesg)"
echo "  Relevant ASoC/I2S messages:"
dmesg | grep -iE "asoc|pcm5102|i2s|simple-audio|bcm2835|sound" \
      | tail -20 \
      | sed 's/^/  /' \
      || warn "No relevant dmesg entries found"

# ── 3. ALSA card enumeration ──────────────────────────────────────────────────
section "ALSA Card List"
aplay -l 2>/dev/null | sed 's/^/  /' || bad "aplay -l failed"

# ── 4. /proc/asound entries ───────────────────────────────────────────────────
section "/proc/asound"
if [[ -d /proc/asound ]]; then
    ok "Cards registered:"
    cat /proc/asound/cards | sed 's/^/  /'

    # PCM capabilities
    CARD_DIR=$(ls /proc/asound/ | grep -i "rpi\|i2s\|pcm5102\|snd" | head -1)
    if [[ -n "$CARD_DIR" && -f "/proc/asound/$CARD_DIR/pcm0p/info" ]]; then
        echo ""
        ok "PCM Playback info:"
        cat "/proc/asound/$CARD_DIR/pcm0p/info" | sed 's/^/  /'
    fi
else
    bad "/proc/asound not found — ALSA not loaded?"
fi

# ── 5. I2S clock check ────────────────────────────────────────────────────────
section "I2S GPIO Pins (BCM2711)"
# GPIO 18/19/21 should be in ALT0 mode for I2S
if command -v raspi-gpio &>/dev/null; then
    ok "GPIO 18 (BCK):"
    raspi-gpio get 18 | sed 's/^/  /'
    ok "GPIO 19 (LRCK):"
    raspi-gpio get 19 | sed 's/^/  /'
    ok "GPIO 21 (DOUT):"
    raspi-gpio get 21 | sed 's/^/  /'
else
    warn "raspi-gpio not found. Install: sudo apt-get install raspi-gpio"
    warn "Manually check: cat /sys/kernel/debug/pinctrl/*/pins | grep -E 'gpio1[89]|gpio21'"
fi

# ── 6. Module check ───────────────────────────────────────────────────────────
section "Kernel Modules"
for MOD in snd_soc_bcm2835_i2s snd_soc_simple_card snd_soc_pcm5102a; do
    if lsmod | grep -q "^$MOD"; then
        ok "Module loaded: $MOD"
    else
        warn "Module not visible: $MOD (may be built-in — check /proc/asound)"
    fi
done

echo ""
echo "  Done. For detailed debug: dmesg -T | grep -iE 'asoc|i2s|sound'"
echo ""
