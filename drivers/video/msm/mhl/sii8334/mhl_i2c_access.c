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

#include "mhl_i2c_access.h"

static int client_num;

struct i2c_client_data client_data[] = {
	{ DEV_PAGE_TPI, NULL },
	{ DEV_PAGE_TX_L1, NULL },
	{ DEV_PAGE_TX_2, NULL },
	{ DEV_PAGE_TX_3, NULL },
	{ DEV_PAGE_CBUS, NULL },
};

void SiiRegReadBlock(uint16_t reg, uint8_t *data, uint8_t len)
{
	int i = 0;
	if (!data) {
		pr_err("%s:empty data!!!\n", __func__);
		return;
	}

	for (i=0; i<len; i++)
			data[i] = SiiRegRead(reg + i);
}

void SiiRegWriteBlock(uint16_t reg, uint8_t *data, uint8_t len)
{
	int i = 0;
	if (!data) {
		pr_err("%s:empty data!!!\n", __func__);
		return;
	}

	for (i=0; i<len; i++)
		SiiRegWrite(reg + i, data[i]);
}

int SiiClientGet(struct i2c_client_data **pclient)
{
	*pclient = client_data;
	client_num = sizeof(client_data)/sizeof(struct i2c_client_data);

	return client_num;
}

uint8_t SiiRegRead(uint16_t reg)
{
	char buf;
	unsigned char index = reg >> 8;
	struct i2c_client *client;

	if (index >= client_num) {
		pr_err("%s: wrong index %d for client_data\n", __func__, index);
		goto exit;
	}

	client = client_data[index].client;
	if (client == NULL) {
		pr_err("%s: the slave address 0x%x doesn't do the i2c register\n",
				__func__, client_data[index].sladdr);
		goto exit;
	}

	buf = reg & 0xFF;
	if (1 != i2c_master_send(client, &buf, 1)) {
		pr_err("%s: i2c failed to 0x%x slave address\n",
				__func__, client_data[index].sladdr);
		goto exit;
	}

	if (1 != i2c_master_recv(client, &buf, 1)) {
		pr_err("%s: i2c failed to 0x%x slave address\n",
				__func__, client_data[index].sladdr);
		goto exit;
	}

	return buf;

exit:
	return 0xFF;
}
EXPORT_SYMBOL(SiiRegRead);

void SiiRegWrite(uint16_t reg, uint8_t value)
{
	char buf[2];
	unsigned char index = reg >> 8;
	struct i2c_client *client;

	if (index >= client_num) {
		pr_err("%s: wrong index 0x%d for client_data\n",
				__func__, index);
		return;
	}

	client = client_data[index].client;
	if (client == NULL) {
		pr_err("%s: the slave address 0x%x doesn't do the i2c register\n",
				__func__, client_data[index].sladdr);
		return;
	}

	buf[0] = reg & 0xFF;
	buf[1] = value;
	if (2 != i2c_master_send(client, buf, 2))
		pr_err("%s: i2c failed to 0x%x slave address\n",
				__func__, client_data[index].sladdr);
}
EXPORT_SYMBOL(SiiRegWrite);

void SiiRegModify(uint16_t reg, uint8_t mask, uint8_t value)
{
	uint8_t aByte;

	aByte = SiiRegRead(reg);
	aByte &= (~mask);
	aByte |= (mask & value);
	SiiRegWrite(reg, aByte);
}
EXPORT_SYMBOL(SiiRegModify);
