# generic-linux-asoc — ASoC Driver Skeleton for Custom Boards

A production-style template for writing a Linux ASoC audio driver from scratch. Designed for engineers porting audio to a new embedded platform.

## What's Included

| File | Purpose |
|------|---------|
| `driver/my_codec.c` | ASoC codec driver — regmap, DAPM, kcontrols, DAI ops |
| `driver/my_machine.c` | ASoC machine driver — DAI link, MCLK setup, GPIO |
| `driver/Makefile` | Build against running kernel or cross-compile target |
| `dts/sound_node.dtsi` | Device Tree snippet for codec + I2S + machine nodes |

## Porting Checklist

Before building, adapt the following:

- [ ] Replace `MY_CODEC_NAME` / `my_codec` with your codec's identifier
- [ ] Update register definitions (`MY_CODEC_REG_*`) from your datasheet
- [ ] Set `reg_defaults[]` to your codec's power-on register state
- [ ] Implement `hw_params()` sample rate → register value mapping
- [ ] Adjust TLV scale to match your codec's volume register range
- [ ] Update DAPM widgets and routes for your codec topology
- [ ] Set `dai_fmt` in machine driver to match your hardware (I2S / Left-J / DSP)
- [ ] Update `sound_node.dtsi` I2C address and bus references
- [ ] Set MCLK ratio in `my_machine_hw_params()` (256fs / 384fs / 512fs)

## Build

```bash
cd driver

# Native build (on target board)
make

# Cross-compile (ARM64 example)
make ARCH=arm64 \
     CROSS_COMPILE=aarch64-linux-gnu- \
     KDIR=/path/to/kernel/source

# Load modules
make load

# Check dmesg
dmesg | grep -iE "asoc|my.codec|sound"
```

## Reference Implementations

These upstream drivers are excellent references when implementing specific features:

| Codec | File in kernel | Notable for |
|-------|---------------|-------------|
| PCM5102A | `sound/soc/codecs/pcm5102a.c` | Simple no-register codec |
| WM8960 | `sound/soc/codecs/wm8960.c` | Full-featured codec, PLL setup |
| TLV320AIC3x | `sound/soc/codecs/tlv320aic3x.c` | Complex DAPM topology |
| Simple card | `sound/soc/generic/simple-card.c` | DT-driven machine driver |
