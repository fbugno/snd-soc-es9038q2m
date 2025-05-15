// SPDX-License-Identifier: GPL-3.0-only
/*
 * es9038q2m.h  --  ES9038Q2M codec driver
 *
 * Author: Felipe Bugno <felipe.bugno@filled.space>
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/gpio/consumer.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/pm_runtime.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/initval.h>
#include <sound/tlv.h>
#include <sound/soc-dapm.h>

#include "es9038q2m.h"

/* Register bit definitions */
#define ES9038Q2M_FILTER_MUTE_MUTE      0x01

static const struct reg_default es9038q2m_reg_defaults[] = {
    { ES9038Q2M_SYSTEM,                 0x00 },
    { ES9038Q2M_INPUT_SEL,              0xCC },
    { ES9038Q2M_MIXING,                 0x34 },
    { ES9038Q2M_SPDIF_CFG,              0x40 },
    { ES9038Q2M_AUTOMUTE_TIME,          0x00 },
    { ES9038Q2M_AUTOMUTE_LEVEL,         0x68 },
    { ES9038Q2M_DEEMPH,                 0x42 },
    { ES9038Q2M_FILTER_MUTE,            0x80 },
    { ES9038Q2M_GPIO12_CFG,             0xDD },
    { ES9038Q2M_MASTER_MODE,            0x02 },
    { ES9038Q2M_SPDIF_SELECT,           0x00 },
    { ES9038Q2M_ASRC_DPLL_BW,           0x5A },
    { ES9038Q2M_THD_BYPASS,             0x40 },
    { ES9038Q2M_SOFT_START,             0x0A },
    { ES9038Q2M_VOLUME1,                0x50 },
    { ES9038Q2M_VOLUME2,                0x50 },
    { ES9038Q2M_MASTER_TRIM1,           0x7F },
    { ES9038Q2M_MASTER_TRIM2,           0xFF },
    { ES9038Q2M_MASTER_TRIM3,           0xFF },
    { ES9038Q2M_MASTER_TRIM4,           0xFF },
    { ES9038Q2M_GPIO_INPUT_SEL,         0x00 },
    { ES9038Q2M_THD_COMP_C2_1,          0x00 },
    { ES9038Q2M_THD_COMP_C2_2,          0x00 },
    { ES9038Q2M_THD_COMP_C3_1,          0x00 },
    { ES9038Q2M_THD_COMP_C3_2,          0x00 },
    { ES9038Q2M_GENERAL_CFG,            0xD4 },
    { ES9038Q2M_GPIO_CFG,               0x00 },
    { ES9038Q2M_CP_CLK_1,               0x00 },
    { ES9038Q2M_CP_CLK_2,               0x00 },
    { ES9038Q2M_INTERRUPT_MASK,         0x3C },
    { ES9038Q2M_NCO_1,                  0x00 },
    { ES9038Q2M_NCO_2,                  0x00 },
    { ES9038Q2M_NCO_3,                  0x00 },
    { ES9038Q2M_NCO_4,                  0x00 },
    { ES9038Q2M_GENERAL_CFG_2,          0x00 },
    { ES9038Q2M_PROG_FIR_ADDR,          0x00 },
    { ES9038Q2M_PROG_FIR_DATA_1,        0x00 },
    { ES9038Q2M_PROG_FIR_DATA_2,        0x00 },
    { ES9038Q2M_PROG_FIR_DATA_3,        0x00 },
    { ES9038Q2M_PROG_FIR_CFG,           0x00 },
    { ES9038Q2M_LOW_POWER,              0x04 },
    { ES9038Q2M_ADC_CFG,                0x00 },
    { ES9038Q2M_ADC_FILTER_1,           0x03 },
    { ES9038Q2M_ADC_FILTER_2,           0xE0 },
    { ES9038Q2M_ADC_FILTER_3,           0x04 },
    { ES9038Q2M_ADC_FILTER_4,           0x00 },
    { ES9038Q2M_ADC_FILTER_5,           0x04 },
    { ES9038Q2M_ADC_FILTER_6,           0x00 },
};

static bool es9038q2m_readable_reg(struct device *dev, unsigned int reg)
{
    /* All registers up to ADC_READBACK_3 are readable */
    if (reg <= ES9038Q2M_ADC_READBACK_3)
        return true;

    return false;
}

static bool es9038q2m_volatile_reg(struct device *dev, unsigned int reg)
{
    /* Read-only status registers are volatile */
    if (reg >= ES9038Q2M_CHIP_ID)
        return true;

    return false;
}

struct es9038q2m_priv {
    struct regmap *regmap;
    struct i2c_client *i2c;
    unsigned int rate;
    unsigned int fmt;
    unsigned int clock_frequency;
};

/* DAI operations */
static int es9038q2m_set_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
    struct snd_soc_component *component = dai->component;
    struct es9038q2m_priv *es9038q2m = snd_soc_component_get_drvdata(component);
    unsigned int master_mode;

    es9038q2m->fmt = fmt;

    /* Set master/slave mode */
    regmap_read(es9038q2m->regmap, ES9038Q2M_MASTER_MODE, &master_mode);

    switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
    case SND_SOC_DAIFMT_CBS_CFS: /* Codec is slave, CPU is master */
        master_mode &= ~0x80;
        break;
    case SND_SOC_DAIFMT_CBM_CFM: /* Codec is master, CPU is slave */
        master_mode |= 0x80;
        break;
    default:
        return -EINVAL;
    }

    regmap_write(es9038q2m->regmap, ES9038Q2M_MASTER_MODE, master_mode);

    /* Disable auto-select and set to serial data input */
    regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0x0C, 0x00);
    regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0x03, 0x00);

    /* Set format (I2S, left-justified, etc.) */
    switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
    case SND_SOC_DAIFMT_I2S:
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0x30, 0x00);
        break;
    case SND_SOC_DAIFMT_LEFT_J:
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0x30, 0x10);
        break;
    case SND_SOC_DAIFMT_RIGHT_J:
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0x30, 0x30);
        break;
    default:
        return -EINVAL;
    }

    return 0;
}

static int es9038q2m_hw_params(struct snd_pcm_substream *substream,
                             struct snd_pcm_hw_params *params,
                             struct snd_soc_dai *dai)
{
    struct snd_soc_component *component = dai->component;
    struct es9038q2m_priv *es9038q2m = snd_soc_component_get_drvdata(component);
    unsigned int rate = params_rate(params);
    unsigned int bits = params_width(params);
    int ret = 0;
    long nco = 0;

    es9038q2m->rate = rate;

    /* go down */
    ret = regmap_update_bits(es9038q2m->regmap, ES9038Q2M_SOFT_START, 0x80, 0x00);
    if (ret < 0)
        return ret;

    /* Set bit depth */
    switch (bits) {
    case 16:
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0xC0, 0x00);
        break;
    case 24:
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0xC0, 0x40);
        break;
    case 32:
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_INPUT_SEL, 0xC0, 0x80);
        break;
    default:
        return -EINVAL;
    }

    /* Mumbo jumbo to set the sample rate
     FOR NOW ONLY MASTER MODE, MUST DISABLE ALL THAT FOR SLAVE */
    regmap_update_bits(es9038q2m->regmap, ES9038Q2M_MASTER_MODE, 0x10, 0x00);
    nco = (rate * 0x100000000) / es9038q2m->clock_frequency;
    dev_info(&es9038q2m->i2c->dev, "NCO: %lu\n", nco);
    regmap_write(es9038q2m->regmap, ES9038Q2M_NCO_4, (nco >> 24) & 0xFF);
    regmap_write(es9038q2m->regmap, ES9038Q2M_NCO_3, (nco >> 16) & 0xFF);
    regmap_write(es9038q2m->regmap, ES9038Q2M_NCO_2, (nco >> 8) & 0xFF);
    regmap_write(es9038q2m->regmap, ES9038Q2M_NCO_1, nco & 0xFF);

    /* go up! */
    ret = regmap_update_bits(es9038q2m->regmap, ES9038Q2M_SOFT_START, 0x80, 0x80);
    if (ret < 0)
        return ret;

    return ret;
}

static int es9038q2m_digital_mute(struct snd_soc_dai *dai, int mute, int direction)
{
    struct snd_soc_component *component = dai->component;
    struct es9038q2m_priv *es9038q2m = snd_soc_component_get_drvdata(component);

    if (mute)
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_FILTER_MUTE,
                          ES9038Q2M_FILTER_MUTE_MUTE, 0x01);
    else
        regmap_update_bits(es9038q2m->regmap, ES9038Q2M_FILTER_MUTE,
                          ES9038Q2M_FILTER_MUTE_MUTE, 0x0);

    return 0;
}

static int es9038q2m_set_bias_level(struct snd_soc_component *component,
                                  enum snd_soc_bias_level level)
{
    struct es9038q2m_priv *es9038q2m = snd_soc_component_get_drvdata(component);
    int ret = 0;

    /* tô com preguiça caraio */
    switch (level) {
    case SND_SOC_BIAS_ON:
        /* Full power mode */
        break;
    case SND_SOC_BIAS_PREPARE:
        /* Prepare for audio playback */
        break;
    case SND_SOC_BIAS_STANDBY:
        /* Low-power standby state */
        break;
    case SND_SOC_BIAS_OFF:
        /* Power off */
        break;
    }

    return ret;
}

/* ALSA controls */
static const DECLARE_TLV_DB_SCALE(volume_tlv, -12750, 50, 1);

static const struct snd_kcontrol_new es9038q2m_controls[] = {
    SOC_DOUBLE_R_TLV("Master Playback Volume", ES9038Q2M_VOLUME1, ES9038Q2M_VOLUME2,
                    0, 255, 1, volume_tlv),
    SOC_SINGLE("Mute Switch", ES9038Q2M_FILTER_MUTE, 0, 1, 0),
};

/* Component operations */
static const struct snd_soc_component_driver es9038q2m_component_driver = {
    .set_bias_level = es9038q2m_set_bias_level,
    .controls = es9038q2m_controls,
    .num_controls = ARRAY_SIZE(es9038q2m_controls),
    .idle_bias_on = 1,
    .use_pmdown_time = 1,
    .endianness = 1,
};

static const struct snd_soc_dai_ops es9038q2m_dai_ops = {
    .set_fmt = es9038q2m_set_fmt,
    .hw_params = es9038q2m_hw_params,
    .mute_stream = es9038q2m_digital_mute,
};

static struct snd_soc_dai_driver es9038q2m_dai = {
    .name = "es9038q2m",
    .playback = {
        .stream_name = "Playback",
        .channels_min = 2,
        .channels_max = 2,
        .rates = SNDRV_PCM_RATE_8000_192000,
        .formats = SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE |
                  SNDRV_PCM_FMTBIT_S32_LE | SNDRV_PCM_FMTBIT_DSD_U8 |
                  SNDRV_PCM_FMTBIT_DSD_U16_LE,
    },
    .ops = &es9038q2m_dai_ops,
};

const struct regmap_config es9038q2m_regmap_config = {
    .reg_bits = 8,
    .val_bits = 8,
    .max_register = ES9038Q2M_ADC_READBACK_3,
    .reg_defaults = es9038q2m_reg_defaults,
    .num_reg_defaults = ARRAY_SIZE(es9038q2m_reg_defaults),
    .cache_type = REGCACHE_MAPLE,
    .readable_reg = es9038q2m_readable_reg,
    .volatile_reg = es9038q2m_volatile_reg,
};

int es9038q2m_i2c_probe(struct i2c_client *i2c)
{
    struct es9038q2m_priv *es9038q2m;
    struct regmap *regmap;
    int ret;
    unsigned int chip_id;
    struct device *dev = &i2c->dev;
    struct device_node *np = dev->of_node;

    es9038q2m = devm_kzalloc(dev, sizeof(*es9038q2m), GFP_KERNEL);
    if (!es9038q2m)
        return -ENOMEM;

    ret = of_property_read_u32(np, "clock-frequency", &es9038q2m->clock_frequency);
    if (ret < 0) {
            dev_err(dev, "Failed to retrieve clock frequency for the codec\n");
            return ret;
    }

    dev_info(dev, "Clock frequency: %u Hz\n", es9038q2m->clock_frequency);

    regmap = devm_regmap_init_i2c(i2c, &es9038q2m_regmap_config);
    if (IS_ERR(regmap)) {
        ret = PTR_ERR(regmap);
        dev_err(dev, "Failed to allocate regmap: %d\n", ret);
        return ret;
    }

    es9038q2m->regmap = regmap;
    es9038q2m->i2c = i2c;

    /* Read chip ID to verify communication */
    ret = regmap_read(regmap, ES9038Q2M_CHIP_ID, &chip_id);
    if (ret < 0) {
        dev_err(dev, "Failed to read chip ID: %d\n", ret);
        return ret;
    }

    /* Verify chip ID (bits [7:2] should be 0x1C) */
    if ((chip_id >> 2) != 0x1C) {
        dev_err(dev, "Unexpected chip ID: 0x%x\n", chip_id);
        return -EINVAL;
    }

    i2c_set_clientdata(i2c, es9038q2m);

    dev_info(dev, "ES9038Q2M DAC initialized at address 0x%x\n", i2c->addr);

    /* reset the codec */
    regmap_write(es9038q2m->regmap, ES9038Q2M_SYSTEM, 0x01);

    ret = devm_snd_soc_register_component(dev, &es9038q2m_component_driver,
                                          &es9038q2m_dai, 1);
    if (ret < 0) {
        dev_err(dev, "Failed to register CODEC: %d\n", ret);
        return ret;
    }

    return 0;
}

EXPORT_SYMBOL_GPL(es9038q2m_i2c_probe);

// void es9038q2m_i2c_remove(struct i2c_client *i2c)
// {
//     /* No specific cleanup needed as we used devm_* functions */
//     return 0;
// }

static const struct i2c_device_id es9038q2m_i2c_id[] = {
    { "es9038q2m", 0 },
    { }
};
MODULE_DEVICE_TABLE(i2c, es9038q2m_i2c_id);

#if defined(CONFIG_OF)
static const struct of_device_id es9038q2m_of_match[] = {
    { .compatible = "ess,es9038q2m", },
    { }
};
MODULE_DEVICE_TABLE(of, es9038q2m_of_match);
#endif

static struct i2c_driver es9038q2m_i2c_driver = {
    .driver = {
        .name = "es9038q2m",
        .of_match_table = of_match_ptr(es9038q2m_of_match),
    },
    .probe = es9038q2m_i2c_probe,
//    .remove = es9038q2m_i2c_remove,
    .id_table = es9038q2m_i2c_id,
};

module_i2c_driver(es9038q2m_i2c_driver);

MODULE_DESCRIPTION("ESS Technology ES9038Q2M DAC driver");
MODULE_AUTHOR("Felipe Bugno <felipe.bugno@filled.space>");
MODULE_LICENSE("GPL v2");
