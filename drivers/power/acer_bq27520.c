/* Copyright (C) 2008 Rodolfo Giometti <giometti@linux.it>
 * Copyright (C) 2008 Eurotech S.p.A. <info@eurotech.it>
 * Based on a previous work by Copyright (C) 2008 Texas Instruments, Inc.
 *
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
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
//#define POLLER_ENABLE
#define FW_UPDATE_CHECK
//#define IT_AUTO_ENABLE
#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <asm/unaligned.h>
#include <asm/uaccess.h>
#include <linux/time.h>
#include <linux/i2c/acer_bq27520.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/debugfs.h>
#include "../../arch/arm/mach-msm/board-acer-8960.h"

/* Bq27520 standard data commands */
#define BQ27520_REG_CNTL		0x00
#define BQ27520_REG_AR			0x02
#define BQ27520_REG_ARTTE		0x04
#define BQ27520_REG_TEMP		0x06
#define BQ27520_REG_VOLT		0x08
#define BQ27520_REG_FLAGS		0x0A
#define BQ27520_REG_NAC			0x0C
#define BQ27520_REG_FAC			0x0e
#define BQ27520_REG_RM			0x10
#define BQ27520_REG_FCC			0x12
#define BQ27520_REG_AI			0x14
#define BQ27520_REG_TTE			0x16
#define BQ27520_REG_TTF			0x18
#define BQ27520_REG_SI			0x1a
#define BQ27520_REG_STTE		0x1c
#define BQ27520_REG_MLI			0x1e
#define BQ27520_REG_MLTTE		0x20
#define BQ27520_REG_AE			0x22
#define BQ27520_REG_AP			0x24
#define BQ27520_REG_TTECP		0x26
#define BQ27520_REG_SOH			0x28
#define BQ27520_REG_SOC			0x2c
#define BQ27520_REG_NIC			0x2e
#define BQ27520_REG_ICR			0x30
#define BQ27520_REG_LOGIDX		0x32
#define BQ27520_REG_LOGBUF		0x34
/* Flags */
#define BQ27520_FLAG_DSC		BIT(0)
#define BQ27520_FLAG_FC			BIT(9)
#define BQ27520_FLAG_OTC		BIT(15)
#define BQ27520_FLAG_OTD		BIT(14)
#define BQ27520_FLAG_BAT_DET		BIT(3)
/* CONTROL_STATUS */
#define BQ27520_CS_DLOGEN		BIT(15)
#define BQ27520_CS_FAS		    BIT(14)
#define BQ27520_CS_SS		    BIT(13)
#define BQ27520_CS_VOK			BIT(1)
#define BQ27520_CS_QEN		    BIT(0)
/* Control subcommands */
#define BQ27520_SUBCMD_CTNL_STATUS  0x0000
#define BQ27520_SUBCMD_DEVCIE_TYPE  0x0001
#define BQ27520_SUBCMD_FW_VER  0x0002
#define BQ27520_SUBCMD_HW_VER  0x0003
#define BQ27520_SUBCMD_DF_CSUM  0x0004
#define BQ27520_SUBCMD_PREV_MACW   0x0007
#define BQ27520_SUBCMD_CHEM_ID   0x0008
#define BQ27520_SUBCMD_BD_OFFSET   0x0009
#define BQ27520_SUBCMD_INT_OFFSET  0x000a
#define BQ27520_SUBCMD_CC_VER   0x000b
#define BQ27520_SUBCMD_OCV  0x000c
#define BQ27520_SUBCMD_BAT_INS   0x000d
#define BQ27520_SUBCMD_BAT_REM   0x000e
#define BQ27520_SUBCMD_SET_HIB   0x0011
#define BQ27520_SUBCMD_CLR_HIB   0x0012
#define BQ27520_SUBCMD_SET_SLP   0x0013
#define BQ27520_SUBCMD_CLR_SLP   0x0014
#define BQ27520_SUBCMD_FCT_RES   0x0015
#define BQ27520_SUBCMD_ENABLE_DLOG  0x0018
#define BQ27520_SUBCMD_DISABLE_DLOG 0x0019
#define BQ27520_SUBCMD_SEALED   0x0020
#define BQ27520_SUBCMD_ENABLE_IT    0x0021
#define BQ27520_SUBCMD_DISABLE_IT   0x0023
#define BQ27520_SUBCMD_CAL_MODE  0x0040
#define BQ27520_SUBCMD_RESET   0x0041
#define BQ27520_SUBCMD_ROM_MODE   0x0F00
#define BQ27520_SUBCMD_UNSEALED   0x1000
#define BQ27520_SUBCMD_UNSEALED1   0x0414
#define BQ27520_SUBCMD_UNSEALED2  0x3672
#define BQ27520_SUBCMD_FULLACCESS   0x2000
#define BQ27520_SUBCMD_FULLACCESS1   0xFFFF
#define BQ27520_SUBCMD_FULLACCESS2  0xFFFF

#define BATTERY_PRESENT 0x1;
#define BATTERY_OVER_TEMP 0x2;

#define ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN   (-2731)
#define BQ27520_INIT_DELAY ((HZ)*1)
#define BQ27520_POLLING_STATUS ((HZ)*5)
#define BQ27520_COULOMB_POLL ((HZ)*30)
#define BQ27520_FW_CHECK_DELAY ((HZ)*5)

#define I2C_DELAY_US 66

extern int acer_board_id;
extern int acer_boot_mode;

struct bq27520_device_info;
struct bq27520_access_methods {
	int (*read)(u8 reg, int *rt_value, int b_single,
		struct bq27520_device_info *di);
};

struct bq27520_device_info {
	struct device				*dev;
	int					id;
	struct bq27520_access_methods		*bus;
	struct i2c_client			*client;
	const struct bq27520_platform_data	*pdata;
	struct work_struct			counter;
	/* 300ms delay is needed after bq27520 is powered up
	 * and before any successful I2C transaction
	 */
	struct  delayed_work			hw_config;
	uint32_t				soc_irq;
	uint32_t				bat_gd_irq;
	uint32_t				bat_low_irq;
	struct dentry			*dent;
	struct delayed_work		update_battery_status_work;
	struct delayed_work		firmware_check_work;
	int last_flags;
};

enum {
	GET_BATTERY_STATUS,
	GET_BATTERY_TEMPERATURE,
	GET_BATTERY_VOLTAGE,
	GET_BATTERY_CAPACITY,
	GET_BATTERY_CURRENT,
	GET_BATTERY_HEALTH,
	GET_BATTERY_FCC,
	GET_BATTERY_FAC,
	GET_BATTERY_RM,
	NUM_OF_STATUS,
};

enum {
	CMD_WRITE,
	CMD_READ,
	CMD_COMPARE,
	CMD_DELAY,
};

enum {
	SEALED_MODE,
	UNSEALED_MODE,
	FULL_ACCESS_MODE,
};

enum {
	FW_MODE,
	ROM_MODE,
};

struct bq27520_status {
	/* Informations owned and maintained by Bq27520 driver, updated
	 * by poller or SOC_INT interrupt, decoupling from I/Oing
	 * hardware directly
	 */
	int			status[NUM_OF_STATUS];
	spinlock_t		lock;
	struct delayed_work	poller;
};

extern void pm8921_battery_gauge_register(struct pmic_battery_gauge *batt_gauge);
extern void pm8921_battery_gauge_unregister(struct pmic_battery_gauge *batt_gauge);

static struct bq27520_status current_battery_status;
static struct bq27520_device_info *bq27520_di;
static int coulomb_counter;
static spinlock_t lock; /* protect access to coulomb_counter */
static struct timer_list timer; /* charge counter timer every 30 secs */
struct wake_lock		update_wake_lock;
struct wake_lock		bat_low_wake_lock;
struct wake_lock		refresh_wake_lock;
struct mutex			read_mutex_lock;

static void bq27520_cntl_cmd(struct bq27520_device_info *di, int subcmd);
static int bq27520_i2c_txsubcmd(u8 reg, unsigned short subcmd,
		struct bq27520_device_info *di);
static int bq27520_read(u8 reg, int *rt_value, int b_single,
		struct bq27520_device_info *di);

static int bq27520_i2c_write(struct i2c_client *client, char *buf, int count);
static int bq27520_i2c_read(struct i2c_client *client, char *buf, int count);
static int bq27520_cotrol(int subcmd);
static int bq27520_std_data(int stdcmd);
static int bq27520_access_mode(int mode);
static void bq27520_status_refresh(void);
static int check_mode(struct i2c_client *client);
int gauge_firmware_update(bool it_enable);
int gauge_it_enable(bool enable);

static bool is_gauge_suspend = false;
static bool is_gauge_failed = false;
static bool is_firmware_updating = false;

static int bq27520_get_dfi_ver(void)
{
	char buf[2] = {0x00};
	int ret = -1;

	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x39;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x40;
	if (bq27520_i2c_read(bq27520_di->client, buf, 1) < 0) {
		pr_err("bq27520_i2c_read error\n");
		return -1;
	}
	ret = buf[0];

	return ret;
}

static int bq27520_read(u8 reg, int *rt_value, int b_single,
			struct bq27520_device_info *di)
{
	int ret;

	mutex_lock(&read_mutex_lock);
	msleep(2);
	ret = di->bus->read(reg, rt_value, b_single, di);
	if (ret < 0 || *rt_value == 0xff00) {  //Something wrong, do it again
		msleep(2);
		ret = di->bus->read(reg, rt_value, b_single, di);
	}
	mutex_unlock(&read_mutex_lock);

	// Gauge crash workaround
	if (ret==-ENOTCONN && reg!=BQ27520_REG_CNTL) {
		if (check_mode(di->client) == ROM_MODE) {
			char cmd1[2] = {0x00, 0x0F};
			char cmd2[3] = {0x64, 0x0F, 0x00};
			//Exit ROM mode
			pr_info("%s: BQ27520 still in ROM mode\n", __func__);
			di->client->addr = 0x16 >> 1;
			ret = bq27520_i2c_write(di->client, cmd1, sizeof(cmd1));
			if (ret < 0) {
				pr_err("bq27520_i2c_write(0x%02x) error, ret=%d\n", cmd1[0], ret);
				return -1;
			}
			ret = bq27520_i2c_write(di->client, cmd2, sizeof(cmd2));
			if (ret < 0) {
				pr_err("bq27520_i2c_write(0x%02x) error, ret=%d\n", cmd2[0], ret);
				return -1;
			}
			di->client->addr = 0xAA >> 1;
			msleep(200);
		}
		ret = -1;
	}

	return ret;
}

static int bq27520_signed_read(u8 reg, int *rt_value, int b_single,
			struct bq27520_device_info *di)
{
	int ret = 0;

	mutex_lock(&read_mutex_lock);
	msleep(2);
	ret = di->bus->read(reg, rt_value, b_single, di);
	if (ret < 0 || *rt_value == 0xff00) {  //Something wrong, do it again
		msleep(2);
		ret = di->bus->read(reg, rt_value, b_single, di);;
	}
	if (*rt_value >> 15) {
		*rt_value = -((~*rt_value + 1) & 0xFFFF);
	}
	mutex_unlock(&read_mutex_lock);

	return ret;
}

/*
 * Return the battery temperature in tenths of degree Celsius
 * Or < 0 if something fails.
 */
#define MAX_TEMP_10DEGC 900
static int bq27520_battery_temperature(struct bq27520_device_info *di)
{
	int ret, temp = 0;

	ret = bq27520_read(BQ27520_REG_TEMP, &temp, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading temperature\n", ret);
		return MAX_TEMP_10DEGC+1;
	}

	return temp + ZERO_DEGREE_CELSIUS_IN_TENTH_KELVIN;
}

/*
 * Return the battery Voltage in milivolts
 * Or < 0 if something fails.
 */
#define MAX_VOLTAGE_MV 4500
static int bq27520_battery_voltage(struct bq27520_device_info *di)
{
	int ret, volt = 0;

	ret = bq27520_read(BQ27520_REG_VOLT, &volt, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading voltage\n", ret);
		return MAX_VOLTAGE_MV+1;
	}

	return volt;
}

/*
 * Return the battery Relative State-of-Charge
 * Or < 0 if something fails.
 */
static bool is_first_soc_finished = false;
//Battery voltage mV when first reading
static int batt_mv_table[] = {
	3700,  //2%
	3790,  //20%
	3880,  //40%
	3970,  //60%
	4060,  //80%
};
static int bq27520_battery_rsoc(struct bq27520_device_info *di)
{
	int ret, rsoc = 0;

	ret = bq27520_read(BQ27520_REG_SOC, &rsoc, 0, di);
	if (ret < 0) {
		dev_err(di->dev,"error %d reading relative State-of-Charge\n", ret);
		return ret;
	}

	if (rsoc == 0) {
		int temp = 0, volt = 0;
		// Check FCC
		ret = bq27520_read(BQ27520_REG_FCC, &temp, 0, di);
		if (ret < 0) {
			dev_err(di->dev,"error %d reading relative FCC\n", ret);
			return ret;
		} else {
			if (temp == 0) {
				dev_err(di->dev,"error, SoC=0%% and FCC=%dmAh\n", temp);
				return -1;
			}
		}
		/* Bugfree 4057 workaround.
		 * If battery thermal pin touches connector before positive pin,
		 * Gauge will use 3.2V for first reading and report 0%.
		 */
		ret = bq27520_read(BQ27520_REG_VOLT, &volt, 0, di);
		if (ret < 0) {
			dev_err(di->dev,"error %d reading relative Voltage\n", ret);
			return ret;
		} else {
			if (volt >= 3700) {
				int i = 0;
				if (is_first_soc_finished)
					return -1;
				dev_err(di->dev,"SoC abnormal, take a OCV again\n");
				bq27520_cntl_cmd(di, BQ27520_SUBCMD_OCV);
				for (i=4; i>=0; i--) {
					if (volt-batt_mv_table[i] >= 0) {
						rsoc = i*20+10;
						pr_info("Volt=%d, Set SoC to %d%%\n", volt, rsoc);
						break;
					}
				}
			}
		}
	}

	is_first_soc_finished = true;
	return rsoc;
}

/*
 * Return the battery current in mA
 * Or < 0 if something fails.
 */
#define MAX_CURRENT_MA 2500
static int bq27520_battery_current(struct bq27520_device_info *di)
{
	int ret, ampere = 0;

	ret = bq27520_signed_read(BQ27520_REG_AI, &ampere, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading current\n", ret);
		return MAX_CURRENT_MA+1;
	}

	return ampere;
}

static int bq27520_battery_fcc(struct bq27520_device_info *di)
{
	int ret, fcc = 0;

	ret = bq27520_read(BQ27520_REG_FCC, &fcc, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading fcc\n", ret);
		return -1;
	}

	return fcc;
}

static int bq27520_battery_rm(struct bq27520_device_info *di)
{
	int ret, rm = 0;

	ret = bq27520_read(BQ27520_REG_RM, &rm, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading rm\n", ret);
		return -1;
	}

	return rm;
}

static int bq27520_battery_fac(struct bq27520_device_info *di)
{
	int ret, fac = 0;

	ret = bq27520_read(BQ27520_REG_FAC, &fac, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading fac\n", ret);
		return -1;
	}

	return fac;
}

static void bq27520_cntl_cmd(struct bq27520_device_info *di,
				int subcmd)
{
	bq27520_i2c_txsubcmd(BQ27520_REG_CNTL, subcmd, di);
	udelay(I2C_DELAY_US);
}

/*
 * i2c specific code
 */
static int bq27520_i2c_txsubcmd(u8 reg, unsigned short subcmd,
		struct bq27520_device_info *di)
{
	struct i2c_msg msg;
	unsigned char data[3];

	if (!di->client)
		return -ENODEV;

	memset(data, 0, sizeof(data));
	data[0] = reg;
	data[1] = subcmd & 0x00FF;
	data[2] = (subcmd & 0xFF00) >> 8;

	msg.addr = di->client->addr;
	msg.flags = 0;
	msg.len = 3;
	msg.buf = data;

	if (i2c_transfer(di->client->adapter, &msg, 1) < 0)
		return -EIO;

	return 0;
}

static int bq27520_chip_config(struct bq27520_device_info *di)
{
	int flags = 0, ret = 0;

	bq27520_cntl_cmd(di, BQ27520_SUBCMD_CTNL_STATUS);
	ret = bq27520_read(BQ27520_REG_CNTL, &flags, 0, di);
	if (ret < 0) {
		dev_err(di->dev, "error %d reading register %02x\n",
			 ret, BQ27520_REG_CNTL);
		return ret;
	}
#ifdef IT_AUTO_ENABLE
	bq27520_cntl_cmd(di, BQ27520_SUBCMD_ENABLE_IT);
#endif
	if (di->pdata->enable_dlog && !(flags & BQ27520_CS_DLOGEN)) {
		bq27520_cntl_cmd(di, BQ27520_SUBCMD_ENABLE_DLOG);
	}

	return 0;
}

static void bq27520_every_30secs(unsigned long data)
{
	struct bq27520_device_info *di = (struct bq27520_device_info *)data;

	schedule_work(&di->counter);
	mod_timer(&timer, jiffies + BQ27520_COULOMB_POLL);
}

static void bq27520_coulomb_counter_work(struct work_struct *work)
{
	int value = 0, temp = 0, index = 0, ret = 0, count = 0;
	struct bq27520_device_info *di;
	unsigned long flags;

	di = container_of(work, struct bq27520_device_info, counter);

	/* retrieve 30 values from FIFO of coulomb data logging buffer
	 * and average over time
	 */
	do {
		ret = bq27520_read(BQ27520_REG_LOGBUF, &temp, 0, di);
		if (ret < 0)
			break;
		if (temp != 0x7FFF) {
			++count;
			value += temp;
		}
		ret = bq27520_read(BQ27520_REG_LOGIDX, &index, 0, di);
		if (ret < 0)
			break;
	} while (index != 0 || temp != 0x7FFF);

	if (ret < 0) {
		dev_err(di->dev, "Error %d reading datalog register\n", ret);
		return;
	}

	if (count) {
		spin_lock_irqsave(&lock, flags);
		coulomb_counter = value/count;
		spin_unlock_irqrestore(&lock, flags);
	}
}

static int bq27520_status_getter(int function)
{
	int status = 0;
	unsigned long flags;

	spin_lock_irqsave(&current_battery_status.lock, flags);
	status = current_battery_status.status[function];
	spin_unlock_irqrestore(&current_battery_status.lock, flags);

	return status;
}

static int bq27520_is_battery_present(void)
{
	return bq27520_status_getter(GET_BATTERY_HEALTH) & BATTERY_PRESENT;
}

static int bq27520_is_battery_temp_over(void)
{
	return bq27520_status_getter(GET_BATTERY_HEALTH) & BATTERY_OVER_TEMP;
}

static int bq27520_get_battery_mvolts(void)
{
	return bq27520_status_getter(GET_BATTERY_VOLTAGE);
}

static int bq27520_get_battery_temperature(void)
{
	return bq27520_status_getter(GET_BATTERY_TEMPERATURE);
}

static int bq27520_get_battery_status(void)
{
	return bq27520_status_getter(GET_BATTERY_STATUS);
}

static int bq27520_get_remaining_capacity(void)
{
	return bq27520_status_getter(GET_BATTERY_CAPACITY);
}

static int bq27520_get_battery_current(void)
{
	return bq27520_status_getter(GET_BATTERY_CURRENT);
}

static int bq27520_get_battery_current_now(void)
{
	unsigned long flag;
	int ret = bq27520_battery_current(bq27520_di);
	if (ret >= -MAX_CURRENT_MA && ret <= MAX_CURRENT_MA) {
		spin_lock_irqsave(&current_battery_status.lock, flag);
		current_battery_status.status[GET_BATTERY_CURRENT] = ret;
		spin_unlock_irqrestore(&current_battery_status.lock, flag);
	}
	return bq27520_status_getter(GET_BATTERY_CURRENT);
}

static struct pmic_battery_gauge bq27520_batt_gauge = {
	.get_battery_mvolts		= bq27520_get_battery_mvolts,
	.get_battery_temperature	= bq27520_get_battery_temperature,
	.get_battery_status		= bq27520_get_battery_status,
	.get_battery_capacity	= bq27520_get_remaining_capacity,
	.get_battery_current	= bq27520_get_battery_current,
	.get_firmware_version = bq27520_get_dfi_ver,
	.is_battery_present		= bq27520_is_battery_present,
	.is_battery_temp_over	= bq27520_is_battery_temp_over,
	.update_firmware = gauge_firmware_update,
	.it_enable = gauge_it_enable,
	.bq27520_control = bq27520_cotrol,
	.bq27520_std_data = bq27520_std_data,
	.get_battery_current_now	= bq27520_get_battery_current_now,
	.bq27520_status_refresh	= bq27520_status_refresh,
};

static int get_battery_status(struct bq27520_device_info *di,
						int *status, int *health)
{
	int ret = 0, flags = 0, flags_2nd = 0;

	ret = bq27520_read(BQ27520_REG_FLAGS, &flags, 0, di);
	pr_debug("flags = 0x%04x\n", flags);
	if (di->last_flags) {
		if (ret < 0 || (flags & 0xFEFE) != (di->last_flags & 0xFEFE)) {
			pr_err("flags = 0x%04x(1)\n", flags);
			ret = bq27520_read(BQ27520_REG_FLAGS, &flags, 0, di);
			if (ret < 0 || (flags & 0xFEFE) != (di->last_flags & 0xFEFE)) {
				pr_err("flags = 0x%04x(2)\n", flags);
				ret = bq27520_read(BQ27520_REG_FLAGS, &flags_2nd, 0, di);
				if (ret < 0 || (flags & 0xFEFE) != (flags_2nd & 0xFEFE)) {
					dev_err(di->dev, "error %d reading register %02x, flags=0x%04x\n",
							ret, BQ27520_REG_FLAGS, flags_2nd);
					return -1;
				}
			}
		}
	} else {  //First battery status reading
		if (ret < 0) {
			dev_err(di->dev, "error %d reading register %02x, flags=0x%04x\n",
					ret, BQ27520_REG_FLAGS, flags);
			return -1;
		} else {
			pr_info("BQ27520 flags = 0x%04x\n", flags);
		}
	}

	// Get battery charge status
	if (flags & BQ27520_FLAG_FC)
		*status = POWER_SUPPLY_STATUS_FULL;
	else if (flags & BQ27520_FLAG_DSC)
		*status = POWER_SUPPLY_STATUS_DISCHARGING;
	else
		*status = POWER_SUPPLY_STATUS_CHARGING;
	// Get battery present
	if (flags & BQ27520_FLAG_BAT_DET)
		*health = BATTERY_PRESENT;
	// Get battery over temperature
	if (flags & BQ27520_FLAG_OTC || flags & BQ27520_FLAG_OTD)
		*health = *health | BATTERY_OVER_TEMP;

	di->last_flags = flags;

	return ret;
}

#define FCC_FAC_FACTOR 104
#define VBAT_LOW_MV 3300
#define RESERVE_CAP_MAH 50
static int vbat_low_cnt = 0;
static void update_current_battery_status(struct bq27520_device_info *di)
{
	int status[NUM_OF_STATUS];
	int ret = 0, chg_status = 0, health = 0;
	unsigned long flag;

	if (is_firmware_updating)
		return;

	wake_lock(&refresh_wake_lock);
	memcpy(status, current_battery_status.status, sizeof(status));

	ret = bq27520_battery_rsoc(di);
	if (ret >= 0 && ret <= 100)
		status[GET_BATTERY_CAPACITY] = ret;

	ret = bq27520_battery_temperature(di);
	if (ret >= -MAX_TEMP_10DEGC && ret <= MAX_TEMP_10DEGC)
		status[GET_BATTERY_TEMPERATURE] = ret;

	//Get battery charge status
	ret = get_battery_status(di, &chg_status, &health);
	if (ret < 0) {
		pr_err("Get battery status error, using previous data\n");
		chg_status = current_battery_status.status[GET_BATTERY_STATUS];
		health = current_battery_status.status[GET_BATTERY_HEALTH];
	}

	ret = bq27520_battery_voltage(di);
	if (ret > 0 && ret <= MAX_VOLTAGE_MV)
		status[GET_BATTERY_VOLTAGE] = ret;

	ret = bq27520_battery_current(di);
	if (ret >= -MAX_CURRENT_MA && ret <= MAX_CURRENT_MA)
		status[GET_BATTERY_CURRENT] = ret;

	ret = bq27520_battery_fcc(di);
	if (ret >= 0)
		status[GET_BATTERY_FCC] = ret;

	ret = bq27520_battery_fac(di);
	if (ret >= 0)
		status[GET_BATTERY_FAC] = ret;

	ret = bq27520_battery_rm(di);
	if (ret >= 0)
		status[GET_BATTERY_RM] = ret;

	//FCC drop workaround
	if (status[GET_BATTERY_CAPACITY] == 0 &&
		chg_status == POWER_SUPPLY_STATUS_DISCHARGING) {
		int fcc = 0, fac = 0;
		ret = bq27520_read(BQ27520_REG_FCC, &fcc, 0, di);
		if (ret < 0) {
			dev_err(di->dev,"error %d reading relative FCC\n", ret);
			goto save_status;
		}
		ret = bq27520_read(BQ27520_REG_FAC, &fac, 0, di);
		if (ret < 0) {
			dev_err(di->dev,"error %d reading relative FAC\n", ret);
			goto save_status;
		}
		if (fac*100 >= (fcc*FCC_FAC_FACTOR+RESERVE_CAP_MAH*100)) {
			pr_info("FCC=%d FAC=%d\n", fcc, fac);
			status[GET_BATTERY_CAPACITY] = 1;
			if (status[GET_BATTERY_VOLTAGE] <= VBAT_LOW_MV) {
				vbat_low_cnt++;
				pr_info("Vbat=%d Count=%d\n",
						status[GET_BATTERY_VOLTAGE],vbat_low_cnt);
				if (vbat_low_cnt >= 3) {
					status[GET_BATTERY_CAPACITY] = 0;
				}
			} else {
				vbat_low_cnt = 0;
			}
		}
	} else {
		vbat_low_cnt = 0;
	}
save_status:
	spin_lock_irqsave(&current_battery_status.lock, flag);
	current_battery_status.status[GET_BATTERY_STATUS] = chg_status;
	current_battery_status.status[GET_BATTERY_HEALTH] = health;
	current_battery_status.status[GET_BATTERY_VOLTAGE] =
						status[GET_BATTERY_VOLTAGE];
	current_battery_status.status[GET_BATTERY_TEMPERATURE] =
						status[GET_BATTERY_TEMPERATURE];
	current_battery_status.status[GET_BATTERY_CAPACITY] =
						status[GET_BATTERY_CAPACITY];
	current_battery_status.status[GET_BATTERY_CURRENT] =
						status[GET_BATTERY_CURRENT];
	current_battery_status.status[GET_BATTERY_FCC] =
						status[GET_BATTERY_FCC];
	current_battery_status.status[GET_BATTERY_FAC] =
						status[GET_BATTERY_FAC];
	current_battery_status.status[GET_BATTERY_RM] =
						status[GET_BATTERY_RM];
	spin_unlock_irqrestore(&current_battery_status.lock, flag);
	pr_debug("Cap=%d Volt=%d Amp=%d Temp=%d Status=%d Health=%d\n",
			current_battery_status.status[GET_BATTERY_CAPACITY],
			current_battery_status.status[GET_BATTERY_VOLTAGE],
			current_battery_status.status[GET_BATTERY_CURRENT],
			current_battery_status.status[GET_BATTERY_TEMPERATURE],
			current_battery_status.status[GET_BATTERY_STATUS],
			current_battery_status.status[GET_BATTERY_HEALTH]);
	if (current_battery_status.status[GET_BATTERY_CAPACITY] == 0) {
		pr_info("Cap=0\n");
		if (wake_lock_active(&bat_low_wake_lock) == 0)
			wake_lock(&bat_low_wake_lock);
	} else {
		if (wake_lock_active(&bat_low_wake_lock))
			wake_unlock(&bat_low_wake_lock);
	}
	if (current_battery_status.status[GET_BATTERY_TEMPERATURE]>=500 ||
		current_battery_status.status[GET_BATTERY_TEMPERATURE]<=0)
		pr_info("Temp=%d\n", current_battery_status.status[GET_BATTERY_TEMPERATURE]);
	if (current_battery_status.status[GET_BATTERY_VOLTAGE] <= 3000)
		pr_info("Volt=%d\n", current_battery_status.status[GET_BATTERY_VOLTAGE]);
	wake_unlock(&refresh_wake_lock);
}

static void update_battery_status(struct work_struct *work)
{
	struct bq27520_device_info *di;

	if (is_gauge_suspend)
		return;

	di  = container_of(work, struct bq27520_device_info,
						update_battery_status_work.work);

	update_current_battery_status(di);
}

#ifdef POLLER_ENABLE
static void battery_status_poller(struct work_struct *work)
{
	update_current_battery_status(bq27520_di);

	schedule_delayed_work(&current_battery_status.poller,
				BQ27520_POLLING_STATUS);
}
#endif

static void bq27520_status_refresh(void)
{
	update_current_battery_status(bq27520_di);
}

static void bq27520_hw_config(struct work_struct *work)
{
	int ret = 0, flags = 0, type = 0, fw_ver = 0;
	struct bq27520_device_info *di;

	di  = container_of(work, struct bq27520_device_info, hw_config.work);

	pr_info(KERN_INFO "Enter bq27520_hw_config\n");
	ret = bq27520_chip_config(di);
	if (ret) {
		dev_err(di->dev, "Failed to config Bq27520 ret = %d\n", ret);
		is_gauge_failed = true;
		schedule_delayed_work(&di->firmware_check_work, BQ27520_FW_CHECK_DELAY);
		return;
	}

	/* bq27520 is ready for access, update current_battery_status by reading
	 * from hardware
	 */
	update_current_battery_status(di);
	pm8921_battery_gauge_register(&bq27520_batt_gauge);

	enable_irq(di->soc_irq);
	enable_irq(di->bat_gd_irq);
	if (acer_board_id >= HW_ID_DVT2_2) {
		enable_irq(di->bat_low_irq);
		enable_irq_wake(di->bat_low_irq);
	}
#ifdef POLLER_ENABLE
	/* poll battery status every 5 seconds, if charging status changes,
	 * notify msm_charger
	 */
	schedule_delayed_work(&current_battery_status.poller,
				BQ27520_POLLING_STATUS);
#endif
	if (di->pdata->enable_dlog) {
		schedule_work(&di->counter);
		init_timer(&timer);
		timer.function = &bq27520_every_30secs;
		timer.data = (unsigned long)di;
		timer.expires = jiffies + BQ27520_COULOMB_POLL;
		add_timer(&timer);
	}

	bq27520_cntl_cmd(di, BQ27520_SUBCMD_CTNL_STATUS);
	bq27520_read(BQ27520_REG_CNTL, &flags, 0, di);
	bq27520_cntl_cmd(di, BQ27520_SUBCMD_DEVCIE_TYPE);
	bq27520_read(BQ27520_REG_CNTL, &type, 0, di);
	bq27520_cntl_cmd(di, BQ27520_SUBCMD_FW_VER);
	bq27520_read(BQ27520_REG_CNTL, &fw_ver, 0, di);

	dev_info(di->dev, "DEVICE_TYPE is 0x%02X\n", type);
	dev_info(di->dev, "FIRMWARE_VERSION is 0x%02X\n", fw_ver);
	dev_info(di->dev, "Complete bq27520 configuration 0x%02X\n", flags);
	dev_info(di->dev, "Cap=%d Volt=%d Amp=%d Temp=%d Status=%d Health=%d\n",
			current_battery_status.status[GET_BATTERY_CAPACITY],
			current_battery_status.status[GET_BATTERY_VOLTAGE],
			-current_battery_status.status[GET_BATTERY_CURRENT],
			current_battery_status.status[GET_BATTERY_TEMPERATURE],
			current_battery_status.status[GET_BATTERY_STATUS],
			current_battery_status.status[GET_BATTERY_HEALTH]);

	//Check DFI firmware
	schedule_delayed_work(&di->firmware_check_work, BQ27520_FW_CHECK_DELAY);
}

static int bq27520_read_i2c(u8 reg, int *rt_value, int b_single,
			struct bq27520_device_info *di)
{
	struct i2c_client *client = di->client;
	struct i2c_msg msg[1];
	unsigned char data[2];
	int err;

	if (!client->adapter)
		return -ENODEV;

	msg->addr = client->addr;
	msg->flags = 0;
	msg->len = 1;
	msg->buf = data;

	data[0] = reg;
	err = i2c_transfer(client->adapter, msg, 1);

	if (err >= 0) {
		if (!b_single)
			msg->len = 2;
		else
			msg->len = 1;

		msg->flags = I2C_M_RD;
		err = i2c_transfer(client->adapter, msg, 1);
		if (err >= 0) {
			if (!b_single)
				*rt_value = get_unaligned_le16(data);
			else
				*rt_value = data[0];

			return 0;
		}
	}
	return err;
}

// Path /sys/kernel/debug/bq27520
static int reg;
static int subcmd;
static int cmd_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t std_cmd_read(struct file *file,
				     char __user *buffer,
				     size_t count,
				     loff_t *offset)
{
	int ret;
	int temp = 0;
	char buf[16];

	if (reg <= BQ27520_REG_ICR && reg > 0x00) {
		ret = bq27520_read(reg, &temp, 0, bq27520_di);
		if (ret < 0)
			ret = snprintf(buf, sizeof(buf), "Read Error!\n");
		else
			ret = snprintf(buf, sizeof(buf), "0x%04x\n", temp);
	} else
		ret = snprintf(buf, sizeof(buf), "Register Error!\n");

	return simple_read_from_buffer(buffer, count, offset, buf, ret);
}

static ssize_t std_cmd_write(struct file *file,
				      const char __user *buf,
				      size_t count,
				      loff_t *ppos)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int cmd;

	sscanf(buf, "%x", &cmd);
	reg = cmd;
	pr_info("recv'd cmd is 0x%02X\n", reg);
	return ret;
}

const struct file_operations std_cmd_fops = {
	.open = cmd_open,
	.write = std_cmd_write,
	.read = std_cmd_read,
};

static ssize_t sub_cmd_read(struct file *file,
				     char __user *buffer,
				     size_t count,
				     loff_t *offset)
{
	int ret, temp = 0;
	char buf[16];

	if (subcmd == BQ27520_SUBCMD_DEVCIE_TYPE ||
		 subcmd == BQ27520_SUBCMD_FW_VER ||
		 subcmd == BQ27520_SUBCMD_HW_VER ||
		 subcmd == BQ27520_SUBCMD_CHEM_ID ||
		 subcmd == BQ27520_SUBCMD_CTNL_STATUS) {

		bq27520_cntl_cmd(bq27520_di, subcmd);/* Retrieve Chip status */
		ret = bq27520_read(BQ27520_REG_CNTL, &temp, 0, bq27520_di);
		if (ret < 0)
			ret = snprintf(buf, sizeof(buf), "Read Error!\n");
		else
			ret = snprintf(buf, sizeof(buf), "0x%04x\n", temp);
	} else
		ret = snprintf(buf, sizeof(buf), "Register Error!\n");

	return simple_read_from_buffer(buffer, count, offset, buf, ret);
}

static ssize_t sub_cmd_write(struct file *file,
				      const char __user *buf,
				      size_t count,
				      loff_t *ppos)
{
	ssize_t ret = strnlen(buf, PAGE_SIZE);
	int cmd;

	sscanf(buf, "%x", &cmd);
	if (cmd == BQ27520_SUBCMD_DF_CSUM ||
		cmd == BQ27520_SUBCMD_BD_OFFSET ||
		cmd == BQ27520_SUBCMD_INT_OFFSET ||
		cmd == BQ27520_SUBCMD_CC_VER ||
		cmd == BQ27520_SUBCMD_FCT_RES ||
		cmd == BQ27520_SUBCMD_ENABLE_IT ||
		cmd == BQ27520_SUBCMD_DISABLE_IT ||
		cmd == BQ27520_SUBCMD_CAL_MODE ||
		cmd == BQ27520_SUBCMD_RESET) {
		pr_info("recv'd cmd is 0x%02X\n", cmd);
		bq27520_cntl_cmd(bq27520_di, cmd);
	} else if (cmd == BQ27520_SUBCMD_UNSEALED) {
		bq27520_access_mode(UNSEALED_MODE);
	} else if (cmd == BQ27520_SUBCMD_SEALED) {
		bq27520_access_mode(SEALED_MODE);
	} else if (cmd == BQ27520_SUBCMD_FULLACCESS) {
		bq27520_access_mode(FULL_ACCESS_MODE);
	} else {
		subcmd = cmd;
		pr_info("recv'd cmd is 0x%02X\n", subcmd);
	}
	return ret;
}

const struct file_operations sub_cmd_fops = {
	.open = cmd_open,
	.write = sub_cmd_write,
	.read = sub_cmd_read,
};

static ssize_t std_list_read(struct file *file,
				     char __user *buffer,
				     size_t count,
				     loff_t *offset)
{
	int temp = 0, n = 0;
	static char buf[4096];

	bq27520_read(BQ27520_REG_AR, &temp, 0, bq27520_di);
	n = scnprintf(buf, sizeof(buf), "AtRate = %dmA\n", temp);
	bq27520_read(BQ27520_REG_ARTTE, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "AtRateTimeToEmpty = %dMins\n", temp);
	bq27520_read(BQ27520_REG_TEMP, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "Temperature = %d.%ddegC\n", temp/100, temp%100);
	bq27520_read(BQ27520_REG_VOLT, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "Voltage = %dmV\n", temp);
	bq27520_read(BQ27520_REG_FLAGS, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "Flag = 0x%04x\n", temp);
	bq27520_read(BQ27520_REG_NAC, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "NominalAvailableCapacity = %dmAh\n", temp);
	bq27520_read(BQ27520_REG_FAC, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "FullAvailableCapacity = %dmh\n", temp);
	bq27520_read(BQ27520_REG_RM, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "RemainingCapacity = %dmh\n", temp);
	bq27520_read(BQ27520_REG_FCC, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "FullChargeCapacity = %dmAh\n", temp);
	bq27520_signed_read(BQ27520_REG_AI, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "AverageCurrent = %dmA\n", temp);
	bq27520_read(BQ27520_REG_TTE, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "TimeToEmpty = %dMins\n", temp);
	bq27520_read(BQ27520_REG_TTF, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "TimeToFull = %dMins\n", temp);
	bq27520_signed_read(BQ27520_REG_SI, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "StandbyCurrent = %dmA\n", temp);
	bq27520_read(BQ27520_REG_STTE, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "StandbyTimeToEmpty = %dMins\n", temp);
	bq27520_signed_read(BQ27520_REG_MLI, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "MaxLoadCurrent = %dmA\n", temp);
	bq27520_read(BQ27520_REG_MLTTE, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "MaxLoadTimeToEmpty = %dMins\n", temp);
	bq27520_read(BQ27520_REG_AE, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "AvailableEnergy = %dmWh\n", temp);
	bq27520_signed_read(BQ27520_REG_AP, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "AveragePower = %dmW\n", temp);
	bq27520_read(BQ27520_REG_TTECP, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "TTEatConstantPower = %dMins\n", temp);
	bq27520_read(BQ27520_REG_SOH, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "StateOfHealth = %d%%/num\n", temp);
	bq27520_read(BQ27520_REG_SOC, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "StateOfCharge = %d%%\n", temp);
	bq27520_read(BQ27520_REG_NIC, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "NormalizedImpedanceCal = %dmohm\n", temp);
	bq27520_signed_read(BQ27520_REG_ICR, &temp, 0, bq27520_di);
	n += scnprintf(buf+n, sizeof(buf)-n, "InstaneousCurrentReading = %dmA\n", temp);
	buf[n] = 0;

	return simple_read_from_buffer(buffer, count, offset, buf, n);
}

const struct file_operations std_list_fops = {
	.open = cmd_open,
	.read = std_list_read,
};

static void create_debugfs_entries(struct bq27520_device_info *di)
{
	di->dent = debugfs_create_dir("bq27520", NULL);

	if (IS_ERR(di->dent)) {
		pr_err("bq27520 gauge couldnt create debugfs dir\n");
		return;
	}

	debugfs_create_file("std_cmd", S_IRUGO|S_IWUGO, di->dent, NULL, &std_cmd_fops);
	debugfs_create_file("sub_cmd", S_IRUGO|S_IWUGO, di->dent, NULL, &sub_cmd_fops);
	debugfs_create_file("std_list", S_IRUGO, di->dent, NULL, &std_list_fops);
}

//sysfs /sys/devices/i2c-10/10-0055/mode
static ssize_t mode_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct bq27520_device_info *di = dev_get_drvdata(dev);
	int ret;

	ret = check_mode(di->client);
	if (ret == -1){
		pr_err("%s: Check Mode failed!!\n", __func__);
		ret = -1;
	}

	return sprintf(buf, "%d\n", ret);
}
static DEVICE_ATTR(mode, 0644, mode_show, NULL);

static int create_device_sysfs(struct bq27520_device_info *di)
{
	int err;

	err = device_create_file(di->dev, &dev_attr_mode);
	if (err)
		goto err_mode;

	return 0;
err_mode:
	device_remove_file(di->dev, &dev_attr_mode);
	return err;
}

static irqreturn_t soc_irqhandler(int irq, void *dev_id)
{
#ifndef CONFIG_MACH_ACER_A9
	pr_info("soc_irqhandler!!!\n");
#endif

	if (!is_gauge_suspend)
		schedule_work(&bq27520_di->update_battery_status_work.work);

	return IRQ_HANDLED;
}

static irqreturn_t bat_gd_irqhandler(int irq, void *dev_id)
{
	pr_info("bat_gd_irqhandler!!!\n");

	if (!is_gauge_suspend)
		schedule_work(&bq27520_di->update_battery_status_work.work);

	return IRQ_HANDLED;
}

static irqreturn_t bat_low_irqhandler(int irq, void *dev_id)
{
	pr_info("bat_low_irqhandler!!!\n");

	if (!is_gauge_suspend)
		schedule_work(&bq27520_di->update_battery_status_work.work);

	return IRQ_HANDLED;
}

static int bq27520_irq(bool enable, struct bq27520_device_info *di)
{
	int rc = 0;
	const struct bq27520_platform_data *platdata;

	platdata = di->pdata;
	if (enable) {
		struct pm_gpio pm_gpio_param = {
			.direction = PM_GPIO_DIR_IN,
			.pull = PM_GPIO_PULL_NO,
			.vin_sel = PM_GPIO_VIN_S4,
			.function = PM_GPIO_FUNC_NORMAL,
		};

		// SoC IRQ
		rc = pm8xxx_gpio_config(platdata->soc_int, &pm_gpio_param);
		if (rc) {
			pr_err("%s: pm8921 gpio %d config failed(%d)\n",
					__func__, platdata->soc_int, rc);
			return rc;
		}
		di->soc_irq = gpio_to_irq(platdata->soc_int);
		rc = request_threaded_irq(di->soc_irq, NULL, soc_irqhandler,
				IRQF_TRIGGER_FALLING,
				"BQ27520_SOC_IRQ", di);
		if (rc) {
			dev_err(di->dev, "%s: fail to request irq %d (%d)\n",
				__func__, platdata->soc_int, rc);
			goto soc_irqreq_fail;
		} else {
			disable_irq_nosync(di->soc_irq);
		}

		//Battery Good IRQ
		rc = pm8xxx_gpio_config(platdata->bat_gd, &pm_gpio_param);
		if (rc) {
			pr_err("%s: pm8921 gpio %d config failed(%d)\n",
					__func__, platdata->soc_int, rc);
			goto bat_gd_gpio_fail;
		}
		di->bat_gd_irq = gpio_to_irq(platdata->bat_gd);
		rc = request_threaded_irq(di->bat_gd_irq, NULL, bat_gd_irqhandler,
				IRQF_TRIGGER_FALLING|IRQF_TRIGGER_RISING,
				"BQ27520_BAT_GD_IRQ", di);
		if (rc) {
			dev_err(di->dev, "%s: fail to request irq %d (%d)\n",
				__func__, platdata->bat_gd, rc);
			goto bat_gd_irqreq_fail;
		} else {
			disable_irq_nosync(di->bat_gd_irq);
		}

		//Battery Low IRQ
		rc = pm8xxx_gpio_config(platdata->bat_low, &pm_gpio_param);
		if (rc) {
			pr_err("%s: pm8921 gpio %d config failed(%d)\n",
					__func__, platdata->bat_low, rc);
			goto bat_gd_irqreq_fail;
		}
		di->bat_low_irq = gpio_to_irq(platdata->bat_low);
		rc = request_threaded_irq(di->bat_low_irq, NULL, bat_low_irqhandler,
				IRQF_TRIGGER_RISING, "BQ27520_BAT_LOW_IRQ", di);
		if (rc) {
			dev_err(di->dev, "%s: fail to request irq %d (%d)\n",
				__func__, platdata->bat_low, rc);
			goto bat_low_irqreq_fail;
		} else {
			disable_irq_nosync(di->bat_low_irq);
		}
	} else {
		free_irq(di->soc_irq, di);
		free_irq(di->bat_gd_irq, di);
		free_irq(di->bat_low_irq, di);
		gpio_free(platdata->soc_int);
		gpio_free(platdata->bat_gd);
		gpio_free(platdata->bat_low);
	}
	return rc;

bat_low_irqreq_fail:
	free_irq(di->bat_low_irq, di);
	gpio_free(platdata->bat_low);
bat_gd_irqreq_fail:
	free_irq(di->bat_gd_irq, di);
	gpio_free(platdata->bat_gd);
bat_gd_gpio_fail:
soc_irqreq_fail:
	free_irq(di->soc_irq, di);
	gpio_free(platdata->soc_int);
	return rc;
}

static int pow_fun(int x, int y)
{
	int result = x, i;
	if (y == 0)
		return 1;
	for (i=1; i<y; i++) {
		result *= x;
	}
	return result;
}

static int str_to_int(char *buf, int len)
{
	int ret = 0, i = 0;

	if (buf == NULL)
		return -1;

	for (i=0; i<len; i++) {
		if (buf[len-i-1] >= '0' && buf[len-i-1] <= '9') {
			ret = ret + (buf[len-i-1] - '0') * pow_fun(10, i);
		} else {
			pr_err("String transfer error[%d]\n", i);
		}
	}
	return ret;
}

static char str_to_hex(char *buf)
{
	char hi = 0, lo = 0;

	if (buf == NULL)
		return -1;

	if (buf[0] >= 'A' && buf[0] <= 'F') {
		hi = buf[0] - 'A' + 10;
	} else if (buf[0] >= '0' && buf[0] <= '9') {
		hi = buf[0] - '0';
	} else {
		pr_err("String transfer error[0]\n");
	}

	if (buf[1] >= 'A' && buf[1] <= 'F') {
		lo = buf[1] - 'A' + 10;
	} else if (buf[1] >= '0' && buf[1] <= '9') {
		lo = buf[1] - '0';
	} else {
		pr_err("String transfer error[1]\n");
	}

	return hi*16+lo;
}

static int bq27520_i2c_read(struct i2c_client *client, char *buf, int count)
{
	struct i2c_msg msg;
	int ret;

	if (!client->adapter)
		return -ENODEV;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 1;
	msg.buf = buf;

	ret = i2c_transfer(client->adapter, &msg, 1);
	if (ret >= 0) {
		msg.len = count;
		msg.flags = I2C_M_RD;
		ret = i2c_transfer(client->adapter, &msg, 1);
	}
	return (ret == 1) ? count : ret;
}

static int bq27520_i2c_write(struct i2c_client *client, char *buf, int count)
{
	if (!client->adapter)
		return -ENODEV;

	return i2c_master_send(client, buf, count);
}

static int check_mode(struct i2c_client *client)
{
	int cur_addr = client->addr;
	char buf[2] = {0x00};
	int mode = 0;

	//Check Firmware mode
	client->addr = 0xAA >> 1;
	buf[0] = 0x04;
	if (bq27520_i2c_read(client, buf, 1) < 0) {
		//Check ROM mode
		client->addr = 0x16 >> 1;
		if (bq27520_i2c_read(client, buf, 1) < 0) {
			mode = -1;
		} else {
			pr_info("ROM Mode\n");
			mode = ROM_MODE;
		}
	} else {
		pr_info("Firmware Mode\n");
		mode = FW_MODE;
	}
	client->addr = cur_addr;
	return mode;
}

#ifdef HDQ_ACCESS
static int hdq_check_mode(struct i2c_client *client)
{
	int mode = 0;
	char x = 0x00, y = 0x00, buf[2] = {0x00};

	buf[0] = 0x04;
	if (bq27520_i2c_read(client, buf, 1) < 0) {
		pr_err("%s: bq27520_i2c_read error1\n", __func__);
		return -1;
	}
	x = buf[0];
	x = ~x;
	buf[0] = 0x04;
	buf[1] = x;
	if (bq27520_i2c_write(client, buf, 2) < 0) {
		pr_err("%s: bq27520_i2c_write error2\n", __func__);
		return -1;
	}
	msleep(2);
	buf[0] = 0x04;
	if (bq27520_i2c_read(client, buf, 1) < 0) {
		pr_err("%s: bq27520_i2c_read error3\n", __func__);
		return -1;
	}
	y = buf[0];
	if (x == y) {
		pr_info("ROM Mode\n");
		mode = ROM_MODE;
	} else {
		pr_info("Firmware Mode\n");
		mode = FW_MODE;
	}

	return mode;
}
#endif

static int bq27520_cotrol(int subcmd)
{
	int ret, temp = 0;

	if (subcmd == BQ27520_SUBCMD_DF_CSUM ||
		 subcmd == BQ27520_SUBCMD_BD_OFFSET ||
		 subcmd == BQ27520_SUBCMD_INT_OFFSET ||
		 subcmd == BQ27520_SUBCMD_CC_VER ||
		 subcmd == BQ27520_SUBCMD_FCT_RES ||
		 subcmd == BQ27520_SUBCMD_SEALED ||
		 subcmd == BQ27520_SUBCMD_ENABLE_IT ||
		 subcmd == BQ27520_SUBCMD_DISABLE_IT ||
		 subcmd == BQ27520_SUBCMD_CAL_MODE ||
		 subcmd == BQ27520_SUBCMD_RESET) {
		bq27520_cntl_cmd(bq27520_di, subcmd);
		return 0;
	} else {
		bq27520_cntl_cmd(bq27520_di, subcmd);/* Retrieve Chip status */
		ret = bq27520_read(BQ27520_REG_CNTL, &temp, 0, bq27520_di);
		if (ret < 0) {
			pr_err("Read Error!\n");
			return -EIO;
		} else {
			return temp;
		}
	}
}

static int bq27520_std_data(int stdcmd)
{
	int ret, data = 0;

	if (stdcmd == BQ27520_REG_RM) {
		return current_battery_status.status[GET_BATTERY_RM];
	} else if (stdcmd == BQ27520_REG_FCC) {
		return current_battery_status.status[GET_BATTERY_FCC];
	} else if (stdcmd == BQ27520_REG_FAC) {
		return current_battery_status.status[GET_BATTERY_FAC];
	} else {
		ret = bq27520_read(stdcmd, &data, 0, bq27520_di);
		if (ret < 0) {
			dev_err(bq27520_di->dev, "error %d reading standard data\n", ret);
			return ret;
		}
	}
	return data;
}

static int is_it_enable(void)
{
	int status = 0;

	status = bq27520_cotrol(BQ27520_SUBCMD_CTNL_STATUS);
	if (status < 0) {
		pr_err("get CONTROL_STATUS fail\n");
		return -1;
	}
	// Check VOK and QEN flag
	if (status & BQ27520_CS_QEN) {  //IT Enable
		return 1;
	} else {  //IT Disable
		return 0;
	}
}

static int get_access_mode(void)
{
	int status = 0;

	status = bq27520_cotrol(BQ27520_SUBCMD_CTNL_STATUS);
	if (status < 0) {
		pr_err("get CONTROL_STATUS fail\n");
		return -1;
	}
	// Check FAS and SS flag
	if (status & BQ27520_CS_SS) {
		pr_info("Sealed mode\n");
		return SEALED_MODE;
	} else {
		if (!(status & BQ27520_CS_FAS)) {
			pr_info("Full Access mode\n");
			return FULL_ACCESS_MODE;
		} else {
			pr_info("Unsealed mode\n");
			return UNSEALED_MODE;
		}
	}
}

static int do_dfi_update(struct i2c_client *client, char *fw_buf, int length)
{
	int file_pos = 0, line_pos = 0, data_pos = 0;
	char send_buf[100] = {0x00}, recv_buf[100] = {0x00};
	char addr = 0x00, reg = 0x00;
	int cmd = 0, ret = 0;

	/* update firmware */
	while(file_pos < length) {
		//Get command
		if (line_pos == 0) {  //First char
			if (fw_buf[file_pos] == 'W') {
				cmd = CMD_WRITE;
			} else if (fw_buf[file_pos] == 'R') {
				cmd = CMD_READ;
			} else if (fw_buf[file_pos] == 'C') {
				cmd = CMD_COMPARE;
			} else if (fw_buf[file_pos] == 'X') {
				cmd = CMD_DELAY;
			}
			file_pos = file_pos + 3;  //Jump to the address char
			line_pos = line_pos + 3;  //Jump to the address char
		}
		//Get address, register and setting data
		if (cmd != CMD_DELAY) {
			if (line_pos == 3) {  //Get address
				addr = str_to_hex(&fw_buf[file_pos]);
				pr_debug("Addr=0x%02x", addr);
			} else if (line_pos%3 == 0) {  //Get setting data
				send_buf[data_pos] = str_to_hex(&fw_buf[file_pos]);
				if (data_pos == 0) {
					reg = send_buf[data_pos];
					pr_debug("Reg=0x%02x", reg);
				}
				pr_debug("Data[%d]=0x%02x", data_pos,send_buf[data_pos]);
				data_pos++;
			}
		}
		file_pos++;
		line_pos++;
		if (fw_buf[file_pos] == '\n') {  //End of line
			client->addr = addr>>1;  //Set I2C address
			if (cmd == CMD_DELAY) {
				int wait_buf_len = line_pos - 4;
				int wait = str_to_int(&fw_buf[file_pos-wait_buf_len-1],
										wait_buf_len);
				pr_debug("Delay(%d)", wait);
				msleep(wait);
			} else if (cmd == CMD_WRITE) {
				pr_debug("W: 0x%02x(%d)", reg, data_pos);
				ret = bq27520_i2c_write(client, send_buf, data_pos);
				if (ret != data_pos) {
					pr_err("W: bq27520_i2c_write(0x%02x) error (%d)\n",send_buf[0] , ret);
					return -1;
				}
			} else if (cmd == CMD_READ) {

			} else if (cmd == CMD_COMPARE) {
				pr_debug("C: 0x%02x(%d)", reg, data_pos);
				recv_buf[0] = reg;
				ret = bq27520_i2c_read(client, recv_buf, data_pos-1);
				if (ret != (data_pos-1)) {
					pr_err("C: bq27520_i2c_read error (%d)\n", ret);
					return -1;
				}
				ret = memcmp(&send_buf[1],recv_buf,data_pos-1);
				if (ret != 0) {
					pr_err("C: compare error (%d)\n", ret);
					return -1;
				}
			}
			line_pos = 0;
			data_pos = 0;
			file_pos = file_pos + 1; //Jump to the next line head
		}
	}
	/* check mode */
	if (check_mode(client) == ROM_MODE) {
#ifdef AUTO_EXIT_ROM_MODE
		char cmd1[2] = { 0x00, 0x0F};
		char cmd2[3] = { 0x64, 0x0F, 0x00};
		//Exit ROM mode
		pr_info("%s: BQ27520 still in ROM mode\n", __func__);
		ret = bq27520_i2c_write(client, cmd1, sizeof(cmd1));
		if (ret != data_pos) {
			pr_err("E: i2c_master_send(0x%02x) error (%d)\n", cmd1[0], ret);
			return -1;
		}
		ret = bq27520_i2c_write(client, cmd2, sizeof(cmd2));
		if (ret != data_pos) {
			pr_err("E: i2c_master_send(0x%02x) error (%d)\n", cmd2[0], ret);
			return -1;
		}
		if (check_mode(client) != FW_MODE) {
			pr_err("%s: Exit ROM mode failed!!\n", __func__);
			return -1;
		}
#else
		pr_err("%s: Exit ROM mode failed!!\n", __func__);
		return -1;
#endif
	}

	return 0;
}

#define MAX_DVT2_DFI_VER 16
#define MAX_DVT2_2_DFI_VER 25
static int dfi_firmware_update(bool check)
{
	struct i2c_client *client = bq27520_di->client;
	struct file     *filp = NULL;
	struct inode    *inode = NULL;
	mm_segment_t    oldfs;
	int	length = 0, ret = 0;
	char *fw_buf = NULL;//, *buffer;
	char dfi_path[50] = {0};
	int dfi_file_ver = 0;
	int max_dfi_ver = 0, min_dfi_ver = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);

	//Check DFI version
#ifdef FW_UPDATE_CHECK
	if (acer_board_id <= HW_ID_DVT2) {
		max_dfi_ver = MAX_DVT2_DFI_VER;
		min_dfi_ver = 0;
	} else {  //HW_ID_DVT2_2
		max_dfi_ver = MAX_DVT2_2_DFI_VER;
		min_dfi_ver = MAX_DVT2_DFI_VER + 1;
	}
#else
	max_dfi_ver = MAX_DVT2_2_DFI_VER;
	min_dfi_ver = 0;
#endif
	for (dfi_file_ver=max_dfi_ver; dfi_file_ver>=min_dfi_ver; dfi_file_ver--) {
		snprintf(dfi_path, sizeof(dfi_path),
             "/system/etc/firmware/a9_gauge_%02d.dfi", dfi_file_ver);
		filp = filp_open(dfi_path, O_RDONLY, S_IRUSR);
		if (IS_ERR(filp)) {
			pr_debug("%s: file %s filp_open error, is_err = %x\n",
							__func__, dfi_path, (int)IS_ERR(filp));
		} else {
			pr_info("%s: DFI file version = %d\n", __func__, dfi_file_ver);
			break;
		}
		if (dfi_file_ver == min_dfi_ver) {
			pr_err("%s: Can not find DFI file\n", __func__);
			return -1;
		}
	}

	//Start to update DFI firmware
	wake_lock(&update_wake_lock);
	disable_irq_nosync(bq27520_di->soc_irq);
	disable_irq_nosync(bq27520_di->bat_gd_irq);
	disable_irq_nosync(bq27520_di->bat_low_irq);
	is_firmware_updating = true;
	if (check) {
		if (bq27520_get_dfi_ver() == dfi_file_ver) {
			pr_info("%s: Firmware already updated\n", __func__);
			is_firmware_updating = false;
			enable_irq(bq27520_di->bat_low_irq);
			enable_irq(bq27520_di->bat_gd_irq);
			enable_irq(bq27520_di->soc_irq);
			wake_unlock(&update_wake_lock);
			return 0;
		}
	}

	if (!filp->f_op) {
		pr_info("%s: File Operation Method Error\n", __func__);
		goto File_Check_DONE;
	}

	inode = filp->f_path.dentry->d_inode;
	if (!inode) {
		pr_info("%s: Get inode from filp failed\n", __func__);
		goto File_Check_DONE;
	}

	pr_info("%s: file offset opsition: %xh\n", __func__, (unsigned)filp->f_pos);

	length = i_size_read(inode->i_mapping->host);
	if (length == 0) {
		pr_info("%s: Try to get file size error\n", __func__);
		goto File_Check_DONE;
	}

	pr_info("%s: length=%d\n", __func__, length);
	fw_buf = (char *)kmalloc((length + 1), GFP_KERNEL);
	if (fw_buf == NULL) {
		pr_info("%s: kernel memory alloc error\n", __func__);
		goto File_Check_DONE;
	}
	ret = filp->f_op->read(filp, fw_buf, length, &filp->f_pos);
	if (ret != length) {
		pr_info("%s: file read error, ret=%d\n", __func__, ret);
		kfree(fw_buf);
		goto File_Check_DONE;
	}
	filp_close(filp, NULL);
	set_fs(oldfs);

	/* check mode */
	ret = check_mode(client);
	if (ret == -1){
		pr_err("%s: Check Mode failed!!\n", __func__);
		ret = -1;
		goto Transfer_DONE;
	} else {
		if (ret == FW_MODE) {
			pr_info("%s: Enter BQ27520 ROM Mode\n", __func__);
			bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_ROM_MODE);
			msleep(2);
			if (check_mode(client) != ROM_MODE) {
				pr_err("%s: Enter ROM mode failed!!\n", __func__);
				ret = -1;
				goto Transfer_DONE;
			}
		}
	}

	/* start to flash firmware */
	pr_info("%s: Firmware Updating...\n", __func__);
	client->addr = 0x16 >> 1;  //ROM mode address
	if (do_dfi_update(client, fw_buf, length) < 0) {
		pr_err("%s: Firmware Update failed, update again...\n", __func__);
		if (do_dfi_update(client, fw_buf, length) < 0) {
			pr_err("%s: Firmware Update failed\n", __func__);
			ret = -1;
			goto Transfer_DONE;
		}
	}
	client->addr = 0xAA >> 1;  //Firmware mode address

	/* check dfi version */
	ret = bq27520_get_dfi_ver();
	if (ret != dfi_file_ver) {
		pr_err("%s: Firmware version check failed (ver:%d)\n", __func__, ret);
		ret = -1;
		goto Transfer_DONE;
	} else {
		ret = 0;
	}
	pr_info("%s: Update BQ27520 firmware done\n", __func__);

Transfer_DONE:
	kfree(fw_buf);
	is_firmware_updating = false;
	enable_irq(bq27520_di->bat_low_irq);
	enable_irq(bq27520_di->soc_irq);
	enable_irq(bq27520_di->bat_gd_irq);
	wake_unlock(&update_wake_lock);
	return ret;

File_Check_DONE:
	filp_close(filp, NULL);
	set_fs(oldfs);
	is_firmware_updating = false;
	enable_irq(bq27520_di->bat_low_irq);
	enable_irq(bq27520_di->soc_irq);
	enable_irq(bq27520_di->bat_gd_irq);
	wake_unlock(&update_wake_lock);
	return -1;
}

static int bq27520_access_mode(int mode)
{
	int ret = 0;

	switch (mode) {
		case SEALED_MODE:
			bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_SEALED);
			break;
		case UNSEALED_MODE:
			bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_UNSEALED1);
			bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_UNSEALED2);
			break;
		case FULL_ACCESS_MODE:
			bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_FULLACCESS1);
			bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_FULLACCESS2);
			break;
		default:
			break;
	}
	ret = bq27520_cotrol(BQ27520_SUBCMD_CTNL_STATUS);
	pr_info("CS=0x%04x\n", ret);
	return ret;
}

// Device Type : 0x0520
int gauge_device_type(void)
{
	int type = 0;

	bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_DEVCIE_TYPE);
	bq27520_read(BQ27520_REG_CNTL, &type, 0, bq27520_di);

	return type;
}
EXPORT_SYMBOL(gauge_device_type);

int gauge_it_enable(bool enable)
{
	if (enable) {
		bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_ENABLE_IT);
	} else {
		bq27520_cntl_cmd(bq27520_di, BQ27520_SUBCMD_DISABLE_IT);
	}
	msleep(1500);
	//Check IT status
	if (is_it_enable() == enable) {
		pr_info("IT %s successful\n", enable ? "Enable" : "Disable");
		return 1;
	} else {
		pr_info("IT %s failed\n", enable ? "Enable" : "Disable");
		return 0;
	}
}
EXPORT_SYMBOL(gauge_it_enable);

#define UPDATE_RETRY_TIMES 3
int gauge_firmware_update(bool it_enable)
{
	int ret = 0, i =0;

	if (get_access_mode() != FULL_ACCESS_MODE) {
		pr_err("Gauge is not in Full Access Mode\n");
		return -1;
	}

	for (i=0; i<UPDATE_RETRY_TIMES; i++) {
		ret = dfi_firmware_update(false);
		if (ret < 0) {
			pr_err("[%d]Firmware update fail, ret=%d\n",i , ret);
			msleep(500);
		} else {
			gauge_it_enable(it_enable);
			ret = bq27520_get_dfi_ver();
			break;
		}
	}

	return ret;
}
EXPORT_SYMBOL(gauge_firmware_update);

static int bq27520_battery_1p(void)
{
	char buf[17] = {0x00};
	int ret = 1;

	pr_info("1P Battery\n");
	/* Read and compare 1P Battery CC Gain and CC Delta of Gauge.
	 * If the same, no need to update again.
	 */
	msleep(2);
	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error6\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x68;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error7\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error8\n");
		return -1;
	}
	msleep(2);
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x40;
	if (bq27520_i2c_read(bq27520_di->client, buf, 16) < 0) {
		pr_err("bq27520_i2c_read error\n");
		return -1;
	}
	if (buf[0]==0x7F && buf[4]==0x93) {
		pr_err("bq27520 is already updated\n");
		return ret;
	}
	/* Write 1P Battery CC Gain and CC Delta into Gauge */
	msleep(2);
	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error1\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x68;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error2\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error3\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x40;
	buf[1] = 0x7F;
	buf[2] = 0x3B;
	buf[3] = 0x4B;
	buf[4] = 0xC6;
	buf[5] = 0x93;
	buf[6] = 0x55;
	buf[7] = 0x19;
	buf[8] = 0x0C;
	buf[9] = 0xFB;
	buf[10] = 0x0A;
	buf[11] = 0x00;
	buf[12] = 0x4C;
	buf[13] = 0xF6;
	buf[14] = 0x00;
	buf[15] = 0x00;
	buf[16] = 0x02;
	if (bq27520_i2c_write(bq27520_di->client, buf, 17) < 0) {
		pr_err("bq27520_i2c_write error4\n");
		return -1;
	}
	msleep(10);
	buf[0] = 0x60;
	buf[1] = 0xDE;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error5\n");
		return -1;
	}
	/* Read and compare 1P Battery CC Gain and CC Delta of Gauge */
	msleep(200);
	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error6\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x68;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error7\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error8\n");
		return -1;
	}
	msleep(2);
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x40;
	if (bq27520_i2c_read(bq27520_di->client, buf, 16) < 0) {
		pr_err("bq27520_i2c_read error\n");
		return -1;
	}
	if (buf[0]!=0x7F || buf[4]!=0x93) {
		pr_err("bq27520 compare error, buf[0]=%02x, buf[4]=%02x\n", buf[0], buf[4]);
		return -1;
	}
	return ret;
}

static int bq27520_battery_2p(void)
{
	char buf[17] = {0x00};
	int ret = 1;

	pr_info("2P Battery\n");
	/* Read and compare 2P Battery CC Gain and CC Delta of Gauge.
	 * If the same, no need to update again.
	 */
	msleep(2);
	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error6\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x68;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error7\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error8\n");
		return -1;
	}
	msleep(2);
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x40;
	if (bq27520_i2c_read(bq27520_di->client, buf, 16) < 0) {
		pr_err("bq27520_i2c_read error\n");
		return -1;
	}
	if (buf[0]==0x7E && buf[4]==0x92) {
		pr_err("bq27520 is already updated\n");
		return ret;
	}
	/* Write 2P Battery CC Gain and CC Delta into Gauge */
	msleep(2);
	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error1\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x68;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error2\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error3\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x40;
	buf[1] = 0x7E;
	buf[2] = 0x3B;
	buf[3] = 0x4B;
	buf[4] = 0xC6;
	buf[5] = 0x92;
	buf[6] = 0x55;
	buf[7] = 0x19;
	buf[8] = 0x0C;
	buf[9] = 0xF8;
	buf[10] = 0xC9;
	buf[11] = 0x00;
	buf[12] = 0x4C;
	buf[13] = 0xF6;
	buf[14] = 0x00;
	buf[15] = 0x00;
	buf[16] = 0x02;
	if (bq27520_i2c_write(bq27520_di->client, buf, 17) < 0) {
		pr_err("bq27520_i2c_write error4\n");
		return -1;
	}
	msleep(10);
	buf[0] = 0x60;
	buf[1] = 0x24;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error5\n");
		return -1;
	}
	/* Read and compare 2P Battery CC Gain and CC Delta of Gauge */
	msleep(200);
	buf[0] = 0x61;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error6\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3E;
	buf[1] = 0x68;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error7\n");
		return -1;
	}
	msleep(2);
	buf[0] = 0x3F;
	buf[1] = 0x00;
	if (bq27520_i2c_write(bq27520_di->client, buf, 2) < 0) {
		pr_err("bq27520_i2c_write error8\n");
		return -1;
	}
	msleep(2);
	memset(buf, 0, sizeof(buf));
	buf[0] = 0x40;
	if (bq27520_i2c_read(bq27520_di->client, buf, 16) < 0) {
		pr_err("bq27520_i2c_read error\n");
		return -1;
	}
	if (buf[0]!=0x7E || buf[4]!=0x92) {
		pr_err("bq27520 compare error, buf[0]=%02x, buf[4]=%02x\n", buf[0], buf[4]);
		return -1;
	}
	return ret;
}

static int64_t read_battery_id(void)
{
	int rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(CHANNEL_BATT_ID, &result);
	if (rc) {
		pr_err("error reading batt id channel, rc = %d\n", rc);
		return rc;
	}
	pr_debug("batt_id phy = %lld meas = 0x%llx\n", result.physical,
						result.measurement);
	return result.physical;
}

//10K
#define BAT_ID_1P_HIGH  340000  //0.34V
#define BAT_ID_1P_LOW   240000  //0.24V
//51K
#define BAT_ID_2P_HIGH  940000  //0.94V
#define BAT_ID_2P_LOW   840000  //0.84V
static void bq27520_battery_id_check(void)
{
	int64_t batt_id_uv = read_battery_id();

	pr_info("Battery ID = %llduV\n", batt_id_uv);
	if (batt_id_uv>BAT_ID_1P_LOW && batt_id_uv<BAT_ID_1P_HIGH) {
		int i;
		for (i=0;i<=5;i++) {
			if (bq27520_battery_1p() < 0)
				pr_err("BQ27520 set 1P battery error\n");
			else
				break;
			msleep(2);
		}
	} else if (batt_id_uv>BAT_ID_2P_LOW && batt_id_uv<BAT_ID_2P_HIGH) {
		int i;
		for (i=0;i<=5;i++) {
			if (bq27520_battery_2p() < 0)
				pr_err("BQ27520 set 2P battery error\n");
			else
				break;
			msleep(2);
		}
	} else {
		pr_err("Battery ID abnormal\n");
	}
}

static void bq27520_firmware_check(struct work_struct *work)
{
	int ret = 0, i =0;
	struct bq27520_device_info *di;

	pr_info("[ BQ27520 Firmware Check ]\n");

	if (acer_boot_mode == 0x01) {  //CHARGER_BOOT
		bq27520_battery_id_check();
		return;
	}

	di  = container_of(work, struct bq27520_device_info, firmware_check_work.work);

	if (is_gauge_failed) {
		pr_info("Gauge Failed!!!!!\n");
		ret = dfi_firmware_update(false);
		if (ret < 0) {
			pr_err("Firmware update fail, ret=%d\n", ret);
		} else {
			is_gauge_failed = false;
			gauge_it_enable(true);
			schedule_work(&di->hw_config.work);
		}
	} else {
		if (get_access_mode() != FULL_ACCESS_MODE) {
			pr_err("Gauge is not in Full Access Mode\n");
			return;
		}
		for (i=0; i<UPDATE_RETRY_TIMES; i++) {
			ret = dfi_firmware_update(true);
			if (ret < 0) {
				pr_err("[%d]Firmware update fail, ret=%d\n",i , ret);
				msleep(500);
			} else {
				bq27520_battery_id_check();
				if (!is_it_enable())
					gauge_it_enable(true);
				return;
			}
		}
	}
}

static int bq27520_battery_probe(struct i2c_client *client,
				 const struct i2c_device_id *id)
{
	struct bq27520_device_info *di;
	struct bq27520_access_methods *bus;
	const struct bq27520_platform_data  *pdata;
	int retval = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C))
		return -ENODEV;

	pdata = client->dev.platform_data;

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		dev_err(&client->dev, "failed to allocate device info data\n");
		retval = -ENOMEM;
		goto batt_failed_1;
	}

	di->pdata = pdata;

	bus = kzalloc(sizeof(*bus), GFP_KERNEL);
	if (!bus) {
		dev_err(&client->dev, "failed to allocate data\n");
		retval = -ENOMEM;
		goto batt_failed_2;
	}

	i2c_set_clientdata(client, di);
	di->dev = &client->dev;
	bus->read = &bq27520_read_i2c;
	di->bus = bus;
	di->client = client;

	// Create debug sysfs "bq27520"
	create_debugfs_entries(di);
	// Create /sys/devices/i2c-10/10-0055 sysfs
	create_device_sysfs(di);

	retval = bq27520_irq(true, di);
	if (retval) {
		dev_err(&client->dev, "failed to powerup ret = %d\n", retval);
		goto batt_failed_3;
	}

	spin_lock_init(&lock);
	mutex_init(&read_mutex_lock);
	wake_lock_init(&update_wake_lock, WAKE_LOCK_SUSPEND, "bq27520_update");
	wake_lock_init(&bat_low_wake_lock, WAKE_LOCK_SUSPEND, "bq27520_bat_low");
	wake_lock_init(&refresh_wake_lock, WAKE_LOCK_SUSPEND, "bq27520_refresh");

	bq27520_di = di;
	if (pdata->enable_dlog)
		INIT_WORK(&di->counter, bq27520_coulomb_counter_work);

	INIT_DELAYED_WORK(&di->update_battery_status_work,
			update_battery_status);
	INIT_DELAYED_WORK(&di->firmware_check_work, bq27520_firmware_check);
#ifdef POLLER_ENABLE
	INIT_DELAYED_WORK(&current_battery_status.poller,
			battery_status_poller);
#endif
	INIT_DELAYED_WORK(&di->hw_config, bq27520_hw_config);
	schedule_delayed_work(&di->hw_config, BQ27520_INIT_DELAY);

	pr_info("BQ27520 Probe Done\n");
	return 0;

batt_failed_3:
	kfree(bus);
batt_failed_2:
	kfree(di);
batt_failed_1:
	return retval;
}

static int bq27520_battery_remove(struct i2c_client *client)
{
	struct bq27520_device_info *di = i2c_get_clientdata(client);

	if (di->pdata->enable_dlog) {
		del_timer_sync(&timer);
		cancel_work_sync(&di->counter);
		bq27520_cntl_cmd(di, BQ27520_SUBCMD_DISABLE_DLOG);
	}
#ifdef IT_AUTO_ENABLE
	bq27520_cntl_cmd(di, BQ27520_SUBCMD_DISABLE_IT);
#endif
	cancel_delayed_work_sync(&di->hw_config);
#ifdef POLLER_ENABLE
	cancel_delayed_work_sync(&current_battery_status.poller);
#endif

	bq27520_irq(false, di);

	kfree(di->bus);
	kfree(di);
	return 0;
}

#ifdef CONFIG_PM
static int bq27520_suspend(struct device *dev)
{
	struct bq27520_device_info *di = dev_get_drvdata(dev);

	is_gauge_suspend = true;
	disable_irq_nosync(di->soc_irq);
	disable_irq_nosync(di->bat_gd_irq);
	if (di->pdata->enable_dlog) {
		del_timer_sync(&timer);
		cancel_work_sync(&di->counter);
	}
#ifdef POLLER_ENABLE
	cancel_delayed_work_sync(&current_battery_status.poller);
#else
	if (delayed_work_pending(&di->update_battery_status_work))
		cancel_delayed_work_sync(&di->update_battery_status_work);
#endif
	//Set SET_SLEEP+
	bq27520_cntl_cmd(di, BQ27520_SUBCMD_SET_SLP);

	return 0;
}

static int bq27520_resume(struct device *dev)
{
	struct bq27520_device_info *di = dev_get_drvdata(dev);

	enable_irq(di->soc_irq);
	enable_irq(di->bat_gd_irq);
	if (di->pdata->enable_dlog)
		add_timer(&timer);
#ifdef POLLER_ENABLE
	schedule_delayed_work(&current_battery_status.poller, HZ*2);
#else
	schedule_delayed_work(&di->update_battery_status_work, HZ*2);
#endif
	is_gauge_suspend = false;

	return 0;
}

static const struct dev_pm_ops bq27520_pm_ops = {
	.suspend = bq27520_suspend,
	.resume = bq27520_resume,
};
#endif

static const struct i2c_device_id bq27520_id[] = {
	{ "bq27520", 1 },
	{},
};
MODULE_DEVICE_TABLE(i2c, BQ27520_id);

static struct i2c_driver bq27520_battery_driver = {
	.driver = {
		.name = "bq27520-battery",
		.owner = THIS_MODULE,
#ifdef CONFIG_PM
		.pm = &bq27520_pm_ops,
#endif
	},
	.probe = bq27520_battery_probe,
	.remove = bq27520_battery_remove,
	.id_table = bq27520_id,
};

static void init_battery_status(void)
{
	spin_lock_init(&current_battery_status.lock);
	current_battery_status.status[GET_BATTERY_STATUS] =
			POWER_SUPPLY_STATUS_UNKNOWN;
}

static int __init bq27520_battery_init(void)
{
	int ret;

	/* initialize current_battery_status, and register with msm-charger */
	init_battery_status();

	ret = i2c_add_driver(&bq27520_battery_driver);
	if (ret)
		printk(KERN_ERR "Unable to register driver ret = %d\n", ret);

	return ret;
}
module_init(bq27520_battery_init);

static void __exit bq27520_battery_exit(void)
{
	i2c_del_driver(&bq27520_battery_driver);
	pm8921_battery_gauge_unregister(&bq27520_batt_gauge);
}
module_exit(bq27520_battery_exit);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Acer, Inc.");
MODULE_DESCRIPTION("BQ27520 battery monitor driver");
