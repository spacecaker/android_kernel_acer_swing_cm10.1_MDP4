/* Copyright (c) 2012, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __ARCH_ARM_MACH_MSM_BOARD_ACER_8960_H
#define __ARCH_ARM_MACH_MSM_BOARD_ACER_8960_H

#include <linux/regulator/msm-gpio-regulator.h>
#include <linux/mfd/pm8xxx/pm8921.h>
#include <linux/i2c.h>
#include <mach/irqs.h>
#include <mach/rpm-regulator.h>
#include <mach/msm_memtypes.h>
#include <mach/msm_rtb.h>
#include <mach/msm_cache_dump.h>

/* Macros assume PMIC GPIOs and MPPs start at 1 */
#define PM8921_GPIO_BASE		NR_GPIO_IRQS
#define PM8921_GPIO_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_GPIO_BASE)
#define PM8921_MPP_BASE			(PM8921_GPIO_BASE + PM8921_NR_GPIOS)
#define PM8921_MPP_PM_TO_SYS(pm_gpio)	(pm_gpio - 1 + PM8921_MPP_BASE)
#define PM8921_IRQ_BASE			(NR_MSM_IRQS + NR_GPIO_IRQS)

extern struct pm8xxx_regulator_platform_data
	msm_pm8921_regulator_pdata[] __devinitdata;

extern int msm_pm8921_regulator_pdata_len __devinitdata;

#define GPIO_VREG_ID_EXT_5V		0
#define GPIO_VREG_ID_EXT_L2		1
#define GPIO_VREG_ID_EXT_3P3V		2

extern struct gpio_regulator_platform_data
	msm_gpio_regulator_pdata[] __devinitdata;

extern struct regulator_init_data msm_saw_regulator_pdata_s5;
extern struct regulator_init_data msm_saw_regulator_pdata_s6;

extern struct rpm_regulator_platform_data msm_rpm_regulator_pdata __devinitdata;

#endif

extern struct msm_camera_board_info msm8960_camera_board_info;
extern int acer_board_id;
extern int acer_sku_id;

void msm8960_init_cam(void);
void msm8960_init_fb(void);
void msm8960_init_pmic(void);
void msm8960_init_mmc(void);
int msm8960_init_gpiomux(void);
unsigned char msm8960_hdmi_as_primary_selected(void);
void msm8960_allocate_fb_region(void);
void msm8960_set_display_params(char *prim_panel, char *ext_panel);
void msm8960_pm8921_gpio_mpp_init(void);
void msm8960_mdp_writeback(struct memtype_reserve *reserve_table);
#define MSM_8960_GSBI4_QUP_I2C_BUS_ID 4
#define MSM_8960_GSBI3_QUP_I2C_BUS_ID 3
#define MSM_8960_GSBI10_QUP_I2C_BUS_ID 10

extern struct msm_rtb_platform_data msm8960_rtb_pdata;
extern struct msm_cache_dump_platform_data msm8960_cache_dump_pdata;

#define MSM_GSBI4_PHYS         0x16300000
#define GSBI_DUAL_MODE_CODE    0x60
#define GPIO_MSM_UART_TXD      18
#define GPIO_MSM_UART_RXD      19


enum {
	HW_ID_UNKNOWN           = 0x00,
	HW_ID_EVB               = 0x01,
	HW_ID_EVT1              = 0x02,
	HW_ID_EVT2              = 0x03,
	HW_ID_DVT1              = 0x04,
	HW_ID_DVT1_2            = 0x05,
	HW_ID_DVT1_3            = 0x06,
	HW_ID_DVT2              = 0x07,
	HW_ID_DVT2_2            = 0x08,
	HW_ID_PVT1              = 0x09,
	HW_ID_MAX               = 0xFF,
};

enum {
	SKU_ID_UNKNOWN          = 0x00,
	SKU_ID_LTE_WO_TI_GAUGE  = 0x01,
	SKU_ID_3G_WO_TI_GAUGE   = 0x02,
	SKU_ID_LTE_WI_TI_GAUGE  = 0x03,
	SKU_ID_3G_WI_TI_GAUGE   = 0x04,
	SKU_ID_DC_WO_TI_GAUGE   = 0x05,
	SKU_ID_DC_WI_TI_GAUGE   = 0x06,
	SKU_ID_MAX              = 0xFF,
};

enum {
	NORMAL_BOOT             = 0x00,
	CHARGER_BOOT            = 0x01,
};

