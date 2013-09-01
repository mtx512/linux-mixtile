/* linux/arch/arm/mach-exynos/setup-fb-s5p.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base FIMD controller configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/io.h>

#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>
#include <mach/gpio.h>
#include <mach/board_rev.h>
#include <mach/gpio-mixtile4x12.h>

#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/cpu.h>

struct platform_device; /* don't need the contents */
extern void tp_power(int onoff);

#ifdef CONFIG_FB_S5P
static void s3cfb_gpio_setup_24bpp(unsigned int start, unsigned int size,
		unsigned int cfg, s5p_gpio_drvstr_t drvstr)
{
	u32 reg;

	s3c_gpio_cfgrange_nopull(start, size, cfg);

	for (; size > 0; size--, start++)
		s5p_gpio_set_drvstr(start, drvstr);

	/* Set FIMD0 bypass */
	reg = __raw_readl(S3C_VA_SYS + 0x0210);
	reg |= (1<<1);
	__raw_writel(reg, S3C_VA_SYS + 0x0210);
}

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	s3cfb_gpio_setup_24bpp(EXYNOS4_GPF0(0), 8, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV1);
	s3cfb_gpio_setup_24bpp(EXYNOS4_GPF1(0), 8, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV1);
	s3cfb_gpio_setup_24bpp(EXYNOS4_GPF2(0), 8, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV1);
	s3cfb_gpio_setup_24bpp(EXYNOS4_GPF3(0), 4, S3C_GPIO_SFN(2), S5P_GPIO_DRVSTR_LV1);
}
#endif /* CONFIG_FB_S5P */

int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk)
{
	struct clk *sclk = NULL;
	struct clk *mout_mpll = NULL;
	struct clk *lcd_clk = NULL;

	u32 rate = 0;
	int ret = 0;

	lcd_clk = clk_get(&pdev->dev, "lcd");
	if (IS_ERR(lcd_clk)) {
		dev_err(&pdev->dev, "failed to get operation clk for fimd\n");
		goto err_clk0;
	}

	ret = clk_enable(lcd_clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_enable of lcd clk for fimd\n");
		goto err_clk0;
	}
	clk_put(lcd_clk);

	sclk = clk_get(&pdev->dev, "sclk_fimd");
	if (IS_ERR(sclk)) {
		dev_err(&pdev->dev, "failed to get sclk for fimd\n");
		goto err_clk1;
	}

	if (soc_is_exynos4210())
		mout_mpll = clk_get(&pdev->dev, "mout_mpll");
	else
		mout_mpll = clk_get(&pdev->dev, "mout_mpll_user");

	if (IS_ERR(mout_mpll)) {
		dev_err(&pdev->dev, "failed to get mout_mpll for fimd\n");
		goto err_clk2;
	}

	ret = clk_set_parent(sclk, mout_mpll);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_set_parent for fimd\n");
		goto err_clk2;
	}

	if ((soc_is_exynos4412()) && (samsung_rev() >= EXYNOS4412_REV_2_0))
		ret = clk_set_rate(sclk, 880000000);
	else
		ret = clk_set_rate(sclk, 800000000);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_set_rate of sclk for fimd\n");
		goto err_clk2;
	}
	dev_dbg(&pdev->dev, "set fimd sclk rate to %d\n", rate);

	clk_put(mout_mpll);

	ret = clk_enable(sclk);
	if (ret < 0) {
		dev_err(&pdev->dev, "failed to clk_enable of sclk for fimd\n");
		goto err_clk2;
	}

	*s3cfb_clk = sclk;

	return 0;

err_clk2:
	clk_put(mout_mpll);
err_clk1:
	clk_put(sclk);
err_clk0:
	clk_put(lcd_clk);

	return -EINVAL;
}

int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk)
{
	struct clk *lcd_clk = NULL;

	lcd_clk = clk_get(&pdev->dev, "lcd");
	if (IS_ERR(lcd_clk)) {
		printk(KERN_ERR "failed to get ip clk for fimd0\n");
		goto err_clk0;
	}

	clk_disable(lcd_clk);
	clk_put(lcd_clk);

	clk_disable(*clk);
	clk_put(*clk);

	*clk = NULL;

	return 0;

err_clk0:
	clk_put(lcd_clk);

	return -EINVAL;
}

void s3cfb_get_clk_name(char *clk_name)
{
	strcpy(clk_name, "sclk_fimd");
}

int s3cfb_lcd_on(struct platform_device *pdev)
{
  printk("%s\n", __func__);
  gpio_direction_output(GPIO_LCD_POWER, 1);
  gpio_direction_output(GPIO_LVDS_POWER, 1);
  return 0;
}

int s3cfb_lcd_off(struct platform_device *pdev)
{
  printk("%s\n", __func__);
  gpio_direction_output(GPIO_LVDS_POWER, 0);
  gpio_direction_output(GPIO_LCD_POWER, 0);
  return 0;
}

int s3cfb_backlight_on(struct platform_device *pdev)
{
  printk("%s\n", __func__);
  s3cfb_lcd_on(pdev);

  gpio_direction_output(GPIO_LCD_BACKLED, 1);
  tp_power(1);//启用TP
  return 0;
}

int s3cfb_backlight_off(struct platform_device *pdev)
{
  printk("%s\n", __func__);
  gpio_direction_output(GPIO_LCD_BACKLED, 0);
  
  s3cfb_lcd_off(pdev);
  tp_power(0);//禁用TP，省电
	return 0;
}
