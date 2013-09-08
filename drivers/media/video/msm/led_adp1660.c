/* Copyright (C) 2010, Acer Inc.
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/slab.h>
#include <linux/export.h>
#include "led_adp1660.h"

struct adp1660_work {
	struct work_struct work;
};
static struct adp1660_work *adp1660_flash;
static struct i2c_client *adp1660_client;

#ifdef debug_flash
#ifdef CDBG
#undef CDBG
#endif
#define CDBG(fmt, args...) pr_info(fmt, ##args)
#endif

/* Modify them for changing default flash driver ic output current */
#define DEFAULT_FLASH_HIGH_CURRENT	ADP1660_I_FL_750mA
#define DEFAULT_FLASH_LOW_CURRENT	ADP1660_I_FL_300mA
#define DEFAULT_TORCH_CURRENT		ADP1660_I_TOR_150mA

static int32_t adp1660_i2c_txdata(unsigned short saddr, unsigned char *txdata,
					int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer(adp1660_client->adapter, msg, 1) < 0) {
		pr_err("%s failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int32_t adp1660_i2c_write_b(unsigned short saddr, unsigned short baddr,
					unsigned short bdata)
{
	int32_t rc = -EIO;
	unsigned char buf[2];

	memset(buf, 0, sizeof(buf));
	buf[0] = baddr;
	buf[1] = bdata;
	rc = adp1660_i2c_txdata(saddr, buf, 2);

	if (rc < 0)
		pr_err("%s failed, saddr = 0x%x, baddr = 0x%x,\
			 bdata = 0x%x!\n", __func__, saddr, baddr, bdata);

	return rc;
}

static int32_t adp1660_set_drv_ic_output_torch(uint32_t torch)
{
	int32_t rc = -EIO;

	CDBG("%s called.\n", __func__);
	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED1_TORCH_ASSIST_SET, torch);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED1_FLASH_CURRENT_SET failed\n", __func__);

	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED2_TORCH_ASSIST_SET, torch);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED2_FLASH_CURRENT_SET failed\n", __func__);

	return rc;
}

static int32_t adp1660_set_drv_ic_output_flash(uint32_t flashc)
{
	int32_t rc = -EIO;

	CDBG("%s called.\n", __func__);
	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_BATTERY_LOW_MODE4, ADP1660_V_VB_LO_3P5V);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_BATTERY_LOW_MODE4 failed\n", __func__);

	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_BATTERY_LOW_MODE5, ADP1660_I_VB_LO_500mA);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_BATTERY_LOW_MODE5 failed\n", __func__);

	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED1_FLASH_CURRENT_SET, flashc);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED1_FLASH_CURRENT_SET failed\n", __func__);

	rc = adp1660_i2c_write_b(adp1660_client->addr,
				 ADP1660_REG_LED2_FLASH_CURRENT_SET, flashc);
	if (rc < 0)
		pr_err("%s: Set ADP1660_REG_LED2_FLASH_CURRENT_SET failed\n", __func__);

	return rc;
}

int32_t adp1660_flash_mode_control(int ctrl)
{
	int32_t rc = -EIO;
	unsigned short reg = 0, data = 0;

	CDBG("%s called. case(%d)\n", __func__, ctrl);
	reg = ADP1660_REG_OUTPUT_MODE;
	data = (ADP1660_IL_PEAK_3P25A << ADP1660_IL_PEAK_SHIFT) |
	       (ADP1660_STR_LV_LEVEL_SENSITIVE << ADP1660_STR_LV_SHIFT) |
	       (ADP1660_STR_MODE_SW << ADP1660_STR_MODE_SHIFT);

	switch (ctrl) {
	case MSM_CAMERA_LED_OFF:
		/* Disable flash light output */
		data |= ADP1660_LED_MODE_STANDBY;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Disable flash light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Disable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Disable failed\n", __func__);

		break;

	case MSM_CAMERA_LED_HIGH:
		/* Enable flash light output at high current (750 mA)*/
		adp1660_set_drv_ic_output_flash(DEFAULT_FLASH_HIGH_CURRENT);
		data |= ADP1660_LED_MODE_FLASH;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Enable flash light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Enable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Enable failed\n", __func__);

		break;

	case MSM_CAMERA_LED_LOW:
		/* Enable flash light output at low current (300 mA)*/
		adp1660_set_drv_ic_output_flash(DEFAULT_FLASH_LOW_CURRENT);
		data |= ADP1660_LED_MODE_FLASH;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Enable flash light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Enable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Enable failed\n", __func__);

		break;

	default:
		CDBG("%s: Illegal flash light parameter\n", __func__);
		break;
	}
	return rc;
}

int32_t adp1660_torch_mode_control(int ctrl)
{
	int32_t rc = -EIO;
	unsigned short reg = 0, data = 0;

	CDBG("%s called(%d).\n", __func__,ctrl);
	reg = ADP1660_REG_OUTPUT_MODE;
	data = (ADP1660_IL_PEAK_2P75A << ADP1660_IL_PEAK_SHIFT) |
	       (ADP1660_STR_LV_LEVEL_SENSITIVE << ADP1660_STR_LV_SHIFT) |
	       (ADP1660_STR_MODE_SW << ADP1660_STR_MODE_SHIFT);

	switch (ctrl) {
	case MSM_CAMERA_LED_TORCH_OFF:
		CDBG("%s: MSM_CAMERA_LED_TORCH_OFF\n", __func__);
		/* Disable torch light output */
		data |= ADP1660_LED_MODE_STANDBY;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Disable torch light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Disable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Disable failed\n", __func__);

		break;

	case MSM_CAMERA_LED_TORCH_ON:
		CDBG("%s: MSM_CAMERA_LED_TORCH_ON\n", __func__);
		/* Enable torch light output */
		adp1660_set_drv_ic_output_torch(DEFAULT_TORCH_CURRENT);
		data |= ADP1660_LED_MODE_ASSIST;
		rc = adp1660_i2c_write_b(adp1660_client->addr, reg, data);
		if (rc < 0)
			pr_err("%s: Enable torch light failed\n", __func__);

		rc = adp1660_i2c_write_b(adp1660_client->addr,
					 ADP1660_REG_LED_Enable_Mode, ADP1660_LED_ALL_EN_Enable);
		if (rc < 0)
			pr_err("%s: ADP1660_LED_ALL_EN_Enable failed\n", __func__);

		break;

	default:
		CDBG("%s: Illegal torch light parameter\n", __func__);
		break;
	}

	return rc;
}

static int adp1660_i2c_probe(struct i2c_client *client,
			     const struct i2c_device_id *id)
{
	int rc = 0;

	CDBG("%s called.\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c_check_functionality failed.\n", __func__);
		goto probe_failure;
	}

	adp1660_flash = kzalloc(sizeof(struct adp1660_work), GFP_KERNEL);
	if (!adp1660_flash) {
		pr_err("%s: kzalloc failed.\n", __func__);
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, adp1660_flash);
	adp1660_client = client;

	mdelay(50);

	CDBG("%s successed.\n", __func__);
	return 0;

probe_failure:
	pr_err("%s failed. rc = %d\n", __func__, rc);
	return rc;
}

static const struct i2c_device_id adp1660_i2c_id[] = {
	{"adp1660", 0},
	{}
};

static struct i2c_driver adp1660_i2c_driver = {
	.id_table = adp1660_i2c_id,
	.probe = adp1660_i2c_probe,
	.remove = __exit_p(adp1660_i2c_remove),
	.driver = {
		   .name = "adp1660",
		  },
};

int adp1660_open_init(void)
{
	int rc = 0;

	CDBG("%s called.\n", __func__);
	rc = i2c_add_driver(&adp1660_i2c_driver);
	if (rc < 0 || adp1660_client == NULL) {
		rc = -ENOTSUPP;
		pr_err("%s adp1660 i2c driver failed. rc = %d\n", __func__, rc);
	}

	return rc;
}

void adp1660_release(void)
{
	CDBG("%s called.\n", __func__);
	i2c_del_driver(&adp1660_i2c_driver);
	adp1660_client = NULL;
}
