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

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_sharp.h"

extern int acer_boot_mode;

static struct dsi_buf sharp_tx_buf;
static struct dsi_buf sharp_rx_buf;
static struct dcs_cmd_req cmdreq;
static int mipi_sharp_lcd_init(void);

static char manufacturer[2] = {0xB0, 0x04};
static char unlock[2] = {0x00, 0x00};
static char sequencer[2] = {0xD6, 0x01};
static char brightness[3] = {0x51, 0x0F, 0xFF};
static char bl_reg[3] = {0x51, 0x0F, 0xFF};
static char control_disp[2] = {0x53, 0x04};
static char display_on[2] = {0x29, 0x00};
static char exit_sleep[2] = {0x11, 0x00};

static char display_off[2] = {0x28, 0x00};
static char enter_sleep[2] = {0x10, 0x00};

static struct dsi_cmd_desc sharp_display_on_cmds[] = {
        {DTYPE_DCS_WRITE1, 1, 0, 0, 1,
                sizeof(manufacturer), manufacturer},
        {DTYPE_DCS_WRITE, 1, 0, 0, 1,
                sizeof(unlock), unlock},
        {DTYPE_DCS_WRITE, 1, 0, 0, 1,
                sizeof(unlock), unlock},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 1,
                sizeof(sequencer), sequencer},
        {DTYPE_DCS_LWRITE, 1, 0, 0, 1,
                sizeof(brightness), brightness},
        {DTYPE_DCS_WRITE1, 1, 0, 0, 1,
                sizeof(control_disp), control_disp},
        {DTYPE_DCS_WRITE, 1, 0, 0, 1,
                sizeof(display_on), display_on},
        {DTYPE_DCS_WRITE, 1, 0, 0, 1,
                sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc sharp_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 50, sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, 120, sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc sharp_cmd_bright_level = {
	DTYPE_DCS_LWRITE, 1, 0, 0, 1, sizeof(bl_reg), bl_reg,
};

static int mipi_sharp_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("%s\n", __func__);
	mfd = platform_get_drvdata(pdev);
	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

		mipi_dsi_cmds_tx(&sharp_tx_buf, sharp_display_on_cmds,
			ARRAY_SIZE(sharp_display_on_cmds));

	printk("%s done\n", __func__);
	return 0;
}

static int mipi_sharp_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	printk("%s\n", __func__);
	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	mipi_dsi_cmds_tx(&sharp_tx_buf, sharp_display_off_cmds,
			ARRAY_SIZE(sharp_display_off_cmds));

	printk("%s done\n", __func__);
	return 0;
}

static void mipi_sharp_set_backlight(struct msm_fb_data_type *mfd)
{
	struct mipi_panel_info *mipi;
	mipi  = &mfd->panel_info.mipi;

	printk("backlight level = %d \n", mfd->bl_level);

	bl_reg[2] = (unsigned char)(mfd->bl_level);
	cmdreq.cmds = &sharp_cmd_bright_level;
	cmdreq.cmds_cnt = 1;
	cmdreq.flags = CMD_REQ_COMMIT;
	cmdreq.rlen = 0;
	cmdreq.cb = NULL;

	mipi_dsi_cmdlist_put(&cmdreq);
	return;
}

static int __devinit mipi_sharp_lcd_probe(struct platform_device *pdev)
{
	pr_info("%s\n", __func__);
	msm_fb_add_device(pdev);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_sharp_lcd_probe,
	.driver = {
		.name   = "mipi_sharp",
	},
};

static struct msm_fb_panel_data sharp_panel_data = {
	.on		= mipi_sharp_lcd_on,
	.off		= mipi_sharp_lcd_off,
	.set_backlight = mipi_sharp_set_backlight,
};

static int mipi_sharp_lcd_init(void)
{
	mipi_dsi_buf_alloc(&sharp_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&sharp_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

static int ch_used[3];

int mipi_sharp_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;

	ret = mipi_sharp_lcd_init();
	if (ret) {
		pr_err("mipi_sharp_lcd_init() failed with ret %u\n", ret);
		return ret;
	}

	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_sharp", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	sharp_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &sharp_panel_data,
		sizeof(sharp_panel_data));
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
