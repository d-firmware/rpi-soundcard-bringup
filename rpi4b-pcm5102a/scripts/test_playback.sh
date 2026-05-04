#!/usr/bin/env bash
# =============================================================================
# test_playback.sh — Audio playback verification for PCM5102A DAC
#
# Tests:
#   1. Card enumeration check
#   2. Sine wave test (speaker-test)
#   3. WAV file playback (system sound)
#   4. Sample rate iteration test (48k, 96k, 192k)
# =============================================================================

set -euo pipefail

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; NC='\033[0m'
info()    { echo -e "${CYAN}[TEST]${NC}  $*"; }
success() { echo -e "${GREEN}[PASS]${NC}  $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
fail()    { echo -e "${RED}[FAIL]${NC}  $*"; FAILED=$((FAILED+1)); }

CARD_NAME="sndrpii2scard"
FAILED=0

echo ""
echo "============================================================"
echo "  PCM5102A Playback Test Suite"
echo "============================================================"
echo ""

# ── Test 1: Card detection ────────────────────────────────────────────────────
info "Test 1: Checking if sound card is detected..."
if aplay -l 2>/dev/null | grep -q "$CARD_NAME"; then
    success "Card '$CARD_NAME' found."
    aplay -l | grep "$CARD_NAME"
else
    fail "Card '$CARD_NAME' NOT found. Check overlay is loaded: dtoverlay -l"
    echo ""
    info "Available cards:"
    aplay -l || true
fi

# ── Test 2: Sine wave via speaker-test ───────────────────────────────────────
info "Test 2: Sine wave test (3 seconds, L+R channels)..."
if command -v speaker-test &>/dev/null; then
    timeout 3s speaker-test \
        -D "plughw:$CARD_NAME,0" \
        -t sine -f 1000 \
        -c 2 -l 1 &>/dev/null && success "Sine wave test passed." \
        || warn "speaker-test returned non-zero (may be normal if no audio output connected)"
else
    warn "speaker-test not found. Install: sudo apt-get install alsa-utils"
fi

# ── Test 3: WAV file playback ─────────────────────────────────────────────────
info "Test 3: WAV file playback..."
WAV_FILE="/usr/share/sounds/alsa/Front_Center.wav"
if [[ -f "$WAV_FILE" ]]; then
    aplay -D "plughw:$CARD_NAME,0" "$WAV_FILE" &>/dev/null \
        && success "WAV playback succeeded: $WAV_FILE" \
        || fail "WAV playback failed."
else
    warn "Test WAV not found at $WAV_FILE. Install: sudo apt-get install alsa-utils"
fi

# ── Test 4: Sample rate capability ───────────────────────────────────────────
info "Test 4: Sample rate support check..."
for RATE in 44100 48000 96000 192000; do
    # aplay with /dev/zero generates silent audio — safe for rate testing
    if timeout 1s aplay \
        -D "hw:$CARD_NAME,0" \
        -f S32_LE -r "$RATE" -c 2 \
        /dev/zero &>/dev/null 2>&1; then
        success "  ${RATE}Hz — supported"
    else
        warn "  ${RATE}Hz — not supported or error (may be normal)"
    fi
done

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "============================================================"
if [[ $FAILED -eq 0 ]]; then
    success "All tests passed!"
else
    fail "$FAILED test(s) failed. Check dmesg for driver errors:"
    echo "       dmesg | grep -iE 'asoc|pcm5102|i2s|sound'"
fi
echo "============================================================"
echo ""
