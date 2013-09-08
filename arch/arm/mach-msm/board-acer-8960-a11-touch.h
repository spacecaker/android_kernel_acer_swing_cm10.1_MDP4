/* arch/arm/mach-msm/board-acer-8960-touch.h
 *
 * Copyright (C) 2011 Acer.
 * Author: muming tsao <muming_tsao@acer.com.tw>
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

#ifndef __ARCH_ARM_MACH_MSM_BOARD_ACER_8960_TOUCH_H
#define __ARCH_ARM_MACH_MSM_BOARD_ACER_8960_TOUCH_H

#include <mach/board.h>

extern void __init a11_ts_init(void);

#define CYTTSP4_I2C_TCH_ADR 0x24
#define A11_GPIO_CYP_TS_INT_N            11
#define A11_GPIO_CYP_TS_RESET_N          50

#define VERG_L29_VTG_MIN_UV        1800000
#define VERG_L29_VTG_MAX_UV        1800000
#define VERG_L29_CURR_24HZ_UA      17500
#define VERG_LVS4_VTG_MIN_UV       1800000
#define VERG_LVS4_VTG_MAX_UV       1800000
#define VERG_LVS4_CURR_UA          9630

struct cypress_ts_regulator {
	const char *name;
	u32	min_uV;
	u32	max_uV;
	u32	load_uA;
};

#endif /* __ARCH_ARM_MACH_MSM_BOARD_ACER_8960_TOUCH_H */
