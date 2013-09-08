/*
 * Acer Headset detection driver.
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
#include <mach/pmic.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/mfd/pm8xxx/misc.h>
#include <linux/regulator/consumer.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/types.h>
#include <linux/workqueue.h>
#include <linux/printk.h>
#include "wcd9310.h"
#include "acer_headset.h"
#ifdef CONFIG_ACER_HEADSET_BUTT
#include "acer_headset_butt.h"
#endif

struct hs_res {
	struct switch_dev sdev;
	unsigned int det;
	unsigned int irq;
	unsigned int mic_bias_en;
	unsigned int hph_amp_en;
	bool headsetOn;
	struct hrtimer timer;
	ktime_t debounce_time;
};

enum {
	NO_DEVICE			= 0,
	ACER_HEADSET		= 1,
	ACER_HEADSET_NO_MIC	= 2,
};

static bool hsed_vdd_on = false;
static struct hs_res *hr;
static struct regulator *hsed_vdd;
static struct work_struct short_wq;

int acer_hs_detect(void);
static void acer_headset_delay_set_work(struct work_struct *work);
static void acer_headset_delay_butt_work(struct work_struct *work);
static DECLARE_DELAYED_WORK(set_hs_state_wq, acer_headset_delay_set_work);
static DECLARE_DELAYED_WORK(set_hs_butt_wq, acer_headset_delay_butt_work);

extern void acer_headset_mic_bias(int state);
extern void acer_headset_jack_report(int headset_type);
extern void acer_headset_control(int plug_in);

static ssize_t acer_hs_print_name(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(&hr->sdev)) {
		case NO_DEVICE:
			return sprintf(buf, "No Device\n");
		case ACER_HEADSET:
			return sprintf(buf, "Headset\n");
		case ACER_HEADSET_NO_MIC:
			return sprintf(buf, "Headset no mic\n");
	}
	return -EINVAL;
}

static ssize_t acer_hs_print_state(struct switch_dev *sdev, char *buf)
{
	switch (switch_get_state(&hr->sdev)) {
		case NO_DEVICE:
			return sprintf(buf, "%s\n", "0");
		case ACER_HEADSET:
			return sprintf(buf, "%s\n", "1");
		case ACER_HEADSET_NO_MIC:
			return sprintf(buf, "%s\n", "2");
	}
	return -EINVAL;
}

void start_button_irq(void)
{
	schedule_delayed_work(&set_hs_butt_wq, 200);
}

void stop_button_irq(void)
{
	set_hs_state(false);
	cancel_delayed_work_sync(&set_hs_butt_wq);
}

static void remove_headset(void)
{
	pr_debug("Remove Headset\n");
	switch_set_state(&hr->sdev, NO_DEVICE);
}

static void acer_headset_delay_butt_work(struct work_struct *work)
{
	printk("Enable headset button !!!\n");
#ifdef CONFIG_ACER_HEADSET_BUTT
	set_hs_state(true);
#endif
}

static void acer_update_state_work(struct work_struct *work)
{
#ifdef CONFIG_ACER_HEADSET_BUTT
	set_hs_state(false);
#endif
}

static int hsedbias2_vreg_enable(void)
{
	int rc = 0;

	rc = regulator_set_optimum_mode(hsed_vdd, 600000);
	if (rc < 0) {
		pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
		__func__, rc);
	}

	rc = regulator_set_voltage(hsed_vdd, 2200000, 2200000);
	if (rc) {
		pr_err("%s: regulator_set_voltage"
				"failed rc =%d\n", __func__, rc);
		goto error_vdd;
	}

	rc = regulator_enable(hsed_vdd);
	if (rc) {
		pr_err("%s: regulator_enable failed rc =%d\n",
				__func__, rc);
		goto error_vdd;
	}

	hsed_vdd_on = true;

error_vdd:
	return rc;
}

static int hsedbias2_vreg_disable(void)
{
	int rc = 0;

	if (!hsed_vdd_on)
		return rc;

	rc = regulator_disable(hsed_vdd);
	if (rc) {
		pr_err("%s: regulator_enable failed rc =%d\n",
				__func__, rc);
		goto error_vdd;
	}

	rc = regulator_set_optimum_mode(hsed_vdd, 0);
	if (rc < 0) {
		pr_err("%s: regulator_set_optimum_mode failed rc=%d\n",
		__func__, rc);
	}

	rc = regulator_set_voltage(hsed_vdd, 0, 2200000);
	if (rc) {
		pr_err("%s: regulator_set_voltage"
				"failed rc =%d\n", __func__, rc);
		goto error_vdd;
	}

	hsed_vdd_on = false;

error_vdd:
	return rc;
}

static void acer_headset_delay_set_work(struct work_struct *work)
{
	bool hs_disable;
	u64 adc_reply;

	hs_disable = gpio_get_value(hr->det);
	printk("acer hs detect state = %d...\n", hs_disable);

	if (!hs_disable) {
		// HSED_BIAS2 enable
		hsedbias2_vreg_enable();

		// Delay time for waiting Headset MIC BIAS working voltage.
		mdelay(HEADSET_MIC_BIAS_WORK_DELAY_TIME);

		get_mpp_hs_mic_adc(&adc_reply);
		if (adc_reply < 0x6500) {
			acer_headset_jack_report(ACER_HEADSET_NO_MIC);
			printk("ACER_HEADSET without MIC\n");
		} else {
			acer_headset_jack_report(ACER_HEADSET);
			printk("ACER_HEADSET\n");
		}

#ifdef CONFIG_ACER_HEADSET_BUTT
		start_button_irq();
#endif
	} else {
#ifdef CONFIG_ACER_HEADSET_BUTT
		set_hs_state(false);
#endif

		// HSED_BIAS2 disable
		hsedbias2_vreg_disable();

#ifdef CONFIG_ACER_HEADSET_BUTT
		stop_button_irq();
#endif
		acer_headset_jack_report(NO_DEVICE);
		printk("Remove HEADSET\n");
	}
}

void acer_hs_detect_work(void)
{
	// Check headset status when device boot.
	schedule_delayed_work(&set_hs_state_wq, 50);
}

static enum hrtimer_restart detect_event_timer_func(struct hrtimer *data)
{
	schedule_work(&short_wq);
	schedule_delayed_work(&set_hs_state_wq, 20);

	return HRTIMER_NORESTART;
}

static irqreturn_t hs_det_irq(int irq, void *dev_id)
{
	hrtimer_start(&hr->timer, hr->debounce_time, HRTIMER_MODE_REL);

	return IRQ_HANDLED;
}

int acer_hs_init(void)
{
	int ret;

	pr_debug("acer_hs_probe start...\n");

	hr = kzalloc(sizeof(struct hs_res), GFP_KERNEL);
	if (!hr)
		return -ENOMEM;

	hr->debounce_time = ktime_set(0, 500000000);  /* 500 ms */

	INIT_WORK(&short_wq, acer_update_state_work);
	hr->sdev.name = "h2w";
	hr->sdev.print_name = acer_hs_print_name;
	hr->sdev.print_state = acer_hs_print_state;
	hr->headsetOn = false;
	hr->det = GPIO_HS_DET;
	hr->irq = MSM_GPIO_TO_INT(hr->det);

	ret = switch_dev_register(&hr->sdev);
	if (ret < 0) {
		pr_err("switch_dev fail!\n");
		goto err_switch_dev_register;
	}

	if (gpio_is_valid(hr->det)) {
		/* configure touchscreen reset out gpio */
		ret = gpio_request(hr->det, "hs_detect");
		if (ret) {
			pr_err("%s: unable to request reset gpio %d\n", __func__, hr->det);
			goto err_request_detect_irq;
		}

		ret = gpio_direction_input(hr->det);
		if (ret) {
			pr_err("%s: unable to set direction for gpio %d\n", __func__, hr->det);
			goto err_request_detect_gpio;
		}
	}

	hrtimer_init(&hr->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	hr->timer.function = detect_event_timer_func;

	ret = request_irq(hr->irq, hs_det_irq,  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "hs_detect", NULL);
	if (ret < 0) {
		pr_err("request detect irq fail!\n");
		goto err_request_detect_irq;
	}


	hsed_vdd = regulator_get(NULL, "8921_l17");
	if (IS_ERR(hsed_vdd)) {
		ret = PTR_ERR(hsed_vdd);
		pr_err("%s:regulator get failed rc=%d\n", __func__, ret);
	}

	pr_debug("acer_hs_probe done.\n");

	return 0;

err_request_detect_irq:
	free_irq(hr->irq, 0);
err_request_detect_gpio:
	gpio_free(hr->det);
err_switch_dev_register:
	pr_err("ACER-HS: Failed to register driver\n");

	return ret;
}

int acer_hs_remove(void)
{
	if (switch_get_state(&hr->sdev))
		remove_headset();

	gpio_free(hr->det);
	free_irq(hr->irq, 0);
	switch_dev_unregister(&hr->sdev);

	return 0;
}

int acer_hs_detect(void)
{
	int curstate;
	bool hs_disable;

	hs_disable = gpio_get_value(hr->det);

	if (!hs_disable) {
		pr_debug("ACER_HEADSET!!\n");
		curstate = ACER_HEADSET;
	} else {
		pr_debug("NO_DEVICE!!\n");
		curstate = NO_DEVICE;
	}

	return curstate;
}
