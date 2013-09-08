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

#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <mach/gpiomux.h>
#include <mach/socinfo.h>
#include "devices.h"
#include "board-acer-8960.h"

static struct gpiomux_setting gpio_unused_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting gsbi3_suspended_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting gsbi3_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting external_vfr[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_3,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_KEEPER,
	},
};

static struct gpiomux_setting gsbi_uart = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi9_active_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gsbi9_suspended_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting gsbi10 = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting gsbi12 = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting cdc_mclk = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

#ifndef CONFIG_MACH_ACER_A9
static struct gpiomux_setting audio_auxpcm[] = {
	/* Suspended state */
	{
		.func = GPIOMUX_FUNC_GPIO,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_DOWN,
	},
	/* Active state */
	{
		.func = GPIOMUX_FUNC_1,
		.drv = GPIOMUX_DRV_2MA,
		.pull = GPIOMUX_PULL_NONE,
	},
};
#endif

static struct gpiomux_setting slimbus = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_KEEPER,
};

static struct gpiomux_setting wcnss_5wire_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv  = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting wcnss_5wire_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv  = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#if defined(CONFIG_MACH_ACER_A9) || defined(CONFIG_MACH_ACER_A11RD)
#if defined(CONFIG_MACH_ACER_A11RD)
#elif defined(CONFIG_MACH_ACER_A9)
static struct gpiomux_setting ts_issp_clk_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ts_issp_clk_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ts_issp_data_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ts_issp_data_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif
static struct gpiomux_setting ts_rst_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ts_rst_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_6MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ts_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ts_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct gpiomux_setting hsic_act_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_12MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting hsic_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_OUT_LOW,
};

static struct gpiomux_setting hsic_hub_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
};
#endif

static struct gpiomux_setting hap_lvl_shft_suspended_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hap_lvl_shft_active_config = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting ap2mdm_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdm2ap_status_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting mdm2ap_errfatal_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_16MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting ap2mdm_kpdpwr_n_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting mdp_vsync_active_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
#ifdef CONFIG_ACER_HDMI_MHL_SII8334
static struct gpiomux_setting sii8334_irq_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting sii8334_irq_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};
#endif
#if defined(CONFIG_MACH_ACER_A11RD)
#elif defined(CONFIG_MACH_ACER_A9)
static struct gpiomux_setting hdmi_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting hdmi_active_1_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting hdmi_active_2_cfg = {
	.func = GPIOMUX_FUNC_1,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};
#endif
#endif

#ifdef CONFIG_ACER_HEADSET
static struct gpiomux_setting a9_hs_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting a9_hs_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
#endif

#ifdef CONFIG_ACER_HEADSET_BUTT
static struct gpiomux_setting a9_hs_butt_int_act_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};

static struct gpiomux_setting a9_hs_butt_int_sus_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_UP,
	.dir = GPIOMUX_IN,
};
#endif

static struct gpiomux_setting nfc_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting nfc_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_NONE,
	.dir = GPIOMUX_OUT_LOW,
};
static struct gpiomux_setting nfc_int_active_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
static struct gpiomux_setting nfc_int_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
	.dir = GPIOMUX_IN,
};
#if defined(CONFIG_MACH_ACER_A11RD)
static struct msm_gpiomux_config a11_evt_gpio_unused_configs[] __initdata = {
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 56,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 71,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 92,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 101,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 128,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 134,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 140,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 141,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 150,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};
#elif defined(CONFIG_MACH_ACER_A9)
static struct msm_gpiomux_config a9_dvt1_gpio_unused_configs[] __initdata = {
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 23,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 56,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 71,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 92,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 98,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 128,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 134,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 140,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 141,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};

static struct msm_gpiomux_config a9_dvt1_3_gpio_unused_configs[] __initdata = {
	{
		.gpio = 1,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 4,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 6,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 7,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 8,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 9,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 12,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 13,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 14,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 15,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 23,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 25,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 32,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 33,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 34,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 41,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 42,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 43,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 48,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 51,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 52,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 53,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 55,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 56,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 71,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 72,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 82,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 91,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 92,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 98,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 99,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 107,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 109,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 111,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 113,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 120,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 121,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 124,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 128,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 129,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 131,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 132,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},

	{
		.gpio = 135,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 140,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 141,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 144,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 145,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 149,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
	{
		.gpio = 151,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gpio_unused_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config msm8960_fusion_gsbi_configs[] = {
	{
		.gpio = 93,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 94,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 95,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
	{
		.gpio = 96,
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi9_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi9_active_cfg,
		}
	},
};

static struct msm_gpiomux_config msm8960_gsbi_configs[] __initdata = {
	{
		.gpio      = 16,	/* GSBI3 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 17,	/* GSBI3 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi3_suspended_cfg,
			[GPIOMUX_ACTIVE] = &gsbi3_active_cfg,
		},
	},
	{
		.gpio      = 44,	/* GSBI12 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 45,	/* GSBI12 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi12,
		},
	},
	{
		.gpio      = 73,	/* GSBI10 I2C QUP SDA */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
	{
		.gpio      = 74,	/* GSBI10 I2C QUP SCL */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi10,
		},
	},
};

static struct msm_gpiomux_config msm8960_gsbi4_uart_configs[] __initdata = {
	{
		.gpio      = GPIO_MSM_UART_TXD,        /* GSBI4 UART0 TX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
	{
		.gpio      = GPIO_MSM_UART_RXD,        /* GSBI4 UART0 RX */
		.settings = {
			[GPIOMUX_SUSPENDED] = &gsbi_uart,
		},
	},
};

static struct msm_gpiomux_config msm8960_external_vfr_configs[] __initdata = {
	{
		.gpio      = 23,        /* EXTERNAL VFR */
		.settings = {
			[GPIOMUX_SUSPENDED] = &external_vfr[0],
			[GPIOMUX_ACTIVE] = &external_vfr[1],
		},
	},
};

static struct msm_gpiomux_config msm8960_slimbus_config[] __initdata = {
	{
		.gpio	= 60,		/* slimbus data */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
	{
		.gpio	= 61,		/* slimbus clk */
		.settings = {
			[GPIOMUX_SUSPENDED] = &slimbus,
		},
	},
};

static struct msm_gpiomux_config msm8960_audio_codec_configs[] __initdata = {
	{
		.gpio = 59,
		.settings = {
			[GPIOMUX_SUSPENDED] = &cdc_mclk,
		},
	},
};

#ifndef CONFIG_MACH_ACER_A9
static struct msm_gpiomux_config msm8960_audio_auxpcm_configs[] __initdata = {
	{
		.gpio = 63,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
#if defined(CONFIG_MACH_ACER_A11RD)
#elif defined(CONFIG_MACH_ACER_A9)
	{
		.gpio = 64,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
#endif
	{
		.gpio = 65,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
	{
		.gpio = 66,
		.settings = {
			[GPIOMUX_SUSPENDED] = &audio_auxpcm[0],
			[GPIOMUX_ACTIVE] = &audio_auxpcm[1],
		},
	},
};
#endif

static struct msm_gpiomux_config wcnss_5wire_interface[] = {
	{
		.gpio = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
	{
		.gpio = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &wcnss_5wire_active_cfg,
			[GPIOMUX_SUSPENDED] = &wcnss_5wire_suspend_cfg,
		},
	},
};

#if defined(CONFIG_MACH_ACER_A9) || defined(CONFIG_MACH_ACER_A11RD)
static struct msm_gpiomux_config ts_configs[] __initdata = {
	{	/* TS INTERRUPT */
		.gpio = 11,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ts_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &ts_int_sus_cfg,
		},
	},
	{	/* TS RESET */
		.gpio = 50,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ts_rst_act_cfg,
			[GPIOMUX_SUSPENDED] = &ts_rst_sus_cfg,
		},
	},
#if defined(CONFIG_MACH_ACER_A11RD)
#elif defined(CONFIG_MACH_ACER_A9)
	{	/* TS ISSP DATA */
		.gpio = 96,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ts_issp_data_act_cfg,
			[GPIOMUX_SUSPENDED] = &ts_issp_data_sus_cfg,
		},
	},
	{	/* TS ISSP CLK */
		.gpio = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &ts_issp_clk_act_cfg,
			[GPIOMUX_SUSPENDED] = &ts_issp_clk_sus_cfg,
		},
	},
#endif
};
#endif

#ifdef CONFIG_USB_EHCI_MSM_HSIC
static struct msm_gpiomux_config msm8960_hsic_configs[] = {
#if defined(CONFIG_MACH_ACER_A11RD)
#elif defined(CONFIG_MACH_ACER_A9)
	{
		.gpio = 150,               /*HSIC_STROBE */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
#endif
	{
		.gpio = 151,               /* HSIC_DATA */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};

static struct msm_gpiomux_config msm8960_hsic_hub_configs[] = {
	{
		.gpio = 91,               /* HSIC_HUB_RESET */
		.settings = {
			[GPIOMUX_ACTIVE] = &hsic_hub_act_cfg,
			[GPIOMUX_SUSPENDED] = &hsic_sus_cfg,
		},
	},
};
#endif

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
static struct gpiomux_setting sdcc4_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc4_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc4_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdcc4_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8960_sdcc4_configs[] __initdata = {
	{
		/* SDC4_DATA_3 */
		.gpio      = 83,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_2 */
		.gpio      = 84,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_1 */
		.gpio      = 85,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_data_1_suspend_cfg,
		},
	},
	{
		/* SDC4_DATA_0 */
		.gpio      = 86,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_CMD */
		.gpio      = 87,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
	{
		/* SDC4_CLK */
		.gpio      = 88,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc4_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc4_suspend_cfg,
		},
	},
};
#endif


static struct msm_gpiomux_config hap_lvl_shft_config[] __initdata = {
	{
		.gpio = 47,
		.settings = {
			[GPIOMUX_SUSPENDED] = &hap_lvl_shft_suspended_config,
			[GPIOMUX_ACTIVE] = &hap_lvl_shft_active_config,
		},
	},
};

static struct msm_gpiomux_config sglte_configs[] __initdata = {
	/* AP2MDM_STATUS */
	{
		.gpio = 77,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* MDM2AP_STATUS */
	{
		.gpio = 24,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_status_cfg,
		}
	},
	/* MDM2AP_ERRFATAL */
	{
		.gpio = 40,
		.settings = {
			[GPIOMUX_SUSPENDED] = &mdm2ap_errfatal_cfg,
		}
	},
	/* AP2MDM_ERRFATAL */
	{
		.gpio = 80,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
	/* AP2MDM_KPDPWR_N */
	{
		.gpio = 79,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
	/* AP2MDM_PMIC_PWR_EN */
	{
		.gpio = 22,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_kpdpwr_n_cfg,
		}
	},
#ifndef CONFIG_ACER_HEADSET_BUTT
	/* AP2MDM_SOFT_RESET */
	{
		.gpio = 78,
		.settings = {
			[GPIOMUX_SUSPENDED] = &ap2mdm_cfg,
		}
	},
#endif
};

static struct msm_gpiomux_config msm8960_mdp_vsync_configs[] __initdata = {
	{
		.gpio = 0,
		.settings = {
			[GPIOMUX_ACTIVE]    = &mdp_vsync_active_cfg,
			[GPIOMUX_SUSPENDED] = &mdp_vsync_suspend_cfg,
		},
	}
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct msm_gpiomux_config msm8960_hdmi_configs[] __initdata = {
#if defined(CONFIG_MACH_ACER_A11RD)
#elif defined(CONFIG_MACH_ACER_A9)
	{
		.gpio = 100,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 101,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_1_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
	{
		.gpio = 102,
		.settings = {
			[GPIOMUX_ACTIVE]    = &hdmi_active_2_cfg,
			[GPIOMUX_SUSPENDED] = &hdmi_suspend_cfg,
		},
	},
#endif
#ifdef CONFIG_ACER_HDMI_MHL_SII8334
	{
		.gpio = 35,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sii8334_irq_active_cfg,
			[GPIOMUX_SUSPENDED] = &sii8334_irq_suspend_cfg,
		},
	},

#endif
};
#endif

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
static struct gpiomux_setting sdcc2_clk_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_NONE,
};

static struct gpiomux_setting sdcc2_cmd_data_0_3_actv_cfg = {
	.func = GPIOMUX_FUNC_2,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct gpiomux_setting sdcc2_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_2MA,
	.pull = GPIOMUX_PULL_DOWN,
};

static struct gpiomux_setting sdcc2_data_1_suspend_cfg = {
	.func = GPIOMUX_FUNC_GPIO,
	.drv = GPIOMUX_DRV_8MA,
	.pull = GPIOMUX_PULL_UP,
};

static struct msm_gpiomux_config msm8960_sdcc2_configs[] __initdata = {
	{
		/* DATA_3 */
		.gpio      = 92,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* DATA_2 */
		.gpio      = 91,
		.settings = {
		[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
		[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* DATA_1 */
		.gpio      = 90,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_data_1_suspend_cfg,
		},
	},
	{
		/* DATA_0 */
		.gpio      = 89,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* CMD */
		.gpio      = 97,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_cmd_data_0_3_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
	{
		/* CLK */
		.gpio      = 98,
		.settings = {
			[GPIOMUX_ACTIVE]    = &sdcc2_clk_actv_cfg,
			[GPIOMUX_SUSPENDED] = &sdcc2_suspend_cfg,
		},
	},
};
#endif

#ifdef CONFIG_ACER_HEADSET
static struct msm_gpiomux_config a9_hs_configs[] __initdata = {
	{	/* HS INTERRUPT */
		.gpio = 36,
		.settings = {
			[GPIOMUX_ACTIVE]    = &a9_hs_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &a9_hs_int_sus_cfg,
		},
	},
};
#endif

#ifdef CONFIG_ACER_HEADSET_BUTT
static struct msm_gpiomux_config a9_hs_butt_configs[] __initdata = {
	{	/* HS BUTT INTERRUPT */
		.gpio = 78,
		.settings = {
			[GPIOMUX_ACTIVE]    = &a9_hs_butt_int_act_cfg,
			[GPIOMUX_SUSPENDED] = &a9_hs_butt_int_sus_cfg,
		},
	},
};
#endif

static struct msm_gpiomux_config a9_nfc_configs[] __initdata = {
	{
		.gpio = 81,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_active_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_suspend_cfg,
		},
	},
	{
		.gpio = 106,
		.settings = {
			[GPIOMUX_ACTIVE]    = &nfc_int_active_cfg,
			[GPIOMUX_SUSPENDED] = &nfc_int_suspend_cfg,
		},
	},
};

int __init msm8960_init_gpiomux(void)
{
	int rc = msm_gpiomux_init(NR_GPIO_IRQS);
	if (rc) {
		pr_err(KERN_ERR "msm_gpiomux_init failed %d\n", rc);
		return rc;
	}

#if defined(CONFIG_MACH_ACER_A11RD)
	pr_info("use A11 msm8960 unused table");
	msm_gpiomux_install(a11_evt_gpio_unused_configs,
			ARRAY_SIZE(a11_evt_gpio_unused_configs));
#elif defined(CONFIG_MACH_ACER_A9)
	if (acer_board_id <= HW_ID_DVT1_2) {
		pr_info("use DVT1 msm8960 unused table");
		msm_gpiomux_install(a9_dvt1_gpio_unused_configs,
				ARRAY_SIZE(a9_dvt1_gpio_unused_configs));
	} else {
		pr_info("use DVT1-3 msm8960 unused table");
		msm_gpiomux_install(a9_dvt1_3_gpio_unused_configs,
				ARRAY_SIZE(a9_dvt1_3_gpio_unused_configs));
	}
#endif

	msm_gpiomux_install(msm8960_gsbi_configs,
			ARRAY_SIZE(msm8960_gsbi_configs));

#if defined(CONFIG_MACH_ACER_A9) || defined(CONFIG_MACH_ACER_A11RD)
	msm_gpiomux_install(ts_configs,
			ARRAY_SIZE(ts_configs));
#endif

#ifdef CONFIG_ACER_HEADSET
	msm_gpiomux_install(a9_hs_configs,
			ARRAY_SIZE(a9_hs_configs));
#endif

#ifdef CONFIG_ACER_HEADSET_BUTT
	msm_gpiomux_install(a9_hs_butt_configs,
			ARRAY_SIZE(a9_hs_butt_configs));
#endif

	msm_gpiomux_install(a9_nfc_configs,
			ARRAY_SIZE(a9_nfc_configs));

	msm_gpiomux_install(msm8960_slimbus_config,
			ARRAY_SIZE(msm8960_slimbus_config));

	msm_gpiomux_install(msm8960_audio_codec_configs,
			ARRAY_SIZE(msm8960_audio_codec_configs));

#ifndef CONFIG_MACH_ACER_A9
	msm_gpiomux_install(msm8960_audio_auxpcm_configs,
			ARRAY_SIZE(msm8960_audio_auxpcm_configs));
#endif

	msm_gpiomux_install(wcnss_5wire_interface,
			ARRAY_SIZE(wcnss_5wire_interface));

#ifdef CONFIG_MMC_MSM_SDC4_SUPPORT
	msm_gpiomux_install(msm8960_sdcc4_configs,
		ARRAY_SIZE(msm8960_sdcc4_configs));
#endif

	if (machine_is_msm8960_mtp() || machine_is_msm8960_fluid() ||
		machine_is_msm8960_liquid() || machine_is_msm8960_cdp())
		msm_gpiomux_install(hap_lvl_shft_config,
			ARRAY_SIZE(hap_lvl_shft_config));

#ifdef CONFIG_USB_EHCI_MSM_HSIC
	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1) &&
		machine_is_msm8960_liquid())
		msm_gpiomux_install(msm8960_hsic_configs,
			ARRAY_SIZE(msm8960_hsic_configs));

	if ((SOCINFO_VERSION_MAJOR(socinfo_get_version()) != 1) &&
			machine_is_msm8960_liquid())
		msm_gpiomux_install(msm8960_hsic_hub_configs,
			ARRAY_SIZE(msm8960_hsic_hub_configs));
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
	msm_gpiomux_install(msm8960_hdmi_configs,
			ARRAY_SIZE(msm8960_hdmi_configs));
#endif

	msm_gpiomux_install(msm8960_mdp_vsync_configs,
			ARRAY_SIZE(msm8960_mdp_vsync_configs));

	msm_gpiomux_install(msm8960_gsbi4_uart_configs,
		ARRAY_SIZE(msm8960_gsbi4_uart_configs));

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE) {
		/* For 8960 Fusion 2.2 Primary IPC */
		msm_gpiomux_install(msm8960_fusion_gsbi_configs,
			ARRAY_SIZE(msm8960_fusion_gsbi_configs));
		/* For SGLTE 8960 Fusion External VFR */
		msm_gpiomux_install(msm8960_external_vfr_configs,
			ARRAY_SIZE(msm8960_external_vfr_configs));
	}

#ifdef CONFIG_MMC_MSM_SDC2_SUPPORT
	msm_gpiomux_install(msm8960_sdcc2_configs,
		ARRAY_SIZE(msm8960_sdcc2_configs));
#endif

	if (socinfo_get_platform_subtype() == PLATFORM_SUBTYPE_SGLTE)
		msm_gpiomux_install(sglte_configs,
			ARRAY_SIZE(sglte_configs));

	return 0;
}
