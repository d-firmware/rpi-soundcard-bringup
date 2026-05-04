# I2S Protocol — Embedded Audio Bringup Reference

## What is I2S?

**I2S (Inter-IC Sound)** is a serial bus standard developed by Philips Semiconductors in 1986 for connecting digital audio components. It is the dominant audio interface in embedded systems — found in smartphones, smart speakers, automotive infotainment, and industrial devices.

---

## Signal Lines

| Signal | Direction | Description |
|--------|-----------|-------------|
| **SCK** / **BCLK** | Master → Slave | Bit Clock. Clocks each audio sample bit. |
| **WS** / **LRCLK** | Master → Slave | Word Select / Frame Sync. Low = Left, High = Right. Frequency = sample rate. |
| **SD** / **SDATA** | Varies | Serial Data. Can be SDIN (codec input) or SDOUT (codec output). |
| **MCLK** | SoC → Codec | Master Clock (optional). Higher frequency reference (usually 256× or 384× sample rate). Some codecs require MCLK for their internal PLLs. |

---

## Timing Diagram

```
         ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐ ┌─┐
BCLK  ───┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └─┘ └───
       ┌───────────────────┐                   ┌──
LRCLK ─┘   LEFT CHANNEL   └── RIGHT CHANNEL ──┘
         ┌──┐     ┌──┐                 ┌──┐
SDATA ───┘  └─────┘  └─────  ... ─────┘  └─────

         MSB                           MSB
         ↑ Data valid on rising edge of BCLK (standard I2S)
```

**Key rules:**
- LRCLK transitions one BCLK cycle **before** MSB of each channel
- Data is MSB-first
- Left channel transmitted when LRCLK = 0
- Right channel transmitted when LRCLK = 1

---

## I2S Variants

| Format | LRCLK | Data position | Used in |
|--------|-------|---------------|---------|
| **I2S (Philips)** | Toggles 1 BCLK before MSB | 1 bit delay from LRCLK edge | Most codecs (PCM5102A, WM8960) |
| **Left-Justified** | Toggles with MSB | MSB on first BCLK after LRCLK | Some TI codecs |
| **Right-Justified** | Toggles N bits before LSB | LSB aligned to LRCLK edge | Older Sony codecs |
| **DSP/PCM Mode A** | Single pulse | MSB on 2nd BCLK after LRCLK pulse | Bluetooth codecs |
| **DSP/PCM Mode B** | Single pulse | MSB on 1st BCLK after LRCLK pulse | Voice codecs |

---

## BCLK Frequency Calculation

```
BCLK = sample_rate × channels × bits_per_sample

Examples:
  48000 Hz × 2 ch × 32 bit = 3,072,000 Hz  = 3.072 MHz
  48000 Hz × 2 ch × 16 bit = 1,536,000 Hz  = 1.536 MHz
 192000 Hz × 2 ch × 32 bit = 12,288,000 Hz = 12.288 MHz
```

The SoC I2S controller (as clock master) must generate this exact BCLK frequency.

---

## MCLK (Master Clock)

MCLK is a higher-frequency reference clock provided to the codec for its internal PLL and DAC/ADC operation.

```
MCLK = sample_rate × oversampling_ratio

Common ratios:
  256fs:  48000 × 256  = 12.288 MHz   (very common)
  384fs:  48000 × 384  = 18.432 MHz
  512fs:  48000 × 512  = 24.576 MHz
```

**PCM5102A note:** The PCM5102A does NOT require external MCLK — it recovers its clock from BCLK and LRCLK internally. This simplifies hardware design significantly.

---

## Linux ASoC Clock Terminology

In the Linux kernel, clock direction is described from the **component's perspective**:

| Term | Meaning |
|------|---------|
| `SND_SOC_DAIFMT_CBC_CFC` | **C**lock **B**it **C**onsumer, **C**lock **F**rame **C**onsumer → Codec is slave |
| `SND_SOC_DAIFMT_CBP_CFP` | **C**lock **B**it **P**rovider, **C**lock **F**rame **P**rovider → Codec is master |

Most embedded designs: **CPU is clock provider, Codec is consumer** = `CBC_CFC`

---

## Common Bringup Issues

| Symptom | Likely cause |
|---------|-------------|
| No sound, no error | LRCLK/BCLK not reaching codec (scope check) |
| White noise / distortion | Wrong I2S format (I2S vs Left-J vs DSP) |
| Only left channel | SDATA wiring to wrong codec pin |
| High-pitched clicking | BCLK frequency mismatch |
| `no space left on device` in dmesg | DMA buffer configuration issue |
| Card appears in `aplay -l` but no sound | DAPM routing incomplete (widget not powered) |

---

## Useful Commands

```bash
# List all sound cards and devices
aplay -l

# Show ALSA PCM hardware parameters for a device
cat /proc/asound/<card>/pcm0p/sub0/hw_params

# Play test tone
speaker-test -D hw:0,0 -t sine -f 1000 -c 2

# Monitor ASoC DAPM state (kernel 5.x+)
cat /sys/kernel/debug/asoc/*/dapm_pop_time

# Dump ALSA mixer state
amixer -c 0 contents

# Check I2S clock is running (BCM2711 example)
sudo cat /sys/kernel/debug/clk/clk_summary | grep i2s
```

---

## References

- [Philips I2S Bus Specification](https://web.archive.org/web/20070102004400/http://www.nxp.com/acrobat_download/various/I2SBUS.pdf)
- [Linux ASoC Documentation](https://www.kernel.org/doc/html/latest/sound/soc/index.html)
- [PCM5102A Datasheet](https://www.ti.com/lit/ds/symlink/pcm5102a.pdf)
