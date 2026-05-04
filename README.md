# rpi-soundcard-bringup
General code for audio bringup in raspberry pi and general linux boards
=======

![Linux](https://img.shields.io/badge/Linux-Kernel%205.x%2F6.x-blue?logo=linux)
![Platform](https://img.shields.io/badge/Platform-RPi%204B%20%7C%20Generic%20ARM-green)
![Language](https://img.shields.io/badge/Language-C%20%7C%20Shell%20%7C%20DeviceTree-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

A practical, production-style reference for **soundcard bringup on Linux embedded platforms** using the ALSA/ASoC framework. Covers two approaches:

| Approach | Target | Description |
|---|---|---|
| [`rpi4b-pcm5102a/`](./rpi4b-pcm5102a/) | Raspberry Pi 4B | Bringup of PCM5102A I2S DAC using custom Device Tree Overlay |
| [`generic-linux-asoc/`](./generic-linux-asoc/) | Any ARM Linux board | ASoC codec + machine driver skeleton for porting to custom hardware |

---

## Architecture Overview

```
 ┌─────────────────────────────────────────────────────────┐
 │                   User Space                            │
 │         aplay / arecord / GStreamer / PulseAudio        │
 └────────────────────┬────────────────────────────────────┘
                      │ ALSA API (libasound)
 ┌────────────────────▼────────────────────────────────────┐
 │               ALSA Core (Kernel)                        │
 │         PCM / Control / Mixer Interface                 │
 └────────────────────┬────────────────────────────────────┘
                      │
 ┌────────────────────▼────────────────────────────────────┐
 │                 ASoC Framework                          │
 │  ┌─────────────┐ ┌──────────────┐ ┌─────────────────┐  │
 │  │   Machine   │ │   Platform   │ │  Codec Driver   │  │
 │  │   Driver    │ │   Driver     │ │  (PCM5102A etc) │  │
 │  │ (board glue)│ │  (I2S/DMA)   │ │                 │  │
 │  └──────┬──────┘ └──────┬───────┘ └────────┬────────┘  │
 └─────────┼───────────────┼──────────────────┼───────────┘
           │               │                  │
 ┌─────────▼───────────────▼──────────────────▼───────────┐
 │                Device Tree (DTS/DTSI)                   │
 │         I2S node | Codec node | Sound card node         │
 └─────────────────────────────────────────────────────────┘
                      │
 ┌────────────────────▼────────────────────────────────────┐
 │                  Hardware                               │
 │         SoC I2S Controller ←──────→ Audio Codec        │
 │         (BCM2711 / Generic)           (PCM5102A)        │
 └─────────────────────────────────────────────────────────┘
```

---

## Repository Structure

```
rpi-soundcard-bringup/
├── rpi4b-pcm5102a/              # Raspberry Pi 4B + PCM5102A DAC
│   ├── dts/
│   │   └── pcm5102a-overlay.dts     # Custom Device Tree Overlay
│   ├── alsa/
│   │   └── asound.conf              # ALSA configuration
│   └── scripts/
│       ├── setup.sh                 # Full setup automation
│       ├── test_playback.sh         # Audio playback test
│       └── verify_codec.sh          # Codec register/health check
│
├── generic-linux-asoc/          # Generic ASoC driver for custom boards
│   ├── driver/
│   │   ├── my_codec.c               # ASoC Codec driver skeleton
│   │   ├── my_machine.c             # ASoC Machine driver skeleton
│   │   └── Makefile
│   └── dts/
│       └── sound_node.dtsi          # DTS snippet for sound card node
│
├── docs/
│   ├── i2s_protocol.md              # I2S protocol deep-dive
│   ├── asoc_architecture.md         # Linux ASoC framework explained
│   └── wiring_diagram.md            # PCM5102A wiring reference
│
├── LICENSE
└── README.md
```

---

## Quick Start

### Raspberry Pi 4B + PCM5102A
```bash
git clone https://github.com/YOUR_USERNAME/rpi-soundcard-bringup.git
cd rpi-soundcard-bringup/rpi4b-pcm5102a
chmod +x scripts/setup.sh
sudo ./scripts/setup.sh
# Reboot, then:
aplay -D plughw:CARD=sndrpii2scard,DEV=0 /usr/share/sounds/alsa/Front_Center.wav
```

### Generic ASoC Driver
```bash
cd generic-linux-asoc/driver
# Edit MY_CODEC_NAME, register map, and DAI formats to match your codec
make -C /lib/modules/$(uname -r)/build M=$(pwd) modules
sudo insmod my_codec.ko
sudo insmod my_machine.ko
dmesg | grep -i "sound\|asoc\|codec"
```

---

## Hardware: PCM5102A DAC

The **PCM5102A** (Texas Instruments) is a 112dB stereo DAC with:
- I2S / Left-Justified / Right-Justified input formats
- 2.1V to 3.6V digital supply
- No I2C/SPI control interface (hardware pin control only)
- 32kHz to 384kHz sample rates

See [`docs/wiring_diagram.md`](./docs/wiring_diagram.md) for full connection reference.

---

## Kernel / Distro Compatibility

| Component | Tested On |
|---|---|
| Kernel | 5.15 LTS, 6.1 LTS, 6.6 LTS |
| Distro | Raspberry Pi OS (Bookworm), Ubuntu 22.04/24.04 for RPi |
| Board | Raspberry Pi 4B (BCM2711) |
| ASoC Generic | Any ARM board with mainline kernel |

---

## References

- [ALSA Project Documentation](https://www.alsa-project.org/wiki/Main_Page)
- [Linux Kernel ASoC Documentation](https://www.kernel.org/doc/html/latest/sound/soc/index.html)
- [PCM5102A Datasheet – Texas Instruments](https://www.ti.com/product/PCM5102A)
- [Raspberry Pi Device Tree Overlays](https://www.raspberrypi.com/documentation/computers/configuration.html#device-trees-overlays-and-parameters)

---

## License

MIT License. See [LICENSE](./LICENSE).

