// SPDX-License-Identifier: GPL-2.0+
/*
 * (C) Copyright 2020 Samuel Holland <samuel@sholland.org>
 * (C) Copyright 2024 Chris Morgan <macromorgan@hotmail.com>
 */

#include <i2c.h>
#include <sunxi_gpio.h>
#ifdef CONFIG_PINEPHONE_DT_SELECTION
#include <asm/arch/prcm.h>
#endif
#include <asm/gpio.h>
#include <linux/delay.h>
#include <linux/sizes.h>

#include "board_select.h"

#ifdef CONFIG_PINE64_DT_SELECTION
/* Differentiate the Pine A64 boards by their DRAM size. */
int board_get_name_match(const char **name) {

	if (strstr(*name, "-pine64-plus")) {
		if (gd->ram_size == SZ_512M)
			*name = "sun50i-a64-pine64";
	}

	return 0;
}
#endif

#ifdef CONFIG_PINEPHONE_DT_SELECTION
int board_get_name_match(const char **name) {

	if (!strstr(*name, "-pinephone"))
		return 0;

	prcm_apb0_enable(PRCM_APB0_GATE_PIO);
	sunxi_gpio_set_pull(SUNXI_GPL(6), SUNXI_GPIO_PULL_UP);
	sunxi_gpio_set_cfgpin(SUNXI_GPL(6), SUNXI_GPIO_INPUT);
	udelay(100);

	if (gpio_get_value(SUNXI_GPL(6)) == 0)
		*name = "sun50i-a64-pinephone-1.2";
	else
		*name = "sun50i-a64-pinephone-1.1";

	sunxi_gpio_set_cfgpin(SUNXI_GPL(6), SUNXI_GPIO_DISABLE);
	sunxi_gpio_set_pull(SUNXI_GPL(6), SUNXI_GPIO_PULL_DISABLE);
	prcm_apb0_disable(PRCM_APB0_GATE_PIO);

	return 0;
}
#endif

#ifdef CONFIG_RG35XX_H700_DT_SELECTION

#define NXP_PCF8563_ADDR		0x51

struct rg35xx_model {
	const char *board;
	const bool adc;
	const bool wifi;
	const bool ext_rtc;
};

enum rg35xx_device_id {
	RG35XX_2024,
	RG35XX_H,
	RG35XX_PLUS,
	RG35XX_SP,
};

static const struct rg35xx_model rg35xx_model_details[] = {
	[RG35XX_2024] = {
		.board = "allwinner/sun50i-h700-anbernic-rg35xx-2024",
		.adc = 0,
		.wifi = 0,
		.ext_rtc = 1,
	},
	[RG35XX_H] = {
		.board = "allwinner/sun50i-h700-anbernic-rg35xx-h",
		.adc = 1,
		.wifi = 1,
		.ext_rtc = 0,
	},
	[RG35XX_PLUS] = {
		.board = "allwinner/sun50i-h700-anbernic-rg35xx-plus",
		.adc = 0,
		.wifi = 1,
		.ext_rtc = 0,
	},
	[RG35XX_SP] = {
		.board = "allwinner/sun50i-h700-anbernic-rg35xx-sp",
		.adc = 0,
		.wifi = 1,
		.ext_rtc = 1,
	},
};

/*
 * Differentiate the Anbernic RG revisions by GPIO inputs. ADC joystick
 * will cause PE11 to report low. WiFi will cause PG5 to report low.
 * Some devices have an external RTC on the same bus as the PMIC. With
 * each of these data points we can determine the specific device.
 */
int board_get_name_match(const char **name) {
	u8 data = 0;
	bool adc = 0;
	bool wifi = 0;
	bool ext_rtc = 0;
	int ret = 0;
	int i;

	if (!strstr(*name, "sun50i-h700-anbernic-rg"))
		return 0;

	sunxi_gpio_set_pull(SUNXI_GPE(11), SUNXI_GPIO_PULL_UP);
	sunxi_gpio_set_cfgpin(SUNXI_GPE(11), SUNXI_GPIO_INPUT);
	sunxi_gpio_set_pull(SUNXI_GPG(5), SUNXI_GPIO_PULL_UP);
	sunxi_gpio_set_cfgpin(SUNXI_GPG(5), SUNXI_GPIO_INPUT);
	udelay(100);

	adc = (!gpio_get_value(SUNXI_GPE(11)));
	wifi = (!gpio_get_value(SUNXI_GPG(5)));

	/* Just read the first byte, all we care is that it reads. */
	ret = i2c_read(NXP_PCF8563_ADDR, 0, 1, &data, 1);
	if (!ret)
		ext_rtc = 1;

	sunxi_gpio_set_cfgpin(SUNXI_GPE(11), SUNXI_GPIO_DISABLE);
	sunxi_gpio_set_pull(SUNXI_GPE(11), SUNXI_GPIO_PULL_DISABLE);
	sunxi_gpio_set_cfgpin(SUNXI_GPG(5), SUNXI_GPIO_DISABLE);
	sunxi_gpio_set_pull(SUNXI_GPG(5), SUNXI_GPIO_PULL_DISABLE);

	for (i = 0; i < ARRAY_SIZE(rg35xx_model_details); i++) {
		if ((rg35xx_model_details[i].adc == adc) &&
		    (rg35xx_model_details[i].wifi == wifi) &&
		    (rg35xx_model_details[i].ext_rtc == ext_rtc)) {
			*name = rg35xx_model_details[i].board;
			return 0;
		}
	}

	printf("Unable to determine board\n");
	return -ENODEV;
}
#endif
