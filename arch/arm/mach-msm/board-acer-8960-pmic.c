/* Copyright (c) 2011-2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/interrupt.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#if defined(CONFIG_MACH_ACER_A11RD)
#include <linux/leds.h>
#include <linux/leds-pm8xxx.h>
#endif
#ifdef CONFIG_PMIC8XXX_VIBRATOR
#include <linux/mfd/pm8xxx/vibrator.h>
#endif
#include <linux/msm_ssbi.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/restart.h>
#include "devices.h"
#include "board-acer-8960.h"
#ifdef CONFIG_ACER_BQ27520
#include <linux/i2c/acer_bq27520.h>
#endif

extern int acer_board_id;
extern int acer_sku_id;
int acer_sku_wi_gauge = 0;

struct pm8xxx_gpio_init {
	unsigned			gpio;
	struct pm_gpio			config;
};

struct pm8xxx_mpp_init {
	unsigned			mpp;
	struct pm8xxx_mpp_config_data	config;
};

#define PM8XXX_GPIO_INIT(_gpio, _dir, _buf, _val, _pull, _vin, _out_strength, \
			_func, _inv, _disable) \
{ \
	.gpio	= PM8921_GPIO_PM_TO_SYS(_gpio), \
	.config	= { \
		.direction	= _dir, \
		.output_buffer	= _buf, \
		.output_value	= _val, \
		.pull		= _pull, \
		.vin_sel	= _vin, \
		.out_strength	= _out_strength, \
		.function	= _func, \
		.inv_int_pol	= _inv, \
		.disable_pin	= _disable, \
	} \
}

#define PM8XXX_MPP_INIT(_mpp, _type, _level, _control) \
{ \
	.mpp	= PM8921_MPP_PM_TO_SYS(_mpp), \
	.config	= { \
		.type		= PM8XXX_MPP_TYPE_##_type, \
		.level		= _level, \
		.control	= PM8XXX_MPP_##_control, \
	} \
}

#define PM8XXX_GPIO_DISABLE(_gpio) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, 0, 0, 0, PM_GPIO_VIN_S4, \
			 0, 0, 0, 1)

#define PM8XXX_GPIO_OUTPUT(_gpio, _val) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_INPUT(_gpio, _pull) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_IN, PM_GPIO_OUT_BUF_CMOS, 0, \
			_pull, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_NO, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_FUNC(_gpio, _val, _func) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			PM_GPIO_STRENGTH_HIGH, \
			_func, 0, 0)

#define PM8XXX_GPIO_OUTPUT_VIN(_gpio, _val, _vin) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, _vin, \
			PM_GPIO_STRENGTH_HIGH, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

#define PM8XXX_GPIO_OUTPUT_STRENGTH(_gpio, _val, _out_strength) \
	PM8XXX_GPIO_INIT(_gpio, PM_GPIO_DIR_OUT, PM_GPIO_OUT_BUF_CMOS, _val, \
			PM_GPIO_PULL_NO, PM_GPIO_VIN_S4, \
			_out_strength, \
			PM_GPIO_FUNC_NORMAL, 0, 0)

/* Initial PM8921 GPIO configurations */
#if defined(CONFIG_MACH_ACER_A11RD)
static struct pm8xxx_gpio_init a11_evt_pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_DISABLE(1),
	PM8XXX_GPIO_DISABLE(2),
	PM8XXX_GPIO_DISABLE(3),
	PM8XXX_GPIO_DISABLE(4),
	PM8XXX_GPIO_DISABLE(5),
	PM8XXX_GPIO_DISABLE(6),
	PM8XXX_GPIO_DISABLE(8),
	PM8XXX_GPIO_DISABLE(9),
	PM8XXX_GPIO_DISABLE(10),
	PM8XXX_GPIO_DISABLE(11),
	PM8XXX_GPIO_DISABLE(12),
	PM8XXX_GPIO_DISABLE(13),
	PM8XXX_GPIO_DISABLE(14),
	PM8XXX_GPIO_DISABLE(15),
	PM8XXX_GPIO_DISABLE(16),
	PM8XXX_GPIO_DISABLE(17),
	PM8XXX_GPIO_DISABLE(19),
	PM8XXX_GPIO_DISABLE(20),
	PM8XXX_GPIO_DISABLE(21),
	PM8XXX_GPIO_DISABLE(22),
	PM8XXX_GPIO_OUTPUT(23, PM_GPIO_PULL_NO), /* VLTG_SEL */
	PM8XXX_GPIO_DISABLE(24),
	PM8XXX_GPIO_DISABLE(25),
	PM8XXX_GPIO_DISABLE(26),
	PM8XXX_GPIO_DISABLE(28),
	PM8XXX_GPIO_DISABLE(31),
	PM8XXX_GPIO_DISABLE(32),
	PM8XXX_GPIO_OUTPUT(33, PM_GPIO_PULL_NO), /* LCDC_P5V_EN */
	PM8XXX_GPIO_OUTPUT_STRENGTH(34, 0, PM_GPIO_STRENGTH_MED), /* WCD9310_REST_N */
	PM8XXX_GPIO_OUTPUT(35, PM_GPIO_PULL_NO), /* EXT_BUCK_EN */
	PM8XXX_GPIO_OUTPUT(36, PM_GPIO_PULL_NO), /* USIM_DETECT_N */
	PM8XXX_GPIO_DISABLE(37),
	PM8XXX_GPIO_DISABLE(38),
	PM8XXX_GPIO_DISABLE(40),
	PM8XXX_GPIO_DISABLE(41),
	PM8XXX_GPIO_OUTPUT(42, PM_GPIO_PULL_NO), /* LCDC_N5V_EN */
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(43, 0),                  /* Sharp Display Reset */
#else
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
#endif
};

#elif defined(CONFIG_MACH_ACER_A9)
static struct pm8xxx_gpio_init a9_dvt1_pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_DISABLE(1),
	PM8XXX_GPIO_DISABLE(2),
	PM8XXX_GPIO_DISABLE(3),
	PM8XXX_GPIO_DISABLE(4),
	PM8XXX_GPIO_DISABLE(5),
	PM8XXX_GPIO_DISABLE(6),
	PM8XXX_GPIO_DISABLE(8),
	PM8XXX_GPIO_DISABLE(9),
	PM8XXX_GPIO_DISABLE(10),
	PM8XXX_GPIO_DISABLE(11),
	PM8XXX_GPIO_DISABLE(12),
	PM8XXX_GPIO_DISABLE(13),
	PM8XXX_GPIO_DISABLE(14),
	PM8XXX_GPIO_DISABLE(15),
	PM8XXX_GPIO_DISABLE(16),
	PM8XXX_GPIO_DISABLE(17),
	PM8XXX_GPIO_DISABLE(19),
	PM8XXX_GPIO_DISABLE(20),
	PM8XXX_GPIO_DISABLE(21),
	PM8XXX_GPIO_DISABLE(22),
	PM8XXX_GPIO_DISABLE(23),
	PM8XXX_GPIO_DISABLE(24),
	PM8XXX_GPIO_DISABLE(25),
	PM8XXX_GPIO_INPUT(26, PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_DISABLE(28),
	PM8XXX_GPIO_DISABLE(31),
	PM8XXX_GPIO_DISABLE(32),
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_DISABLE(33),
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(33, 0),
#else
	PM8XXX_GPIO_DISABLE(33),
#endif
	PM8XXX_GPIO_OUTPUT_STRENGTH(34, 0, PM_GPIO_STRENGTH_MED), /* WCD9310_REST_N */
	PM8XXX_GPIO_DISABLE(35),
	PM8XXX_GPIO_DISABLE(36),
	PM8XXX_GPIO_DISABLE(37),
	PM8XXX_GPIO_DISABLE(38),
	PM8XXX_GPIO_DISABLE(40),
	PM8XXX_GPIO_DISABLE(41),
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_DISABLE(42),
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(42, 0),
#else
	PM8XXX_GPIO_DISABLE(42),
#endif
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(43, 0),                  /* Sharp Display Reset */
#else
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
#endif
};

static struct pm8xxx_gpio_init a9_dvt1_2_pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_DISABLE(1),
	PM8XXX_GPIO_DISABLE(2),
	PM8XXX_GPIO_DISABLE(3),
	PM8XXX_GPIO_DISABLE(4),
	PM8XXX_GPIO_DISABLE(8),
	PM8XXX_GPIO_DISABLE(9),
	PM8XXX_GPIO_DISABLE(10),
	PM8XXX_GPIO_DISABLE(11),
	PM8XXX_GPIO_DISABLE(12),
	PM8XXX_GPIO_DISABLE(13),
	PM8XXX_GPIO_DISABLE(14),
	PM8XXX_GPIO_DISABLE(15),
	PM8XXX_GPIO_DISABLE(16),
	PM8XXX_GPIO_DISABLE(17),
	PM8XXX_GPIO_DISABLE(19),
	PM8XXX_GPIO_DISABLE(20),
	PM8XXX_GPIO_DISABLE(21),
	PM8XXX_GPIO_DISABLE(22),
	PM8XXX_GPIO_DISABLE(23),
	PM8XXX_GPIO_DISABLE(24),
	PM8XXX_GPIO_DISABLE(25),
	PM8XXX_GPIO_INPUT(26, PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_DISABLE(28),
	PM8XXX_GPIO_DISABLE(31),
	PM8XXX_GPIO_DISABLE(32),
	PM8XXX_GPIO_DISABLE(33),
	PM8XXX_GPIO_OUTPUT_STRENGTH(34, 0, PM_GPIO_STRENGTH_MED), /* WCD9310_REST_N */
	PM8XXX_GPIO_DISABLE(35),
	PM8XXX_GPIO_DISABLE(36),
	PM8XXX_GPIO_DISABLE(37),
	PM8XXX_GPIO_DISABLE(38),
	PM8XXX_GPIO_DISABLE(40),
	PM8XXX_GPIO_DISABLE(41),
	PM8XXX_GPIO_DISABLE(42),
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
	PM8XXX_GPIO_DISABLE(44),
};

static struct pm8xxx_gpio_init a9_dvt1_3_pm8921_gpios[] __initdata = {
	PM8XXX_GPIO_DISABLE(1),
	PM8XXX_GPIO_DISABLE(2),
	PM8XXX_GPIO_DISABLE(3),
	PM8XXX_GPIO_DISABLE(4),
	PM8XXX_GPIO_DISABLE(8),
	PM8XXX_GPIO_DISABLE(9),
	PM8XXX_GPIO_DISABLE(10),
	PM8XXX_GPIO_DISABLE(11),
	PM8XXX_GPIO_DISABLE(12),
	PM8XXX_GPIO_DISABLE(13),
	PM8XXX_GPIO_DISABLE(14),
	PM8XXX_GPIO_DISABLE(15),
	PM8XXX_GPIO_DISABLE(16),
	PM8XXX_GPIO_DISABLE(17),
	PM8XXX_GPIO_DISABLE(19),
	PM8XXX_GPIO_DISABLE(20),
	PM8XXX_GPIO_DISABLE(21),
	PM8XXX_GPIO_DISABLE(22),
	PM8XXX_GPIO_DISABLE(23),
	PM8XXX_GPIO_DISABLE(25),
	PM8XXX_GPIO_INPUT(26, PM_GPIO_PULL_UP_30), /* SD_CARD_DET_N */
	PM8XXX_GPIO_DISABLE(28),
	PM8XXX_GPIO_DISABLE(31),
	PM8XXX_GPIO_DISABLE(32),
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_DISABLE(33),
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(33, 0),
#else
	PM8XXX_GPIO_DISABLE(33),
#endif
	PM8XXX_GPIO_OUTPUT_STRENGTH(34, 0, PM_GPIO_STRENGTH_MED), /* WCD9310_REST_N */
	PM8XXX_GPIO_DISABLE(35),
	PM8XXX_GPIO_DISABLE(36),
	PM8XXX_GPIO_DISABLE(37),
	PM8XXX_GPIO_DISABLE(38),
	PM8XXX_GPIO_DISABLE(40),
	PM8XXX_GPIO_DISABLE(41),
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_DISABLE(42),
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(42, 0),
#else
	PM8XXX_GPIO_DISABLE(42),
#endif
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	PM8XXX_GPIO_OUTPUT(43, 0),                  /* Sharp Display Reset */
#else
	PM8XXX_GPIO_OUTPUT(43, PM_GPIO_PULL_UP_30), /* DISP_RESET_N */
#endif
	PM8XXX_GPIO_DISABLE(44),
};
#endif

/* Initial PM8921 MPP configurations */
#if defined(CONFIG_MACH_ACER_A11RD)
static struct pm8xxx_mpp_init a11_evt_pm8921_mpps[] __initdata = {
	PM8XXX_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW), // NC
	PM8XXX_MPP_INIT(8, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(9, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(10, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(11, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH6, AOUT_CTRL_DISABLE), /* For Headset ADC */
	PM8XXX_MPP_INIT(12, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH6, AOUT_CTRL_DISABLE), // Project ID
};
#elif defined(CONFIG_MACH_ACER_A9)
static struct pm8xxx_mpp_init a9_dvt1_pm8921_mpps[] __initdata = {
	PM8XXX_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW), // Enable MHL 5V
	PM8XXX_MPP_INIT(8, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(9, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(10, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(11, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(12, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
};

static struct pm8xxx_mpp_init a9_dvt1_2_pm8921_mpps[] __initdata = {
	PM8XXX_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW), // Enable MHL 5V
	PM8XXX_MPP_INIT(8, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(9, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(10, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(11, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(12, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
};

static struct pm8xxx_mpp_init a9_dvt1_3_pm8921_mpps[] __initdata = {
	PM8XXX_MPP_INIT(7, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW), // Enable MHL 5V
	PM8XXX_MPP_INIT(8, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(9, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(10, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
	PM8XXX_MPP_INIT(11, A_INPUT, PM8XXX_MPP_AIN_AMUX_CH6, AOUT_CTRL_DISABLE),
	PM8XXX_MPP_INIT(12, D_OUTPUT, PM8921_MPP_DIG_LEVEL_S4, DOUT_CTRL_LOW),
};
#endif

void __init msm8960_pm8921_gpio_mpp_init(void)
{
	int i, rc;
	struct pm8xxx_gpio_init *pm8921_gpios = NULL;
	struct pm8xxx_mpp_init *pm8921_mpps = NULL;
	int pm8921_gpios_count = 0;
	int pm8921_mpps_count = 0;

#if defined(CONFIG_MACH_ACER_A11RD)
	pr_info("use A11 pm8921 init data");
	pm8921_gpios = a11_evt_pm8921_gpios;
	pm8921_gpios_count = ARRAY_SIZE(a11_evt_pm8921_gpios);
	pm8921_mpps = a11_evt_pm8921_mpps;
	pm8921_mpps_count = ARRAY_SIZE(a11_evt_pm8921_mpps);
#elif defined(CONFIG_MACH_ACER_A9)
	if (acer_board_id <= HW_ID_DVT1) {
		pr_info("use DVT1 pm8921 init data");
		pm8921_gpios = a9_dvt1_pm8921_gpios;
		pm8921_gpios_count = ARRAY_SIZE(a9_dvt1_pm8921_gpios);
		pm8921_mpps = a9_dvt1_pm8921_mpps;
		pm8921_mpps_count = ARRAY_SIZE(a9_dvt1_pm8921_mpps);
	} else if (acer_board_id <= HW_ID_DVT1_2) {
		pr_info("use DVT1-2 pm8921 init data");
		pm8921_gpios = a9_dvt1_2_pm8921_gpios;
		pm8921_gpios_count = ARRAY_SIZE(a9_dvt1_2_pm8921_gpios);
		pm8921_mpps = a9_dvt1_2_pm8921_mpps;
		pm8921_mpps_count = ARRAY_SIZE(a9_dvt1_2_pm8921_mpps);
	} else {
		pr_info("use DVT1-3 pm8921 init data");
		pm8921_gpios = a9_dvt1_3_pm8921_gpios;
		pm8921_gpios_count = ARRAY_SIZE(a9_dvt1_3_pm8921_gpios);
		pm8921_mpps = a9_dvt1_3_pm8921_mpps;
		pm8921_mpps_count = ARRAY_SIZE(a9_dvt1_3_pm8921_mpps);
	}
#endif

	if ((pm8921_gpios == NULL) || (pm8921_mpps == NULL)) {
		pr_info("No match pmic8921 gpios and mpps table");
		return;
	}

	for (i = 0; i < pm8921_gpios_count; i++) {
		rc = pm8xxx_gpio_config(pm8921_gpios[i].gpio,
					&pm8921_gpios[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_gpio_config: rc=%d\n", __func__, rc);
			break;
		}
	}

	for (i = 0; i < pm8921_mpps_count; i++) {
		rc = pm8xxx_mpp_config(pm8921_mpps[i].mpp,
					&pm8921_mpps[i].config);
		if (rc) {
			pr_err("%s: pm8xxx_mpp_config: rc=%d\n", __func__, rc);
			break;
		}
	}
}

static struct pm8xxx_adc_amux pm8xxx_adc_channels_data[] = {
	{"vcoin", CHANNEL_VCOIN, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vbat", CHANNEL_VBAT, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"dcin", CHANNEL_DCIN, CHAN_PATH_SCALING4, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ichg", CHANNEL_ICHG, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"vph_pwr", CHANNEL_VPH_PWR, CHAN_PATH_SCALING2, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"ibat", CHANNEL_IBAT, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"batt_therm", CHANNEL_BATT_THERM, CHAN_PATH_SCALING1, AMUX_RSV2,
		ADC_DECIMATION_TYPE2, ADC_SCALE_BATT_THERM},
	{"batt_id", CHANNEL_BATT_ID, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"usbin", CHANNEL_USBIN, CHAN_PATH_SCALING3, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pmic_therm", CHANNEL_DIE_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PMIC_THERM},
	{"625mv", CHANNEL_625MV, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"125v", CHANNEL_125V, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"chg_temp", CHANNEL_CHG_TEMP, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_DEFAULT},
	{"pa_therm1", ADC_MPP_1_AMUX8, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"xo_therm", CHANNEL_MUXOFF, CHAN_PATH_SCALING1, AMUX_RSV0,
		ADC_DECIMATION_TYPE2, ADC_SCALE_XOTHERM},
	{"pa_therm0", ADC_MPP_1_AMUX3, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE2, ADC_SCALE_PA_THERM},
	{"hs_butt", ADC_MPP_1_AMUX6, CHAN_PATH_SCALING1, AMUX_RSV1,
		ADC_DECIMATION_TYPE4, ADC_SCALE_DEFAULT},
};

static struct pm8xxx_adc_properties pm8xxx_adc_data = {
	.adc_vdd_reference	= 1800, /* milli-voltage for this adc */
	.bitresolution		= 15,
	.bipolar                = 0,
};

static struct pm8xxx_adc_platform_data pm8xxx_adc_pdata = {
	.adc_channel            = pm8xxx_adc_channels_data,
	.adc_num_board_channel  = ARRAY_SIZE(pm8xxx_adc_channels_data),
	.adc_prop               = &pm8xxx_adc_data,
	.adc_mpp_base		= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_irq_platform_data pm8xxx_irq_pdata __devinitdata = {
	.irq_base		= PM8921_IRQ_BASE,
	.devirq			= MSM_GPIO_TO_INT(104),
	.irq_trigger_flag	= IRQF_TRIGGER_LOW,
};

static struct pm8xxx_gpio_platform_data pm8xxx_gpio_pdata __devinitdata = {
	.gpio_base	= PM8921_GPIO_PM_TO_SYS(1),
};

static struct pm8xxx_mpp_platform_data pm8xxx_mpp_pdata __devinitdata = {
	.mpp_base	= PM8921_MPP_PM_TO_SYS(1),
};

static struct pm8xxx_rtc_platform_data pm8xxx_rtc_pdata __devinitdata = {
	.rtc_write_enable       = true,
	.rtc_alarm_powerup	= false,
};

static struct pm8xxx_pwrkey_platform_data pm8xxx_pwrkey_pdata = {
	.pull_up		= 1,
	.kpd_trigger_delay_us	= 15625,
	.wakeup			= 1,
};

/* Rotate lock key is not available so use F1 */
#define KEY_ROTATE_LOCK KEY_F1

static const unsigned int keymap_liquid[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(1, 3, KEY_ROTATE_LOCK),
	KEY(1, 4, KEY_HOME),
};

static struct matrix_keymap_data keymap_data_liquid = {
	.keymap_size    = ARRAY_SIZE(keymap_liquid),
	.keymap         = keymap_liquid,
};

static struct pm8xxx_keypad_platform_data keypad_data_liquid = {
	.input_name             = "keypad_8960_liquid",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 2,
	.num_cols               = 5,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data_liquid,
};


static const unsigned int keymap[] = {
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(0, 2, KEY_CAMERA_SNAPSHOT),
	KEY(0, 3, KEY_CAMERA_FOCUS),
};

static struct matrix_keymap_data keymap_data = {
	.keymap_size    = ARRAY_SIZE(keymap),
	.keymap         = keymap,
};

static struct pm8xxx_keypad_platform_data keypad_data = {
	.input_name             = "keypad_8960",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 1,
	.num_cols               = 5,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data,
};

static const unsigned int keymap_sim[] = {
	KEY(0, 0, KEY_7),
	KEY(0, 1, KEY_DOWN),
	KEY(0, 2, KEY_UP),
	KEY(0, 3, KEY_RIGHT),
	KEY(0, 4, KEY_ENTER),
	KEY(0, 5, KEY_L),
	KEY(0, 6, KEY_BACK),
	KEY(0, 7, KEY_M),

	KEY(1, 0, KEY_LEFT),
	KEY(1, 1, KEY_SEND),
	KEY(1, 2, KEY_1),
	KEY(1, 3, KEY_4),
	KEY(1, 4, KEY_CLEAR),
	KEY(1, 5, KEY_MSDOS),
	KEY(1, 6, KEY_SPACE),
	KEY(1, 7, KEY_COMMA),

	KEY(2, 0, KEY_6),
	KEY(2, 1, KEY_5),
	KEY(2, 2, KEY_8),
	KEY(2, 3, KEY_3),
	KEY(2, 4, KEY_NUMERIC_STAR),
	KEY(2, 5, KEY_UP),
	KEY(2, 6, KEY_DOWN),
	KEY(2, 7, KEY_LEFTSHIFT),

	KEY(3, 0, KEY_9),
	KEY(3, 1, KEY_NUMERIC_POUND),
	KEY(3, 2, KEY_0),
	KEY(3, 3, KEY_2),
	KEY(3, 4, KEY_SLEEP),
	KEY(3, 5, KEY_F1),
	KEY(3, 6, KEY_F2),
	KEY(3, 7, KEY_F3),

	KEY(4, 0, KEY_BACK),
	KEY(4, 1, KEY_HOME),
	KEY(4, 2, KEY_MENU),
	KEY(4, 3, KEY_VOLUMEUP),
	KEY(4, 4, KEY_VOLUMEDOWN),
	KEY(4, 5, KEY_F4),
	KEY(4, 6, KEY_F5),
	KEY(4, 7, KEY_F6),

	KEY(5, 0, KEY_R),
	KEY(5, 1, KEY_T),
	KEY(5, 2, KEY_Y),
	KEY(5, 3, KEY_LEFTALT),
	KEY(5, 4, KEY_KPENTER),
	KEY(5, 5, KEY_Q),
	KEY(5, 6, KEY_W),
	KEY(5, 7, KEY_E),

	KEY(6, 0, KEY_F),
	KEY(6, 1, KEY_G),
	KEY(6, 2, KEY_H),
	KEY(6, 3, KEY_CAPSLOCK),
	KEY(6, 4, KEY_PAGEUP),
	KEY(6, 5, KEY_A),
	KEY(6, 6, KEY_S),
	KEY(6, 7, KEY_D),

	KEY(7, 0, KEY_V),
	KEY(7, 1, KEY_B),
	KEY(7, 2, KEY_N),
	KEY(7, 3, KEY_MENU),
	KEY(7, 4, KEY_PAGEDOWN),
	KEY(7, 5, KEY_Z),
	KEY(7, 6, KEY_X),
	KEY(7, 7, KEY_C),

	KEY(8, 0, KEY_P),
	KEY(8, 1, KEY_J),
	KEY(8, 2, KEY_K),
	KEY(8, 3, KEY_INSERT),
	KEY(8, 4, KEY_LINEFEED),
	KEY(8, 5, KEY_U),
	KEY(8, 6, KEY_I),
	KEY(8, 7, KEY_O),

	KEY(9, 0, KEY_4),
	KEY(9, 1, KEY_5),
	KEY(9, 2, KEY_6),
	KEY(9, 3, KEY_7),
	KEY(9, 4, KEY_8),
	KEY(9, 5, KEY_1),
	KEY(9, 6, KEY_2),
	KEY(9, 7, KEY_3),

	KEY(10, 0, KEY_F7),
	KEY(10, 1, KEY_F8),
	KEY(10, 2, KEY_F9),
	KEY(10, 3, KEY_F10),
	KEY(10, 4, KEY_FN),
	KEY(10, 5, KEY_9),
	KEY(10, 6, KEY_0),
	KEY(10, 7, KEY_DOT),

	KEY(11, 0, KEY_LEFTCTRL),
	KEY(11, 1, KEY_F11),
	KEY(11, 2, KEY_ENTER),
	KEY(11, 3, KEY_SEARCH),
	KEY(11, 4, KEY_DELETE),
	KEY(11, 5, KEY_RIGHT),
	KEY(11, 6, KEY_LEFT),
	KEY(11, 7, KEY_RIGHTSHIFT),
	KEY(0, 0, KEY_VOLUMEUP),
	KEY(0, 1, KEY_VOLUMEDOWN),
	KEY(0, 2, KEY_CAMERA_SNAPSHOT),
	KEY(0, 3, KEY_CAMERA_FOCUS),
};

static struct matrix_keymap_data keymap_data_sim = {
	.keymap_size    = ARRAY_SIZE(keymap_sim),
	.keymap         = keymap_sim,
};

static struct pm8xxx_keypad_platform_data keypad_data_sim = {
	.input_name             = "keypad_8960",
	.input_phys_device      = "keypad_8960/input0",
	.num_rows               = 12,
	.num_cols               = 8,
	.rows_gpio_start	= PM8921_GPIO_PM_TO_SYS(9),
	.cols_gpio_start	= PM8921_GPIO_PM_TO_SYS(1),
	.debounce_ms            = 15,
	.scan_delay_ms          = 32,
	.row_hold_ns            = 91500,
	.wakeup                 = 1,
	.keymap_data            = &keymap_data_sim,
};

static int pm8921_therm_mitigation[] = {
	1100,
	700,
	600,
	325,
};


#if defined(CONFIG_MACH_ACER_A11RD)
#define MAX_VOLTAGE_MV		4350
#define CHG_TERM_MA		100
static struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 480,
	.ttrkl_time		= 50,
	.update_time		= 60000,
	.max_voltage		= MAX_VOLTAGE_MV,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 80,
	.term_current		= CHG_TERM_MA,
	.cool_temp		= INT_MIN,
	.warm_temp		= INT_MIN,
	.max_bat_chg_current	= 1400,
	.trkl_voltage		= 2800,
	.weak_voltage		= 3200,
	.trkl_current		= 50,
	.weak_current		= 325,
	.vin_min		= 4350,
	.cold_thr		= 0,
	.hot_thr		= 1,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
	.rconn_mohm		= 51,
};
#else //CONFIG_MACH_ACER_A9
#define MAX_VOLTAGE_MV		4200
#define CHG_TERM_MA		100
static struct pm8921_charger_platform_data pm8921_chg_pdata __devinitdata = {
	.safety_time		= 360,
	.ttrkl_time		= 50,
	.update_time		= 60000,
	.max_voltage		= MAX_VOLTAGE_MV,
	.min_voltage		= 3200,
	.resume_voltage_delta	= 60,
	.term_current		= CHG_TERM_MA,
	.cool_temp		= INT_MIN,
	.warm_temp		= INT_MIN,
	.max_bat_chg_current	= 1400,
	.trkl_voltage		= 2800,
	.weak_voltage		= 3200,
	.trkl_current		= 50,
	.weak_current		= 325,
	.vin_min		= 4350,
	.cold_thr		= 0,
	.hot_thr		= 1,
	.thermal_mitigation	= pm8921_therm_mitigation,
	.thermal_levels		= ARRAY_SIZE(pm8921_therm_mitigation),
	.rconn_mohm		= 37,
};
#endif

static struct pm8xxx_misc_platform_data pm8xxx_misc_pdata = {
	.priority		= 0,
};


#if defined(CONFIG_MACH_ACER_A11RD)
static struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.battery_type			= BATT_UNKNOWN,
	.r_sense			= 10,
	.v_cutoff			= 3200,
	.max_voltage_uv			= MAX_VOLTAGE_MV * 1000,
	.rconn_mohm			= 51,
	.enable_fcc_learning	= 1,
	.shutdown_soc_valid_limit	= 20,
	.adjust_soc_low_threshold	= 25,
	.chg_term_ua			= CHG_TERM_MA * 1000,
};
#else //CONFIG_MACH_ACER_A9
static struct pm8921_bms_platform_data pm8921_bms_pdata __devinitdata = {
	.battery_type			= BATT_UNKNOWN,
	.r_sense			= 12,
	.v_cutoff			= 3200,
	.max_voltage_uv			= MAX_VOLTAGE_MV * 1000,
	.rconn_mohm			= 37,
	.enable_fcc_learning	= 1,
	.shutdown_soc_valid_limit	= 20,
	.adjust_soc_low_threshold	= 25,
	.chg_term_ua			= CHG_TERM_MA * 1000,
};
#endif

#if defined(CONFIG_MACH_ACER_A11RD)
#define	PM8921_LC_LED_MAX_CURRENT	4	/* I = 4mA */
#define	PM8921_LC_LED_LOW_CURRENT	1	/* I = 1mA */
#define PM8XXX_LED_PWM_PERIOD		100000
#define PM8XXX_LED_PWM_DUTY_MS		10

/**
 * PM8XXX_PWM_CHANNEL_NONE shall be used when LED shall not be
 * driven using PWM feature.
 */
#define PM8XXX_PWM_CHANNEL_NONE		-1
#endif
#ifdef CONFIG_PMIC8XXX_VIBRATOR
static struct pm8xxx_vibrator_platform_data pm8xxx_vib_pdata = {
	.level_mV = 3000,
	.max_timeout_ms = 15000,
};
#endif
#if defined(CONFIG_MACH_ACER_A11RD)
static struct led_info pm8921_led_info_liquid[] = {
	{
		.name		= "led:red",
		.flags		= PM8XXX_ID_LED_0,
		.default_trigger	= "battery-charging",
	},
	{
		.name		= "led:green",
		.flags		= PM8XXX_ID_LED_1,
		.default_trigger	= "battery-full",
	},
	{
		.name		= "led:blue",
		.flags		= PM8XXX_ID_LED_2,
		.default_trigger	= "notification",
	},
};

static struct pm8xxx_led_config pm8921_led_configs_liquid[] = {
	[0] = {
		.id = PM8XXX_ID_LED_0,
		.mode = PM8XXX_LED_MODE_MANUAL,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
	},
	[1] = {
		.id = PM8XXX_ID_LED_1,
		.mode = PM8XXX_LED_MODE_MANUAL,
		.max_current = PM8921_LC_LED_LOW_CURRENT,
	},
	[2] = {
		.id = PM8XXX_ID_LED_2,
		.mode = PM8XXX_LED_MODE_MANUAL,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
	},
};

static struct led_platform_data pm8xxx_leds_core_liquid = {
	.num_leds = ARRAY_SIZE(pm8921_led_info_liquid),
	.leds = pm8921_led_info_liquid,
};

static struct pm8xxx_led_platform_data pm8xxx_leds_pdata_liquid = {
	.led_core = &pm8xxx_leds_core_liquid,
	.configs = pm8921_led_configs_liquid,
	.num_configs = ARRAY_SIZE(pm8921_led_configs_liquid),
};

static struct led_info pm8921_led_info[] = {
	[0] = {
		.name			= "led:Home",
		.default_trigger	= "battery-charging",
	},
	[1] = {
		.name			= "led:Multi-task",
		.default_trigger	= "battery-full",
	},
	[2] = {
		.name 			= "led:Back",
		.default_trigger 	= "notification",
        },
};

static struct led_platform_data pm8921_led_core_pdata = {
	.num_leds = ARRAY_SIZE(pm8921_led_info),
	.leds = pm8921_led_info,
};

static int pm8921_led0_pwm_duty_pcts[56] = {
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100,
};

static int pm8921_led1_pwm_duty_pcts[56] = {
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100,
};

static int pm8921_led2_pwm_duty_pcts[56] = {
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
                    100, 100, 100, 100, 100,
};

/*
 * Note: There is a bug in LPG module that results in incorrect
 * behavior of pattern when LUT index 0 is used. So effectively
 * there are 63 usable LUT entries.
 */
static struct pm8xxx_pwm_duty_cycles pm8921_led0_pwm_duty_cycles = {
	.duty_pcts = (int *)&pm8921_led0_pwm_duty_pcts,
	.num_duty_pcts = ARRAY_SIZE(pm8921_led0_pwm_duty_pcts),
	.duty_ms = PM8XXX_LED_PWM_DUTY_MS,
	.start_idx = 1,
};

static struct pm8xxx_pwm_duty_cycles pm8921_led1_pwm_duty_cycles = {
	.duty_pcts = (int *)&pm8921_led1_pwm_duty_pcts,
	.num_duty_pcts = ARRAY_SIZE(pm8921_led1_pwm_duty_pcts),
	.duty_ms = PM8XXX_LED_PWM_DUTY_MS,
	.start_idx = 1,
};

static struct pm8xxx_pwm_duty_cycles pm8921_led2_pwm_duty_cycles = {
	.duty_pcts = (int *)&pm8921_led2_pwm_duty_pcts,
	.num_duty_pcts = ARRAY_SIZE(pm8921_led2_pwm_duty_pcts),
	.duty_ms = PM8XXX_LED_PWM_DUTY_MS,
	.start_idx = 1,
};

static struct pm8xxx_led_config pm8921_led_configs[] = {
	[0] = {
		.id = PM8XXX_ID_LED_0,
		.mode = PM8XXX_LED_MODE_PWM2,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
		.pwm_channel = 5,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_duty_cycles = &pm8921_led0_pwm_duty_cycles,
	},
	[1] = {
		.id = PM8XXX_ID_LED_1,
		.mode = PM8XXX_LED_MODE_PWM1,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
		.pwm_channel = 4,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_duty_cycles = &pm8921_led1_pwm_duty_cycles,
	},
	[2] = {
		.id = PM8XXX_ID_LED_2,
		.mode = PM8XXX_LED_MODE_PWM3,
		.max_current = PM8921_LC_LED_MAX_CURRENT,
		.pwm_channel = 6,
		.pwm_period_us = PM8XXX_LED_PWM_PERIOD,
		.pwm_duty_cycles = &pm8921_led2_pwm_duty_cycles,
        },
};

static struct pm8xxx_led_platform_data pm8xxx_leds_pdata = {
		.led_core = &pm8921_led_core_pdata,
		.configs = pm8921_led_configs,
		.num_configs = ARRAY_SIZE(pm8921_led_configs),
};
#endif

static struct pm8xxx_ccadc_platform_data pm8xxx_ccadc_pdata = {
	.r_sense		= 10,
	.calib_delay_ms		= 600000,
};

#if defined(CONFIG_MACH_ACER_A11RD)
/**
 * PM8XXX_PWM_DTEST_CHANNEL_NONE shall be used when no LPG
 * channel should be in DTEST mode.
 */

#define PM8XXX_PWM_DTEST_CHANNEL_NONE   (-1)

static struct pm8xxx_pwm_platform_data pm8xxx_pwm_pdata = {
	.dtest_channel	= PM8XXX_PWM_DTEST_CHANNEL_NONE,
};
#endif
static struct pm8921_platform_data pm8921_platform_data __devinitdata = {
	.irq_pdata		= &pm8xxx_irq_pdata,
	.gpio_pdata		= &pm8xxx_gpio_pdata,
	.mpp_pdata		= &pm8xxx_mpp_pdata,
	.rtc_pdata              = &pm8xxx_rtc_pdata,
	.pwrkey_pdata		= &pm8xxx_pwrkey_pdata,
	.keypad_pdata		= &keypad_data,
	.misc_pdata		= &pm8xxx_misc_pdata,
	.regulator_pdatas	= msm_pm8921_regulator_pdata,
	.charger_pdata		= &pm8921_chg_pdata,
	.bms_pdata		= &pm8921_bms_pdata,
	.adc_pdata		= &pm8xxx_adc_pdata,
#if defined(CONFIG_MACH_ACER_A11RD)
	.leds_pdata		= &pm8xxx_leds_pdata,
#endif
	.ccadc_pdata		= &pm8xxx_ccadc_pdata,
#if defined(CONFIG_MACH_ACER_A11RD)
	.pwm_pdata		= &pm8xxx_pwm_pdata,
#endif
#ifdef CONFIG_PMIC8XXX_VIBRATOR
	.vibrator_pdata		= &pm8xxx_vib_pdata,
#endif
};

static struct msm_ssbi_platform_data msm8960_ssbi_pm8921_pdata __devinitdata = {
	.controller_type = MSM_SBI_CTRL_PMIC_ARBITER,
	.slave	= {
		.name			= "pm8921-core",
		.platform_data		= &pm8921_platform_data,
	},
};

#ifdef CONFIG_ACER_BQ27520
static struct bq27520_platform_data bq27520_pdata = {
	.name		= "fuel-gauge",
	.vreg_name	= "8921_s4",
	.vreg_value	= 1800000,
	.soc_int	= PM8921_GPIO_PM_TO_SYS(5),
	.bat_gd		= PM8921_GPIO_PM_TO_SYS(6),
	.bat_low	= PM8921_GPIO_PM_TO_SYS(10),
	//.bi_tout	= GPIO_CAP_GAUGE_BI_TOUT,
	//.chip_en	= GPIO_BATT_GAUGE_EN,
	.enable_dlog	= 0, /* if enable coulomb counter logger */
};

static struct i2c_board_info msm_bq27520_board_info[] = {
	{
		I2C_BOARD_INFO("bq27520", 0xaa>>1),
		.platform_data = &bq27520_pdata,
	},
};

void __init a9_bq27520_init(void)
{
	if ((acer_sku_id == SKU_ID_LTE_WI_TI_GAUGE) ||
		(acer_sku_id == SKU_ID_3G_WI_TI_GAUGE) ||
		(acer_sku_id == SKU_ID_DC_WI_TI_GAUGE)) {
		i2c_register_board_info(MSM_8960_GSBI10_QUP_I2C_BUS_ID,
					msm_bq27520_board_info,
					ARRAY_SIZE(msm_bq27520_board_info));
		acer_sku_wi_gauge = 1;
	} else {
		pr_info("Board ID=0x%02x,SKU=0x%02x,Disable BQ27520.\n",
						acer_board_id, acer_sku_id);
		acer_sku_wi_gauge = 0;
	}
}
#endif

void __init msm8960_init_pmic(void)
{
	pmic_reset_irq = PM8921_IRQ_BASE + PM8921_RESOUT_IRQ;
	msm8960_device_ssbi_pmic.dev.platform_data =
				&msm8960_ssbi_pm8921_pdata;
	pm8921_platform_data.num_regulators = msm_pm8921_regulator_pdata_len;

	/* Simulator supports a QWERTY keypad */
	if (machine_is_msm8960_sim())
		pm8921_platform_data.keypad_pdata = &keypad_data_sim;

	if (machine_is_msm8960_liquid()) {
		pm8921_platform_data.keypad_pdata = &keypad_data_liquid;
#if defined(CONFIG_MACH_ACER_A11RD)
		pm8921_platform_data.leds_pdata = &pm8xxx_leds_pdata_liquid;
#endif
		pm8921_platform_data.bms_pdata->battery_type = BATT_DESAY;
	} else if (machine_is_msm8960_mtp()) {
		pm8921_platform_data.bms_pdata->battery_type = BATT_PALLADIUM;
	}

	if (machine_is_msm8960_fluid())
		pm8921_bms_pdata.rconn_mohm = 20;
}