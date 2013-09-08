#include <linux/input.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/earlysuspend.h>
#include <mach/board.h>
#include <linux/cypress_ts.h>
#include <linux/slab.h>
#include <asm/mach-types.h>
#include <linux/regulator/consumer.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/file.h>
#include <linux/mm.h>

struct cypress_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct  work;
#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
	int user_panel;
	bool (*enable_fw_update)(void);
#endif
	int prev_points;
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int abs_pressure_min;
	int abs_pressure_max;
	int abs_id_min;
	int abs_id_max;
	int points_max;
	int firmware_path;
	int irqflags;
	int (*hw_init)(struct device *dev, bool on);
	int (*power)(struct device *dev, int ch);
	int (*gpio_cfg)(bool on);
	struct regulator **vdd;
};

struct _pos {
	uint x;
	uint y;
	uint z;
	uint id;
};

struct sensitivity_mapping {
	int symbol;
	int value;
};

static struct sensitivity_mapping sensitivity_table[] = {
	{TOUCH_SENSITIVITY_SYMBOL_HIGH,    15},
	{TOUCH_SENSITIVITY_SYMBOL_MEDIUM,  20},
	{TOUCH_SENSITIVITY_SYMBOL_LOW,     30},
};

#define TOUCH_SENSITIVITY_SYMBOL_DEFAULT TOUCH_SENSITIVITY_SYMBOL_MEDIUM;
#define TOUCH_FIRMWARE_UPDATE_PATH_DEFAULT AUTO;

#define FILEPATH_A9     "/system/etc/firmware/a9_tp_fw.hex"
#define FILEPATH_A9_DVT "/system/etc/firmware/a9_tp_fw_dvt.hex"
#define FILEPATH_USER	"/sdcard/a9_tp_fw_dvt.hex"
#define FILEPATH_USEREX	"/mnt/external_sd/a9_tp_fw_dvt.hex"

static struct workqueue_struct *cypress_wq;
static struct kobject *touchdebug_kobj;
static struct kobject *touchdebug_kobj_info;
struct cypress_ts_data *ts = NULL;
static struct _pos pos[4];
static struct sensitivity_mapping sensitivity_table[TOUCH_SENSITIVITY_SYMBOL_COUNT];

int myatoi(const char *a)
{
	int s = 0;

	while(*a >= '0' && *a <= '9')
		s = (s << 3) + (s << 1) + *a++ - '0';
	return s;
}

static int read_fw_version(struct cypress_ts_data *ts, uint8_t *data)
{
	uint8_t wdata[1] = {0x1B};
	int retry = 5;

	/* vote to turn on power */
	if (ts->power(&ts->client->dev, TS_VDD_POWER_ON))
		pr_err("%s: power on failed\n", __func__);

	msleep(500);

	/* 0x1B: FW version */
	wdata[0] = 0x1B;
	while (retry > 0) {
		if (1 != i2c_master_send(ts->client, wdata, 1))
			pr_info("%s: i2c send err\n", __func__);

		if (1 != i2c_master_recv(ts->client, data, 1)) {
			pr_info("%s: i2c recv err\n", __func__);
			retry--;
		} else
			break;
		msleep(500);
	}

	/* vote to turn off power because update is finish */
	if (ts->power(&ts->client->dev, TS_VDD_POWER_OFF))
		pr_err("%s: power off failed\n", __func__);

	if (ts->power(&ts->client->dev, TS_RESET))
		pr_err("%s: reset failed\n", __func__);

	if (retry == 0)
		return -EIO;
	else
		return 0;
}

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
static int firmware_update_func(struct cypress_ts_data *ts)
{
	signed char rc;

	/* vote to turn on power */
	if (ts->power(&ts->client->dev, TS_VDD_POWER_ON))
		pr_err("%s: power on failed\n", __func__);

	pr_info("================ download_firmware_main ===============");
	if (ts->firmware_path == AUTO)
		rc = download_firmware_main(FILEPATH_A9_DVT);
	else if (ts->firmware_path == USER)
		rc = download_firmware_main(FILEPATH_USER);
	else if (ts->firmware_path == USER_EX)
		rc = download_firmware_main(FILEPATH_USEREX);
	else
		rc = download_firmware_main(FILEPATH_A9_DVT);

	/* vote to turn off power because update is finish */
	if (ts->power(&ts->client->dev, TS_VDD_POWER_OFF))
		pr_err("%s: power off failed\n", __func__);

	if (rc)
		return rc;

	if (ts->power(&ts->client->dev, TS_RESET))
		pr_info("%s: fail to reset tp\n", __func__);

	return 0;
}

static ssize_t update_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char * buf)
{
	uint8_t rdata[1] = {0};
	uint8_t rdata_old[1] = {0};
	int rc, retry = 0;

	rc = read_fw_version(ts, rdata_old);
	if (rc)
		pr_err("%s: read fw version fail\n", __func__);

	do {
		rc = firmware_update_func(ts);
		if (rc)
			goto original_fw_err;

		msleep(1000);

		rc = read_fw_version(ts, rdata);
		if (!rc) {
			return sprintf(buf, "new ver: %x\n", rdata[0]);
		}
		pr_info("%s: retry %d times\n", __func__, retry);
	} while (retry++ < 10);

	return sprintf(buf, "Firmware update fail\n");
original_fw_err:
	return sprintf(buf, "Original FW is wrong, ver: %x\n", rdata_old[0]);
}

static ssize_t update_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char * buf, size_t n)
{
	if (!strcmp(buf, "AUTO\n"))
		ts->firmware_path = AUTO;
	else if (!strcmp(buf, "USER\n"))
		ts->firmware_path = USER;
	else if (!strcmp(buf, "USER_EX\n"))
		ts->firmware_path = USER_EX;
	else
		ts->firmware_path = TOUCH_FIRMWARE_UPDATE_PATH_DEFAULT;

	pr_info("%s: frimware_path = %d\n", __func__, ts->firmware_path);

	return n;
}
#endif

static ssize_t firmware_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char * buf)
{
	uint8_t rdata[1] = {0};
	int rc;

	rc = read_fw_version(ts, rdata);
	if (rc) {
		pr_info("%s: read fw version fail\n", __func__);
		goto i2c_err;
	}

	return sprintf(buf, "CY0-%x0-12060100\n", rdata[0]);

i2c_err:
	return sprintf(buf, "Unknown firmware\n");
}

static ssize_t sensitivity_store(struct kobject *kobj,
				struct kobj_attribute *attr,
				const char * buf, size_t n)
{
	uint8_t wdata[2] = {0};
	int symbol = -1;

	symbol = myatoi(buf);
	pr_info("sensitive_store value:%d\n", symbol);

	wdata[0] = 0x1C;
	wdata[1] = sensitivity_table[symbol].value;
	if (2 != i2c_master_send(ts->client, wdata, 2))
		pr_err("Can not write sensitivity\n");

	return n;
}

static ssize_t sensitivity_show(struct kobject *kobj,
				struct kobj_attribute *attr,
				char * buf)
{
	uint8_t wdata[1] = {0};
	uint8_t rdata[1] = {0};
	int i, symbol = -1;

	wdata[0] = 0x1C;
	if (1 != i2c_master_send(ts->client, wdata, 1))
		goto i2c_err;
	if (1 != i2c_master_recv(ts->client, rdata, 1))
		goto i2c_err;

	for (i = 0; i < TOUCH_SENSITIVITY_SYMBOL_COUNT; i++) {
		if (sensitivity_table[i].value == rdata[0]) {
			symbol = sensitivity_table[i].symbol;
			break;
		}
	}

i2c_err:
	if (symbol == -1) {
		pr_info("touch sensitivity default value\n");
		symbol = TOUCH_SENSITIVITY_SYMBOL_DEFAULT;
	}

	return sprintf(buf, "%d\n", symbol);
}

#define touch_attr(_name) \
static struct kobj_attribute _name##_attr = { \
	.attr = { \
	.name = __stringify(_name), \
	.mode = 0644, \
	}, \
	.show = _name##_show, \
	.store = _name##_store, \
}

static struct kobj_attribute firmware_attr = { \
	.attr = { \
	.name = __stringify(firmware), \
	.mode = 0644, \
	}, \
	.show = firmware_show, \
};

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
touch_attr(update);
#endif
touch_attr(sensitivity);

static struct attribute * g[] = {
	&update_attr.attr,
	&sensitivity_attr.attr,
	NULL,
};

static struct attribute * g_info[] = {
	&firmware_attr.attr,
	NULL,
};

static struct attribute_group attr_group = {
	.attrs = g,
};

static struct attribute_group attr_group_info = {
	.attrs = g_info,
};

static int cypress_set_power_state(int status, struct i2c_client *client)
{
	uint8_t wdata[2] = {0};

	pr_debug("%s: status: %x\n", __func__, status);
	switch (status) {
	case INIT_STATE:
		/* TODO: read fw version in initial state */
		break;
	case SUSPEND_STATE:
		/* set deep sleep mode */
		wdata[0] = 0;
		wdata[1] = 2;
		if (2 != i2c_master_send(client, wdata, 2))
			goto i2c_err;
		break;
	case RESUME_STATE:
		/* set operating mode */
		wdata[0] = 0;
		wdata[1] = 4;
		if (2 != i2c_master_send(client, wdata, 2))
			goto i2c_err;
		break;
	default:
		break;
	}

	return 0;

i2c_err:
	pr_err("%s: i2c error (%d)\n", __func__, status);
	return -ENXIO;
}

static void cypress_work_func(struct work_struct *work)
{
	struct cypress_ts_data *ts =
		container_of(work, struct cypress_ts_data, work);
	uint8_t rdata[19] = {0};
	uint8_t wdata[1] = {0};
	int points = 0;
	int i;
	int palm;

	wdata[0] = 0x02;
	if (1 != i2c_master_send(ts->client, wdata, 1))
		goto i2c_err;
	if (7 != i2c_master_recv(ts->client, rdata, 7)) {
		pr_err("%s: i2c recv error\n", __func__);
		goto i2c_err;
	}

	points = ((rdata[0] & 0x0F) > ts->points_max)
		? ts->points_max : (rdata[0] & 0x0F);

	palm = (rdata[0] & 0x10) ? 1 : 0;
	if (palm) {
		pr_info("Palm rejection!");
	}

	if (points > 0) {
		pos[0].x = rdata[1] << 8 | rdata[2];
		pos[0].y = rdata[3] << 8 | rdata[4];
		pos[0].z = rdata[5];
		pos[0].id = (rdata[6] >> 4) & 0x0F;
	}

	if (points > 1) {
		wdata[0] = 0x08;
		if (1 != i2c_master_send(ts->client, wdata, 1))
			goto i2c_err;
		if (19 != i2c_master_recv(ts->client, rdata, 19))
			goto i2c_err;

		pos[1].x = rdata[1] << 8 | rdata[2];
		pos[1].y = rdata[3] << 8 | rdata[4];
		pos[1].z = rdata[5];
		pos[1].id = rdata[0] & 0x0F;
		pos[2].x = rdata[8] << 8 | rdata[9];
		pos[2].y = rdata[10] << 8 | rdata[11];
		pos[2].z = rdata[12];
		pos[2].id = (rdata[13] >> 4) & 0x0F;
		pos[3].x = rdata[14] << 8 | rdata[15];
		pos[3].y = rdata[16] << 8 | rdata[17];
		pos[3].z = rdata[18];
		pos[3].id = rdata[13] & 0x0F;
	}

	for (i = 0; i < ts->prev_points - points; i++) {
		input_mt_sync(ts->input_dev);
	}
	ts->prev_points = points;

	for (i = 0; i < points; i++) {
		/*
		   pr_info("x%d = %u,  y%d = %u, z%d = %u, id%d=%u,  points = %d\n",
		   i, pos[i].x, i, pos[i].y, i, pos[i].z, i, pos[i].id, points);
		 */
		if(pos[i].y > ts->abs_y_max)
			continue;

		input_report_abs(ts->input_dev, ABS_MT_POSITION_X,
			(pos[i].x > ts->abs_x_max) ? 0 : pos[i].x);
		input_report_abs(ts->input_dev, ABS_MT_POSITION_Y,
			(pos[i].y > ts->abs_y_max) ? 0 : pos[i].y);
		input_report_abs(ts->input_dev, ABS_MT_TOUCH_MAJOR,
			((pos[i].z > ts->abs_pressure_max) || palm) ? 0 : pos[i].z);
		input_report_abs(ts->input_dev, ABS_MT_PRESSURE,
			((pos[i].z > ts->abs_pressure_max) || palm) ? 0 : pos[i].z);
		if (ts->abs_id_max > 0)
			input_report_abs(ts->input_dev, ABS_MT_TRACKING_ID,
				((pos[i].id-1) > ts->abs_id_max) ? 0 : (pos[i].id-1));
		input_mt_sync(ts->input_dev);
	}

	input_sync(ts->input_dev);

	enable_irq(ts->client->irq);
	return ;

i2c_err:
	pr_info("Touch I2C error: hardware reset\n");
	if (ts->power(&ts->client->dev, TS_RESET))
		pr_info("%s: fail to reset tp\n", __func__);
	enable_irq(ts->client->irq);
}

static irqreturn_t cypress_ts_interrupt(int irq, void *dev_id)
{
	struct cypress_ts_data *ts = dev_id;

	disable_irq_nosync(ts->client->irq);
	queue_work(cypress_wq, &ts->work);

	return IRQ_HANDLED;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
void cypress_early_suspend(struct early_suspend *h)
{
	struct cypress_ts_data *ts;

	pr_info("%s: enter\n", __func__);
	ts = container_of(h, struct cypress_ts_data, early_suspend);

	cypress_set_power_state(SUSPEND_STATE, ts->client);
}

void cypress_early_resume(struct early_suspend *h)
{
	struct cypress_ts_data *ts;

	pr_info("%s: enter\n", __func__);
	ts = container_of(h, struct cypress_ts_data, early_suspend);

	cypress_set_power_state(RESUME_STATE, ts->client);
}
#endif

static int cypress_probe(
	struct i2c_client *client, const struct i2c_device_id *id)
{
	struct cypress_i2c_platform_data *pdata;
	int ret = 0;

	pr_info("%s: enter\n", __func__);
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_ERR "%s: need I2C_FUNC_I2C\n", __func__);
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	ts = kzalloc(sizeof(struct cypress_ts_data), GFP_KERNEL);
	if (!ts) {
		ret = -ENOMEM;
		goto err_alloc_data_failed;
	}

	memset(pos, 0, sizeof(pos));
	INIT_WORK(&ts->work, cypress_work_func);
	ts->client = client;
	strlcpy(client->name, CYPRESS_TS_DRIVER_NAME,
		strlen(CYPRESS_TS_DRIVER_NAME));
	i2c_set_clientdata(client, ts);
	pdata = client->dev.platform_data;

	if (pdata) {
		ts->hw_init = pdata->hw_init;
		ts->power = pdata->power;
		ts->gpio_cfg = pdata->gpio_cfg;
		ts->power = pdata->power;
		ts->abs_x_max = pdata->abs_x_max;
		ts->abs_y_max = pdata->abs_y_max;
		ts->abs_pressure_max = pdata->abs_pressure_max;
		ts->abs_id_max = pdata->abs_id_max;
		ts->points_max = pdata->points_max;
		ts->firmware_path = TOUCH_FIRMWARE_UPDATE_PATH_DEFAULT;
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
		ts->enable_fw_update = pdata->enable_fw_update;
#endif
		ret = ts->gpio_cfg(true);
		if (ret) {
			pr_err("%s: cypress gpio configure failed\n", __func__);
			goto error_gpio_cfg;
		}

		ret = ts->hw_init(&ts->client->dev, true);
		if (ret) {
			pr_err("%s: hw init failed\n", __func__);
			ts->gpio_cfg(false);
			goto err_hw_init_failed;
		}

		ret = ts->power(&ts->client->dev, TS_RESET);
		if (ret) {
			pr_err("%s: reset failed\n", __func__);
			goto err_power_on_failed;
		}
	}

	if (cypress_set_power_state(INIT_STATE, ts->client) != 0) {
		pr_info("%s: set mode  failed\n", __func__);
		goto err_power_on_failed;
	}

	ts->input_dev = input_allocate_device();
	if (ts->input_dev == NULL) {
		ret = -ENOMEM;
		pr_err("%s: Failed to allocate input device\n", __func__);
		goto err_input_dev_alloc_failed;
	}

	ts->input_dev->name = CYPRESS_TS_DRIVER_NAME;
	set_bit(EV_SYN, ts->input_dev->evbit);
	set_bit(EV_ABS, ts->input_dev->evbit);
	set_bit(EV_KEY, ts->input_dev->evbit);
	set_bit(ABS_MT_TOUCH_MAJOR, ts->input_dev->keybit);

	input_set_abs_params(ts->input_dev, ABS_MT_TOUCH_MAJOR,
		pdata->abs_pressure_min, pdata->abs_pressure_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_X,
		pdata->abs_x_min, pdata->abs_x_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_POSITION_Y,
		pdata->abs_y_min, pdata->abs_y_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_PRESSURE,
		0, pdata->points_max, 0, 0);
	input_set_abs_params(ts->input_dev, ABS_MT_TRACKING_ID,
		pdata->abs_id_min, pdata->abs_id_max, 0, 0);

	ret = input_register_device(ts->input_dev);
	if (ret) {
		pr_err("%s: Unable to register %s input device\n",
			__func__, ts->input_dev->name);
		goto err_input_register_device_failed;
	}

	if (client->irq) {
		ret = request_irq(client->irq, cypress_ts_interrupt,
			pdata->irqflags, client->name, ts);
		if (ret) {
			pr_err("%s: Unable to register %s irq\n",
				__func__, ts->input_dev->name);
			ret = -ENOTSUPP;
			goto err_request_irq;
		}
	}

	touchdebug_kobj = kobject_create_and_add("Touch", NULL);
	if (touchdebug_kobj == NULL)
		pr_err("%s: subsystem_register failed\n", __func__);

	ret = sysfs_create_group(touchdebug_kobj, &attr_group);
	if (ret)
		pr_err("%s:sysfs_create_group failed\n", __func__);

	touchdebug_kobj_info = kobject_create_and_add("dev-info_touch", NULL);
	if (touchdebug_kobj == NULL)
		pr_err("%s: subsystem_register failed\n", __func__);

	ret = sysfs_create_group(touchdebug_kobj_info, &attr_group_info);
	if (ret)
		pr_err("%s:sysfs_create_group failed\n", __func__);

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	ts->early_suspend.suspend = cypress_early_suspend;
	ts->early_suspend.resume = cypress_early_resume;
	register_early_suspend(&ts->early_suspend);
#endif
	return 0;

err_request_irq:
	free_irq(client->irq, ts);
err_input_register_device_failed:
	input_free_device(ts->input_dev);
err_input_dev_alloc_failed:
err_power_on_failed:
err_hw_init_failed:
	ts->hw_init(&ts->client->dev, false);
	kfree(ts);
err_alloc_data_failed:
err_check_functionality_failed:
error_gpio_cfg:
	return ret;
}

static int cypress_remove(struct i2c_client *client)
{
	struct cypress_ts_data *ts = i2c_get_clientdata(client);

	pr_info("%s: enter\n", __func__);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	free_irq(client->irq, ts);
	input_unregister_device(ts->input_dev);
	if (ts->hw_init(&ts->client->dev, false))
		pr_err("%s: hw deinit failed\n", __func__);
	kfree(ts);
	return 0;
}

static const struct i2c_device_id cypress_id[] = {
	{ CYPRESS_TS_DRIVER_NAME, 0 },
	{ }
};

static struct i2c_driver cypress_ts_driver = {
	.probe		= cypress_probe,
	.remove		= cypress_remove,
	.id_table	= cypress_id,
	.driver		= {
		.name = CYPRESS_TS_DRIVER_NAME,
	},
};

static int __init cypress_init(void)
{
	pr_info("%s: enter\n", __func__);
	cypress_wq = create_singlethread_workqueue("cypress_wq");
	if (!cypress_wq)
		return -ENOMEM;

	return i2c_add_driver(&cypress_ts_driver);
}

static void __exit cypress_exit(void)
{
	pr_info("%s: enter\n", __func__);
	i2c_del_driver(&cypress_ts_driver);
	if (cypress_wq)
		destroy_workqueue(cypress_wq);
}

module_init(cypress_init);
module_exit(cypress_exit);

MODULE_AUTHOR("MuMing Tsao <MuMing_Tsao@acer.com.tw>");
MODULE_DESCRIPTION("CYPRESS driver");
MODULE_LICENSE("GPL v2");

