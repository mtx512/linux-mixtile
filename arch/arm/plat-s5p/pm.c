/* linux/arch/arm/plat-s5p/pm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com
 *
 * S5P Power Manager (Suspend-To-RAM) support
 *
 * Based on arch/arm/plat-s3c24xx/pm.c
 * Copyright (c) 2004,2006 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/suspend.h>
#include <plat/pm.h>
//Apollo +
#include <linux/io.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-pmu.h>
#include <mach/regs-gpio.h>
#include <mach/gpio-exynos4.h>
#include <mach/gpio-mixtile4x12.h>

static unsigned restore_state;
//Apollo -

#define PFX "s5p pm: "

/* s3c_pm_configure_extint
 *
 * configure all external interrupt pins
*/

void s3c_pm_configure_extint(void)
{
	/* nothing here yet */
//Apollo +
	unsigned int tmp;
  s3c_gpio_cfgpin(GPIO_POWER_KEY, S3C_GPIO_SFN(0xF));
	s3c_gpio_setpull(GPIO_POWER_KEY, S3C_GPIO_PULL_UP);

	/*Set irq type*/
	tmp = __raw_readl(S5P_EINT_CON(POWER_KEY_EINT));
	restore_state = tmp;
	tmp = (tmp&(~(0x7<<(IRQ_POWER_WAKEUP*4)))) | (0x3<<(IRQ_POWER_WAKEUP*4));
	__raw_writel(tmp, S5P_EINT_CON(POWER_KEY_EINT));
//printk("S5P_EINT_CON(%d) = 0x%08x 0x%08x\n", POWER_KEY_EINT, tmp, __raw_readl(S5P_EINT_CON(POWER_KEY_EINT)));

	/*clear mask*/
	tmp = __raw_readl(S5P_EINT_MASK(POWER_KEY_EINT));
	tmp &= ~(0x1<<IRQ_POWER_WAKEUP);
	__raw_writel(tmp, S5P_EINT_MASK(POWER_KEY_EINT));
//printk("S5P_EINT_MASK(%d) = 0x%08x 0x%08x\n", POWER_KEY_EINT, tmp, __raw_readl(S5P_EINT_MASK(POWER_KEY_EINT)));

	/*clear pending*/
	tmp = __raw_readl(S5P_EINT_PEND(POWER_KEY_EINT));
	tmp &= (0x1<<IRQ_POWER_WAKEUP);
	__raw_writel(tmp, S5P_EINT_PEND(POWER_KEY_EINT));
//printk("S5P_EINT_PEND(%d) = 0x%08x 0x%08x\n", POWER_KEY_EINT, tmp, __raw_readl(S5P_EINT_PEND(POWER_KEY_EINT)));

	/*Enable wake up mask*/
	s3c_irqwake_eintmask &= ~(1 << IRQ_POWER_WAKEUP);
	
//	s3c_irqwake_intmask = 0xFFFFFFFF & ~(1 << IRQ_RTC_WAKEUP);
	s3c_irqwake_intmask = 0xFFFFFFFF;
//Apollo -
}

void s3c_pm_restore_core(void)
{
	/* nothing here yet */
  	__raw_writel(restore_state, S5P_EINT_CON(POWER_KEY_EINT));
}

void s3c_pm_save_core(void)
{
	/* nothing here yet */
	restore_state = __raw_readl(S5P_EINT_CON(POWER_KEY_EINT));
}

