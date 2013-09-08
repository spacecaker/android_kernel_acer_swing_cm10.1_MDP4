/*
 * SiI8334 <Firmware or Driver>
 *
 * Copyright (C) 2011 Acer Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed .as is. WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mhl_sii8334.h>
#include <linux/kthread.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/earlysuspend.h>

#include "mhl_porting.h"
#include "si_drv_mhl_tx.h"
#include "si_drvisrconfig.h"
#include "si_mhl_tx_api.h"
#include "mhl_i2c_access.h"

#define KEY_REL_DETECT_TIME    100  //ms

struct sii8334_data {
	int chip_id;
	struct timer_list key_release_timer;
	struct work_struct irq_work;
	struct work_struct key_release_work;
	struct mutex key_mutex;
	struct workqueue_struct *wq;
	struct i2c_client_data *pclient;
	struct input_dev *input;
	struct early_suspend early_suspend;
	struct wake_lock mhl_wakelock;
};

static struct sii8334_data *data;

struct mhl_key_map {
	unsigned char rcp_key;
	unsigned char std_key;
};

static const struct mhl_key_map key_mapping[] = {
	{0x00, KEY_ENTER},
	{0x01, KEY_UP},
	{0x02, KEY_DOWN},
	{0x03, KEY_LEFT},
	{0x04, KEY_RIGHT},
	{0x09, KEY_HOME},
	{0x0A, KEY_MENU},
	{0x0B, KEY_MENU},
	{0x0C, KEY_MENU},
	{0x0D, KEY_BACK},
	{0x20, KEY_0},
	{0x21, KEY_1},
	{0x22, KEY_2},
	{0x23, KEY_3},
	{0x24, KEY_4},
	{0x25, KEY_5},
	{0x26, KEY_6},
	{0x27, KEY_7},
	{0x28, KEY_8},
	{0x29, KEY_9},
	{0x2B, KEY_ENTER},
	{0x37, KEY_PAGEUP},
	{0x38, KEY_PAGEDOWN},
	{0x41, KEY_VOLUMEUP},
	{0x42, KEY_VOLUMEDOWN},
	{0x43, KEY_MUTE},
	{0x44, KEY_PLAYPAUSE},
	{0x45, KEY_STOPCD},
	{0x46, KEY_PLAYPAUSE},
	{0x48, KEY_REWIND},
	{0x49, KEY_FASTFORWARD},
	{0x4B, KEY_NEXTSONG},
	{0x4C, KEY_PREVIOUSSONG},
	{0xff, 0xff},
};

int get_mhl_chip_id(void)
{
	return data ? data->chip_id : -1;
}

void mhl_enable_charging(unsigned int chg_type)
{
	static bool is_charging;

	if(!is_mhl_dongle_on() && (chg_type != POWER_SUPPLY_TYPE_BATTERY)) {
		pr_info("%s:set wrong charging current\n", __func__);
		return;
	}

	if (!is_charging && (chg_type == POWER_SUPPLY_TYPE_BATTERY)) {
		pr_info("%s: should not disable charging!\n", __func__);
		return;
	}

	if (CHARGING_FUNC_NAME) {
		pr_info("MHL set charing type to %d\n", chg_type);
		is_charging = (chg_type != POWER_SUPPLY_TYPE_BATTERY) ? true : false;
		if (is_charging)
			wake_lock(&data->mhl_wakelock);
		else
			wake_unlock(&data->mhl_wakelock);
		CHARGING_FUNC_NAME(chg_type);
	} else
		pr_info("%s:Can't find charging function!\n", __func__);
}

static void mhl_dev_id(void)
{
	uint8_t id_lo, id_hi;

	id_lo = SiiRegRead(TX_PAGE_TPI | 0x02);
	id_hi = SiiRegRead(TX_PAGE_TPI | 0x03);
	if (data) {
		data->chip_id = id_hi << 8 | id_lo;
		pr_info("%s: Dev_ID = %x\n", __func__, data->chip_id);
	} else
		pr_info("%s: warning, data is null!\n", __func__);
}

static void sii8334_irq_work(struct work_struct *work)
{
	SiiMhlTxDeviceIsr();
}

static irqreturn_t sii8334_irq(int irq, void *handle)
{
	queue_work(data->wq, &data->irq_work);
	return IRQ_HANDLED;
}

static void sii8334_key_release_work(struct work_struct *work)
{
	AppNotifyMhlEvent(MHL_TX_RCP_KEY_RELEASE, 0);
}

static void key_release_poll_func(unsigned long arg)
{
	queue_work(data->wq, &data->key_release_work);
}

static inline unsigned char map_key(unsigned char buf)
{
	unsigned char i = 0;
	while((key_mapping[i].rcp_key != buf) &&
		(key_mapping[i].rcp_key != 0xff)) {
		i++;
	}
	return key_mapping[i].std_key;
}

void AppNotifyMhlEvent(unsigned char eventCode, unsigned char eventParam)
{
	static unsigned char last_key_code = 0xff;
	unsigned char key_code;
	static bool key_pressed;

	if ((eventCode == MHL_TX_EVENT_RCP_RECEIVED) ||
		(eventCode == MHL_TX_RCP_KEY_RELEASE)) {

		key_code = map_key(eventParam);
		if (key_code == 0xff) {
			pr_info("%s:get worng key:%x\n", __func__, key_code);
			return;
		}

		if (!data || !data->input) {
			pr_info("%s:MHL input device is not ready!\n", __func__);
			return;
		}

		TX_DEBUG_PRINT("rcp key code:%x %x\n", eventCode, key_code);
		del_timer_sync(&data->key_release_timer);

		mutex_lock(&data->key_mutex);
		if (eventCode == MHL_TX_EVENT_RCP_RECEIVED) {
			if ((last_key_code != key_code) && key_pressed) {
				input_report_key(data->input, last_key_code, 0);
				input_sync(data->input);
			}

			input_report_key(data->input, key_code, 1);
			input_sync(data->input);
			key_pressed = true;
			last_key_code = key_code;
			mod_timer(&data->key_release_timer,
					jiffies + msecs_to_jiffies(KEY_REL_DETECT_TIME));
		} else {
			if (key_pressed) {
				input_report_key(data->input, last_key_code, 0);
				input_sync(data->input);
			}
			key_pressed = false;
		}
		mutex_unlock(&data->key_mutex);
	}
}

void sii8334_early_suspend(struct early_suspend *h)
{
	struct sii8334_platform_data *pdata;

	pdata = data->pclient[TX_PAGE_TPI].client->dev.platform_data;
	if (pdata)
		pdata->enable_5v(false);
}

void sii8334_late_resume(struct early_suspend *h)
{
	struct sii8334_platform_data *pdata;

	pdata = data->pclient[TX_PAGE_TPI].client->dev.platform_data;
	if (pdata)
		pdata->enable_5v(true);
}

static int sii8334_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int ret = 0;
	int i, client_num;
	struct sii8334_platform_data *pdata;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		pr_err("%s: i2c_check_functionality failed!\n", __func__);
		ret = -EIO;
		goto exit_i2c_check_failed;
	}

	pdata = client->dev.platform_data;
	if (!pdata) {
		pr_err("%s: platform data not set\n", __func__);
		ret = -EFAULT;
		goto exit_platform_data_failed;
	}

	data = kzalloc(sizeof(struct sii8334_data), GFP_KERNEL);
	if (data == NULL) {
		pr_err("%s: no memory for driver data\n", __func__);
		ret = -ENOMEM;
		goto exit_kzalloc_failed;
	}
	wake_lock_init(&data->mhl_wakelock, WAKE_LOCK_SUSPEND, "MHL dongle");

	client_num = SiiClientGet(&data->pclient);
	data->pclient[TX_PAGE_TPI].client = client;
	i2c_set_clientdata(client, data);

	for (i = 1; i < client_num; i++) {
		data->pclient[i].client =
			i2c_new_dummy(client->adapter, data->pclient[i].sladdr >> 1);
		pr_info("%s: 0x%x dummy sladdr is registered\n", __func__, data->pclient[i].sladdr);
	}

	/* Set mhl_int as input */
	if (pdata->gpio_irq) {
		ret = gpio_request(pdata->gpio_irq, "mhl_driver_irq");
		if (ret) {
			pr_err("%s: gpio_request failed on pin [%d] (rc=%d)\n",
					__func__, pdata->gpio_irq, ret);
			goto err_irq_gpio_req;
		}
		gpio_direction_input(pdata->gpio_irq);
	}

	/* Set mhl_reset_n as output low */
	if (pdata->gpio_reset) {
		ret = gpio_request(pdata->gpio_reset, "mhl_driver_reset");
		if (ret) {
			pr_err("%s: gpio_request failed on pin [%d] (rc=%d)\n",
					__func__, pdata->gpio_reset, ret);
			goto err_reset_gpio_request;
		}
		gpio_direction_output(pdata->gpio_reset, 0);
	}

	data->wq = create_workqueue("mhl_sii8334_wq");
	if (!data->wq) {
		pr_err("%s: can't create workqueue\n", __func__);
		ret = -EFAULT;
		goto exit_workqueue_failed;
	}

	INIT_WORK(&data->irq_work, sii8334_irq_work);
	INIT_WORK(&data->key_release_work, sii8334_key_release_work);

	init_timer(&data->key_release_timer);
	data->key_release_timer.function = key_release_poll_func;
	data->key_release_timer.data = (unsigned long)NULL;

	/*  Link interrupt routine with the irq */
	if (client->irq) {
		ret = request_irq(client->irq, sii8334_irq,
				IRQF_TRIGGER_FALLING, MHL_SII8334_DRIVER_NAME, data);
		if (ret < 0) {
			pr_err("%s: request_irq failed! on IRQ [%d]\n",
					__func__, client->irq);
			goto err_irq_request;
		}
	}


	/* Input register for RCP */
	data->input = input_allocate_device();
	if (!data->input) {
		pr_err("%s: input_allocate_device failed!\n", __func__);
	} else {
		mutex_init(&data->key_mutex);
		data->input->name = MHL_SII8334_DRIVER_NAME;
		set_bit(EV_KEY, data->input->evbit);
		i = 0;
		while (key_mapping[i].rcp_key != 0xff) {
			input_set_capability(data->input, EV_KEY, key_mapping[i].std_key);
			i++;
		}
		ret = input_register_device(data->input);
		if (ret) {
			pr_err("%s input_register_device failed!\n", __func__);
			data->input = NULL;
		}
	}

	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	data->early_suspend.suspend = sii8334_early_suspend;
	data->early_suspend.resume = sii8334_late_resume;
	register_early_suspend(&data->early_suspend);

	gpio_direction_output(pdata->gpio_reset, 0);
	if (pdata->core_power)
		pdata->core_power(true);

	if (pdata->enable_5v)
		pdata->enable_5v(true);
	udelay(1);
	gpio_direction_output(pdata->gpio_reset, 1);
	mdelay(1);

	mhl_dev_id();
	SiiMhlTxInitialize(30);

	return 0;

err_irq_request:
exit_workqueue_failed:
	gpio_free(pdata->gpio_reset);
err_reset_gpio_request:
	gpio_free(pdata->gpio_irq);
err_irq_gpio_req:
	kfree(data);
exit_kzalloc_failed:
exit_platform_data_failed:
exit_i2c_check_failed:
	return ret;
}

static int sii8334_remove(struct i2c_client *client)
{
	struct sii8334_platform_data *pdata = client->dev.platform_data;

	gpio_free(pdata->gpio_reset);
	gpio_free(pdata->gpio_irq);
	wake_lock_destroy(&data->mhl_wakelock);
	kfree(data);
	i2c_set_clientdata(client, NULL);

	return 0;
}

static const struct i2c_device_id sii8334_id[] = {
	{ MHL_SII8334_DRIVER_NAME, 0 },
	{ }
};

static struct i2c_driver sii8334_driver = {
	.probe     = sii8334_probe,
	.remove    = sii8334_remove,
	.id_table  = sii8334_id,
	.driver    = {
		.name      = MHL_SII8334_DRIVER_NAME,
	},
};

static int __init sii8334_init(void)
{
	int res = 0;

	res = i2c_add_driver(&sii8334_driver);
	if (res) {
		pr_err("%s: i2c_add_driver failed!\n", __func__);
		return res;
	}

	return 0;
}

static void __exit sii8334_exit(void)
{
	i2c_del_driver(&sii8334_driver);
}

module_init(sii8334_init);
module_exit(sii8334_exit);

MODULE_AUTHOR("Brad Chen <ChunHung_Chen@acer.com.tw");
MODULE_DESCRIPTION("MHL SiI8334 driver");
MODULE_LICENSE("GPL");
