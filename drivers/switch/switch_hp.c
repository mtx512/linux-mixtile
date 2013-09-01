/*
 *  drivers/switch/switch_gpio.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Author: Mike Lockwood <lockwood@android.com>
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

/*
androd 中对各类设备值的定义
    private static final int BIT_HEADSET = (1 << 0);            带MIC的耳机
    private static final int BIT_HEADSET_NO_MIC = (1 << 1);     不带MIC的耳机
    private static final int BIT_USB_HEADSET_ANLG = (1 << 2);   模拟USB耳机
    private static final int BIT_USB_HEADSET_DGTL = (1 << 3);   数字USB耳机
    private static final int BIT_HDMI_AUDIO = (1 << 4);         HDMI音频
    private static final int HEADSETS_WITH_MIC = BIT_HEADSET;
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/switch.h>
#include <linux/workqueue.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <asm/io.h>
#include <plat/gpio-cfg.h>
#include <plat/adc.h>

#ifdef CONFIG_SND_SOC_WM8960
extern bool wm8960_setbias(int onoff);
#endif /* CONFIG_SND_SOC_WM8960 */
extern int dev_ver;
static struct hp_switch_data *my_switch_data;
static struct s3c_adc_client	*client = NULL;
bool head_mic_connect = true; //默认为真，为了在启动后没有任何操作时，系统没有打开偏置电压直接按耳机的hook键，系统无反应的情况
unsigned long head_mic_connet_time = 0;

struct hp_switch_data {
	struct switch_dev sdev;
	unsigned gpio;
	const char *name_on;
	const char *name_off;
	int irq;
	struct work_struct work;
};

bool check_headset_mic(void)
{
	int adc_value;
#ifdef CONFIG_SND_SOC_WM8960
  adc_value = 8;
  while(adc_value-->0)
  {
    if(wm8960_setbias(1))
      break;
    msleep(500);
  }
#endif
  if(client == NULL)
  {
    printk("client == NULL\n");
    return false;
  }
  adc_value = s3c_adc_read(client, 2);
  if(adc_value == 0)
  {
    msleep(500);
    adc_value = s3c_adc_read(client, 2);
  }
//printk("adc = %04d \n", adc_value);
  if(adc_value>0)
    return true;
  
  return false;
}

static void hp_switch_work(struct work_struct *work)
{
	int state;
	int gpio_value;
	struct hp_switch_data	*data = container_of(work, struct hp_switch_data, work);

  msleep(500);
	gpio_value = gpio_get_value(data->gpio);
	if(dev_ver == 1) //HTPC
	  gpio_value = 0;

  if(!gpio_value)
  {
    printk("HP pull in\n");
    gpio_value = 2;
 
    if(check_headset_mic() && (dev_ver != 1))
    {
      gpio_value = 1;
      head_mic_connect = true;
      head_mic_connet_time = jiffies;
//printk("set head_mic_connect true\n");
    }
    else
    {
      head_mic_connect = false;
//printk("set head_mic_connect false\n");
    }
  }
  else
  {
    printk("HP pull out\n");
    gpio_value = 0;
    head_mic_connet_time = jiffies;
    head_mic_connect = false;
//printk("set head_mic_connect false\n");
  }

  state = gpio_value;
	switch_set_state(&data->sdev, state);
}

static irqreturn_t hp_irq_handler(int irq, void *dev_id)
{
	struct hp_switch_data *switch_data = (struct hp_switch_data *)dev_id;

	schedule_work(&switch_data->work);
	return IRQ_HANDLED;
}

//extern void dump_regs(void);
static ssize_t switch_hp_print_state(struct switch_dev *sdev, char *buf)
{
  int state;

//  dump_regs();
  state = switch_get_state(sdev);
	if (state)
		return sprintf(buf, "%d\n", state);
	return -1;
}

static int hp_switch_probe(struct platform_device *pdev)
{
	struct gpio_switch_platform_data *pdata = pdev->dev.platform_data;
	struct hp_switch_data *switch_data;
	int ret = 0;

	if (!pdata)
		return -EBUSY;

	switch_data = kzalloc(sizeof(struct hp_switch_data), GFP_KERNEL);
	if (!switch_data)
		return -ENOMEM;

	switch_data->sdev.name = pdata->name;
	switch_data->gpio = pdata->gpio;
	switch_data->name_on = pdata->name_on;
	switch_data->name_off = pdata->name_off;
	switch_data->sdev.print_state = switch_hp_print_state;

    ret = switch_dev_register(&switch_data->sdev);
	if (ret < 0)
		goto err_switch_dev_register;

	ret = gpio_request(switch_data->gpio, pdev->name);
	if (ret < 0)
		goto err_request_gpio;

	ret = gpio_direction_input(switch_data->gpio);
	if (ret < 0)
		goto err_set_gpio_input;

	INIT_WORK(&switch_data->work, hp_switch_work);

	switch_data->irq = gpio_to_irq(switch_data->gpio);
	if (switch_data->irq < 0) {
		ret = switch_data->irq;
		goto err_detect_irq_num_failed;
	}

	ret = request_irq(switch_data->irq, hp_irq_handler, IRQ_TYPE_EDGE_BOTH, pdev->name, switch_data);
	if (ret < 0)
		goto err_request_irq;

  client = s3c_adc_register(pdev, NULL, NULL, 0);
	if (IS_ERR(client)) {
		dev_err(&pdev->dev, "cannot register adc\n");
		goto err_request_irq;
	}

	/* Perform initial detection */
//	hp_switch_work(&switch_data->work);
  schedule_work(&switch_data->work);

  my_switch_data = switch_data;
	return 0;

err_request_irq:
err_detect_irq_num_failed:
err_set_gpio_input:
	gpio_free(switch_data->gpio);
err_request_gpio:
    switch_dev_unregister(&switch_data->sdev);
err_switch_dev_register:
	kfree(switch_data);

	return ret;
}

static int __devexit hp_switch_remove(struct platform_device *pdev)
{
	struct hp_switch_data *switch_data = platform_get_drvdata(pdev);

	cancel_work_sync(&switch_data->work);
	gpio_free(switch_data->gpio);
    switch_dev_unregister(&switch_data->sdev);
	kfree(switch_data);

  if(client != NULL)
    s3c_adc_release(client);
  client = NULL;
	return 0;
}

static int hp_switch_resume(struct platform_device *pdev)
{
  schedule_work(&my_switch_data->work);
  return 0;
}

static int hp_switch_suspend(struct platform_device *pdev, pm_message_t state)
{
  return 0;
}

static struct platform_driver gpio_switch_driver = {
	.probe		= hp_switch_probe,
	.remove		= __devexit_p(hp_switch_remove),
	.suspend  = hp_switch_suspend,
	.resume   = hp_switch_resume,
	.driver		= {
		.name	= "switch-hp",
		.owner	= THIS_MODULE,
	},
};

static int __init hp_switch_init(void)
{
	return platform_driver_register(&gpio_switch_driver);
}

static void __exit hp_switch_exit(void)
{
	platform_driver_unregister(&gpio_switch_driver);
}

module_init(hp_switch_init);
module_exit(hp_switch_exit);

MODULE_AUTHOR("Apollo Yang <Apollo5520@hotmail.com>");
MODULE_DESCRIPTION("GPIO Switch driver");
MODULE_LICENSE("GPL");
