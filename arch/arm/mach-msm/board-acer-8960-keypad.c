/*
 *
 * Copyright (C) 2011 Acer Corporation.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/keyreset.h>
#include <linux/platform_device.h>
#include <mach/vreg.h>

#include <asm/mach-types.h>

#define ACER_GPIO_KEY_VOL_UP      89
#define ACER_GPIO_KEY_VOL_DOWN    90

#define GPIO_KEY(_id, _gpio, _iswake)		\
	{					\
		.code = _id,			\
		.gpio = _gpio,			\
		.active_low = 1,		\
		.desc = #_id,			\
		.type = EV_KEY,			\
		.wakeup = _iswake,		\
		.debounce_interval = 10,	\
	}

static struct gpio_keys_button acer_keys[] = {
	[0] = GPIO_KEY(KEY_VOLUMEDOWN, ACER_GPIO_KEY_VOL_DOWN, 0),
	[1] = GPIO_KEY(KEY_VOLUMEUP, ACER_GPIO_KEY_VOL_UP, 0),
};

static struct gpio_keys_platform_data acer_keys_platform_data = {
	.buttons        = acer_keys,
	.nbuttons       = ARRAY_SIZE(acer_keys),
};

static struct platform_device acer_keys_device = {
	.name   = "gpio-keys",
	.id     = 0,
	.dev    = {
		.platform_data  = &acer_keys_platform_data,
	},
};

static struct platform_device acer_input_device = {
	.name   = "acer-input",
	.id     = 0,
	.dev    = {
	.platform_data  = NULL,
	},
};

int __init acer_input_init(void)
{
	int ret;

	ret = platform_device_register(&acer_input_device);

	if (ret) {
		pr_err("%s: register platform device fail (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

int __init acer_keys_init(void)
{
	int ret;

	ret = platform_device_register(&acer_keys_device);

	if (ret) {
		pr_err("%s: register platform device fail (%d)\n", __func__, ret);
		return ret;
	}

	return 0;
}

device_initcall(acer_keys_init);
