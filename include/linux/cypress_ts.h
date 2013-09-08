#ifndef CYPRESS_I2C_H
#define CYPRESS_I2C_H

#include <linux/types.h>

#include <linux/input.h>
#include <linux/timer.h>
#include <linux/workqueue.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/earlysuspend.h>

#include <asm/mach-types.h>

#define CYPRESS_TS_DRIVER_NAME "cypress-ts"

#include "../../../arch/arm/mach-msm/board-acer-8960.h"

struct cypress_i2c_platform_data {
	int abs_x_min;
	int abs_x_max;
	int abs_y_min;
	int abs_y_max;
	int abs_pressure_min;
	int abs_pressure_max;
	int abs_id_min;
	int abs_id_max;
	int y_max;	/* max Y coordinate, include virtual keys if needed */
	int points_max;
	int irqflags;
	int (*hw_init)(struct device *dev, bool on);
	int (*power)(struct device *dev, int ch);
	int (*gpio_cfg)(bool on);
#ifdef CONFIG_TOUCHSCREEN_CYPRESS_ISSP
	bool (*enable_fw_update)(void);
#endif
};

enum {
	TS_VDD_POWER_OFF,
	TS_VDD_POWER_ON,
	TS_RESET,
};

enum {
	SUSPEND_STATE,
	INIT_STATE,
	RESUME_STATE,
};

enum {
	AUTO = 0,
	USER,
	USER_EX,
};

enum {
	TOUCH_SENSITIVITY_SYMBOL_HIGH = 0,
	TOUCH_SENSITIVITY_SYMBOL_MEDIUM,
	TOUCH_SENSITIVITY_SYMBOL_LOW,
	TOUCH_SENSITIVITY_SYMBOL_COUNT,
};

extern signed char download_firmware_main(char *);

#endif

