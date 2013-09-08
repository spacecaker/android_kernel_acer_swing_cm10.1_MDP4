/*
 * Acer Headset button detection driver.
 *
 * Copyright (C) 2012 Acer Corporation.
 *
 * Authors:
 *    Eric Cheng <Eric.Cheng@acer.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <mach/gpio-v1.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/mfd/pm8xxx/pm8xxx-adc.h>
#include <linux/printk.h>
#include "acer_headset_butt.h"
#include "acer_headset.h"

#define DEBOUNCE_TIME			200000000 /* 200 ms */

struct hs_butt_data {
	struct switch_dev sdev;
	struct input_dev *input;
	struct hrtimer btn_timer;
	ktime_t btn_debounce_time;
	unsigned int butt;
	unsigned int det;
	unsigned int mic;
	unsigned int irq;
};

static bool hs_state;
static struct hs_butt_data *hr;
static struct work_struct work;

extern void press_headset_button(int button_num);

static enum hrtimer_restart button_event_timer_func(struct hrtimer *data)
{
	/* Prevent to trigger the Music_AP after hanging up*/
	if (gpio_get_value(hr->det) == 1)
		return HRTIMER_NORESTART;

	schedule_work(&work);

	return HRTIMER_NORESTART;
}

void set_hs_state(bool state)
{
	hs_state = state;
}

int get_mpp_hs_mic_adc(u64 *val)
{
	uint32_t rc;
	struct pm8xxx_adc_chan_result result;

	rc = pm8xxx_adc_read(ADC_MPP_1_AMUX6, &result);
	if (!rc)
		printk("ADC MPP value chan:%d raw:%x measurement:%lld physical:%lld\n",
			result.chan, result.adc_code, result.measurement, result.physical);

	*val = result.adc_code;

	return 0;
}

static void rpc_call_work(struct work_struct *work)
{
	u64 adc_reply;
	int count = 0;

	if (hs_state) {
		if (!gpio_get_value(hr->butt)) {
			printk("HS button is pressing...\n");
			while (count < 3) {
				get_mpp_hs_mic_adc(&adc_reply);
				mdelay(1);
				count++;
			}

			if (adc_reply >= 0x6000 && adc_reply < 0x6500) {
				printk("HS button : play/pause function...\n");
				press_headset_button(0);
			} else if (adc_reply >= 0x6500 && adc_reply < 0x6a00) {
				printk("HS button : volume up function...\n");
				press_headset_button(1);
			} else if (adc_reply >= 0x6a00 && adc_reply < 0x7500) {
				printk("HS button : volume down function...\n");
				press_headset_button(2);
			}
		}
	}
}

static irqreturn_t hs_butt_interrupt(int irq, void *dev_id)
{
	if (hs_state) {
		printk("hs_butt_interrupt hrtimer_start...\r\n");
		hrtimer_start(&hr->btn_timer, hr->btn_debounce_time, HRTIMER_MODE_REL);
	}

	return IRQ_HANDLED;
}

int acer_hs_butt_init(void)
{
	int ret = 0;

	hr = kzalloc(sizeof(struct hs_butt_data), GFP_KERNEL);
	if (!hr)
		return -ENOMEM;

	hr->btn_debounce_time = ktime_set(0, DEBOUNCE_TIME);

	/* init work queue*/
	hr->sdev.name = "acer_hs_butt";
	hr->det = GPIO_HS_DET;
	hr->butt = GPIO_HS_BUTT;
	hr->irq = MSM_GPIO_TO_INT(hr->butt);

	ret = switch_dev_register(&hr->sdev);
	if (ret < 0) {
		pr_err("switch_dev fail!\n");
		goto err_switch_dev_register;
	} else {
		pr_debug("### hs_butt_switch_dev success register ###\n");
	}

	if (gpio_is_valid(hr->butt)) {
		ret = gpio_request(hr->butt, "hs_butt_detect");
		if (ret) {
			pr_err("%s: unable to request reset gpio %d\n", __func__, hr->butt);
			goto err_request_butt_irq;
		}

		ret = gpio_direction_input(hr->butt);
		if (ret) {
			pr_err("%s: unable to set direction for gpio %d\n", __func__, hr->butt);
			goto err_request_butt_gpio;
		}
	}

	INIT_WORK(&work, rpc_call_work);
	hrtimer_init(&hr->btn_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr->btn_timer.function = button_event_timer_func;

	ret = request_irq(hr->irq, hs_butt_interrupt, IRQF_TRIGGER_FALLING, "hs_butt", NULL);
	if (ret < 0) {
		pr_err("err_request_butt_irq fail!\n");
		goto err_request_butt_irq;
	} else {
		pr_debug("[HS-BUTT] IRQ_%d already request_butt_irq in use\n", hr->irq);
	}

	pr_debug("[HS-BUTT] Probe done\n");

	return 0;

err_request_butt_irq:
	free_irq(hr->irq, 0);
err_request_butt_gpio:
	gpio_free(hr->butt);
err_switch_dev_register:
	pr_err("[HS-BUTT] Probe error\n");
	return ret;
}

int acer_hs_butt_remove(void)
{
	input_unregister_device(hr->input);

	gpio_free(hr->butt);
	free_irq(hr->irq, 0);
	switch_dev_unregister(&hr->sdev);

	return 0;
}
