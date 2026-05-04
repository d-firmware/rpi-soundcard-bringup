# rpi4b-pcm5102a — PCM5102A I2S DAC Bringup on Raspberry Pi 4B

Brings up a **Texas Instruments PCM5102A** stereo DAC on Raspberry Pi 4B using a custom Device Tree Overlay and the Linux ASoC framework.

## Hardware Required

- Raspberry Pi 4B
- PCM5102A DAC module (widely available, ~₹150–300 on Amazon/Robu)
- Jumper wires
- 3.5mm headphone / speaker output

## Wiring

| RPi 4B Pin | GPIO | PCM5102A |
|-----------|------|----------|
| Pin 12 | GPIO18 | BCK |
| Pin 35 | GPIO19 | LRCK |
| Pin 40 | GPIO21 | DATA |
| Pin 1 | 3.3V | VCC |
| Pin 6 | GND | GND |
| — | GND | FLT, DEMP, FMT |
| — | 3.3V | XSMT |

See [`../docs/wiring_diagram.md`](../docs/wiring_diagram.md) for full schematic.

## Setup

```bash
cd rpi4b-pcm5102a
chmod +x scripts/setup.sh scripts/test_playback.sh scripts/verify_codec.sh
sudo ./scripts/setup.sh
sudo reboot
```

## Verify

```bash
./scripts/verify_codec.sh
./scripts/test_playback.sh
```

## Files

| File | Description |
|------|-------------|
| `dts/pcm5102a-overlay.dts` | Device Tree Overlay — I2S + sound card node |
| `alsa/asound.conf` | ALSA configuration with dmix and S32_LE format |
| `scripts/setup.sh` | Full automated setup (compile DT, install overlay, update config.txt) |
| `scripts/test_playback.sh` | Playback test suite (card detect, sine wave, WAV, sample rates) |
| `scripts/verify_codec.sh` | ASoC driver health check (dmesg, /proc/asound, GPIO state) |

## Kernel Compatibility

Tested on Linux 5.15, 6.1, 6.6 with Raspberry Pi OS Bookworm.
