// SPDX-License-Identifier: GPL-2.0-only
/*
 * my_machine.c — Generic ASoC Machine Driver Skeleton
 *
 * The machine driver is the "glue" that ties together:
 *   - Platform driver (I2S controller / DMA — provided by SoC)
 *   - Codec driver    (my_codec.c)
 *   - DAI links       (who drives clocks, what formats are used)
 *
 * This is the file you'll modify most heavily per board.
 * For each new board, you typically only change:
 *   1. dai_link node names / compatible strings
 *   2. Clock setup in machine_startup()
 *   3. DAPM routes unique to this board (e.g., speaker amp GPIO)
 *
 * References:
 *   - sound/soc/bcm/     (RPi machine drivers — great reference)
 *   - sound/soc/samsung/ (Samsung SoC machine drivers)
 *   - Documentation/sound/soc/machine.rst
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio/consumer.h>
#include <sound/core.h>
#include <sound/soc.h>
#include <sound/jack.h>

#define MACHINE_NAME    "my-soundcard"

/* ── Private machine data ──────────────────────────────────────────────────── */
struct my_machine_priv {
    struct gpio_desc *spk_amp_gpio;     /* optional: speaker amp enable pin */
    struct snd_soc_jack headphone_jack;
};

/* ── Board-level DAPM widgets ──────────────────────────────────────────────── *
 * Add widgets specific to your board (speakers, headphone jacks, LEDs etc.)
 * These are in addition to codec DAPM widgets defined in my_codec.c.
 */
static const struct snd_soc_dapm_widget my_machine_dapm_widgets[] = {
    SND_SOC_DAPM_SPK("Speaker",    NULL),
    SND_SOC_DAPM_HP("Headphone",   NULL),
    SND_SOC_DAPM_MIC("Microphone", NULL),
};

/* ── Board-level DAPM routes ───────────────────────────────────────────────── *
 * Connect board-level widgets to codec pins.
 * "OUTL" and "OUTR" must match widget names in my_codec.c.
 */
static const struct snd_soc_dapm_route my_machine_dapm_routes[] = {
    { "Speaker",   NULL, "OUTL" },
    { "Speaker",   NULL, "OUTR" },
    { "Headphone", NULL, "OUTL" },
    { "Headphone", NULL, "OUTR" },
};

/* ── DAI link: startup callback ────────────────────────────────────────────── *
 * Called when a stream opens. Use this to:
 *   - Enable MCLK to the codec
 *   - Power on speaker amplifier
 *   - Set codec sysclk
 */
static int my_machine_startup(struct snd_pcm_substream *substream)
{
    struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
    struct snd_soc_card *card = rtd->card;
    struct my_machine_priv *priv = snd_soc_card_get_drvdata(card);

    /* Enable speaker amplifier GPIO (active high example) */
    if (priv->spk_amp_gpio)
        gpiod_set_value_cansleep(priv->spk_amp_gpio, 1);

    return 0;
}

/* ── DAI link: shutdown callback ───────────────────────────────────────────── */
static void my_machine_shutdown(struct snd_pcm_substream *substream)
{
    struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
    struct snd_soc_card *card = rtd->card;
    struct my_machine_priv *priv = snd_soc_card_get_drvdata(card);

    if (priv->spk_amp_gpio)
        gpiod_set_value_cansleep(priv->spk_amp_gpio, 0);
}

/* ── DAI link: hw_params ───────────────────────────────────────────────────── *
 * Called after codec hw_params. Use this to:
 *   - Set codec sysclk based on negotiated sample rate
 *   - Configure PLL if your codec has one
 *   - Set the CPU DAI's BCLK/LRCLK ratios
 */
static int my_machine_hw_params(struct snd_pcm_substream *substream,
                                 struct snd_pcm_hw_params *params)
{
    struct snd_soc_pcm_runtime *rtd = snd_soc_substream_to_rtd(substream);
    struct snd_soc_dai *codec_dai = snd_soc_rtd_to_codec(rtd, 0);
    unsigned int rate = params_rate(params);

    /*
     * Most codecs need MCLK = N × sample_rate.
     * Common multiples: 256fs, 384fs, 512fs.
     * Example: 48000 × 256 = 12.288 MHz
     *
     * Tell the codec driver what MCLK it will receive:
     */
    unsigned int mclk = rate * 256;

    int ret = snd_soc_dai_set_sysclk(codec_dai,
        0,          /* clk_id — 0 = MCLK, check your codec */
        mclk,
        SND_SOC_CLOCK_IN);

    if (ret < 0) {
        dev_err(rtd->dev, "Failed to set codec sysclk: %d\n", ret);
        return ret;
    }

    return 0;
}

static const struct snd_soc_ops my_machine_ops = {
    .startup    = my_machine_startup,
    .shutdown   = my_machine_shutdown,
    .hw_params  = my_machine_hw_params,
};

/* ── DAI Link ──────────────────────────────────────────────────────────────── *
 * Ties CPU DAI (I2S controller) ↔ Codec DAI.
 * For DT-based boards, cpu/codec are populated from Device Tree.
 */
SND_SOC_DAILINK_DEFS(playback,
    DAILINK_COMP_ARRAY(COMP_CPU("my-i2s-controller.0")),    /* CPU DAI node */
    DAILINK_COMP_ARRAY(COMP_CODEC("my-codec.0",             /* Codec I2C addr */
                                   "my_codec-dai")),         /* Codec DAI name */
    DAILINK_COMP_ARRAY(COMP_PLATFORM("my-i2s-controller.0")));

static struct snd_soc_dai_link my_machine_dai_links[] = {
    {
        .name           = "my-codec-playback",
        .stream_name    = "HiFi Playback",
        .ops            = &my_machine_ops,
        /*
         * DAI format: I2S, Normal clocks, CPU = master (drives BCK + LRCLK)
         * Change to SND_SOC_DAIFMT_CBP_CFP if codec is master.
         */
        .dai_fmt        = SND_SOC_DAIFMT_I2S |
                          SND_SOC_DAIFMT_NB_NF |
                          SND_SOC_DAIFMT_CBC_CFC,
        SND_SOC_DAILINK_REG(playback),
    },
};

/* ── Sound Card ────────────────────────────────────────────────────────────── */
static struct snd_soc_card my_sound_card = {
    .name               = MACHINE_NAME,
    .owner              = THIS_MODULE,
    .dai_link           = my_machine_dai_links,
    .num_links          = ARRAY_SIZE(my_machine_dai_links),
    .dapm_widgets       = my_machine_dapm_widgets,
    .num_dapm_widgets   = ARRAY_SIZE(my_machine_dapm_widgets),
    .dapm_routes        = my_machine_dapm_routes,
    .num_dapm_routes    = ARRAY_SIZE(my_machine_dapm_routes),
    .fully_routed       = true,     /* warn if any widget is unconnected */
};

/* ── Platform driver probe ─────────────────────────────────────────────────── */
static int my_machine_probe(struct platform_device *pdev)
{
    struct my_machine_priv *priv;
    int ret;

    priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
    if (!priv)
        return -ENOMEM;

    /* Optional: get speaker amplifier enable GPIO from DT */
    priv->spk_amp_gpio = devm_gpiod_get_optional(&pdev->dev,
        "spk-amp-en", GPIOD_OUT_LOW);
    if (IS_ERR(priv->spk_amp_gpio))
        return PTR_ERR(priv->spk_amp_gpio);

    my_sound_card.dev = &pdev->dev;
    snd_soc_card_set_drvdata(&my_sound_card, priv);

    /*
     * If using DT for DAI links (recommended for production drivers),
     * populate dai_link cpu/codec from device tree here using:
     *   snd_soc_of_parse_card_name()
     *   snd_soc_of_parse_audio_routing()
     */

    ret = devm_snd_soc_register_card(&pdev->dev, &my_sound_card);
    if (ret) {
        dev_err(&pdev->dev, "snd_soc_register_card failed: %d\n", ret);
        return ret;
    }

    dev_info(&pdev->dev, "Sound card '%s' registered\n", MACHINE_NAME);
    return 0;
}

static const struct of_device_id my_machine_of_match[] = {
    { .compatible = "myvendor,my-soundcard" },
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, my_machine_of_match);

static struct platform_driver my_machine_driver = {
    .probe  = my_machine_probe,
    .driver = {
        .name           = "my-machine",
        .of_match_table = my_machine_of_match,
    },
};

module_platform_driver(my_machine_driver);

MODULE_DESCRIPTION("Generic ASoC Machine Driver Skeleton");
MODULE_AUTHOR("Your Name <your@email.com>");
MODULE_LICENSE("GPL v2");
