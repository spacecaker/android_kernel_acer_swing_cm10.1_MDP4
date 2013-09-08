/* Copyright (c) 2010-2011, Code Aurora Forum. All rights reserved.
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
#include <linux/time.h>
#include <linux/hrtimer.h>
#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_himax.h"
#include "mdp4.h"

static struct mipi_dsi_panel_platform_data *mipi_himax_pdata;
static struct dsi_buf himax_tx_buf;
static struct dsi_buf himax_rx_buf;
static int mipi_himax_lcd_init(void);
static int himax_ic_cut;
static int first_bl = 1;
static int gpio_reset;
static struct dcs_cmd_req cmdreq;

static unsigned long panel_off_time;
static unsigned long panel_on_time;

static char enter_sleep[2] = {0x10, 0x00}; /* DTYPE_DCS_WRITE */
static char exit_sleep[2] = {0x11, 0x00}; /* DTYPE_DCS_WRITE */
static char display_on[2] = {0x29, 0x00}; /* DTYPE_DCS_WRITE */
static char set_te_on[2] = {0x35, 0x00}; /* DTYPE_DCS_WRITE1 */
static char set_ext_cmd[4] = {0xb9, 0xff, 0x83, 0x92}; /* DTYPE_DCS_LWRITE */

static char set_display[13] = { /* DTYPE_DCS_LWRITE */
	0xb2, 0x0f, 0xc8, 0x05,
	0x0f, 0x08, 0x84, 0x00,
	0xff, 0x05, 0x0f, 0x04,
	0x20};

static char set_mode[2] = {0xc2, 0x08}; /*default command mode */

/* 3 lanes */
static char set_num_of_lanes[3] = {0xba, 0x12, 0x83}; /* DTYPE_DCS_LWRITE */
static char blanking_area[3] = {0xc7, 0x00, 0x40}; /* DTYPE_DCS_LWRITE */
static char set_panel[2] = {0xcc, 0x08}; /* DTYPE_DCS_WRITE1 */
static char set_eq_ltps[2] = {0xd4, 0x0c}; /* DTYPE_DCS_WRITE1 */

static char led_pwm1[2] = {0x51, 0x00};	/* DTYPE_DCS_WRITE1 */
static char led_pwm_freq[3] = {0xc9, 0x2f, 0x00};
static char set_cabc_on[2] = {0x53, 0x24};	/* turn on backlight */
static char set_cabc_mode[2] = {0x55, 0x0};	/* 0:cabc off, 1:ui mode, 2:still image, 3:moving image */
static char read_ic_cut[2] = {0xc4, 0x00}; /* DTYPE_DCS_READ */
static char backlight_off[2] = {0x53, 0x0};	/* DTYPE_DCS_WRITE1 */

static struct dsi_cmd_desc read_ic_version = {
	DTYPE_DCS_READ, 1, 0, 0, 5, sizeof(read_ic_cut), read_ic_cut
};
static struct dsi_cmd_desc himax_cmd_backlight_cmds = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(led_pwm1), led_pwm1,
};
static struct dsi_cmd_desc himax_cmd_bl_on[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_cabc_on), set_cabc_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_cabc_mode), set_cabc_mode},
};
static struct dsi_cmd_desc himax_cmd_cabc = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(set_cabc_mode), set_cabc_mode
};
static struct dsi_cmd_desc himax_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(set_ext_cmd), set_ext_cmd},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(set_te_on), set_te_on},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(set_display), set_display},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(set_num_of_lanes), set_num_of_lanes},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(set_mode), set_mode},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(blanking_area), blanking_area},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(set_panel), set_panel},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(set_eq_ltps), set_eq_ltps},
	{DTYPE_DCS_WRITE1, 1, 0, 0, 1,
		sizeof(set_cabc_mode), set_cabc_mode},
	{DTYPE_DCS_LWRITE, 1, 0, 0, 1,
		sizeof(led_pwm_freq), led_pwm_freq},
	{DTYPE_DCS_WRITE, 1, 0, 0, 1,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc himax_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 1,
		sizeof(backlight_off), backlight_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(enter_sleep), enter_sleep}
};

static struct msm_fb_data_type *mfd_ptr;
static void mipi_himax_manufature_cb(u32 data)
{
	himax_ic_cut = data;
	pr_debug("read the himax chip cut:%d\n",himax_ic_cut);
}
static void mipi_himax_read_id(struct msm_fb_data_type *mfd)
{
	cmdreq.cmds = &read_ic_version;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_RX | CMD_REQ_COMMIT;
	cmdreq.rlen = 3;
	cmdreq.cb = mipi_himax_manufature_cb;
	mipi_dsi_cmdlist_put(&cmdreq);
	usleep(10);
}

static void wait_at_least(unsigned long start, unsigned int wait_ms)
{
	unsigned int tmp;

	tmp = jiffies_to_msecs(jiffies - start);
	if ((tmp > 0) && (tmp < wait_ms)) {
		usleep_range((wait_ms - tmp) * 1000, (wait_ms - tmp) * 1000);
	} else if (tmp < 0)
		usleep_range(wait_ms * 1000, wait_ms * 1000);
}

void panel_need_wait(void)
{
	/*the fisrt frame: at least 40ms after disply_on command */
	wait_at_least(panel_on_time, 40);
}

static int mipi_himax_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mfd_ptr = mfd;
	mipi  = &mfd->panel_info.mipi;

	gpio_direction_output(gpio_reset, 0);
	if (!panel_off_time ||
		unlikely(jiffies_to_msecs(jiffies - panel_off_time) < 10)) {
		usleep_range(10 * 1000, 10 * 1000);
	}
	gpio_direction_output(gpio_reset, 1);
	usleep_range(10 * 1000, 10 * 1000);

	if (mipi->mode == DSI_VIDEO_MODE)
		set_mode[1] = 0x03;
	else
		set_mode[1] = 0x08;

	cmdreq.cmds = himax_on_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(himax_on_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;
	mipi_dsi_cmdlist_put(&cmdreq);

	if (mipi->mode == DSI_CMD_MODE) {
		mipi_dsi_cmd_bta_sw_trigger(); /* clean up ack_err_status */

		if (!himax_ic_cut) {
			mipi_himax_read_id(mfd);
			pr_info("%s: himax chip IC cut:%d", __func__, himax_ic_cut);
			if (himax_ic_cut >= 5) {
				set_cabc_mode[1] = 3; //Moving Image mode
				pr_info("CABC mode: Moving mode.");
			} else {
				pr_info("CABC mode: None.");
			}

			if (himax_ic_cut == 0)
				himax_ic_cut = -1;
		}
	}
	panel_on_time = jiffies;
	pr_info("Panel initialize done!\n");
	return 0;
}

static int mipi_himax_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	cmdreq.cmds = himax_display_off_cmds;
	cmdreq.cmds_cnt = ARRAY_SIZE(himax_display_off_cmds);
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
	first_bl = 1;
	gpio_direction_output(gpio_reset, 0);
	usleep_range(8000, 8000);
	panel_off_time = jiffies;
	return 0;
}

static struct dcs_cmd_req cabc_cmdreq;
static void himax_set_cabc(unsigned int mode)
{
	struct mipi_panel_info *mipi;

	if (!mfd_ptr)
		return;

	mipi  = &mfd_ptr->panel_info.mipi;
	if (mipi->mode == DSI_VIDEO_MODE)
		return;

	if (mode > 4) {
		pr_info("%s: get wrong mode\n",__func__);
		return;
	}

	set_cabc_mode[1] = mode;
	cabc_cmdreq.cmds = &himax_cmd_cabc;
	cabc_cmdreq.cmds_cnt = 1;
	cabc_cmdreq.flags = CMD_REQ_COMMIT;
	cabc_cmdreq.rlen = 0;
	cabc_cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cabc_cmdreq);
}

static void mipi_himax_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
	mipi  = &mfd->panel_info.mipi;

	if (first_bl) {
		cmdreq.cmds = himax_cmd_bl_on;
		cmdreq.cmds_cnt = ARRAY_SIZE(himax_cmd_bl_on);
		cmdreq.flags = CMD_REQ_COMMIT;
		cmdreq.rlen = 0;
		cmdreq.cb = NULL;

		mipi_dsi_cmdlist_put(&cmdreq);
		pr_info("[BL] bl level = %d \n",mfd->bl_level);
		first_bl = 0;
	}

	led_pwm1[1] = (unsigned char)mfd->bl_level;

	cmdreq.cmds = &himax_cmd_backlight_cmds;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
}

ssize_t cabc_mode_store(struct device *dev, struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long val;

	if (kstrtoul(buf, 10, &val))
		return -EINVAL;

	if (himax_ic_cut < 5) {
		pr_info("forbid cabc adjusting.");
		return count;
	}

	if ((val < 4) && (val >= 0)) {
		if (!first_bl)
			himax_set_cabc(val); //only set cabc while panel is on
		else
			set_cabc_mode[1] = val;
	}
	return count;
}
ssize_t cabc_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", set_cabc_mode[1]);
}
static struct device_attribute dev_info_attrs =
	__ATTR(cabc_mode, S_IRUGO | S_IWUSR, cabc_mode_show, cabc_mode_store);

static int __devinit mipi_himax_lcd_probe(struct platform_device *pdev)
{
	int ret;

	if (pdev->id == 0) {
		mipi_himax_pdata = pdev->dev.platform_data;
		if (mipi_himax_pdata->gpio) {
			gpio_reset = mipi_himax_pdata->gpio[0];
			ret = gpio_request(gpio_reset, "panel_reset");
			if (ret) {
				pr_err("%s: gpio_request failed on pin [%d] (rc=%d)\n",
						__func__, gpio_reset, ret);
				return -EFAULT;
			}
			gpio_direction_output(gpio_reset, 1);
		}

		if (sysfs_create_file(&pdev->dev.kobj, &dev_info_attrs.attr))
			pr_err("%s:create sysfs error!", __func__);
		return 0;
	}

	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_himax_lcd_probe,
	.driver = {
		.name   = "mipi_himax",
	},
};

static struct msm_fb_panel_data himax_panel_data = {
	.on		= mipi_himax_lcd_on,
	.off		= mipi_himax_lcd_off,
	.set_backlight = mipi_himax_set_backlight,
};

static int ch_used[3];

int mipi_himax_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	ret = mipi_himax_lcd_init();
	if (ret) {
		pr_err("mipi_himax_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	pdev = platform_device_alloc("mipi_himax", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	himax_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &himax_panel_data,
		sizeof(himax_panel_data));
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		printk(KERN_ERR
		  "%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int mipi_himax_lcd_init(void)
{
	mipi_dsi_buf_alloc(&himax_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&himax_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}
