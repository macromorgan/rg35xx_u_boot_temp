// SPDX-License-Identifier: GPL-2.0+
/*
 * AXP717 SPL driver
 * (C) Copyright 2024 Arm Ltd.
 */

#include <common.h>
#include <command.h>
#include <errno.h>
#include <asm/arch/pmic_bus.h>
#include <axp_pmic.h>

enum axp717_reg {
	AXP717_CHIP_VERSION = 0x3,
	AXP717_SHUTDOWN = 0x27,
	AXP717_OUTPUT_CTRL1 = 0x80,
	AXP717_DCDC1_VOLTAGE = 0x83,
};

#define AXP717_CHIP_VERSION_MASK	0xc8
#define AXP717_DCDC_1220MV_OFFSET	71
#define AXP717_POWEROFF			(1U << 0)
#define DCDC_DVM_ENABLE			(1U << 7)

static u8 axp_mvolt_to_cfg(int mvolt, int min, int max, int div)
{
	if (mvolt < min)
		mvolt = min;
	else if (mvolt > max)
		mvolt = max;

	return (mvolt - min) / div;
}

static int axp_set_dcdc(int dcdc_num, unsigned int mvolt)
{
	int ret;
	u8 cfg = DCDC_DVM_ENABLE;

	if (dcdc_num < 1 || dcdc_num > 3)
		return -EINVAL;

	if (mvolt >= 1220)
		cfg |= AXP717_DCDC_1220MV_OFFSET +
			axp_mvolt_to_cfg(mvolt, 1220,
					 dcdc_num == 3 ? 1840 : 1540, 20);
	else
		cfg |= axp_mvolt_to_cfg(mvolt, 500, 1200, 10);

	if (mvolt == 0)
		return pmic_bus_clrbits(AXP717_OUTPUT_CTRL1,
					1U << (dcdc_num -1));

	ret = pmic_bus_write(AXP717_DCDC1_VOLTAGE + dcdc_num - 1, cfg);
	if (ret)
		return ret;

	return pmic_bus_setbits(AXP717_OUTPUT_CTRL1, 1U << (dcdc_num - 1));
}

int axp_set_dcdc1(unsigned int mvolt)
{
	return axp_set_dcdc(1, mvolt);
}

int axp_set_dcdc2(unsigned int mvolt)
{
	return axp_set_dcdc(2, mvolt);
}

int axp_set_dcdc3(unsigned int mvolt)
{
	return axp_set_dcdc(3, mvolt);
}

int axp_init(void)
{
	return pmic_bus_init();
}

#if !CONFIG_IS_ENABLED(ARM_PSCI_FW) && !IS_ENABLED(CONFIG_SYSRESET_CMD_POWEROFF)
int do_poweroff(struct cmd_tbl *cmdtp, int flag, int argc, char *const argv[])
{
	pmic_bus_setbits(AXP717_SHUTDOWN, AXP717_POWEROFF);

	/* infinite loop during shutdown */
	while (1) {}

	/* not reached */
	return 0;
}
#endif
