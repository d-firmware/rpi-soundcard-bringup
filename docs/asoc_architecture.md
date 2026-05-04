# Linux ASoC Framework — Architecture Reference

## Overview

**ASoC (ALSA System-on-Chip)** is the Linux subsystem for audio in embedded systems. It was introduced to solve the limitations of the original ALSA drivers, which mixed platform-specific and codec-specific code together.

ASoC cleanly separates audio drivers into three independent components:

```
┌─────────────────────────────────────────────────┐
│              Machine Driver                     │  ← Board-specific glue
│  (ties CPU DAI + Codec DAI + routes together)   │     my_machine.c
└────────────┬──────────────────┬─────────────────┘
             │                  │
┌────────────▼──────┐  ┌────────▼────────────────┐
│  Platform Driver  │  │    Codec Driver          │  ← my_codec.c
│  (CPU DAI + DMA)  │  │  (register map, DAPM,   │
│  Provided by SoC  │  │   kcontrols, DAI ops)   │
└───────────────────┘  └─────────────────────────┘
```

---

## The Three Layers Explained

### 1. Codec Driver (`my_codec.c`)
Represents the audio codec chip (e.g., PCM5102A, WM8960, TLV320AIC3x).

Responsibilities:
- Register map abstraction via `regmap`
- DAPM widgets (DAC, ADC, PGA, mixer, mux)
- ALSA kcontrols (volume, mute, EQ)
- DAI operations (`set_fmt`, `hw_params`, `set_sysclk`)
- Power management (`suspend`/`resume`)

### 2. Platform Driver (CPU DAI)
Represents the I2S / PCM controller inside the SoC.

Responsibilities:
- DMA buffer management
- BCLK / LRCLK generation (when CPU is clock master)
- Hardware triggering (start/stop DMA on `trigger()`)
- Usually provided by the SoC vendor — you rarely write this

Examples: `sound/soc/bcm/bcm2835-i2s.c`, `sound/soc/samsung/i2s.c`

### 3. Machine Driver (`my_machine.c`)
The board-specific glue that ties codec + platform together.

Responsibilities:
- DAI link definitions (which CPU DAI talks to which codec DAI)
- Negotiates DAI format (`I2S`, `Left-J`, `DSP`)
- Board-level DAPM widgets (speakers, jacks) and routes
- MCLK / PLL setup in `hw_params`
- GPIO handling (speaker amp, headphone detect)

---

## DAPM — Dynamic Audio Power Management

DAPM is the power management subsystem within ASoC. It automatically powers audio components on and off based on active audio paths.

### DAPM Widget Types

| Widget | Description |
|--------|-------------|
| `SND_SOC_DAPM_DAC` | Digital-to-Analog converter block |
| `SND_SOC_DAPM_ADC` | Analog-to-Digital converter block |
| `SND_SOC_DAPM_PGA` | Programmable Gain Amplifier |
| `SND_SOC_DAPM_MIXER` | Mixes multiple inputs together |
| `SND_SOC_DAPM_MUX` | Selects one of several inputs |
| `SND_SOC_DAPM_SWITCH` | Controllable signal path on/off |
| `SND_SOC_DAPM_OUTPUT` | Physical output pin |
| `SND_SOC_DAPM_INPUT` | Physical input pin |
| `SND_SOC_DAPM_SPK` | Speaker (board level) |
| `SND_SOC_DAPM_HP` | Headphone (board level) |
| `SND_SOC_DAPM_MIC` | Microphone (board level) |
| `SND_SOC_DAPM_SUPPLY` | Power supply / clock enable |

### DAPM Path Example (Playback)

```
[AIF Playback] → [DAC Left] → [Output Mixer] → [OUTL pin] → [Speaker]
                                     ↑
                              (kcontrol can disable this path)
```

DAPM traces from "Speaker" backward and powers every widget in the path.

### Debugging DAPM

```bash
# Show all DAPM widgets and their power state
cat /sys/kernel/debug/asoc/*/dapm_widget

# Show active DAPM paths
cat /sys/kernel/debug/asoc/*/dapm_paths

# Force a widget on for testing
echo 1 > /sys/kernel/debug/asoc/<card>/<codec>/dapm/<widget>
```

---

## DAI Link — Connecting CPU to Codec

The DAI link in the machine driver specifies:

```c
static struct snd_soc_dai_link my_dai_link = {
    .name        = "codec-playback",
    .dai_fmt     = SND_SOC_DAIFMT_I2S       /* Audio format */
                 | SND_SOC_DAIFMT_NB_NF     /* Normal clocks */
                 | SND_SOC_DAIFMT_CBC_CFC,  /* CPU is master */
    /* CPU, Codec, Platform components filled from DT or hardcoded */
};
```

### DAI Format Flags

**Audio Format:**
| Flag | Description |
|------|-------------|
| `SND_SOC_DAIFMT_I2S` | Standard I2S (1-bit delay) |
| `SND_SOC_DAIFMT_LEFT_J` | Left-justified (MSB on first BCLK) |
| `SND_SOC_DAIFMT_RIGHT_J` | Right-justified |
| `SND_SOC_DAIFMT_DSP_A` | DSP/PCM mode A |

**Clock Polarity:**
| Flag | Description |
|------|-------------|
| `SND_SOC_DAIFMT_NB_NF` | Normal BCLK, Normal Frame (most common) |
| `SND_SOC_DAIFMT_IB_NF` | Inverted BCLK |
| `SND_SOC_DAIFMT_NB_IF` | Inverted Frame |

**Clock Provider:**
| Flag | Description |
|------|-------------|
| `SND_SOC_DAIFMT_CBC_CFC` | CPU is clock master (most embedded designs) |
| `SND_SOC_DAIFMT_CBP_CFP` | Codec is clock master |

---

## Regmap — Register Abstraction

ASoC codec drivers use `regmap` to abstract hardware register access. This provides:
- Automatic caching (avoids repeated I2C reads)
- Register range validation
- Debug interface via `/sys/kernel/debug/regmap/`

```bash
# Dump all codec registers at runtime
cat /sys/kernel/debug/regmap/*/registers

# Read specific register
echo "0x06" > /sys/kernel/debug/regmap/*/address
cat /sys/kernel/debug/regmap/*/val
```

---

## ASoC Driver Registration Flow

```
i2c_driver.probe()
    ↓
devm_regmap_init_i2c()          ← Set up register map
    ↓
devm_snd_soc_register_component() ← Register codec component
    ↓
platform_driver.probe()          ← Machine driver
    ↓
devm_snd_soc_register_card()    ← Register sound card
    ↓
ASoC core matches DAI links      ← Links CPU + Codec components
    ↓
snd_soc_dai_link_set_capabilities() ← Negotiate formats
    ↓
Card ready — appears in /proc/asound/cards
```

---

## References

- [Kernel ASoC Documentation](https://www.kernel.org/doc/html/latest/sound/soc/index.html)
- [Writing an ASoC Codec Driver](https://www.kernel.org/doc/html/latest/sound/soc/codec.rst)
- [DAPM](https://www.kernel.org/doc/html/latest/sound/soc/dapm.rst)
- [Regmap API](https://www.kernel.org/doc/html/latest/driver-api/regmap.html)
