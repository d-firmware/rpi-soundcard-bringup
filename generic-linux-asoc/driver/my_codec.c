// SPDX-License-Identifier: GPL-2.0-only
/*
 * my_codec.c — Generic ASoC Codec Driver Skeleton
 *
 * This is a template for bringing up a new audio codec on any Linux
 * embedded platform. Designed to be adapted for codecs with an I2C or
 * SPI control interface and I2S/PCM audio data interface.
 *
 * Porting checklist:
 *   [ ] Replace MY_CODEC_NAME with your codec name (e.g., "tlv320aic3x")
 *   [ ] Fill in register map (my_codec_regmap_config)
 *   [ ] Add DAPM widgets and routes for your codec topology
 *   [ ] Set correct DAI formats (I2S / DSP / left-justified / right-justified)
 *   [ ] Implement hw_params() to program sample rate/width registers
 *   [ ] Add kcontrols for volume, mute, EQ if applicable
 *
 * References:
 *   - Linux Kernel: sound/soc/codecs/ (reference implementations)
 *   - Documentation/sound/soc/codec.rst
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/tlv.h>

#define MY_CODEC_NAME       "my-codec"
#define MY_CODEC_DRIVER     "my_codec"

/* ── Register Definitions ──────────────────────────────────────────────────── *
 * Replace with your codec's actual register map.
 * Refer to your codec datasheet's register map section.
 */
#define MY_CODEC_REG_RESET          0x00
#define MY_CODEC_REG_PWR_CTRL       0x01
#define MY_CODEC_REG_DAC_CTRL       0x02
#define MY_CODEC_REG_ADC_CTRL       0x03
#define MY_CODEC_REG_DAI_FMT        0x04    /* Audio interface format */
#define MY_CODEC_REG_SAMPLE_RATE    0x05
#define MY_CODEC_REG_VOL_L          0x06
#define MY_CODEC_REG_VOL_R          0x07
#define MY_CODEC_REG_MUTE           0x08
#define MY_CODEC_REG_STATUS         0x0F    /* Read-only status */

/* Register bit definitions */
#define MY_CODEC_RESET_BIT          BIT(7)
#define MY_CODEC_DAC_EN             BIT(0)
#define MY_CODEC_ADC_EN             BIT(1)
#define MY_CODEC_MUTE_DAC           BIT(0)

/* ── Regmap Configuration ──────────────────────────────────────────────────── */
static const struct reg_default my_codec_reg_defaults[] = {
    { MY_CODEC_REG_PWR_CTRL,    0x00 },
    { MY_CODEC_REG_DAC_CTRL,    0x00 },
    { MY_CODEC_REG_ADC_CTRL,    0x00 },
    { MY_CODEC_REG_DAI_FMT,     0x00 },
    { MY_CODEC_REG_SAMPLE_RATE, 0x00 },
    { MY_CODEC_REG_VOL_L,       0x78 },    /* 0dB default — adjust per datasheet */
    { MY_CODEC_REG_VOL_R,       0x78 },
    { MY_CODEC_REG_MUTE,        0x00 },
};

static const struct regmap_config my_codec_regmap_config = {
    .reg_bits           = 8,
    .val_bits           = 8,
    .max_register       = MY_CODEC_REG_STATUS,
    .reg_defaults       = my_codec_reg_defaults,
    .num_reg_defaults   = ARRAY_SIZE(my_codec_reg_defaults),
    .cache_type         = REGCACHE_RBTREE,
    /*
     * Mark read-only registers. regmap will refuse writes to these.
     * Add any other read-only registers your codec has.
     */
    .readable_reg       = NULL,     /* implement if needed */
    .writeable_reg      = NULL,     /* implement if needed */
    .volatile_reg       = NULL,     /* implement if needed */
};

/* ── Private driver data ───────────────────────────────────────────────────── */
struct my_codec_priv {
    struct regmap   *regmap;
    unsigned int    sysclk;         /* MCLK frequency from platform */
    unsigned int    fmt;            /* DAI format negotiated in set_fmt */
};

/* ── Volume TLV ────────────────────────────────────────────────────────────── *
 * TLV = Type-Length-Value: describes the mapping of register value → dB.
 * Example: 0x00 = -127dB, 0x78 = 0dB, 0x7F = +0.5dB (common for DACs).
 * Adjust min_val and step_size to match your codec datasheet.
 */
static const DECLARE_TLV_DB_SCALE(my_codec_vol_tlv,
    -12700,     /* min: -127.00 dB */
    50,         /* step: 0.5 dB */
    0           /* mute: no separate mute bit in this register */
);

/* ── ALSA kcontrols ────────────────────────────────────────────────────────── *
 * These appear in alsamixer / amixer as mixer controls.
 */
static const struct snd_kcontrol_new my_codec_snd_controls[] = {
    /* Stereo volume control linked to left/right volume registers */
    SOC_DOUBLE_R_TLV("DAC Playback Volume",
        MY_CODEC_REG_VOL_L,
        MY_CODEC_REG_VOL_R,
        0,          /* shift */
        0x7F,       /* max register value */
        0,          /* invert: 0 = higher reg value → louder */
        my_codec_vol_tlv),

    /* Single bit mute control */
    SOC_SINGLE("DAC Mute Switch",
        MY_CODEC_REG_MUTE,
        MY_CODEC_MUTE_DAC,  /* bit */
        1,                  /* max */
        1),                 /* invert: 1 = active-low mute bit */
};

/* ── DAPM Widgets ──────────────────────────────────────────────────────────── *
 * DAPM = Dynamic Audio Power Management.
 * Widgets represent logical audio components. The ASoC core automatically
 * powers components up/down based on active audio paths.
 *
 * Common widget types:
 *   SND_SOC_DAPM_DAC      — Digital-to-Analog Converter block
 *   SND_SOC_DAPM_ADC      — Analog-to-Digital Converter block
 *   SND_SOC_DAPM_OUTPUT   — Physical output pin (connects to speaker/headphone)
 *   SND_SOC_DAPM_INPUT    — Physical input pin (connects to mic/line-in)
 *   SND_SOC_DAPM_MIXER    — Mixer (combines multiple signals)
 *   SND_SOC_DAPM_MUX      — Multiplexer (selects one of many inputs)
 */
static const struct snd_soc_dapm_widget my_codec_dapm_widgets[] = {
    /* DAC block — powered when playback stream is active */
    SND_SOC_DAPM_DAC("DAC Left",  "Playback", MY_CODEC_REG_PWR_CTRL, 0, 0),
    SND_SOC_DAPM_DAC("DAC Right", "Playback", MY_CODEC_REG_PWR_CTRL, 1, 0),

    /* Physical output pins — always-on once DAC is active */
    SND_SOC_DAPM_OUTPUT("OUTL"),
    SND_SOC_DAPM_OUTPUT("OUTR"),

    /*
     * Add ADC widgets here if your codec supports capture:
     * SND_SOC_DAPM_ADC("ADC", "Capture", MY_CODEC_REG_PWR_CTRL, 2, 0),
     * SND_SOC_DAPM_INPUT("INL"),
     * SND_SOC_DAPM_INPUT("INR"),
     */
};

/* ── DAPM Routes ───────────────────────────────────────────────────────────── *
 * Routes describe the signal flow: { sink, control, source }
 * control = NULL means always-connected (no switch/mux in path).
 */
static const struct snd_soc_dapm_route my_codec_dapm_routes[] = {
    /* DAC output → physical pins */
    { "OUTL", NULL, "DAC Left"  },
    { "OUTR", NULL, "DAC Right" },

    /*
     * If your codec has a headphone amplifier between DAC and output:
     * { "HP Amp L", NULL, "DAC Left"  },
     * { "OUTL",     NULL, "HP Amp L"  },
     */
};

/* ── DAI ops: set_sysclk ───────────────────────────────────────────────────── */
static int my_codec_set_sysclk(struct snd_soc_dai *dai, int clk_id,
                                unsigned int freq, int dir)
{
    struct snd_soc_component *component = dai->component;
    struct my_codec_priv *priv = snd_soc_component_get_drvdata(component);

    priv->sysclk = freq;
    dev_dbg(component->dev, "sysclk set to %u Hz\n", freq);

    /*
     * If your codec needs MCLK programmed into a register, do it here.
     * Example: regmap_write(priv->regmap, MY_CODEC_REG_MCLK, clk_val);
     */
    return 0;
}

/* ── DAI ops: set_fmt ──────────────────────────────────────────────────────── *
 * Called by machine driver to negotiate DAI format (I2S, clock polarity, etc.)
 */
static int my_codec_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    struct snd_soc_component *component = dai->component;
    struct my_codec_priv *priv = snd_soc_component_get_drvdata(component);
    u8 dai_fmt_reg = 0;

    priv->fmt = fmt;

    /* Audio format */
    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
    case SND_SOC_DAIFMT_I2S:
        dai_fmt_reg |= 0x00;    /* I2S mode — adjust per datasheet */
        break;
    case SND_SOC_DAIFMT_LEFT_J:
        dai_fmt_reg |= 0x01;
        break;
    case SND_SOC_DAIFMT_RIGHT_J:
        dai_fmt_reg |= 0x02;
        break;
    case SND_SOC_DAIFMT_DSP_A:
        dai_fmt_reg |= 0x03;
        break;
    default:
        dev_err(component->dev, "Unsupported DAI format: 0x%x\n",
                fmt & SND_SOC_DAIFMT_FORMAT_MASK);
        return -EINVAL;
    }

    /* Clock polarity */
    switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
    case SND_SOC_DAIFMT_NB_NF:     /* Normal BCLK, Normal LRCLK */
        break;
    case SND_SOC_DAIFMT_IB_NF:     /* Inverted BCLK */
        dai_fmt_reg |= BIT(4);     /* Example — check your datasheet */
        break;
    default:
        dev_err(component->dev, "Unsupported clock polarity\n");
        return -EINVAL;
    }

    /* Master/Slave — most codecs are slaves (CPU is master) */
    switch (fmt & SND_SOC_DAIFMT_CLOCK_PROVIDER_MASK) {
    case SND_SOC_DAIFMT_CBC_CFC:   /* Codec is clock consumer (slave) */
        break;
    case SND_SOC_DAIFMT_CBP_CFP:   /* Codec is clock provider (master) */
        dai_fmt_reg |= BIT(6);     /* Example — check your datasheet */
        break;
    default:
        return -EINVAL;
    }

    return regmap_write(priv->regmap, MY_CODEC_REG_DAI_FMT, dai_fmt_reg);
}

/* ── DAI ops: hw_params ────────────────────────────────────────────────────── *
 * Called when a stream opens. Program sample rate and bit width here.
 */
static int my_codec_hw_params(struct snd_pcm_substream *substream,
                               struct snd_pcm_hw_params *params,
                               struct snd_soc_dai *dai)
{
    struct snd_soc_component *component = dai->component;
    struct my_codec_priv *priv = snd_soc_component_get_drvdata(component);
    unsigned int rate = params_rate(params);
    unsigned int width = params_physical_width(params);
    u8 rate_reg = 0;

    dev_dbg(component->dev, "hw_params: rate=%u, width=%u\n", rate, width);

    /*
     * Map sample rate to register value.
     * Values below are placeholders — replace with your codec's rate table.
     */
    switch (rate) {
    case 8000:  rate_reg = 0x00; break;
    case 16000: rate_reg = 0x01; break;
    case 32000: rate_reg = 0x02; break;
    case 44100: rate_reg = 0x03; break;
    case 48000: rate_reg = 0x04; break;
    case 96000: rate_reg = 0x05; break;
    case 192000:rate_reg = 0x06; break;
    default:
        dev_err(component->dev, "Unsupported sample rate: %u\n", rate);
        return -EINVAL;
    }

    /*
     * Some codecs also need bit width programmed:
     * switch (width) {
     * case 16: ... case 24: ... case 32: ...
     * }
     */

    return regmap_write(priv->regmap, MY_CODEC_REG_SAMPLE_RATE, rate_reg);
}

/* ── DAI ops: mute_stream ──────────────────────────────────────────────────── */
static int my_codec_mute_stream(struct snd_soc_dai *dai, int mute, int stream)
{
    struct snd_soc_component *component = dai->component;
    struct my_codec_priv *priv = snd_soc_component_get_drvdata(component);

    if (stream != SNDRV_PCM_STREAM_PLAYBACK)
        return 0;

    return regmap_update_bits(priv->regmap,
        MY_CODEC_REG_MUTE,
        MY_CODEC_MUTE_DAC,
        mute ? MY_CODEC_MUTE_DAC : 0);
}

static const struct snd_soc_dai_ops my_codec_dai_ops = {
    .set_sysclk  = my_codec_set_sysclk,
    .set_fmt     = my_codec_set_fmt,
    .hw_params   = my_codec_hw_params,
    .mute_stream = my_codec_mute_stream,
};

/* ── DAI Driver ────────────────────────────────────────────────────────────── *
 * Describes the Digital Audio Interface capabilities.
 * Adjust rates and formats to match your codec's datasheet.
 */
static struct snd_soc_dai_driver my_codec_dai = {
    .name = MY_CODEC_DRIVER "-dai",
    .playback = {
        .stream_name    = "Playback",
        .channels_min   = 1,
        .channels_max   = 2,
        .rates          = SNDRV_PCM_RATE_8000_192000,
        .formats        = SNDRV_PCM_FMTBIT_S16_LE |
                          SNDRV_PCM_FMTBIT_S24_LE |
                          SNDRV_PCM_FMTBIT_S32_LE,
    },
    /*
     * Add capture capabilities if your codec has an ADC:
     * .capture = {
     *     .stream_name  = "Capture",
     *     .channels_min = 1,
     *     .channels_max = 2,
     *     .rates        = SNDRV_PCM_RATE_8000_48000,
     *     .formats      = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE,
     * },
     */
    .ops = &my_codec_dai_ops,
};

/* ── Component Driver ──────────────────────────────────────────────────────── */
static const struct snd_soc_component_driver my_codec_component_driver = {
    .controls           = my_codec_snd_controls,
    .num_controls       = ARRAY_SIZE(my_codec_snd_controls),
    .dapm_widgets       = my_codec_dapm_widgets,
    .num_dapm_widgets   = ARRAY_SIZE(my_codec_dapm_widgets),
    .dapm_routes        = my_codec_dapm_routes,
    .num_dapm_routes    = ARRAY_SIZE(my_codec_dapm_routes),
    .idle_bias_on       = 1,
    .use_pmdown_time    = 1,
    .endianness         = 1,
};

/* ── I2C probe ─────────────────────────────────────────────────────────────── */
static int my_codec_i2c_probe(struct i2c_client *i2c)
{
    struct my_codec_priv *priv;
    int ret;

    priv = devm_kzalloc(&i2c->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    priv->regmap = devm_regmap_init_i2c(i2c, &my_codec_regmap_config);
    if (IS_ERR(priv->regmap)) {
        ret = PTR_ERR(priv->regmap);
        dev_err(&i2c->dev, "regmap init failed: %d\n", ret);
        return ret;
    }

    i2c_set_clientdata(i2c, priv);

    /* Soft reset codec */
    ret = regmap_write(priv->regmap, MY_CODEC_REG_RESET, MY_CODEC_RESET_BIT);
    if (ret) {
        dev_err(&i2c->dev, "Reset failed: %d\n", ret);
        return ret;
    }

    /* Small delay for reset to complete — check your codec's datasheet */
    usleep_range(1000, 2000);

    ret = devm_snd_soc_register_component(&i2c->dev,
        &my_codec_component_driver,
        &my_codec_dai, 1);
    if (ret) {
        dev_err(&i2c->dev, "Failed to register component: %d\n", ret);
        return ret;
    }

    dev_info(&i2c->dev, "%s codec registered\n", MY_CODEC_NAME);
    return 0;
}

/* ── Device Tree match table ───────────────────────────────────────────────── */
static const struct of_device_id my_codec_of_match[] = {
    { .compatible = "myvendor," MY_CODEC_DRIVER },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_codec_of_match);

static const struct i2c_device_id my_codec_i2c_id[] = {
    { MY_CODEC_DRIVER, 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, my_codec_i2c_id);

static struct i2c_driver my_codec_i2c_driver = {
    .driver = {
        .name           = MY_CODEC_DRIVER,
        .of_match_table = my_codec_of_match,
    },
    .probe      = my_codec_i2c_probe,
    .id_table   = my_codec_i2c_id,
};

module_i2c_driver(my_codec_i2c_driver);

MODULE_DESCRIPTION("Generic ASoC Codec Driver Skeleton");
MODULE_AUTHOR("Your Name <your@email.com>");
MODULE_LICENSE("GPL v2");
