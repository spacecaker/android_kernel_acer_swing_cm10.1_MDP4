/*
 * Acer Headset detection driver
 *
 * Copyright (C) 2012 Acer Corporation.
 *
 * Authors:
 *    Eric Cheng <Eric.Cheng@acer.com>
 */

#ifndef __ACER_HEADSET_H
#define __ACER_HEADSET_H

#include "../../../arch/arm/mach-msm/board-acer-8960.h"

#define GPIO_HS_DET							36
#define GPIO_HS_BUTT						78
#define HEADSET_MIC_BIAS_WORK_DELAY_TIME	100

extern int acer_hs_init(void);
extern int acer_hs_remove(void);
extern void start_button_irq(void);
extern void stop_button_irq(void);
extern void acer_hs_detect_work(void);
#endif
