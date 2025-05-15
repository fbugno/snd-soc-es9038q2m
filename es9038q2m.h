/* SPDX-License-Identifier: GPL-3.0-only */
/*
 * es9038q2m.h  --  ES9038Q2M codec driver
 *
 * Author: Felipe Bugno <felipe.bugno@filled.space>
 */

#ifndef _ES9038Q2M_H
#define _ES9038Q2M_H

#include <linux/regmap.h>

/* ES9038Q2M register map */

/* System registers */
#define ES9038Q2M_SYSTEM                 0x00
#define ES9038Q2M_INPUT_SEL              0x01
#define ES9038Q2M_MIXING                 0x02
#define ES9038Q2M_SPDIF_CFG              0x03
#define ES9038Q2M_AUTOMUTE_TIME          0x04
#define ES9038Q2M_AUTOMUTE_LEVEL         0x05
#define ES9038Q2M_DEEMPH                 0x06
#define ES9038Q2M_FILTER_MUTE            0x07
#define ES9038Q2M_GPIO12_CFG             0x08
#define ES9038Q2M_MASTER_MODE            0x0A
#define ES9038Q2M_SPDIF_SELECT           0x0B
#define ES9038Q2M_ASRC_DPLL_BW           0x0C
#define ES9038Q2M_THD_BYPASS             0x0D
#define ES9038Q2M_SOFT_START             0x0E
#define ES9038Q2M_VOLUME1                0x0F
#define ES9038Q2M_VOLUME2                0x10
#define ES9038Q2M_MASTER_TRIM1           0x11
#define ES9038Q2M_MASTER_TRIM2           0x12
#define ES9038Q2M_MASTER_TRIM3           0x13
#define ES9038Q2M_MASTER_TRIM4           0x14
#define ES9038Q2M_GPIO_INPUT_SEL         0x15
#define ES9038Q2M_THD_COMP_C2_1          0x16
#define ES9038Q2M_THD_COMP_C2_2          0x17
#define ES9038Q2M_THD_COMP_C3_1          0x18
#define ES9038Q2M_THD_COMP_C3_2          0x19
#define ES9038Q2M_GENERAL_CFG            0x1B
#define ES9038Q2M_GPIO_CFG               0x1D
#define ES9038Q2M_CP_CLK_1               0x1E
#define ES9038Q2M_CP_CLK_2               0x1F
#define ES9038Q2M_INTERRUPT_MASK         0x21
#define ES9038Q2M_NCO_1                  0x22
#define ES9038Q2M_NCO_2                  0x23
#define ES9038Q2M_NCO_3                  0x24
#define ES9038Q2M_NCO_4                  0x25
#define ES9038Q2M_GENERAL_CFG_2          0x27
#define ES9038Q2M_PROG_FIR_ADDR          0x28
#define ES9038Q2M_PROG_FIR_DATA_1        0x29
#define ES9038Q2M_PROG_FIR_DATA_2        0x2A
#define ES9038Q2M_PROG_FIR_DATA_3        0x2B
#define ES9038Q2M_PROG_FIR_CFG           0x2C
#define ES9038Q2M_LOW_POWER              0x2D
#define ES9038Q2M_ADC_CFG                0x2E
#define ES9038Q2M_ADC_FILTER_1           0x2F
#define ES9038Q2M_ADC_FILTER_2           0x30
#define ES9038Q2M_ADC_FILTER_3           0x31
#define ES9038Q2M_ADC_FILTER_4           0x32
#define ES9038Q2M_ADC_FILTER_5           0x33
#define ES9038Q2M_ADC_FILTER_6           0x34

/* Read-only registers */
#define ES9038Q2M_CHIP_ID                0x40
#define ES9038Q2M_GPIO_READBACK          0x41
#define ES9038Q2M_DPLL_NUM_1             0x42
#define ES9038Q2M_DPLL_NUM_2             0x43
#define ES9038Q2M_DPLL_NUM_3             0x44
#define ES9038Q2M_DPLL_NUM_4             0x45
#define ES9038Q2M_SPDIF_STATUS_1         0x46
/* SPDIF status registers continue through 0x5D */
#define ES9038Q2M_RESERVED_94            0x5E
#define ES9038Q2M_RESERVED_95            0x5F
#define ES9038Q2M_INPUT_STATUS           0x60
#define ES9038Q2M_RESERVED_97            0x61
#define ES9038Q2M_RESERVED_98            0x62
#define ES9038Q2M_RESERVED_99            0x63
#define ES9038Q2M_ADC_READBACK_1         0x64
#define ES9038Q2M_ADC_READBACK_2         0x65
#define ES9038Q2M_ADC_READBACK_3         0x66

extern const struct regmap_config es9038q2m_regmap_config;
//extern const struct dev_pm_ops es9038q2m_pm;

int es9038q2m_i2c_probe(struct i2c_client *i2c);
//void es9038q2m_i2c_remove(struct i2c_client *i2c);

#endif  /* _ES9038Q2M_H */
