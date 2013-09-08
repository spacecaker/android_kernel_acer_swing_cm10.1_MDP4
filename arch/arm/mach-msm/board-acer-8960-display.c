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

#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/bootmem.h>
#include <linux/ion.h>
#include <linux/gpio.h>
#include <asm/mach-types.h>
#include <mach/msm_bus_board.h>
#include <mach/msm_memtypes.h>
#include <mach/board.h>
#include <mach/gpiomux.h>
#include <mach/ion.h>
#include <mach/socinfo.h>

#ifdef CONFIG_ACER_HDMI_MHL_SII8334
#include <linux/i2c.h>
#include <linux/mhl_sii8334.h>
#endif

#include "devices.h"
#include "board-acer-8960.h"

#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
#define PANEL_X_RES	720
#define PANEL_Y_RES	1280
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
#define PANEL_X_RES	1080
#define PANEL_Y_RES	1920
#else
//default
#define PANEL_X_RES	1200
#define PANEL_Y_RES	1920
#endif

#ifdef CONFIG_FB_MSM_TRIPLE_BUFFER
#define MSM_FB_PRIM_BUF_SIZE \
		(roundup((roundup(PANEL_X_RES, 32) * roundup(PANEL_Y_RES, 32) * 4), 4096) * 3)
			/* 4 bpp x 3 pages */
#else
#define MSM_FB_PRIM_BUF_SIZE \
		(roundup((roundup(PANEL_X_RES, 32) * roundup(PANLE_Y_RES, 32) * 4), 4096) * 2)
			/* 4 bpp x 2 pages */
#endif

/* Note: must be multiple of 4096 */
#define MSM_FB_SIZE roundup(MSM_FB_PRIM_BUF_SIZE, 4096)

#ifdef CONFIG_FB_MSM_OVERLAY0_WRITEBACK
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE \
		roundup((roundup(1920, 32) * roundup(1200, 32) * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY0_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY0_WRITEBACK */

#ifdef CONFIG_FB_MSM_OVERLAY1_WRITEBACK
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE \
		roundup((roundup(1920, 32) * roundup(1080, 32) * 3 * 2), 4096)
#else
#define MSM_FB_OVERLAY1_WRITEBACK_SIZE (0)
#endif  /* CONFIG_FB_MSM_OVERLAY1_WRITEBACK */

#define MDP_VSYNC_GPIO 0

#define HDMI_PANEL_NAME	"hdmi_msm"
#define TVOUT_PANEL_NAME	"tvout_msm"

#define MIPI_HIMAX_HD_CMDPANEL_NAME "mipi_cmd_himax_hd"
#define MIPI_HIMAX_HD_VIDEOPANEL_NAME "mipi_video_himax_hd"
#define MIPI_VIDEO_SHARP_WUXGA_PANEL_NAME "mipi_video_sharp_wuxga"

#ifdef CONFIG_FB_MSM_HDMI_AS_PRIMARY
static unsigned char hdmi_is_primary = 1;
#else
static unsigned char hdmi_is_primary;
#endif

unsigned char msm8960_hdmi_as_primary_selected(void)
{
	return hdmi_is_primary;
}

static struct resource msm_fb_resources[] = {
	{
		.flags = IORESOURCE_DMA,
	}
};

#define A9_PANEL_CMD_MODE	1
static int msm_fb_detect_panel(const char *name)
{
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
#if A9_PANEL_CMD_MODE
	if (!strncmp(name, MIPI_HIMAX_HD_CMDPANEL_NAME,
			strnlen(MIPI_HIMAX_HD_CMDPANEL_NAME,
			PANEL_NAME_MAX_LEN)))
		return 0;
#else
	if (!strncmp(name, MIPI_HIMAX_HD_VIDEOPANEL_NAME,
			strnlen(MIPI_HIMAX_HD_VIDEOPANEL_NAME,
			PANEL_NAME_MAX_LEN)))
		return 0;
#endif
#endif
#ifdef CONFIG_FB_MSM_MIPI_DSI_SHARP
	if (!strncmp(name, MIPI_VIDEO_SHARP_WUXGA_PANEL_NAME,
		strnlen(MIPI_VIDEO_SHARP_WUXGA_PANEL_NAME,
			PANEL_NAME_MAX_LEN))) {
		return 0;
	}
#endif
	if (!strncmp(name, HDMI_PANEL_NAME,
			strnlen(HDMI_PANEL_NAME,
				PANEL_NAME_MAX_LEN))) {
		return 0;
	}

	pr_warning("%s: not supported '%s'", __func__, name);
	return -ENODEV;
}

static struct msm_fb_platform_data msm_fb_pdata = {
	.detect_client = msm_fb_detect_panel,
};

static struct platform_device msm_fb_device = {
	.name   = "msm_fb",
	.id     = 0,
	.num_resources     = ARRAY_SIZE(msm_fb_resources),
	.resource          = msm_fb_resources,
	.dev.platform_data = &msm_fb_pdata,
};

static bool dsi_power_on;

#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
static int himax_gpio = PM8921_GPIO_PM_TO_SYS(43);
static struct mipi_dsi_panel_platform_data himax_pdata = {
	.gpio = &himax_gpio,
};

static struct platform_device mipi_dsi_himax_panel_device = {
	.name = "mipi_himax",
	.id = 0,
	.dev = {
		.platform_data = &himax_pdata,
	}
};
static int mipi_dsi_himax_power(int on)
{
	static struct regulator *reg_l8_3p0, *reg_l29_1p8, *reg_l2_1p2;
	int rc;

	if (!dsi_power_on) {
		reg_l8_3p0 = regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdc");
		if (IS_ERR(reg_l8_3p0)) {
			pr_err("could not get 8921_l8, rc = %ld\n",
				PTR_ERR(reg_l8_3p0));
			return -ENODEV;
		}
		reg_l29_1p8= regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vddio");
		if (IS_ERR(reg_l29_1p8)) {
			pr_err("could not get 8921_l29, rc = %ld\n",
				PTR_ERR(reg_l29_1p8));
			return -ENODEV;
		}
		reg_l2_1p2= regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2_1p2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2_1p2));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l8_3p0, 3000000, 3000000);
		if (rc) {
			pr_err("set_voltage l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l29_1p8, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_voltage(reg_l2_1p2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		dsi_power_on = true;
	}

	/* Notice: In A9 EVT & EVT2, panel's 1v8(L23) is shared with cpu pll hence
	 *       we don't need to control it. But in DVT1 and later,
	 *       it should be moved to L29, we should follow power on-off sequence to
	 *       meet panel's spec. (In EVT & EVT2, control L29 don't have effect.)
	 */
	if (on) {
		rc = regulator_set_optimum_mode(reg_l2_1p2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l8_3p0, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l8 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l29_1p8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2_1p2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_enable(reg_l29_1p8);
		if (rc) {
			pr_err("enable l29 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		usleep_range(3000, 3000);
		rc = regulator_enable(reg_l8_3p0);
		if (rc) {
			pr_err("enable l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
	} else {
		rc = regulator_disable(reg_l8_3p0);
		if (rc) {
			pr_err("disable reg_l8 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(3);
		rc = regulator_disable(reg_l29_1p8);
		if (rc) {
			pr_err("disable reg_l29 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_l2_1p2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l8_3p0, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l29_1p8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l29 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_set_optimum_mode(reg_l2_1p2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	return 0;
}
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
static int mipi_dsi_sharp_power(int on)
{
	static struct regulator *reg_l29_1p8, *reg_l2_1p2;
	static int gpio42, gpio43, gpio33;
	int rc;

	/* LDO Deifne:
	 *   L29 1.8V for LCD enable
	 *   L2  1.2V for MIPI_VDD
	 *
	 * PM8921 GPIO Define:
	 *   GPIO 33 for LCDC_P5V_EN
	 *   GPIO 42 for LCDC_N5V_EN
	 *   GPIO 43 for LCDC_RST_N
	 *
	 * MSM8960 GPIO Define:
	 *   GPIO 40 for LCD_BL_OFF
	 */

	pr_info("%s: on:%d dsi_power_on: %d\n", __func__, on, dsi_power_on);
	if (!dsi_power_on) {
		/* LCD enable */
		reg_l29_1p8= regulator_get(&msm_mipi_dsi1_device.dev,
				"lcd_vddio");
		if (IS_ERR(reg_l29_1p8)) {
			pr_err("could not get 8921_l29, rc = %ld\n",
				PTR_ERR(reg_l29_1p8));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l29_1p8, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage l29 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		/* MIPI_VDD */
		reg_l2_1p2= regulator_get(&msm_mipi_dsi1_device.dev,
				"dsi_vdda");
		if (IS_ERR(reg_l2_1p2)) {
			pr_err("could not get 8921_l2, rc = %ld\n",
				PTR_ERR(reg_l2_1p2));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_l2_1p2, 1200000, 1200000);
		if (rc) {
			pr_err("set_voltage l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}

		/* LCDC_P5V_EN */
		gpio33 = PM8921_GPIO_PM_TO_SYS(33);
		pr_debug("PM8921_GPIO_PM_TO_SYS gpio 33 , gpio33=%d\n", gpio33);
		rc = gpio_request(gpio33, "AVDD+");
		pr_debug("request gpio 33 , rc=%d\n", rc);
		if (rc) {
			pr_err("request gpio 33 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_direction_output(gpio33, 0);

		/* LCDC_N5V_EN */
		gpio42 = PM8921_GPIO_PM_TO_SYS(42);
		pr_debug("PM8921_GPIO_PM_TO_SYS gpio 42 , gpio42=%d\n", gpio42);
		rc = gpio_request(gpio42, "AVDD");
		pr_debug("request gpio 42 , rc=%d\n", rc);
		if (rc) {
			pr_err("request gpio 42 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_direction_output(gpio42, 0);

		/* LCDC_RST_N */
		gpio43 = PM8921_GPIO_PM_TO_SYS(43);
		rc = gpio_request(gpio43, "RESET");
		if (rc) {
			pr_err("request gpio 43 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		gpio_direction_output(gpio43, 0);
		dsi_power_on = true;
	}
	if (on) {
		pr_info("\nsharp display power on sequence start\n");
		rc = regulator_set_optimum_mode(reg_l29_1p8, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l29 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l29_1p8);
		if (rc) {
			pr_err("enable l29 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		mdelay(1);
		gpio_direction_output(gpio33, 1);
		gpio_direction_output(gpio42, 1);
		mdelay(10);
		gpio_direction_output(gpio43, 1);

		rc = regulator_set_optimum_mode(reg_l2_1p2, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_l2_1p2);
		if (rc) {
			pr_err("enable l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		/* Turn 0n Backlight */
		gpio_set_value( 40, 0);
		pr_info("\nsharp display power on sequence end\n");
	} else {
		pr_info("\nsharp display power off sequence start\n");
		rc = regulator_disable(reg_l2_1p2);
		if (rc) {
			pr_err("disable reg_l2 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l2_1p2, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l2 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		/* Turn 0ff Backlight */
		gpio_set_value( 40, 1);
		gpio_direction_output(gpio43, 0);
		mdelay(1);
		gpio_direction_output(gpio42, 0);
		mdelay(1);
		gpio_direction_output(gpio33, 0);
		mdelay(110);
		rc = regulator_disable(reg_l29_1p8);
		if (rc) {
			pr_err("disable reg_l29 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_l29_1p8, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l29 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_info("\nsharp display power off sequence end\n");
	}

	return 0;
}
#else
static int mipi_dsi_panel_fake_power(int on)
{
	pr_debug("%s: on=%d\n", __func__, on);
	dsi_power_on = true;
	return 0;
}
#endif

static char mipi_dsi_splash_is_enabled(void);
static int mipi_dsi_panel_power(int on)
{
	int ret;

	pr_debug("%s: on=%d\n", __func__, on);

#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	ret = mipi_dsi_himax_power(on);
#elif defined CONFIG_FB_MSM_MIPI_DSI_SHARP
	ret = mipi_dsi_sharp_power(on);
#else
	ret = mipi_dsi_panel_fake_power(on);
#endif
	return ret;
}

static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_VSYNC_GPIO,
	.dsi_power_save = mipi_dsi_panel_power,
	.splash_is_enabled = mipi_dsi_splash_is_enabled,
};

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors mdp_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors mdp_ui_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_vga_vectors[] = {
	/* VGA and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 216000000 * 2,
		.ib = 270000000 * 2,
	},
};

static struct msm_bus_vectors mdp_720p_vectors[] = {
	/* 720p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 230400000 * 2,
		.ib = 288000000 * 2,
	},
};

static struct msm_bus_vectors mdp_1080p_vectors[] = {
	/* 1080p and less video */
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 334080000 * 2,
		.ib = 417600000 * 2,
	},
};

static struct msm_bus_paths mdp_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(mdp_init_vectors),
		mdp_init_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_ui_vectors),
		mdp_ui_vectors,
	},
	{
		ARRAY_SIZE(mdp_vga_vectors),
		mdp_vga_vectors,
	},
	{
		ARRAY_SIZE(mdp_720p_vectors),
		mdp_720p_vectors,
	},
	{
		ARRAY_SIZE(mdp_1080p_vectors),
		mdp_1080p_vectors,
	},
};

static struct msm_bus_scale_pdata mdp_bus_scale_pdata = {
	mdp_bus_scale_usecases,
	ARRAY_SIZE(mdp_bus_scale_usecases),
	.name = "mdp",
};

#endif

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = MDP_VSYNC_GPIO,
	.mdp_max_clk = 200000000,
#ifdef CONFIG_MSM_BUS_SCALING
	.mdp_bus_scale_table = &mdp_bus_scale_pdata,
#endif
	.mdp_rev = MDP_REV_42,
#ifdef CONFIG_MSM_MULTIMEDIA_USE_ION
	.mem_hid = BIT(ION_CP_MM_HEAP_ID),
#else
	.mem_hid = MEMTYPE_EBI1,
#endif
	.cont_splash_enabled = 0,
	.mdp_iommu_split_domain = 0,
};

void __init msm8960_mdp_writeback(struct memtype_reserve* reserve_table)
{
	mdp_pdata.ov0_wb_size = MSM_FB_OVERLAY0_WRITEBACK_SIZE;
	mdp_pdata.ov1_wb_size = MSM_FB_OVERLAY1_WRITEBACK_SIZE;
#if defined(CONFIG_ANDROID_PMEM) && !defined(CONFIG_MSM_MULTIMEDIA_USE_ION)
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov0_wb_size;
	reserve_table[mdp_pdata.mem_hid].size +=
		mdp_pdata.ov1_wb_size;
#endif
}

static char mipi_dsi_splash_is_enabled(void)
{
	return mdp_pdata.cont_splash_enabled;
}

static struct platform_device mipi_dsi_simulator_panel_device = {
	.name = "mipi_simulator",
	.id = 0,
};

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static struct resource hdmi_msm_resources[] = {
	{
		.name  = "hdmi_msm_qfprom_addr",
		.start = 0x00700000,
		.end   = 0x007060FF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_hdmi_addr",
		.start = 0x04A00000,
		.end   = 0x04A00FFF,
		.flags = IORESOURCE_MEM,
	},
	{
		.name  = "hdmi_msm_irq",
		.start = HDMI_IRQ,
		.end   = HDMI_IRQ,
		.flags = IORESOURCE_IRQ,
	},
};

static int hdmi_enable_5v(int on);
static int hdmi_core_power(int on, int show);
static int hdmi_cec_power(int on);
static int hdmi_gpio_config(int on);
static int hdmi_panel_power(int on);

static struct msm_hdmi_platform_data hdmi_msm_data = {
	.irq = HDMI_IRQ,
	.enable_5v = hdmi_enable_5v,
	.core_power = hdmi_core_power,
	.cec_power = hdmi_cec_power,
	.panel_power = hdmi_panel_power,
	.gpio_config = hdmi_gpio_config,
};

static struct platform_device hdmi_msm_device = {
	.name = "hdmi_msm",
	.id = 0,
	.num_resources = ARRAY_SIZE(hdmi_msm_resources),
	.resource = hdmi_msm_resources,
	.dev.platform_data = &hdmi_msm_data,
};
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

#ifdef CONFIG_ACER_HDMI_MHL_SII8334
#define MSM_8960_GSBI10_QUP_I2C_BUS_ID 10

static int mhl_enable_5v(int on)
{
	static struct regulator *vreg_ext5v;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	if (!vreg_ext5v) {
		vreg_ext5v = regulator_get(NULL, "ext_5v");
		if (IS_ERR(vreg_ext5v)) {
			pr_err("could not get ext_5v, rc = %ld\n",
				PTR_ERR(vreg_ext5v));
			return -ENODEV;
		}
	}

	rc = on ? regulator_enable(vreg_ext5v) : regulator_disable(vreg_ext5v);
	if (rc) {
		pr_err("mhl 5v regulator power %s failed, rc = %d\n", on ? "enable" : "disable", rc);
		return rc;
	}
	prev_on = on;
	return 0;
}

static int mhl_core_power(int on)
{
	static struct regulator *vreg_1v2, *vreg_1v8;

	int rc;
	if (!vreg_1v8) {
		vreg_1v8 = regulator_get(&msm8960_device_qup_i2c_gsbi10.dev, "mhl_1v8");
		if (IS_ERR(vreg_1v8)) {
			pr_err("could not get reg_mhl_1v8, rc = %ld\n",
				PTR_ERR(vreg_1v8));
			return -ENODEV;
		}

		rc = regulator_set_voltage(vreg_1v8, 1800000, 1800000);

		if (rc) {
			pr_err("mhl 1.8v regulator set voltage failed, rc = %d\n", rc);
			vreg_1v8 = NULL;
			return rc;
		}
	}

	if (!vreg_1v2) {
		vreg_1v2 = regulator_get(&msm8960_device_qup_i2c_gsbi10.dev, "mhl_1v2");
		if (IS_ERR(vreg_1v2)) {
			pr_err("could not get reg_mhl_1v2, rc = %ld\n",
				PTR_ERR(vreg_1v2));
			return -ENODEV;
		}

		rc = regulator_set_voltage(vreg_1v2, 1200000, 1200000);
		if (rc) {
			pr_err("mhl 1.2v regulator set voltage failed, rc = %d\n", rc);
			vreg_1v2 = NULL;
			return rc;
		}
	}

	rc = on ? regulator_enable(vreg_1v8) : regulator_disable(vreg_1v8);
	if (rc) {
		pr_err("mhl 1v8 regulator power %s failed, rc = %d\n", on ? "enable" : "disable", rc);
		return rc;
	}

	rc = on ? regulator_enable(vreg_1v2) : regulator_disable(vreg_1v2);
	if (rc) {
		pr_err("mhl 1v2 regulator power %s failed, rc = %d\n", on ? "enable" : "disable", rc);
		return rc;
	}

	return rc;
}

#define MHL_SII8334_IRQ_GPIO 35
#define MHL_SII8334_RESET_GPIO 58

static struct sii8334_platform_data mhl_sii8334_data = {
	.gpio_irq = MHL_SII8334_IRQ_GPIO,
	.gpio_reset = MHL_SII8334_RESET_GPIO,
	.core_power = mhl_core_power,
	.enable_5v = mhl_enable_5v,
};

static struct i2c_board_info mhl_sii8334_info[] __initdata = {
	{
		I2C_BOARD_INFO(MHL_SII8334_DRIVER_NAME, 0x72 >> 1),
		.irq		=  MSM_GPIO_TO_INT(MHL_SII8334_IRQ_GPIO),
		.platform_data = &mhl_sii8334_data,
	},
};
#endif

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
static struct platform_device wfd_panel_device = {
	.name = "wfd_panel",
	.id = 0,
	.dev.platform_data = NULL,
};

static struct platform_device wfd_device = {
	.name          = "msm_wfd",
	.id            = -1,
};
#endif

#ifdef CONFIG_MSM_BUS_SCALING
static struct msm_bus_vectors dtv_bus_init_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 0,
		.ib = 0,
	},
};

static struct msm_bus_vectors dtv_bus_def_vectors[] = {
	{
		.src = MSM_BUS_MASTER_MDP_PORT0,
		.dst = MSM_BUS_SLAVE_EBI_CH0,
		.ab = 566092800 * 2,
		.ib = 707616000 * 2,
	},
};

static struct msm_bus_paths dtv_bus_scale_usecases[] = {
	{
		ARRAY_SIZE(dtv_bus_init_vectors),
		dtv_bus_init_vectors,
	},
	{
		ARRAY_SIZE(dtv_bus_def_vectors),
		dtv_bus_def_vectors,
	},
};
static struct msm_bus_scale_pdata dtv_bus_scale_pdata = {
	dtv_bus_scale_usecases,
	ARRAY_SIZE(dtv_bus_scale_usecases),
	.name = "dtv",
};

static struct lcdc_platform_data dtv_pdata = {
	.bus_scale_table = &dtv_bus_scale_pdata,
	.lcdc_power_save = hdmi_panel_power,
};

static int hdmi_panel_power(int on)
{
	int rc;

	pr_debug("%s: HDMI Core: %s\n", __func__, (on ? "ON" : "OFF"));
	rc = hdmi_core_power(on, 1);
	if (rc)
		rc = hdmi_cec_power(on);

	pr_debug("%s: HDMI Core: %s Success\n", __func__, (on ? "ON" : "OFF"));
	return rc;
}
#endif

#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
static int hdmi_enable_5v(int on)
{
	return 0;
}

static int hdmi_core_power(int on, int show)
{
	static struct regulator *reg_8921_l23, *reg_8921_s4;
	static int prev_on;
	int rc;

	if (on == prev_on)
		return 0;

	/* TBD: PM8921 regulator instead of 8901 */
	if (!reg_8921_l23) {
		reg_8921_l23 = regulator_get(&hdmi_msm_device.dev, "hdmi_avdd");
		if (IS_ERR(reg_8921_l23)) {
			pr_err("could not get reg_8921_l23, rc = %ld\n",
				PTR_ERR(reg_8921_l23));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_l23, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_l23, rc=%d\n", rc);
			return -EINVAL;
		}
	}
	if (!reg_8921_s4) {
		reg_8921_s4 = regulator_get(&hdmi_msm_device.dev, "hdmi_vcc");
		if (IS_ERR(reg_8921_s4)) {
			pr_err("could not get reg_8921_s4, rc = %ld\n",
				PTR_ERR(reg_8921_s4));
			return -ENODEV;
		}
		rc = regulator_set_voltage(reg_8921_s4, 1800000, 1800000);
		if (rc) {
			pr_err("set_voltage failed for 8921_s4, rc=%d\n", rc);
			return -EINVAL;
		}
	}

	if (on) {
		rc = regulator_set_optimum_mode(reg_8921_l23, 100000);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		rc = regulator_enable(reg_8921_l23);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_avdd", rc);
			return rc;
		}
		rc = regulator_enable(reg_8921_s4);
		if (rc) {
			pr_err("'%s' regulator enable failed, rc=%d\n",
				"hdmi_vcc", rc);
			return rc;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		rc = regulator_disable(reg_8921_l23);
		if (rc) {
			pr_err("disable reg_8921_l23 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_disable(reg_8921_s4);
		if (rc) {
			pr_err("disable reg_8921_s4 failed, rc=%d\n", rc);
			return -ENODEV;
		}
		rc = regulator_set_optimum_mode(reg_8921_l23, 100);
		if (rc < 0) {
			pr_err("set_optimum_mode l23 failed, rc=%d\n", rc);
			return -EINVAL;
		}
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;

	return 0;
}

static int hdmi_gpio_config(int on)
{
	int rc = 0;
	static int prev_on;

	if (on == prev_on)
		return 0;

	if (on) {
		rc = gpio_request(100, "HDMI_DDC_CLK");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_CLK", 100, rc);
			return rc;
		}
		rc = gpio_request(101, "HDMI_DDC_DATA");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_DDC_DATA", 101, rc);
			goto error1;
		}
		rc = gpio_request(102, "HDMI_HPD");
		if (rc) {
			pr_err("'%s'(%d) gpio_request failed, rc=%d\n",
				"HDMI_HPD", 102, rc);
			goto error2;
		}
		pr_debug("%s(on): success\n", __func__);
	} else {
		gpio_free(100);
		gpio_free(101);
		gpio_free(102);
		pr_debug("%s(off): success\n", __func__);
	}

	prev_on = on;
	return 0;

error2:
	gpio_free(101);
error1:
	gpio_free(100);
	return rc;
}

static int hdmi_cec_power(int on)
{
	return 0;
}
#endif /* CONFIG_FB_MSM_HDMI_MSM_PANEL */

void __init msm8960_init_fb(void)
{
	platform_device_register(&msm_fb_device);

#ifdef CONFIG_FB_MSM_WRITEBACK_MSM_PANEL
	platform_device_register(&wfd_panel_device);
	platform_device_register(&wfd_device);
#endif

	if (machine_is_msm8960_sim())
		platform_device_register(&mipi_dsi_simulator_panel_device);


	if (!machine_is_msm8960_sim() && !machine_is_msm8960_rumi3()) {
#ifdef CONFIG_FB_MSM_MIPI_DSI_HIMAX
	platform_device_register(&mipi_dsi_himax_panel_device);
#endif
#ifdef CONFIG_FB_MSM_HDMI_MSM_PANEL
		platform_device_register(&hdmi_msm_device);
#endif
	}

	if (machine_is_msm8x60_rumi3()) {
		msm_fb_register_device("mdp", NULL);
		mipi_dsi_pdata.target_type = 1;
	} else
		msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#ifdef CONFIG_MSM_BUS_SCALING
	msm_fb_register_device("dtv", &dtv_pdata);
#endif
#ifdef CONFIG_ACER_HDMI_MHL_SII8334
	i2c_register_board_info(MSM_8960_GSBI10_QUP_I2C_BUS_ID, mhl_sii8334_info,
			ARRAY_SIZE(mhl_sii8334_info));
#endif
}

void __init msm8960_allocate_fb_region(void)
{
	void *addr;
	unsigned long size;

	size = MSM_FB_SIZE;
	addr = alloc_bootmem_align(size, 0x1000);
	msm_fb_resources[0].start = __pa(addr);
	msm_fb_resources[0].end = msm_fb_resources[0].start + size - 1;
	pr_info("allocating %lu bytes at %p (%lx physical) for fb\n",
			size, addr, __pa(addr));
}

void __init msm8960_set_display_params(char *prim_panel, char *ext_panel)
{
	int disable_splash = 1;
	if (disable_splash)
		mdp_pdata.cont_splash_enabled = 0;
}
