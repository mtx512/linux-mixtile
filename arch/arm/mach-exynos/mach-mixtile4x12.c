/* linux/arch/arm/mach-exynos/mach-mixtile4x12.c
 *
 * Copyright (c) 2011 Samsung Electronics Co., Ltd.
 *    http://www.samsung.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/platform_device.h>
#include <linux/serial_core.h>
#include <linux/clk.h>
#include <linux/lcd.h>
#include <linux/gpio.h>
#include <linux/gpio_keys.h>
#include <linux/gpio_event.h>
#include <linux/i2c.h>
#include <linux/pwm_backlight.h>
#include <linux/input.h>
#include <linux/mmc/host.h>
#include <linux/v4l2-mediabus.h>
#include <linux/memblock.h>
#include <linux/delay.h>
#if defined(CONFIG_S5P_MEM_CMA)
#include <linux/cma.h>
#endif
#ifdef CONFIG_ANDROID_PMEM
#include <linux/android_pmem.h>
#endif
#include <linux/notifier.h>
#include <linux/reboot.h>
#include <linux/dm9000.h>
#include <linux/leds.h>
#include <linux/leds_pwm.h>
#include <linux/pm.h>
#include <linux/s3c_adc_battery.h>

#include <asm/mach/arch.h>
#include <asm/mach-types.h>
#include <asm/io.h>
#include <asm/cacheflush.h>

#include <plat/regs-serial.h>
#include <plat/exynos4.h>
#include <plat/cpu.h>
#include <plat/clock.h>
#include <plat/keypad.h>
#include <plat/devs.h>
#include <plat/fb.h>
#include <plat/fb-s5p.h>
#include <plat/fb-core.h>
#include <plat/regs-fb-v4.h>
#include <plat/backlight.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-adc.h>
#include <plat/adc.h>
#include <plat/iic.h>
#include <plat/pd.h>
#include <plat/sdhci.h>
#include <plat/mshci.h>
#include <plat/ehci.h>
#include <plat/usbgadget.h>
#if defined(CONFIG_VIDEO_FIMC)
#include <plat/fimc.h>
#endif
#include <plat/tvout.h>
#include <plat/media.h>
#include <plat/regs-srom.h>
#include <plat/sysmmu.h>
#include <plat/tv-core.h>
#include <media/exynos_flite.h>
#include <media/exynos_fimc_is.h>
#include <video/platform_lcd.h>
#include <mach/system.h>
#include <mach/board_rev.h>
#include <mach/map.h>
#include <mach/exynos-ion.h>
#include <mach/regs-pmu.h>
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
#include <mach/secmem.h>
#endif
#include <mach/dev.h>
#include <mach/ppmu.h>
#if defined(CONFIG_VIDEO_MFC5X)
#include <plat/s5p-mfc.h>
#endif
#include <media/mixtile_camera.h>

#include <plat/fimg2d.h>
#include <mach/dev-sysmmu.h>

#ifdef CONFIG_VIDEO_JPEG_V2X
#include <plat/jpeg.h>
#endif

#if defined(CONFIG_EXYNOS_SETUP_THERMAL)
#include <plat/s5p-tmu.h>
#endif

#include <../../../drivers/video/samsung/s3cfb.h>
#include <../../../drivers/staging/android/timed_gpio.h>
#include <mach/gpio-mixtile4x12.h>
#include "mixtile4x12.h"

int dev_ver=0;
EXPORT_SYMBOL(dev_ver);

struct class *sec_class;
EXPORT_SYMBOL(sec_class);

struct device *switch_dev;
EXPORT_SYMBOL(switch_dev);

extern void mixtile4x12_config_gpio_table(void);
#define REG_INFORM4            (S5P_INFORM4)

/* Following are default values for UCON, ULCON and UFCON UART registers */
#define SMDK4X12_UCON_DEFAULT  (S3C2410_UCON_TXILEVEL |  \
         S3C2410_UCON_RXILEVEL |  \
         S3C2410_UCON_TXIRQMODE |  \
         S3C2410_UCON_RXIRQMODE |  \
         S3C2410_UCON_RXFIFO_TOI |  \
         S3C2443_UCON_RXERR_IRQEN)

#define SMDK4X12_ULCON_DEFAULT  S3C2410_LCON_CS8

#define SMDK4X12_UFCON_DEFAULT  (S3C2410_UFCON_FIFOMODE |  S5PV210_UFCON_TXTRIG4 |  S5PV210_UFCON_RXTRIG4)

static struct s3c2410_uartcfg mixtile4x12_uartcfgs[] __initdata = {
  [0] = {
    .hwport    = 0,
    .flags    = 0,
    .ucon    = SMDK4X12_UCON_DEFAULT,
    .ulcon    = SMDK4X12_ULCON_DEFAULT,
    .ufcon    = SMDK4X12_UFCON_DEFAULT,
  },
  [1] = {
    .hwport    = 1,
    .flags    = 0,
    .ucon    = SMDK4X12_UCON_DEFAULT,
    .ulcon    = SMDK4X12_ULCON_DEFAULT,
    .ufcon    = SMDK4X12_UFCON_DEFAULT,
  },
  [2] = {
    .hwport    = 2,
    .flags    = 0,
    .ucon    = SMDK4X12_UCON_DEFAULT,
    .ulcon    = SMDK4X12_ULCON_DEFAULT,
    .ufcon    = SMDK4X12_UFCON_DEFAULT,
  },
  [3] = {
    .hwport    = 3,
    .flags    = 0,
    .ucon    = SMDK4X12_UCON_DEFAULT,
    .ulcon    = SMDK4X12_ULCON_DEFAULT,
    .ufcon    = SMDK4X12_UFCON_DEFAULT,
  },
};

#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
struct platform_device exynos_device_md0 = {
  .name = "exynos-mdev",
  .id = -1,
};
#endif


#ifdef CONFIG_VIDEO_FIMC
#define WRITEBACK_ENABLED
#ifdef WRITEBACK_ENABLED
static struct i2c_board_info writeback_i2c_info = {
  I2C_BOARD_INFO("WriteBack", 0x0),
};

static struct s3c_platform_camera writeback = {
  .id    = CAMERA_WB,
  .fmt    = ITU_601_YCBCR422_8BIT,
  .order422  = CAM_ORDER422_8BIT_CBYCRY,
  .i2c_busnum  = 0,
  .info    = &writeback_i2c_info,
  .pixelformat  = V4L2_PIX_FMT_YUV444,
  .line_length  = 800,
  .width    = 480,
  .height    = 800,
  .window    = {
    .left  = 0,
    .top  = 0,
    .width  = 480,
    .height  = 800,
  },

  .initialized  = 0,
};
#endif /* WRITEBACK_ENABLED */

int mixtile4x12_cam_f_power(int onoff)
{
  if(onoff)
  {
    printk("CAM_FRONT power on\n");
		gpio_direction_output(GPIO_CAMERA_POWER, 1);
	  msleep(10);
		gpio_direction_output(GPIO_CAMERA_F_PWDN, 0);
    msleep(10);
		gpio_direction_output(GPIO_CAMERA_F_RST, 1);
	  msleep(10);
 	}
	else
	{
    printk("CAM_FRONT power off\n");
		gpio_direction_output(GPIO_CAMERA_F_PWDN, 1);
		gpio_direction_output(GPIO_CAMERA_F_RST, 0);
		gpio_direction_output(GPIO_CAMERA_POWER, 0);
	}
	return 0;
}
EXPORT_SYMBOL(mixtile4x12_cam_f_power);

int mixtile4x12_cam_b_power(int onoff)
{
  if(onoff)
  {
    printk("CAM_BACK power on\n");
		gpio_direction_output(GPIO_CAMERA_POWER, 1);
	  msleep(10);
		gpio_direction_output(GPIO_CAMERA_B_PWDN, 0);
    msleep(10);
		gpio_direction_output(GPIO_CAMERA_B_RST, 1);
 	}
	else
	{
    printk("CAM_BACK power off\n");
		gpio_direction_output(GPIO_CAMERA_B_PWDN, 1);
		gpio_direction_output(GPIO_CAMERA_B_RST, 0);
		gpio_direction_output(GPIO_CAMERA_POWER, 0);
	}
	return 0;
}
EXPORT_SYMBOL(mixtile4x12_cam_b_power);

static int tvp5151_power(int onoff)
{
  if(onoff)
  {
    printk("TVP5151 power on\n");
    gpio_direction_output(GPIO_TVP5151_PWDN, 0);
    msleep(50);
		gpio_direction_output(GPIO_CAMERA_POWER, 1);
		gpio_direction_output(GPIO_TVP5151_PWDN, 1);
		 msleep(50);
		gpio_direction_output(GPIO_TVP5151_RST, 0);
    msleep(10);
		gpio_direction_output(GPIO_TVP5151_RST, 1);
	  msleep(10);
 	}
	else
	{
    printk("TVP5151 power off\n");
		gpio_direction_output(GPIO_TVP5151_PWDN, 0);
		gpio_direction_output(GPIO_TVP5151_RST, 1);
		gpio_direction_output(GPIO_CAMERA_POWER, 0);
	}
	return 0;
}
EXPORT_SYMBOL(tvp5151_power);

#ifdef CONFIG_VIDEO_GC2015
#define GC2015_WIDTH  800
#define GC2015_HEIGHT 600
static struct mixtile_camera_data gc2015_plat = {
	.default_width = GC2015_WIDTH,
	.default_height = GC2015_HEIGHT,
	.pixelformat = V4L2_PIX_FMT_UYVY,
	.freq = 24000000,
	.is_mipi = 0,
};

static struct i2c_board_info gc2015_i2c_info = {
	I2C_BOARD_INFO("GC2015", 0x30),
	.platform_data = &gc2015_plat,
};

static struct s3c_platform_camera gc2015 = {
	.id		= CAMERA_PAR_A,
	.clk_name	= "sclk_cam0",
	.i2c_busnum	= 1,
	.cam_power	= mixtile4x12_cam_f_power,
	.type		= CAM_TYPE_ITU,
	.fmt		= ITU_601_YCBCR422_8BIT,
	.order422	= CAM_ORDER422_8BIT_CBYCRY,
	.info		= &gc2015_i2c_info,
	.pixelformat	= V4L2_PIX_FMT_UYVY,
	.srclk_name	= "xusbxti",
	.clk_rate	= 24000000,
	.line_length	= GC2015_WIDTH,
	.width		= GC2015_WIDTH,
	.height		= GC2015_HEIGHT,
	.window		= {
		.left	= 0,
		.top	= 0,
  	.width		= GC2015_WIDTH,
  	.height		= GC2015_HEIGHT,
	},

	/* Polarity */
	.inv_pclk	= 0,
	.inv_vsync	= 1,
	.inv_href	= 0,
	.inv_hsync	= 0,
	.reset_camera	= 0,
	.initialized	= 1,
};
#endif /* CONFIG_VIDEO_GC2015 */

#ifdef CONFIG_VIDEO_TVP5150
#define TVP5151_WIDTH  720
#define TVP5151_HEIGHT 288
static struct mixtile_camera_data tvp5151_plat = {
	.default_width = TVP5151_WIDTH,
	.default_height = TVP5151_HEIGHT,
	.pixelformat = V4L2_PIX_FMT_YUYV,
	.freq = 27000000,
	.is_mipi = 0,
};

static struct i2c_board_info tvp5151_i2c_info = {
	I2C_BOARD_INFO("tvp5150", 0x5D),
	.platform_data = &tvp5151_plat,
};

static struct s3c_platform_camera tvp5151 = {
  .id             = CAMERA_PAR_A,
  .clk_name       = "sclk_cam0",
  .i2c_busnum     = 1,
  .type           = CAM_TYPE_ITU,
  .fmt            = ITU_656_YCBCR422_8BIT,
  .order422       = CAM_ORDER422_8BIT_CBYCRY,
  .cam_power      = tvp5151_power,
  .info           = &tvp5151_i2c_info,
  .pixelformat    = V4L2_PIX_FMT_YUYV,
	.srclk_name	    = "xusbxti",
  .clk_rate       = 27000000,
//  .line_length    = 1920,
  .width          = TVP5151_WIDTH,
  .height         = TVP5151_HEIGHT,
  .window         = {
    .left   = 0,
    .top    = 0,
    .width  = TVP5151_WIDTH,
    .height = TVP5151_HEIGHT,
  },
  
  /* Polarity */
  .inv_pclk       = 0,
  .inv_vsync      = 0,
  .inv_href       = 1,
  .inv_hsync      = 0,
  .initialized    = 0,
};
#endif /* CONFIG_VIDEO_TVP5150 */

/* Interface setting */
static struct s3c_platform_fimc fimc_plat = {
#ifdef CONFIG_ITU_A
  .default_cam  = CAMERA_PAR_A,
#endif
#ifdef CONFIG_ITU_B
  .default_cam  = CAMERA_PAR_B,
#endif
#ifdef WRITEBACK_ENABLED
  .default_cam  = CAMERA_WB,
#endif
  .camera    = {
#ifdef CONFIG_VIDEO_GC2015
		&gc2015,                //cam_id = 0;
#endif
#ifdef CONFIG_VIDEO_TVP5150
    &tvp5151,
#endif
#ifdef WRITEBACK_ENABLED
    &writeback,
#endif
  },
  .hw_ver    = 0x51,
};

static struct s3c_platform_fimc fimc_plat_htpc = {
#ifdef CONFIG_ITU_A
	.default_cam	= CAMERA_PAR_A,
#endif
#ifdef CONFIG_ITU_B
	.default_cam	= CAMERA_PAR_B,
#endif
#ifdef WRITEBACK_ENABLED
	.default_cam	= CAMERA_WB,
#endif
	.camera		= {
#ifdef CONFIG_VIDEO_GC2015
		&gc2015,                //cam_id = 0;
#endif
#ifdef WRITEBACK_ENABLED
		&writeback,
#endif
	},
	.hw_ver		= 0x51,
};
#endif /* CONFIG_VIDEO_FIMC */

#ifdef CONFIG_FB_S5P
#ifdef CONFIG_FB_LCD_MIXTILE
#ifdef CONFIG_FB_LCD_1024X768
  #define MIXTILE_LCD_WIDTH 1024
  #define MIXTILE_LCD_HEIGHT 768
#endif

#ifdef CONFIG_FB_LCD_1280X720
  #define MIXTILE_LCD_WIDTH 1280
  #define MIXTILE_LCD_HEIGHT 720
#endif

#ifdef CONFIG_FB_S3C_LG97_IPAD1
struct s3cfb_lcd mixtile_lcd = {
  .width = MIXTILE_LCD_WIDTH,
  .height = MIXTILE_LCD_HEIGHT,
  .bpp = 32,
  .freq = 60,

  .timing = {
    .h_fp = 48,
    .h_bp = 32,
    .h_sw = 80,
    .v_fp = 3,
    .v_fpe = 1,
    .v_bp = 15,
    .v_bpe = 1,
    .v_sw = 4,
  },

  .polarity = {
    .rise_vclk = 1,
    .inv_hsync = 1,
    .inv_vsync = 1,
    .inv_vden = 0,
  },
};
#endif /* CONFIG_FB_S3C_LG97_IPAD1 */

#ifdef CONFIG_FB_S3C_VGA
struct s3cfb_lcd mixtile_lcd = {
  .width = MIXTILE_LCD_WIDTH,
  .height = MIXTILE_LCD_HEIGHT,
  .bpp = 32,
  .freq = 60,

  .timing = {
    .h_fp = 110,
    .h_bp = 220,
    .h_sw = 40,
    .v_fp = 5,
    .v_fpe = 1,
    .v_bp = 20,
    .v_bpe = 1,
    .v_sw = 5,
  },

  .polarity = {
    .rise_vclk = 1,
    .inv_hsync = 1,
    .inv_vsync = 1,
    .inv_vden = 0,
  },
};
#endif /* CONFIG_FB_S3C_LG97_IPAD1 */

struct s3cfb_lcd mixtile_lcd_1080p = {
  .width = 1920,
  .height = 1080,
  .bpp = 32,
  .freq = 60,

  .timing = {
    .h_fp = 1,
    .h_bp = 1,
    .h_sw = 1,
    .v_fp = 1,
    .v_fpe = 1,
    .v_bp = 1,
    .v_bpe = 1,
    .v_sw = 1,
  },

  .polarity = {
    .rise_vclk = 0,
    .inv_hsync = 1,
    .inv_vsync = 1,
    .inv_vden = 0,
  },
};

struct s3cfb_lcd mixtile_lcd_720p = {
  .width = 1280,
  .height = 720,
  .bpp = 32,
  .freq = 60,

  .timing = {
    .h_fp = 1,
    .h_bp = 1,
    .h_sw = 1,
    .v_fp = 1,
    .v_fpe = 1,
    .v_bp = 1,
    .v_bpe = 1,
    .v_sw = 1,
  },

  .polarity = {
    .rise_vclk = 0,
    .inv_hsync = 1,
    .inv_vsync = 1,
    .inv_vden = 0,
  },
};

#endif /* CONFIG_FB_LCD_MIXTILE */
#endif /* CONFIG_FB_S5P */

#ifdef CONFIG_SWITCH_HEADPHONE
#include <linux/switch.h>
static struct gpio_switch_platform_data hp_switch_data = {
	.name = "h2w",
	.gpio = GPIO_HEADPHONE,
	.name_on = "PULL IN",
	.name_off = "PULL OUT",
};
			
struct platform_device hp_switch_device = {
	.name	= "switch-hp",
	.dev = {
	   .platform_data    = &hp_switch_data,
	   .parent = &s3c_device_adc.dev,
	 },
};
#endif /* CONFIG_SWITCH_HEADPHONE */

static int exynos4_notifier_call(struct notifier_block *this,  unsigned long code, void *_cmd)
{
  int mode = 0;

  if ((code == SYS_RESTART) && _cmd)
    if (!strcmp((char *)_cmd, "recovery"))
      mode = 0xf;

  __raw_writel(mode, REG_INFORM4);

  return NOTIFY_DONE;
}

static struct notifier_block exynos4_reboot_notifier = {
  .notifier_call = exynos4_notifier_call,
};

#ifdef CONFIG_S3C_DEV_HSMMC
static struct s3c_sdhci_platdata mixtile4x12_hsmmc0_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH0_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC1
static struct s3c_sdhci_platdata mixtile4x12_hsmmc1_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC2
static struct s3c_sdhci_platdata mixtile4x12_hsmmc2_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
#ifdef CONFIG_EXYNOS4_SDHCI_CH2_8BIT
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#endif
};
#endif

#ifdef CONFIG_S3C_DEV_HSMMC3
static struct s3c_sdhci_platdata mixtile4x12_hsmmc3_pdata __initdata = {
	.cd_type		= S3C_SDHCI_CD_INTERNAL,
	.clk_type		= S3C_SDHCI_CLK_DIV_EXTERNAL,
};
#endif

#ifdef CONFIG_S5P_DEV_MSHC
static struct s3c_mshci_platdata exynos4_mshc_pdata __initdata = {
	.cd_type		= S3C_MSHCI_CD_PERMANENT,
	.has_wp_gpio		= true,
	.wp_gpio		= 0xffffffff,
#if defined(CONFIG_EXYNOS4_MSHC_8BIT) && \
	defined(CONFIG_EXYNOS4_MSHC_DDR)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA | MMC_CAP_1_8V_DDR |
				  MMC_CAP_UHS_DDR50,
#elif defined(CONFIG_EXYNOS4_MSHC_8BIT)
	.max_width		= 8,
	.host_caps		= MMC_CAP_8_BIT_DATA,
#elif defined(CONFIG_EXYNOS4_MSHC_DDR)
	.host_caps		= MMC_CAP_1_8V_DDR | MMC_CAP_UHS_DDR50,
#endif
};
#endif /* CONFIG_S5P_DEV_MSHC */

#ifdef CONFIG_USB_EHCI_S5P
static struct s5p_ehci_platdata mixtile4x12_ehci_pdata;

static void __init mixtile4x12_ehci_init(void)
{
  struct s5p_ehci_platdata *pdata = &mixtile4x12_ehci_pdata;
  s5p_ehci_set_platdata(pdata);
}
#endif /* CONFIG_USB_EHCI_S5P */

#ifdef CONFIG_USB_OHCI_S5P
static struct s5p_ohci_platdata mixtile4x12_ohci_pdata;

static void __init mixtile4x12_ohci_init(void)
{
  struct s5p_ohci_platdata *pdata = &mixtile4x12_ohci_pdata;
  s5p_ohci_set_platdata(pdata);
}
#endif /* CONFIG_USB_OHCI_S5P */

/* USB GADGET */
#ifdef CONFIG_USB_GADGET
static struct s5p_usbgadget_platdata mixtile4x12_usbgadget_pdata;

static void __init mixtile4x12_usbgadget_init(void)
{
  struct s5p_usbgadget_platdata *pdata = &mixtile4x12_usbgadget_pdata;
  s5p_usbgadget_set_platdata(pdata);
}
#endif /* CONFIG_USB_GADGET */

/* USB POWER */
void usb_hub_power(int onoff)
{
    gpio_direction_output(GPIO_USBHUB_POWER, onoff);
}

void usb_host_power(int onoff)
{
  gpio_direction_output(GPIO_USB5V_POWER, onoff);
}

///* USB OTG POWER */
//void usb_otg_power(int onoff)
//{
//  printk("usb_otg_power %d\n", onoff);
//  gpio_direction_output(GPIO_USBOTG_POWER, onoff);
//}

#if defined(CONFIG_RTL8188CUS_MODULE) || defined(CONFIG_RTL8188EU_MODULE) 
void usb_wifi_power(int onoff)
{
  gpio_direction_output(GPIO_WIFI_POWER, onoff);
  printk("%s %s\n", __func__, (onoff? "on":"off"));
}
EXPORT_SYMBOL(usb_wifi_power);
#endif

void flash_led_power(int onoff, int mode)
{
  printk("flash_led_power %d %d\n", onoff, mode);
  gpio_direction_output(GPIO_FLASH_EN, onoff);
//  gpio_direction_output(GPIO_FLASH_MODE, 0);
  if(onoff == 0)
    s3c_gpio_cfgpin(GPIO_FLASH_EN, S3C_GPIO_SFN(2));
};

#ifdef CONFIG_LEDS_PWM
static struct led_pwm mixtile4x12_pwm_leds[] = {
	{
		.name		= "flashlight",
		.pwm_id		= 2,
		.max_brightness	= 255,
		.pwm_period_ns	= 700000,
	},
};

static struct led_pwm_platform_data mixtile4x12_pwm_data = {
	.num_leds	= ARRAY_SIZE(mixtile4x12_pwm_leds),
	.leds		= mixtile4x12_pwm_leds,
};

static struct platform_device mixtile4x12_leds_pwm = {
	.name	= "leds_pwm",
	.id	= -1,
	.dev	= {
		.platform_data = &mixtile4x12_pwm_data,
	},
};

void mixtile4x12_leds_pwm_init(void)
{
	platform_device_register(&s3c_device_timer[mixtile4x12_pwm_leds[0].pwm_id]);
};
#endif /* CONFIG_LEDS_PWM */

#ifdef GPIO_MOTOR_POWER
static struct timed_gpio timed_gpios[] = {
  {
    .name = "vibrator",
    .gpio = GPIO_MOTOR_POWER,
    .max_timeout = 15000,
    .active_low = 0,
  },
};

static struct timed_gpio_platform_data timed_gpio_data = {
  .num_gpios	= ARRAY_SIZE(timed_gpios),
  .gpios		= timed_gpios,
};

static struct platform_device mixtile4x12_timed_gpios = {
  .name   = "timed-gpio",
  .id     = -1,
  .dev    = {
    .platform_data = &timed_gpio_data,
  },
};
#endif /* GPIO_MOTOR_POWER */

static struct i2c_board_info i2c_devs0[] __initdata = {
#ifdef CONFIG_TOUCHSCREEN_GT81XX
	{	I2C_BOARD_INFO("Goodix-TS", 0x55),},
#endif
};

static struct i2c_board_info i2c_devs1[] __initdata = {
#ifdef CONFIG_VIDEO_TVOUT
  {
    I2C_BOARD_INFO("s5p_ddc", (0x74 >> 1)),
  },
#endif
#ifdef CONFIG_SND_SOC_WM8960
	{	I2C_BOARD_INFO("wm8960", 0x1A), },
#endif
};

static struct i2c_board_info i2c_devs2[] __initdata = {
};

static struct i2c_board_info i2c_devs3[] __initdata = {
#ifdef CONFIG_SENSORS_MMA8452
	{ I2C_BOARD_INFO("mma8452", 0x1D), },
#endif
};

#ifdef CONFIG_ANDROID_PMEM
static struct android_pmem_platform_data pmem_pdata = {
  .name    = "pmem",
  .no_allocator  = 1,
  .cached    = 0,
  .start    = 0,
  .size    = 0
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
  .name    = "pmem_gpu1",
  .no_allocator  = 1,
  .cached    = 0,
  .start    = 0,
  .size    = 0,
};

static struct platform_device pmem_device = {
  .name  = "android_pmem",
  .id  = 0,
  .dev  = {
    .platform_data = &pmem_pdata
  },
};

static struct platform_device pmem_gpu1_device = {
  .name  = "android_pmem",
  .id  = 1,
  .dev  = {
    .platform_data = &pmem_gpu1_pdata
  },
};

static void __init android_pmem_set_platdata(void)
{
#if defined(CONFIG_S5P_MEM_CMA)
  pmem_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K;
  pmem_gpu1_pdata.size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K;
#endif
}
#endif /* CONFIG_ANDROID_PMEM */

#ifdef CONFIG_KEYBOARD_GPIO
static struct gpio_keys_button mixtile_buttons[] = {
	{
		.gpio			= GPIO_POWER_KEY,
		.code			= KEY_POWER,
		.desc			= "Power",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
	{
		.gpio			= GPIO_VOLUMEUP_KEY,
		.code			= KEY_VOLUMEUP,
		.desc			= "VolumeUp",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
	{
		.gpio			= GPIO_VOLUMEDOWN_KEY,
		.code			= KEY_VOLUMEDOWN,
		.desc			= "VolumeDown",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
	{
		.gpio			= GPIO_MEDIA_KEY,
		.code			= KEY_MEDIA,
		.desc			= "HeadsetHook",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
	{
		.gpio			= GPIO_HOME_KEY,
		.code			= KEY_HOMEPAGE,
		.desc			= "Home",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
	{
		.gpio			= GPIO_MENU_KEY,
		.code			= KEY_MENU,
		.desc			= "MENU",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
	{
		.gpio			= GPIO_BACK_KEY,
		.code			= KEY_ESC,
		.desc			= "ESC",
		.active_low		= 1,
		.debounce_interval	= 5,
		.type     = EV_KEY,
	},
};

static struct gpio_keys_platform_data mixtile_buttons_data  = {
	.buttons	= mixtile_buttons,
	.nbuttons	= ARRAY_SIZE(mixtile_buttons),
};

static struct platform_device mixtile_buttons_device  = {
	.name		= "gpio-keys",
	.id		= 0,
	.num_resources	= 0,
	.dev		= {
		.platform_data	= &mixtile_buttons_data,
	}
};
#endif /* CONFIG_KEYBOARD_GPIO */

#ifdef CONFIG_WAKEUP_ASSIST
static struct platform_device wakeup_assist_device = {
  .name   = "wakeup_assist",
};
#endif /* CONFIG_WAKEUP_ASSIST */

#ifdef CONFIG_VIDEO_FIMG2D
static struct fimg2d_platdata fimg2d_data __initdata = {
  .hw_ver = 0x41,
  .parent_clkname = "mout_g2d0",
  .clkname = "sclk_fimg2d",
  .gate_clkname = "fimg2d",
  .clkrate = 201 * 1000000,  /* 200 Mhz */
};
#endif

#ifdef CONFIG_BUSFREQ_OPP
/* BUSFREQ to control memory/bus*/
static struct device_domain busfreq;
#endif /* CONFIG_BUSFREQ_OPP */

static struct platform_device exynos4_busfreq = {
  .id = -1,
  .name = "exynos-busfreq",
};

#ifdef CONFIG_BATTERY_MIXTILE
static struct s3c_adc_bat_pdata mixtile4x12_bat_cfg = {
  .volt_channel = 0,
};

static struct platform_device mixtile4x12_device_battery = {
	.name	= "mixtile_adc_bat",
	.id	= -1,
	.dev = {
	   .parent = &s3c_device_adc.dev,
	   .platform_data = &mixtile4x12_bat_cfg,
	 },

};
#endif

#ifdef CONFIG_DM9000
#define DM9000_PA (EXYNOS4_PA_SROM_BANK(1) + 0x300)
static struct resource mixtile4x12_dm9000_resources[] = {
	[0] = {
		.start	= DM9000_PA,
		.end	= DM9000_PA + 3,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= DM9000_PA + 8,          //16 bit +8; 8 bit +4
		.end	= DM9000_PA + 8 + 3,
		.flags	= IORESOURCE_MEM,
	},
	[2] = {
		.start	= GPIO_DM9000_IRQ,
		.end	= GPIO_DM9000_IRQ,
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL,
	},
};

static struct dm9000_plat_data mixtile4x12_dm9000_platdata = {
	.flags		= DM9000_PLATF_16BITONLY | DM9000_PLATF_NO_EEPROM,
	.dev_addr	= { 0x00, 0x09, 0xc0, 0xff, 0xec, 0x48 },
};

struct platform_device mixtile4x12_dm9000 = {
	.name		= "dm9000",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(mixtile4x12_dm9000_resources),
	.resource	= mixtile4x12_dm9000_resources,
	.dev		= {
		.platform_data	= &mixtile4x12_dm9000_platdata,
	},
};

static void __init mixtile4x12_dm9000_init(void)
{
	u32 tmp;
	//电平转换芯片使能
//	gpio_request(GPIO_DM9000_CS0, "GPIO_DM9000_CS0");
//	s3c_gpio_cfgpin(GPIO_DM9000_CS0, S3C_GPIO_SFN(1));
//	gpio_free(GPIO_DM9000_CS0);
//	gpio_direction_output(GPIO_DM9000_CS0, 0);//CS0 -> 0
	//DM9000 时序
	tmp = (2<<4)|(2<<8)|(2<<12)|(3<<16)|(2<<24)|(2<<28);
	__raw_writel(tmp, S5P_SROM_BC1);
	//SROM_BW 配置
	tmp = __raw_readl(S5P_SROM_BW);
	tmp &= ~(S5P_SROM_BW__CS_MASK << S5P_SROM_BW__NCS1__SHIFT);
	
	tmp |= (1 << S5P_SROM_BW__NCS1__SHIFT);
	__raw_writel(tmp, S5P_SROM_BW);


  //电源
  //gpio_direction_output(GPIO_DM9000_POWER, 1);		     
}
#endif /* CONFIG_DM9000 */

static void gps_gpio_init(void)
{
	struct device *gps_dev;

	gps_dev = device_create(sec_class, NULL, 0, NULL, "gps");
	if (IS_ERR(gps_dev)) {
		pr_err("Failed to create device(gps)!\n");
		goto err;
	}
	gpio_direction_output(GPIO_GPS_POWER, 0);
	gpio_direction_output(GPIO_GPS_EN, 0);

	s3c_gpio_setpull(GPIO_GPS_RXD, S3C_GPIO_PULL_UP);
//	s3c_gpio_setpull(GPIO_GPS_RTS, S3C_GPIO_PULL_UP);
	
	gpio_export(GPIO_GPS_EN, 1);
	gpio_export(GPIO_GPS_POWER, 1);
	gpio_export_link(gps_dev, "GPS_nRST", GPIO_GPS_EN);
	gpio_export_link(gps_dev, "GPS_PWR_EN", GPIO_GPS_EN);
	gpio_export_link(gps_dev, "GPS_ENABLE", GPIO_GPS_POWER);

 err:
	return;
}

static struct platform_device *smdk4412_devices[] __initdata = {
};

static struct platform_device *mixtile4x12_devices[] __initdata = {
#ifdef CONFIG_ANDROID_PMEM
  &pmem_device,
  &pmem_gpu1_device,
#endif
  /* Samsung Power Domain */
  &exynos4_device_pd[PD_MFC],
  &exynos4_device_pd[PD_G3D],
  &exynos4_device_pd[PD_LCD0],
  &exynos4_device_pd[PD_CAM],
  &exynos4_device_pd[PD_TV],
  &exynos4_device_pd[PD_GPS],
  &exynos4_device_pd[PD_GPS_ALIVE],

#ifdef GPIO_MOTOR_POWER
  &mixtile4x12_timed_gpios,
#endif
#ifdef CONFIG_KEYBOARD_GPIO
  &mixtile_buttons_device,
#endif
  /* legacy fimd */
#ifdef CONFIG_FB_S5P
  &s3c_device_fb,
#endif
  &s3c_device_wdt,
  &s3c_device_rtc,
  &s3c_device_i2c0,
  &s3c_device_i2c1,
  &s3c_device_i2c2,
  &s3c_device_i2c3,
  &s3c_device_i2c4,
  &s3c_device_i2c5,
  &s3c_device_adc,
#ifdef CONFIG_SWITCH_HEADPHONE
  &hp_switch_device,
#endif

#ifdef CONFIG_USB_EHCI_S5P
  &s5p_device_ehci,
#endif
#ifdef CONFIG_USB_OHCI_S5P
  &s5p_device_ohci,
#endif
#ifdef CONFIG_USB_GADGET
  &s3c_device_usbgadget,
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
  &s3c_device_rndis,
#endif
#ifdef CONFIG_USB_ANDROID
  &s3c_device_android_usb,
  &s3c_device_usb_mass_storage,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC
  &s3c_device_hsmmc0,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
  &s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
  &s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
  &s3c_device_hsmmc3,
#endif
#ifdef CONFIG_S5P_DEV_MSHC
  &s3c_device_mshci,
#endif
#ifdef CONFIG_SND_SAMSUNG_I2S
  &exynos_device_i2s0,
#endif
#ifdef CONFIG_SND_SAMSUNG_PCM
  &exynos_device_pcm0,
#endif
#ifdef CONFIG_SND_SAMSUNG_SPDIF
  &exynos_device_spdif,
#endif
#if defined(CONFIG_SND_SAMSUNG_RP) || defined(CONFIG_SND_SAMSUNG_ALP)
  &exynos_device_srp,
#endif
#ifdef CONFIG_VIDEO_TVOUT
  &s5p_device_tvout,
  &s5p_device_cec,
  &s5p_device_hpd,
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TV
  &s5p_device_i2c_hdmiphy,
  &s5p_device_hdmi,
  &s5p_device_sdo,
  &s5p_device_mixer,
  &s5p_device_cec,
#endif
#if defined(CONFIG_VIDEO_FIMC)
  &s3c_device_fimc0,
  &s3c_device_fimc1,
  &s3c_device_fimc2,
  &s3c_device_fimc3,
#endif

#if defined(CONFIG_VIDEO_MFC5X)
  &s5p_device_mfc,
#endif
#ifdef CONFIG_S5P_SYSTEM_MMU
  &SYSMMU_PLATDEV(g2d_acp),
  &SYSMMU_PLATDEV(fimc0),
  &SYSMMU_PLATDEV(fimc1),
  &SYSMMU_PLATDEV(fimc2),
  &SYSMMU_PLATDEV(fimc3),
  &SYSMMU_PLATDEV(jpeg),
  &SYSMMU_PLATDEV(mfc_l),
  &SYSMMU_PLATDEV(mfc_r),
  &SYSMMU_PLATDEV(tv),
#endif /* CONFIG_S5P_SYSTEM_MMU */
#ifdef CONFIG_ION_EXYNOS
  &exynos_device_ion,
#endif
#ifdef CONFIG_VIDEO_FIMG2D
  &s5p_device_fimg2d,
#endif
#ifdef CONFIG_EXYNOS_MEDIA_DEVICE
  &exynos_device_md0,
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
  &s5p_device_jpeg,
#endif
  &samsung_asoc_dma,
  &samsung_asoc_idma,
#ifdef CONFIG_BATTERY_MIXTILE
	&mixtile4x12_device_battery,
#endif
#ifdef CONFIG_DM9000
  &mixtile4x12_dm9000,
#endif
#ifdef CONFIG_WAKEUP_ASSIST
  &wakeup_assist_device,
#endif
#ifdef CONFIG_EXYNOS_SETUP_THERMAL
  &exynos_device_tmu,
#endif
#ifdef CONFIG_S5P_DEV_ACE
  &s5p_device_ace,
#endif
  &exynos4_busfreq,
#ifdef CONFIG_LEDS_PWM
  &mixtile4x12_leds_pwm,
#endif
};

#ifdef CONFIG_EXYNOS_SETUP_THERMAL
/* below temperature base on the celcius degree */
struct tmu_data exynos_tmu_data __initdata = {
  .ts = {
    .stop_throttle  = 82,
    .start_throttle = 85,
    .stop_warning  = 95,
    .start_warning = 103,
    .start_tripping = 110, /* temp to do tripping */
  },
  .efuse_value = 55,
  .slope = 0x10008802,
  .mode = 0,
};
#endif

#if defined(CONFIG_VIDEO_TVOUT)
static struct s5p_platform_hpd hdmi_hpd_data __initdata = {

};
static struct s5p_platform_cec hdmi_cec_data __initdata = {

};
#endif

#if defined(CONFIG_S5P_MEM_CMA)
static void __init exynos4_cma_region_reserve(struct cma_region *regions_normal, struct cma_region *regions_secure)
{
  struct cma_region *reg;
  phys_addr_t paddr_last = 0xFFFFFFFF;

  for (reg = regions_normal; reg->size != 0; reg++) {
    phys_addr_t paddr;

    if (!IS_ALIGNED(reg->size, PAGE_SIZE)) {
      pr_err("S5P/CMA: size of '%s' is NOT page-aligned\n",  reg->name);
      reg->size = PAGE_ALIGN(reg->size);
    }

    if (reg->reserved) {
      pr_err("S5P/CMA: '%s' alread reserved\n", reg->name);
      continue;
    }

    if (reg->alignment) {
      if ((reg->alignment & ~PAGE_MASK) ||
        (reg->alignment & ~reg->alignment)) {
        pr_err("S5P/CMA: Failed to reserve '%s': incorrect alignment 0x%08x.\n", reg->name, reg->alignment);
        continue;
      }
    } else {
      reg->alignment = PAGE_SIZE;
    }

    if (reg->start) {
      if (!memblock_is_region_reserved(reg->start, reg->size) && (memblock_reserve(reg->start, reg->size) == 0))
        reg->reserved = 1;
      else
        pr_err("S5P/CMA: Failed to reserve '%s'\n",  reg->name);

      continue;
    }

    paddr = memblock_find_in_range(0, MEMBLOCK_ALLOC_ACCESSIBLE,
            reg->size, reg->alignment);
    if (paddr != MEMBLOCK_ERROR) {
      if (memblock_reserve(paddr, reg->size)) {
        pr_err("S5P/CMA: Failed to reserve '%s'\n",  reg->name);
        continue;
      }

      reg->start = paddr;
      reg->reserved = 1;
    } else {
      pr_err("S5P/CMA: No free space in memory for '%s'\n",  reg->name);
    }

    if (cma_early_region_register(reg)) {
      pr_err("S5P/CMA: Failed to register '%s'\n", reg->name);
      memblock_free(reg->start, reg->size);
    } else {
      paddr_last = min(paddr, paddr_last);
    }
  }

  if (regions_secure && regions_secure->size) {
    size_t size_secure = 0;
    size_t align_secure, size_region2, aug_size, order_region2;

    for (reg = regions_secure; reg->size != 0; reg++)
      size_secure += reg->size;

    reg--;

    /* Entire secure regions will be merged into 2
     * consecutive regions. */
    align_secure = 1 <<
      (get_order((size_secure + 1) / 2) + PAGE_SHIFT);
    /* Calculation of a subregion size */
    size_region2 = size_secure - align_secure;
    order_region2 = get_order(size_region2) + PAGE_SHIFT;
    if (order_region2 < 20)
      order_region2 = 20; /* 1MB */
    order_region2 -= 3; /* divide by 8 */
    size_region2 = ALIGN(size_region2, 1 << order_region2);

    aug_size = align_secure + size_region2 - size_secure;
    if (aug_size > 0)
      reg->size += aug_size;

    size_secure = ALIGN(size_secure, align_secure);

    if (paddr_last >= memblock.current_limit) {
      paddr_last = memblock_find_in_range(0, MEMBLOCK_ALLOC_ACCESSIBLE, size_secure, reg->alignment);
    } else {
      paddr_last -= size_secure;
      paddr_last = round_down(paddr_last, align_secure);
    }

    if (paddr_last) {
      while (memblock_reserve(paddr_last, size_secure))
        paddr_last -= align_secure;

      do {
        reg->start = paddr_last;
        reg->reserved = 1;
        paddr_last += reg->size;

        if (cma_early_region_register(reg)) {
          memblock_free(reg->start, reg->size);
          pr_err("S5P/CMA: Failed to register secure region '%s'\n", reg->name);
        } else {
          size_secure -= reg->size;
        }
      } while (reg-- != regions_secure);

      if (size_secure > 0)
        memblock_free(paddr_last, size_secure);
    } else {
      pr_err("S5P/CMA: Failed to reserve secure regions\n");
    }
  }
}

static void __init exynos4_reserve_mem(void)
{
  static struct cma_region regions[] = {
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM
    {
      .name = "pmem",
      .size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K,
      .start = 0,
    },
#endif
#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1
    {
      .name = "pmem_gpu1",
      .size = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K,
      .start = 0,
    },
#endif
#ifndef CONFIG_VIDEOBUF2_ION
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV
    {
      .name = "tv",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_TV * SZ_1K,
      .start = 0
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG
    {
      .name = "jpeg",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_JPEG * SZ_1K,
      .start = 0
    },
#endif
#ifdef CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP
    {
      .name = "srp",
      .size = CONFIG_AUDIO_SAMSUNG_MEMSIZE_SRP * SZ_1K,
      .start = 0,
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D
    {
      .name = "fimg2d",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMG2D * SZ_1K,
      .start = 0
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
    {
      .name = "fimd",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
      .start = 0
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
    {
      .name = "fimc0",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0 * SZ_1K,
      .start = 0
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2
    {
      .name = "fimc2",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC2 * SZ_1K,
      .start = 0
    },
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
  defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3)
    {
      .name = "fimc3",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1
    {
      .name = "fimc1",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC1 * SZ_1K,
      .start = 0
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1
    {
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
      .name = "mfc-normal",
#else
      .name = "mfc1",
#endif
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC1 * SZ_1K,
      { .alignment = 1 << 17 },
    },
#endif
#if !defined(CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION) && \
  defined(CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0)
    {
      .name = "mfc0",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
      { .alignment = 1 << 17 },
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC
    {
      .name = "mfc",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC * SZ_1K,
      { .alignment = 1 << 17 },
    },
#endif
#else /* !CONFIG_VIDEOBUF2_ION */
#ifdef CONFIG_FB_S5P
#error CONFIG_FB_S5P is defined. Select CONFIG_FB_S3C, instead
#endif
    {
      .name  = "ion",
      .size  = CONFIG_ION_EXYNOS_CONTIGHEAP_SIZE * SZ_1K,
    },
#endif /* !CONFIG_VIDEOBUF2_ION */
    {
      .size = 0
    },
  };
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
  static struct cma_region regions_secure[] = {
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD_VIDEO
    {
      .name = "video",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD_VIDEO * SZ_1K,
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3
    {
      .name = "fimc3",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC3 * SZ_1K,
    },
#endif
#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0
    {
      .name = "mfc-secure",
      .size = CONFIG_VIDEO_SAMSUNG_MEMSIZE_MFC0 * SZ_1K,
    },
#endif
    {
      .name = "sectbl",
      .size = SZ_1M,
      {
        .alignment = SZ_64M,
      },
    },
    {
      .size = 0
    },
  };
#else /* !CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION */
  struct cma_region *regions_secure = NULL;
#endif
  static const char map[] __initconst =
    "android_pmem.0=pmem;android_pmem.1=pmem_gpu1;"
    "s3cfb.0/fimd=fimd;exynos4-fb.0/fimd=fimd;"
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
    "s3cfb.0/video=video;exynos4-fb.0/video=video;"
#endif
    "s3c-fimc.0=fimc0;s3c-fimc.1=fimc1;s3c-fimc.2=fimc2;s3c-fimc.3=fimc3;"
    "exynos4210-fimc.0=fimc0;exynos4210-fimc.1=fimc1;exynos4210-fimc.2=fimc2;exynos4210-fimc.3=fimc3;"
#ifdef CONFIG_VIDEO_MFC5X
    "s3c-mfc/A=mfc0,mfc-secure;"
    "s3c-mfc/B=mfc1,mfc-normal;"
    "s3c-mfc/AB=mfc;"
#endif
    "samsung-rp=srp;"
    "s5p-jpeg=jpeg;"
    "exynos4-fimc-is/f=fimc_is;"
    "s5p-mixer=tv;"
    "s5p-fimg2d=fimg2d;"
    "ion-exynos=ion,fimd,fimc0,fimc1,fimc2,fimc3,fw,b1,b2;"
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
    "s5p-smem/video=video;"
    "s5p-smem/sectbl=sectbl;"
#endif
    "s5p-smem/mfc=mfc0,mfc-secure;"
    "s5p-smem/fimc=fimc3;"
    "s5p-smem/mfc-shm=mfc1,mfc-normal;";

  cma_set_defaults(NULL, map);

  exynos4_cma_region_reserve(regions, regions_secure);
}
#endif

/* LCD Backlight data */
static struct samsung_bl_gpio_info mixtile4x12_bl_gpio_info = {
  .no = GPIO_LCD_PWM,
  .func = S3C_GPIO_SFN(2),
};

static struct platform_pwm_backlight_data mixtile4x12_bl_data = {
  .pwm_id = 0,
	.pwm_period_ns  = 52631,
	.max_brightness = 255,
	.dft_brightness = 255,
	.lth_brightness = 0,
};

static void __init mixtile4x12_map_io(void)
{
  clk_xusbxti.rate = 24000000;
  s5p_init_io(NULL, 0, S5P_VA_CHIPID);
  s3c24xx_init_clocks(24000000);
  s3c24xx_init_uarts(mixtile4x12_uartcfgs, ARRAY_SIZE(mixtile4x12_uartcfgs));

#if defined(CONFIG_S5P_MEM_CMA)
  exynos4_reserve_mem();
#endif
}

static void __init exynos_sysmmu_init(void)
{
  ASSIGN_SYSMMU_POWERDOMAIN(fimc0, &exynos4_device_pd[PD_CAM].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(fimc1, &exynos4_device_pd[PD_CAM].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(fimc2, &exynos4_device_pd[PD_CAM].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(fimc3, &exynos4_device_pd[PD_CAM].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(jpeg, &exynos4_device_pd[PD_CAM].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(mfc_l, &exynos4_device_pd[PD_MFC].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(mfc_r, &exynos4_device_pd[PD_MFC].dev);
  ASSIGN_SYSMMU_POWERDOMAIN(tv, &exynos4_device_pd[PD_TV].dev);
#ifdef CONFIG_VIDEO_FIMG2D
  sysmmu_set_owner(&SYSMMU_PLATDEV(g2d_acp).dev, &s5p_device_fimg2d.dev);
#endif
#if defined(CONFIG_VIDEO_MFC5X)
  sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_l).dev, &s5p_device_mfc.dev);
  sysmmu_set_owner(&SYSMMU_PLATDEV(mfc_r).dev, &s5p_device_mfc.dev);
#endif
#if defined(CONFIG_VIDEO_FIMC)
  sysmmu_set_owner(&SYSMMU_PLATDEV(fimc0).dev, &s3c_device_fimc0.dev);
  sysmmu_set_owner(&SYSMMU_PLATDEV(fimc1).dev, &s3c_device_fimc1.dev);
  sysmmu_set_owner(&SYSMMU_PLATDEV(fimc2).dev, &s3c_device_fimc2.dev);
  sysmmu_set_owner(&SYSMMU_PLATDEV(fimc3).dev, &s3c_device_fimc3.dev);
#endif
#ifdef CONFIG_VIDEO_EXYNOS_TV
  sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_mixer.dev);
#endif
#ifdef CONFIG_VIDEO_TVOUT
  sysmmu_set_owner(&SYSMMU_PLATDEV(tv).dev, &s5p_device_tvout.dev);
#endif
#ifdef CONFIG_VIDEO_JPEG_V2X
  sysmmu_set_owner(&SYSMMU_PLATDEV(jpeg).dev, &s5p_device_jpeg.dev);
#endif
}

static void mixtile4x12_switch_init(void)
{
	sec_class = class_create(THIS_MODULE, "sec");

	if (IS_ERR(sec_class))
		pr_err("Failed to create class(sec)!\n");

	switch_dev = device_create(sec_class, NULL, 0, NULL, "switch");

	if (IS_ERR(switch_dev))
		pr_err("Failed to create device(switch)!\n");
};

void tp_power(int onoff)
{
printk("%s %d\n", __func__, onoff);
    gpio_direction_output(GPIO_TP_POWER, onoff);
#ifdef CONFIG_TOUCHSCREEN_GT81XX
  if(onoff)
  {
    gpio_direction_output(GPIO_TP_RST, 0);
    msleep(5);
    gpio_direction_output(GPIO_TP_RST, 1);
    msleep(5);
  }
  else
    gpio_direction_output(GPIO_TP_RST, 0);
#endif /* CONFIG_TOUCHSCREEN_GT81XX */
}
EXPORT_SYMBOL(tp_power);

#define REBOOT_MODE_PREFIX	0x12345670
#define REBOOT_MODE_NONE	0
#define REBOOT_MODE_DOWNLOAD	1
#define REBOOT_MODE_UPLOAD	2
#define REBOOT_MODE_CHARGING	3
#define REBOOT_MODE_RECOVERY	4
#define REBOOT_MODE_ARM11_FOTA	5

#define REBOOT_SET_PREFIX	0xabc00000
#define REBOOT_SET_DEBUG	0x000d0000
#define REBOOT_SET_SWSEL	0x000e0000
#define REBOOT_SET_SUD		0x000f0000
static void mixtile4x12_reboot(char str, const char *cmd)
{
 	printk("mixtile4x12_reboot %s\n", cmd ? cmd : "(null)");
	local_irq_disable();
	writel(0x12345678, S5P_INFORM2);	/* Don't enter lpm mode */

	if (!cmd) {
		writel(REBOOT_MODE_PREFIX | REBOOT_MODE_NONE, S5P_INFORM2);
	} else {
		if (!strcmp(cmd, "recovery"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_RECOVERY, S5P_INFORM2);
		else if (!strcmp(cmd, "fota"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_ARM11_FOTA, S5P_INFORM2);
		else if (!strcmp(cmd, "download"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_DOWNLOAD, S5P_INFORM2);
		else if (!strcmp(cmd, "upload"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_UPLOAD, S5P_INFORM2);
		else if (!strcmp(cmd, "chargeing"))
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_CHARGING, S5P_INFORM2);
		else
			writel(REBOOT_MODE_PREFIX | REBOOT_MODE_NONE, S5P_INFORM2);
	}

	flush_cache_all();
	outer_flush_all();
	arch_reset(0, 0);

	pr_emerg("%s: waiting for reboot\n", __func__);
	while (1)
	  msleep(1000);
}

static void mixtile4x12_power_off(void)
{
  int state1, state2;
  state1 = gpio_get_value(GPIO_CHARGE_STATE1);
  state2 = gpio_get_value(GPIO_CHARGE_STATE2);

  if((state1==0) || (state2==0))
    mixtile4x12_reboot('h', "chargeing");
  local_irq_disable();
 	/* PS_HOLD low*/
 	printk("mixtile4x12_power_off %d %d\n", state1, state2);
 	while(1)
 	{
    writel(readl(EXYNOS4_PS_HOLD_CONTROL) & (~(0x1<<8)), EXYNOS4_PS_HOLD_CONTROL);
    msleep(1000);
  }
}

static int mixtile4x12_get_ver(int channel)
{
  return 0;
}

static void __init mixtile4x12_machine_init(void)
{
  dev_ver = mixtile4x12_get_ver(3);
	mixtile4x12_config_gpio_table();

	pm_power_off = mixtile4x12_power_off;
	arm_pm_restart = mixtile4x12_reboot;
#if defined(CONFIG_EXYNOS_DEV_PD) && defined(CONFIG_PM_RUNTIME)
  exynos_pd_disable(&exynos4_device_pd[PD_MFC].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_G3D].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_LCD0].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_CAM].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_TV].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_GPS].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
  exynos_pd_disable(&exynos4_device_pd[PD_ISP].dev);
#elif defined(CONFIG_EXYNOS_DEV_PD)
  /*
   * These power domains should be always on
   * without runtime pm support.
   */
  exynos_pd_enable(&exynos4_device_pd[PD_MFC].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_G3D].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_LCD0].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_CAM].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_TV].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_GPS].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_GPS_ALIVE].dev);
  exynos_pd_enable(&exynos4_device_pd[PD_ISP].dev);
#endif
  s3c_i2c0_set_platdata(NULL);
  i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));

  s3c_i2c1_set_platdata(NULL);
  i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

  s3c_i2c2_set_platdata(NULL);
  i2c_register_board_info(2, i2c_devs2, ARRAY_SIZE(i2c_devs2));

  s3c_i2c3_set_platdata(NULL);
  i2c_register_board_info(3, i2c_devs3, ARRAY_SIZE(i2c_devs3));

  s3c_i2c4_set_platdata(NULL);
  s3c_i2c5_set_platdata(NULL);

#ifdef CONFIG_ANDROID_PMEM
  android_pmem_set_platdata();
#endif
#ifdef CONFIG_FB_S5P
  s3cfb_set_platdata(NULL);
#ifdef CONFIG_EXYNOS_DEV_PD
  s3c_device_fb.dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif
#ifdef CONFIG_USB_EHCI_S5P
  mixtile4x12_ehci_init();
#endif
#ifdef CONFIG_USB_OHCI_S5P
  mixtile4x12_ohci_init();
#endif
#ifdef CONFIG_USB_GADGET
  mixtile4x12_usbgadget_init();
#endif

  samsung_bl_set(&mixtile4x12_bl_gpio_info, &mixtile4x12_bl_data);

#ifdef CONFIG_DM9000
  mixtile4x12_dm9000_init();
#endif

#ifdef CONFIG_S3C_DEV_HSMMC
  s3c_sdhci0_set_platdata(&mixtile4x12_hsmmc0_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC1
  s3c_sdhci1_set_platdata(&mixtile4x12_hsmmc1_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC2
  s3c_sdhci2_set_platdata(&mixtile4x12_hsmmc2_pdata);
#endif
#ifdef CONFIG_S3C_DEV_HSMMC3
  s3c_sdhci3_set_platdata(&mixtile4x12_hsmmc3_pdata);
#endif
#ifdef CONFIG_S5P_DEV_MSHC
  s3c_mshci_set_platdata(&exynos4_mshc_pdata);
#endif
#if defined(CONFIG_VIDEO_EXYNOS_TV) && defined(CONFIG_VIDEO_EXYNOS_HDMI)
  dev_set_name(&s5p_device_hdmi.dev, "exynos4-hdmi");
  clk_add_alias("hdmi", "s5p-hdmi", "hdmi", &s5p_device_hdmi.dev);
  clk_add_alias("hdmiphy", "s5p-hdmi", "hdmiphy", &s5p_device_hdmi.dev);

  s5p_tv_setup();

  /* setup dependencies between TV devices */
  s5p_device_hdmi.dev.parent = &exynos4_device_pd[PD_TV].dev;
  s5p_device_mixer.dev.parent = &exynos4_device_pd[PD_TV].dev;

  s5p_i2c_hdmiphy_set_platdata(NULL);
#endif
#ifdef CONFIG_EXYNOS_SETUP_THERMAL
  s5p_tmu_set_platdata(&exynos_tmu_data);
#endif
#ifdef CONFIG_VIDEO_FIMC
  if(dev_ver == 1)
  {
  	s3c_fimc0_set_platdata(&fimc_plat_htpc);
  	s3c_fimc1_set_platdata(NULL);
  	s3c_fimc2_set_platdata(&fimc_plat_htpc);
  	s3c_fimc3_set_platdata(NULL);
  }
  else
  {
    s3c_fimc0_set_platdata(&fimc_plat);
    s3c_fimc1_set_platdata(&fimc_plat);
    s3c_fimc2_set_platdata(NULL);
    s3c_fimc3_set_platdata(NULL);
  }
#ifdef CONFIG_EXYNOS_DEV_PD
  s3c_device_fimc0.dev.parent = &exynos4_device_pd[PD_CAM].dev;
  s3c_device_fimc1.dev.parent = &exynos4_device_pd[PD_CAM].dev;
  s3c_device_fimc2.dev.parent = &exynos4_device_pd[PD_CAM].dev;
  s3c_device_fimc3.dev.parent = &exynos4_device_pd[PD_CAM].dev;
#ifdef CONFIG_EXYNOS4_CONTENT_PATH_PROTECTION
  secmem.parent = &exynos4_device_pd[PD_CAM].dev;
#endif
#endif

//#if defined(CONFIG_ITU_B)
	mixtile4x12_cam_b_power(1);
	msleep(50);
	mixtile4x12_cam_b_power(0);
//#endif
//#if defined(CONFIG_ITU_A)
	mixtile4x12_cam_f_power(1);
	msleep(50);
	mixtile4x12_cam_f_power(0);
//#endif
//#ifdef CONFIG_VIDEO_TVP5151
  tvp5151_power(1);
	msleep(50);
  tvp5151_power(0);
//#endif
#endif /* CONFIG_VIDEO_FIMC */

#if defined(CONFIG_VIDEO_TVOUT)
  s5p_hdmi_hpd_set_platdata(&hdmi_hpd_data);
  s5p_hdmi_cec_set_platdata(&hdmi_cec_data);
#ifdef CONFIG_EXYNOS_DEV_PD
  s5p_device_tvout.dev.parent = &exynos4_device_pd[PD_TV].dev;
  exynos4_device_pd[PD_TV].dev.parent = &exynos4_device_pd[PD_LCD0].dev;
#endif
#endif

#ifdef CONFIG_VIDEO_JPEG_V2X
#ifdef CONFIG_EXYNOS_DEV_PD
  s5p_device_jpeg.dev.parent = &exynos4_device_pd[PD_CAM].dev;
	if (samsung_rev() == EXYNOS4412_REV_2_0)
		exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 176000000);
	else
  exynos4_jpeg_setup_clock(&s5p_device_jpeg.dev, 160000000);
#endif
#endif

#ifdef CONFIG_ION_EXYNOS
  exynos_ion_set_platdata();
#endif

#if defined(CONFIG_VIDEO_MFC5X)
#ifdef CONFIG_EXYNOS_DEV_PD
  s5p_device_mfc.dev.parent = &exynos4_device_pd[PD_MFC].dev;
#endif
	if (soc_is_exynos4412() && samsung_rev() >= EXYNOS4412_REV_2_0)
		exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 220 * MHZ);
	else if ((soc_is_exynos4412() && samsung_rev() >= EXYNOS4412_REV_1_0) ||
		(soc_is_exynos4212() && samsung_rev() >= EXYNOS4212_REV_1_0))
    exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 200 * MHZ);
  else
    exynos4_mfc_setup_clock(&s5p_device_mfc.dev, 267 * MHZ);
#endif

#ifdef CONFIG_VIDEO_FIMG2D
  s5p_fimg2d_set_platdata(&fimg2d_data);
#endif

#if defined(CONFIG_RTL8188CUS_MODULE) || defined(CONFIG_RTL8188EU_MODULE) 
  usb_wifi_power(0);
#endif
  if(dev_ver==0)
  {
    usb_hub_power(1);
    msleep(200);
    usb_host_power(1);
    msleep(200);
  }
  mixtile4x12_switch_init();
  
  gps_gpio_init();
  exynos_sysmmu_init();

  platform_add_devices(mixtile4x12_devices, ARRAY_SIZE(mixtile4x12_devices));
  if (soc_is_exynos4412())
    platform_add_devices(smdk4412_devices, ARRAY_SIZE(smdk4412_devices));

#ifdef CONFIG_BUSFREQ_OPP
  dev_add(&busfreq, &exynos4_busfreq.dev);
  ppmu_init(&exynos_ppmu[PPMU_DMC0], &exynos4_busfreq.dev);
  ppmu_init(&exynos_ppmu[PPMU_DMC1], &exynos4_busfreq.dev);
  ppmu_init(&exynos_ppmu[PPMU_CPU], &exynos4_busfreq.dev);
#endif
  register_reboot_notifier(&exynos4_reboot_notifier);

#ifdef CONFIG_LEDS_PWM
  mixtile4x12_leds_pwm_init();
#endif
}

MACHINE_START(SMDK4212, "SMDK4X12")
  .boot_params  = S5P_PA_SDRAM + 0x100,
  .init_irq  = exynos4_init_irq,
  .map_io    = mixtile4x12_map_io,
  .init_machine  = mixtile4x12_machine_init,
  .timer    = &exynos4_timer,
MACHINE_END

MACHINE_START(SMDK4412, "SMDK4X12")
  .boot_params  = S5P_PA_SDRAM + 0x100,
  .init_irq  = exynos4_init_irq,
  .map_io    = mixtile4x12_map_io,
  .init_machine  = mixtile4x12_machine_init,
  .timer    = &exynos4_timer,
MACHINE_END
