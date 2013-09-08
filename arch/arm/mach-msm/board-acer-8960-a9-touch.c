#include <linux/input.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/cypress_ts.h>
#include <mach/board.h>
#include "board-acer-8960-a9-touch.h"

#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3

extern int acer_board_id;

static struct regulator **ts_vdd;

static struct cypress_ts_regulator a9_ts_regulator_data[] = {
	{
		.name = "tp_vc",
		.min_uV = VERG_L9_VTG_MIN_UV,
		.max_uV = VERG_L9_VTG_MAX_UV,
		.load_uA = VERG_L9_CURR_24HZ_UA,
	},
	/*  TODO: Remove after runtime PM is enabled in I2C drive */
	{
		.name = "tp_vdd",
		.min_uV = VERG_L23_VTG_MIN_UV,
		.max_uV = VERG_L23_VTG_MAX_UV,
		.load_uA = VERG_L23_CURR_UA,
	},
	{
		.name = "lvs4_i2c",
		.min_uV = VERG_L23_VTG_MIN_UV,
		.max_uV = VERG_L23_VTG_MAX_UV,
		.load_uA = VERG_L23_CURR_UA,
	},
};

static int a9_ts_gpio_cfg(bool on)
{
	int retval = 0;

	if (on == false)
		goto ts_reg_disable;

	if (gpio_is_valid(A9_GPIO_CYP_TS_INT_N)) {
		/* configure touchscreen reset out gpio */
		retval = gpio_request(A9_GPIO_CYP_TS_INT_N,
						"CTP_INT");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A9_GPIO_CYP_TS_INT_N);
			goto error_irq_gpio_req;
		}

		retval = gpio_direction_input(A9_GPIO_CYP_TS_INT_N);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A9_GPIO_CYP_TS_INT_N);
			goto error_irq_gpio_dir;
		}
	}

	if (gpio_is_valid(A9_GPIO_CYP_TS_RESET_N)) {
		/* configure touchscreen reset out gpio */
		retval = gpio_request(A9_GPIO_CYP_TS_RESET_N,
						"ISSP_TP_RESET");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A9_GPIO_CYP_TS_RESET_N);
			goto error_reset_gpio_req;
		}

		retval = gpio_direction_output(
					A9_GPIO_CYP_TS_RESET_N, 0);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A9_GPIO_CYP_TS_RESET_N);
			goto error_reset_gpio_dir;
		}
	}

	if (gpio_is_valid(A9_GPIO_CYP_TP_ISSP_SCLK)) {
		/* configure touchscreen reset out gpio */
		retval = gpio_request(A9_GPIO_CYP_TP_ISSP_SCLK,
						"ISSP_TP_SCLK");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A9_GPIO_CYP_TP_ISSP_SCLK);
			goto error_issp_sclk_gpio_req;
		}

		retval = gpio_direction_output(
					A9_GPIO_CYP_TP_ISSP_SCLK, 1);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A9_GPIO_CYP_TP_ISSP_SCLK);
			goto error_issp_sclk_gpio_dir;
		}
	}

	if (gpio_is_valid(A9_GPIO_CYP_TP_ISSP_SDATA)) {
		/* configure touchscreen reset out gpio */
		retval = gpio_request(A9_GPIO_CYP_TP_ISSP_SDATA,
						"ISSP_TP_SDATA");
		if (retval) {
			pr_err("%s: unable to request reset gpio %d\n",
				__func__, A9_GPIO_CYP_TP_ISSP_SDATA);
			goto error_issp_sdata_gpio_req;
		}

		retval = gpio_direction_output(
					A9_GPIO_CYP_TP_ISSP_SDATA, 1);
		if (retval) {
			pr_err("%s: unable to set direction for gpio %d\n",
				__func__, A9_GPIO_CYP_TP_ISSP_SDATA);
			goto error_issp_sdata_gpio_dir;
		}
	}

	goto success;

ts_reg_disable:
error_issp_sdata_gpio_dir:
	if (gpio_is_valid(A9_GPIO_CYP_TP_ISSP_SDATA))
		gpio_free(A9_GPIO_CYP_TP_ISSP_SDATA);
error_issp_sdata_gpio_req:
	if (gpio_is_valid(A9_GPIO_CYP_TP_ISSP_SDATA))
		gpio_direction_output(A9_GPIO_CYP_TP_ISSP_SDATA, 0);

error_issp_sclk_gpio_dir:
	if (gpio_is_valid(A9_GPIO_CYP_TP_ISSP_SCLK))
		gpio_free(A9_GPIO_CYP_TP_ISSP_SCLK);
error_issp_sclk_gpio_req:
	if (gpio_is_valid(A9_GPIO_CYP_TP_ISSP_SCLK))
		gpio_direction_output(A9_GPIO_CYP_TP_ISSP_SCLK, 0);

error_reset_gpio_dir:
	if (gpio_is_valid(A9_GPIO_CYP_TS_RESET_N))
		gpio_free(A9_GPIO_CYP_TS_INT_N);
error_reset_gpio_req:
	if (gpio_is_valid(A9_GPIO_CYP_TS_RESET_N))
		gpio_direction_output(A9_GPIO_CYP_TS_RESET_N, 0);

error_irq_gpio_dir:
	if (gpio_is_valid(A9_GPIO_CYP_TS_INT_N))
		gpio_free(A9_GPIO_CYP_TS_INT_N);
error_irq_gpio_req:
	if (gpio_is_valid(A9_GPIO_CYP_TS_INT_N))
		gpio_direction_output(A9_GPIO_CYP_TS_INT_N, 1);

success:
	return retval;
}

static int a9_ts_hw_init(struct device *dev, bool on)
{
	int rc = 0, i;
	const struct cypress_ts_regulator *reg_info = NULL;
	u8 num_reg = 0;

	reg_info = a9_ts_regulator_data;
	num_reg = ARRAY_SIZE(a9_ts_regulator_data);

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

static int a9_ts_power(struct device *dev, int ch)
{
	int rc = 0, i;
	u8 num_reg = 0;
	pr_debug("%s: enter\n", __func__);

	num_reg = ARRAY_SIZE(a9_ts_regulator_data);

	if(ts_vdd == NULL) {
		pr_info("ts_vdd does not initialize");
		return 1;
	}

	switch (ch) {
	case TS_VDD_POWER_OFF:
		i = num_reg;

		while (--i >= 0) {
			rc = regulator_disable(ts_vdd[i]);
			if (rc) {
				pr_err("%s: regulator disable failed (%d)\n",
						__func__, rc);
				return rc;
			}
		}

		gpio_set_value(A9_GPIO_CYP_TS_RESET_N, 0);
		break;
	case TS_VDD_POWER_ON:
		i = num_reg;

		while (--i >= 0) {
			rc = regulator_enable(ts_vdd[i]);
			if (rc) {
				pr_err("%s: regulator enable failed (%d)\n",
						__func__, rc);
				return rc;
			}
		}
		break;
	case TS_RESET:
		/* Reset chip */
		gpio_set_value(A9_GPIO_CYP_TS_RESET_N, 1);
		msleep(1);
		gpio_set_value(A9_GPIO_CYP_TS_RESET_N, 0);
		msleep(1);
		gpio_set_value(A9_GPIO_CYP_TS_RESET_N, 1);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
static bool a9_enable_fw_update(void)
{
	return 0;
}
#endif

static struct cypress_i2c_platform_data a9_cypress_ts_data = {
	.abs_x_min = 0,
	.abs_x_max = 719,
	.abs_y_min = 0,
	.abs_y_max = 1279,
	.abs_pressure_min = 0,
	.abs_pressure_max = 255,
	.abs_id_min = 0,
	.abs_id_max = 15,
	.points_max = 4,
	.irqflags = IRQF_TRIGGER_FALLING,
	.hw_init = a9_ts_hw_init,
	.power = a9_ts_power,
	.gpio_cfg = a9_ts_gpio_cfg,
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
	.enable_fw_update = a9_enable_fw_update,
#endif
};

static struct i2c_board_info a9_ts_board_info[] __initdata= {
	{
		I2C_BOARD_INFO("cypress-ts", 0x4d),
		.platform_data = &a9_cypress_ts_data,
		.irq = MSM_GPIO_TO_INT(A9_GPIO_CYP_TS_INT_N),
	},
};

void __init a9_ts_init(void)
{
	i2c_register_board_info(MSM_8960_GSBI3_QUP_I2C_BUS_ID, a9_ts_board_info,
			ARRAY_SIZE(a9_ts_board_info));
}
