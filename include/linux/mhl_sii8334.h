#ifndef __MHL_SII8334_H__
#define __MHL_SII8334_H__

#define MHL_SII8334_DRIVER_NAME "sii8334"

struct sii8334_platform_data {
	int gpio_irq;
	int gpio_reset;
	int (*core_power)(int on);
	int (*enable_5v)(int on);
#ifdef CONFIG_MACH_ACER_A9
	int (*extra_power)(int on);
#endif
};

extern int get_mhl_chip_id(void);
extern int is_mhl_dongle_on(void);
#endif
