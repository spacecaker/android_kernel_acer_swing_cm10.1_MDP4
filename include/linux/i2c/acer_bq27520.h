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

#ifndef __LINUX_ACER_BQ27520_H
#define __LINUX_ACER_BQ27520_H
struct bq27520_platform_data {
	const char *name;
	unsigned int soc_int;
	unsigned int bat_gd;
	unsigned int bat_low;
	unsigned int bi_tout;
	unsigned int chip_en; /* CE */
	const char *vreg_name; /* regulater used by bq27520 */
	int vreg_value; /* its value */
	int enable_dlog; /* if enable on-chip coulomb counter data logger */
};

struct pmic_battery_gauge {
	int (*get_battery_mvolts) (void);
	int (*get_battery_temperature) (void);
	int (*get_battery_status)(void);
	int (*get_battery_capacity) (void);
	int (*get_battery_current) (void);
	int (*get_firmware_version) (void);
	int (*is_battery_present) (void);
	int (*is_battery_temp_over) (void);
	int (*update_firmware) (bool it_enable);
	int (*it_enable) (bool enable);
	int (*bq27520_control) (int subcmd);
	int (*bq27520_std_data) (int stdcmd);
	int (*get_battery_current_now) (void);
	void (*bq27520_status_refresh) (void);
};

#endif /* __LINUX_ACER_BQ27520_H */
