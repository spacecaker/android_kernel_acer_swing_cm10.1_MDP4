#include <linux/input.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board.h>

#if 1 /* def CONFIG_CYPRESS_TTSP */
#include "board-acer-8960-a11-touch.h"
#include <linux/cyttsp4_bus.h>
#include <linux/cyttsp4_core.h>
#include <linux/cyttsp4_i2c.h>
#include <linux/cyttsp4_btn.h>
#include <linux/cyttsp4_mt.h>

#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3

static struct regulator **ts_vdd;

static struct cypress_ts_regulator a11_ts_regulator_data[] = {
       {
               .name = "a11_tp_vc",
               .min_uV = VERG_L29_VTG_MIN_UV,
               .max_uV = VERG_L29_VTG_MAX_UV,
               .load_uA = VERG_L29_CURR_24HZ_UA,
       },
       /*  TODO: Remove after runtime PM is enabled in I2C drive */
       {
               .name = "lvs4_i2c1",
               .min_uV = VERG_LVS4_VTG_MIN_UV,
               .max_uV = VERG_LVS4_VTG_MIN_UV,
               .load_uA = VERG_LVS4_CURR_UA,
       },
};

static int cyttsp4_xres(struct cyttsp4_core_platform_data *pdata, struct device *dev)
{
	int rc = 0;

	gpio_set_value(A11_GPIO_CYP_TS_RESET_N, 1);
	msleep(20);
	gpio_set_value(A11_GPIO_CYP_TS_RESET_N, 0);
	msleep(40);
	gpio_set_value(A11_GPIO_CYP_TS_RESET_N, 1);
	msleep(20);
	dev_info(dev,
		"%s: RESET CYTTSP gpio=%d r=%d\n", __func__,
		A11_GPIO_CYP_TS_RESET_N, rc);
	return rc;
}

static int cyttsp4_init(struct cyttsp4_core_platform_data *pdata, int on, struct device *dev)
{
	int retval = 0;

	if (on == false)
		goto ts_reg_disable;

	if (gpio_is_valid(A11_GPIO_CYP_TS_INT_N)) {
		/* configure touchscreen interrupt out gpio */
		retval = gpio_request(A11_GPIO_CYP_TS_INT_N,
						"CTP_INT");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A11_GPIO_CYP_TS_INT_N);
			goto error_irq_gpio_req;
		}

		retval = gpio_direction_input(A11_GPIO_CYP_TS_INT_N);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A11_GPIO_CYP_TS_INT_N);
			goto error_irq_gpio_dir;
		}
	}

	if (gpio_is_valid(A11_GPIO_CYP_TS_RESET_N)) {
		/* configure touchscreen reset out gpio */
		retval = gpio_request(A11_GPIO_CYP_TS_RESET_N,
						"TP_RESET");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A11_GPIO_CYP_TS_RESET_N);
			goto error_reset_gpio_req;
		}

		retval = gpio_direction_output(
					A11_GPIO_CYP_TS_RESET_N, 0);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A11_GPIO_CYP_TS_RESET_N);
			goto error_reset_gpio_dir;
		}
	}
	goto success;

ts_reg_disable:
error_reset_gpio_dir:
	if (gpio_is_valid(A11_GPIO_CYP_TS_RESET_N))
		gpio_free(A11_GPIO_CYP_TS_RESET_N);
error_reset_gpio_req:
	if (gpio_is_valid(A11_GPIO_CYP_TS_RESET_N))
		gpio_direction_output(A11_GPIO_CYP_TS_RESET_N, 0);

error_irq_gpio_dir:
	if (gpio_is_valid(A11_GPIO_CYP_TS_INT_N))
		gpio_free(A11_GPIO_CYP_TS_INT_N);
error_irq_gpio_req:
	if (gpio_is_valid(A11_GPIO_CYP_TS_INT_N))
		gpio_direction_output(A11_GPIO_CYP_TS_INT_N, 1);

success:
	dev_info(dev,
		"%s: INIT CYTTSP RST gpio=%d and IRQ gpio=%d r=%d\n",
		__func__, A11_GPIO_CYP_TS_RESET_N, A11_GPIO_CYP_TS_INT_N, retval);
	return retval;
}

static int cyttsp4_hw_init(struct cyttsp4_core_platform_data *pdata, int on, struct device *dev)
{
	int rc = 0, i;
	const struct cypress_ts_regulator *reg_info = NULL;
	u8 num_reg = 0;

	reg_info = a11_ts_regulator_data;
	num_reg = ARRAY_SIZE(a11_ts_regulator_data);

	ts_vdd = kzalloc(num_reg * sizeof(struct regulator *), GFP_KERNEL);

	if (!reg_info) {
		pr_err("regulator pdata not specified\n");
		return -EINVAL;
	}

	if (on == false) /* Turn off the regulators */
		goto ts_reg_disable;

	if (!ts_vdd) {
		pr_err("unable to allocate memory\n");
		return -ENOMEM;
	}

	for (i = 0; i < num_reg; i++) {
		ts_vdd[i] = regulator_get(dev, reg_info[i].name);

		if (IS_ERR(ts_vdd[i])) {
			rc = PTR_ERR(ts_vdd[i]);
			pr_err("%s:regulator get failed rc=%d\n",
							__func__, rc);
			goto error_vdd;
		}

		if (regulator_count_voltages(ts_vdd[i]) > 0) {
			rc = regulator_set_voltage(ts_vdd[i],
				reg_info[i].min_uV, reg_info[i].max_uV);
			if (rc) {
				pr_err("%s: regulator_set_voltage"
					"failed rc =%d\n", __func__, rc);
				regulator_put(ts_vdd[i]);
				goto error_vdd;
			}
		}

		rc = regulator_set_optimum_mode(ts_vdd[i],
						reg_info[i].load_uA);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
								__func__, rc);

			regulator_set_voltage(ts_vdd[i], 0,
						reg_info[i].max_uV);
			regulator_put(ts_vdd[i]);
			goto error_vdd;
		}

		rc = regulator_enable(ts_vdd[i]);
		if (rc) {
			pr_err("%s: regulator_enable failed rc =%d\n",
								__func__, rc);

			regulator_set_voltage(ts_vdd[i], 0,
						reg_info[i].max_uV);
			regulator_put(ts_vdd[i]);
			goto error_vdd;
		}
	}

	return rc;

ts_reg_disable:

	i = num_reg;

	while (--i >= 0) {
		ts_vdd[i] = regulator_get(dev, reg_info[i].name);
		regulator_set_voltage(ts_vdd[i], 0,
					reg_info[i].max_uV);
		regulator_disable(ts_vdd[i]);
		regulator_put(ts_vdd[i]);
	}
	kfree(ts_vdd);
error_vdd:
	return rc;
}

static int cyttsp4_wakeup(struct device *dev)
{
	int rc = 0;
	int i = 0;
	u8 num_reg = 0;
	const struct cypress_ts_regulator *reg_info = NULL;

	num_reg = ARRAY_SIZE(a11_ts_regulator_data);
	reg_info = a11_ts_regulator_data;
	i = num_reg;

	dev_info(dev,
		"%s enable ts_vdd\n", __func__);
	while (--i >= 0) {
		regulator_enable(ts_vdd[i]);
		rc = regulator_set_optimum_mode(ts_vdd[i], reg_info[i].load_uA);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
				__func__, rc);
		}
	}

	rc = gpio_direction_output(A11_GPIO_CYP_TS_INT_N, 0);
	if (rc < 0) {
		dev_err(dev,
			"%s: Fail set output gpio=%d\n",
			__func__, A11_GPIO_CYP_TS_INT_N);
	} else {
		udelay(2000);
		rc = gpio_direction_input(A11_GPIO_CYP_TS_INT_N);
		if (rc < 0) {
			dev_err(dev,
				"%s: Fail set input gpio=%d\n",
				__func__, A11_GPIO_CYP_TS_INT_N);
		}
	}

	dev_info(dev,
		"%s: WAKEUP CYTTSP gpio=%d r=%d\n", __func__,
		A11_GPIO_CYP_TS_INT_N, rc);
	return rc;
}

static int cyttsp4_sleep(struct device *dev)
{
	int i = 0;
	int rc = 0;
	u8 num_reg = 0;

	num_reg = ARRAY_SIZE(a11_ts_regulator_data);
	i = num_reg;

	dev_info(dev,
		"%s disable ts_vdd\n", __func__);
	while (--i >= 0) {
		regulator_disable(ts_vdd[i]);
		rc = regulator_set_optimum_mode(ts_vdd[i], 100);
		if (rc < 0) {
			pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
				__func__, rc);
		}
	}

	return 0;
}

static int cyttsp4_power(struct cyttsp4_core_platform_data *pdata, int on,
	struct device *dev, atomic_t *ignore_irq)
{
	if (on)
		return cyttsp4_wakeup(dev);

	return cyttsp4_sleep(dev);
}

/* Button to keycode conversion */
static u16 cyttsp4_btn_keys[] = {
	/* use this table to map buttons to keycodes (see input.h) */
	KEY_HOME,		/* 102 */
	KEY_MENU,		/* 139 */
	KEY_BACK,		/* 158 */
	KEY_SEARCH,		/* 217 */
	KEY_VOLUMEDOWN,		/* 114 */
	KEY_VOLUMEUP,		/* 115 */
	KEY_CAMERA,		/* 212 */
	KEY_POWER		/* 116 */
};

static struct touch_settings cyttsp4_sett_btn_keys = {
	.data = (uint8_t *)&cyttsp4_btn_keys[0],
	.size = ARRAY_SIZE(cyttsp4_btn_keys),
	.tag = 0,
};

static struct cyttsp4_core_platform_data _cyttsp4_core_platform_data = {
	.irq_gpio = A11_GPIO_CYP_TS_INT_N,
	.xres = cyttsp4_xres,
	.init = cyttsp4_init,
	.hw_init = cyttsp4_hw_init,
	.power = cyttsp4_power,
	.sett = {
		NULL,	/* Reserved */
		NULL,	/* Command Registers */
		NULL,	/* Touch Report */
		NULL,	/* Cypress Data Record */
		NULL,	/* Test Record */
		NULL,	/* Panel Configuration Record */
		NULL, /* &cyttsp4_sett_param_regs, */
		NULL, /* &cyttsp4_sett_param_size, */
		NULL,	/* Reserved */
		NULL,	/* Reserved */
		NULL,	/* Operational Configuration Record */
		NULL, /* &cyttsp4_sett_ddata, *//* Design Data Record */
		NULL, /* &cyttsp4_sett_mdata, *//* Manufacturing Data Record */
		NULL,	/* Config and Test Registers */
		&cyttsp4_sett_btn_keys,	/* button-to-keycode table */
	},
};

static struct cyttsp4_core cyttsp4_core_device = {
	.name = CYTTSP4_CORE_NAME,
	.id = "main_ttsp_core",
	.adap_id = CYTTSP4_I2C_NAME,
	.dev = {
		.platform_data = &_cyttsp4_core_platform_data,
	},
};

#define CY_MAXX 1080
#define CY_MAXY 2016
#define CY_MINX 0
#define CY_MINY 0

#define CY_ABS_MIN_X CY_MINX
#define CY_ABS_MIN_Y CY_MINY
#define CY_ABS_MAX_X CY_MAXX
#define CY_ABS_MAX_Y CY_MAXY
#define CY_ABS_MIN_P 0
#define CY_ABS_MIN_W 0
#define CY_ABS_MAX_P 255
#define CY_ABS_MAX_W 255

#define CY_ABS_MIN_T 0

#define CY_ABS_MAX_T 15

#define CY_IGNORE_VALUE 0xFFFF

static const uint16_t cyttsp4_abs[] = {
	ABS_MT_POSITION_X, CY_ABS_MIN_X, CY_ABS_MAX_X, 0, 0,
	ABS_MT_POSITION_Y, CY_ABS_MIN_Y, CY_ABS_MAX_Y, 0, 0,
	ABS_MT_PRESSURE, CY_ABS_MIN_P, CY_ABS_MAX_P, 0, 0,
	CY_IGNORE_VALUE, CY_ABS_MIN_W, CY_ABS_MAX_W, 0, 0,
	ABS_MT_TRACKING_ID, CY_ABS_MIN_T, CY_ABS_MAX_T, 0, 0,
};

struct touch_framework cyttsp4_framework = {
	.abs = (uint16_t *)&cyttsp4_abs[0],
	.size = ARRAY_SIZE(cyttsp4_abs),
	.enable_vkeys = 0,
};

static ssize_t a11_virtual_keys_show(struct kobject *kobj,
		struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf,
			__stringify(EV_KEY) ":" __stringify(KEY_BACK)  ":120:1976:150:80"
			":" __stringify(EV_KEY) ":" __stringify(KEY_HOME)   ":500:1976:150:80"
			":" __stringify(EV_KEY) ":" __stringify(KEY_MENU)   ":900:1976:150:80"
			"\n");
}

static struct kobj_attribute a11_virtual_keys_attr = {
	.attr = {
		.name = "virtualkeys.cyttsp4_mt",
		.mode = S_IRUGO,
	},
	.show = &a11_virtual_keys_show,
};

static struct attribute *a11_properties_attrs[] = {
	&a11_virtual_keys_attr.attr,
	NULL
};

static struct attribute_group a11_properties_attr_group = {
	.attrs = a11_properties_attrs,
};

static struct cyttsp4_mt_platform_data _cyttsp4_mt_platform_data = {
	.frmwrk = &cyttsp4_framework,
	.flags = 0,
	.inp_dev_name = CYTTSP4_MT_NAME,
};

struct cyttsp4_device cyttsp4_mt_device = {
	.name = CYTTSP4_MT_NAME,
	.core_id = "main_ttsp_core",
	.dev = {
		.platform_data = &_cyttsp4_mt_platform_data,
	}
};

static struct cyttsp4_btn_platform_data _cyttsp4_btn_platform_data = {
	.inp_dev_name = CYTTSP4_BTN_NAME,
};

struct cyttsp4_device cyttsp4_btn_device = {
	.name = CYTTSP4_BTN_NAME,
	.core_id = "main_ttsp_core",
	.dev = {
			.platform_data = &_cyttsp4_btn_platform_data,
	}
};

static struct i2c_board_info a11_ts_board_info[] __initdata= {
	{
		I2C_BOARD_INFO(CYTTSP4_I2C_NAME, CYTTSP4_I2C_TCH_ADR),
		.platform_data = CYTTSP4_I2C_NAME,
		.irq = MSM_GPIO_TO_INT(A11_GPIO_CYP_TS_INT_N),
	},
};

void __init a11_ts_init(void)
{
	int ret = 0;
	struct kobject *properties_kobj;

	i2c_register_board_info(MSM_8960_GSBI3_QUP_I2C_BUS_ID, a11_ts_board_info,
			ARRAY_SIZE(a11_ts_board_info));

	cyttsp4_register_core_device(&cyttsp4_core_device);
	cyttsp4_register_device(&cyttsp4_mt_device);
	cyttsp4_register_device(&cyttsp4_btn_device);

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj,
				&a11_properties_attr_group);

	if (!properties_kobj || ret)
		pr_err("failed to create board_properties\n");
}

#endif /* CONFIG_CYPRESS_TTSP */
