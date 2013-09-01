#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/irq.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/errno.h>
#include <asm/mach-types.h>
#include <mach/hardware.h>
#include <mach/gpio-mixtilev310.h>
#include <plat/gpio-cfg.h>
#include <plat/adc.h>

#define FAKE_BAT_LEVEL  100
#define BAT_ADC_CHANNEL 0
#define EXT_ADC_CHANNEL 1
#define POLL_MSTIME     (HZ+HZ)

#define R_UP                  33      //上拉电阻
#define R_DOWN                20      //下拉电阻
#define BATT_MAX_ADC          3747    // 8V - 电量满adc值
#define BATT_MIN_ADC          2903    // 6.2V - 关机adc值
#define BATT_15_ADC           3091    // 6.6V - 低电警告adc值

#define BATT_CORRECTION_ADC   3300    // 电压调整分界线
#define BATT_CORRECTION_VAL0  90      // 高电压充电时校准值
#define BATT_CORRECTION_VAL1  160     // 低电压充电时校准值

extern int dev_ver;
/* Prototypes */
static ssize_t mixtile_adc_bat_show_property(struct device *dev,  struct device_attribute *attr, char *buf);
static ssize_t mixtile_adc_bat_store_property(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static struct device *dev;
static struct wake_lock vbus_wake_lock;
static struct delayed_work bat_work;
struct workqueue_struct *monitor_wqueue;
static int real_adc_value;

static char *status_text[] = {
  [POWER_SUPPLY_STATUS_UNKNOWN]         = "Unknown",
  [POWER_SUPPLY_STATUS_CHARGING]        = "Charging",
  [POWER_SUPPLY_STATUS_DISCHARGING]     = "Discharging",
  [POWER_SUPPLY_STATUS_NOT_CHARGING]    = "Not Charging",
  [POWER_SUPPLY_STATUS_FULL]            = "Full",
};

typedef enum {
  CHARGER_BATTERY = 0,
  CHARGER_USB,
  CHARGER_AC,
  CHARGER_DISCHARGE,
  CHARGE_FULL
} charger_type_t;

struct battery_info {
  u32 batt_id;    /* Battery ID from ADC */
  u32 batt_vol;    /* Battery voltage from ADC */
  u32 batt_vol_adc;  /* Battery ADC value */
  u32 batt_vol_adc_cal;  /* Battery ADC value (calibrated)*/
  u32 batt_temp;    /* Battery Temperature (C) from ADC */
  u32 batt_temp_adc;  /* Battery Temperature ADC value */
  u32 batt_temp_adc_cal;  /* Battery Temperature ADC value (calibrated) */
  u32 batt_current;  /* Battery current from ADC */
  u32 level;    /* formula */
  u32 charging_source;  /* 0: no cable, 1:usb, 2:AC */
  u32 charging_enabled;  /* 0: Disable, 1: Enable */
  u32 batt_health;  /* Battery Health (Authority) */
  u32 batt_is_full;  /* 0 : Not full 1: Full */
};

/* lock to protect the battery info */
//static DEFINE_MUTEX(work_lock);

struct mixtile_adc_bat_info {
  int present;
  int polling;
  unsigned long polling_interval;

 	struct s3c_adc_client		*client;
  struct battery_info bat_info;
};
static struct mixtile_adc_bat_info mixtile_adc_bat_info;

static int samsung_get_bat_adc(void)
{
  return s3c_adc_read(mixtile_adc_bat_info.client, BAT_ADC_CHANNEL);
}

static int samsung_get_bat_level(void)
{
  int adc;
  static int last_level = 0;
  static int level_0_time = 0;
  static int level_15_time = 0;
  int level;

  if(mixtile_adc_bat_info.bat_info.charging_source == CHARGER_AC)
  {
    if(mixtile_adc_bat_info.bat_info.batt_vol_adc < BATT_CORRECTION_ADC)
      adc = mixtile_adc_bat_info.bat_info.batt_vol_adc - BATT_CORRECTION_VAL1;
    else
      adc = mixtile_adc_bat_info.bat_info.batt_vol_adc - BATT_CORRECTION_VAL0;
  }    
  else
    adc = mixtile_adc_bat_info.bat_info.batt_vol_adc;

  if(adc >= BATT_15_ADC)
    level = (adc - BATT_15_ADC)*85/(BATT_MAX_ADC - BATT_15_ADC) + 15;
  else
    level = (adc - BATT_MIN_ADC)*15/(BATT_15_ADC - BATT_MIN_ADC);

  if((adc <= BATT_MIN_ADC) || (level < 0))
    level = 0;
  
  if (adc >= BATT_MAX_ADC)
    level = 100;
  
  if(last_level == 0)
    last_level = level;

  if(mixtile_adc_bat_info.bat_info.charging_source == CHARGER_AC)
  {
    if(level >= 100)          //与充电指示灯保持一致
      level = 99;
    if(level<=0)              //充电时系统不关机
      level = 1;
    
    if(level < last_level)    //充电时电量只增不减
      level = last_level;
  }
//  else
//  {
//    if(level > last_level)    //放电时电量只减不增
//      level = last_level;
//  }

  if(level == 0)
  {//连续6次检测到电量为0，则关机
    level_0_time++;
    if(level_0_time >= 6)
      level = 0;
    else
      level = 1;
  }
  else
  {
    level_0_time = 0;
    if(level <= 15)
      level_15_time++;
    else
      level_15_time = 0;
  }
  if((mixtile_adc_bat_info.bat_info.charging_source != CHARGER_AC) && (level_15_time<5) && (level<=15) && (level>1)) //连续5次低电检测才上报
    level = 16;

//  if(abs(level-last_level) > 5) //忽略跳变大于5
//    level = last_level;

//  printk("adc=%d %d level=%d level_15_time=%d level_0_time=%d\n", mixtile_adc_bat_info.bat_info.batt_vol_adc, adc, level, level_15_time, level_0_time);

  if(dev_ver == 1)
    level = FAKE_BAT_LEVEL;     //模拟电池
  last_level = level;
  return level;
}

static int samsung_get_bat_vol(void)
{
  int bat_vol = 0;
  int adc;
  
  if(mixtile_adc_bat_info.bat_info.charging_source == CHARGER_AC)
    adc = mixtile_adc_bat_info.bat_info.batt_vol_adc - BATT_CORRECTION_VAL0;
  else
    adc = mixtile_adc_bat_info.bat_info.batt_vol_adc;

  bat_vol = (adc*3300*(R_UP+R_DOWN))/(4096*R_DOWN);
  return bat_vol;
}

static u32 samsung_get_bat_health(void)
{
  return mixtile_adc_bat_info.bat_info.batt_health;
}

static int samsung_get_bat_temp(void)
{
  int temp = 0;
  return temp;
}

static int mixtile_adc_bat_get_charging_status(void)
{
  charger_type_t charger = CHARGER_BATTERY;
  int ret = 0;

  charger = mixtile_adc_bat_info.bat_info.charging_source;

  switch (charger) {
  case CHARGER_BATTERY:
    ret = POWER_SUPPLY_STATUS_NOT_CHARGING;
    break;
  case CHARGER_USB:
  case CHARGER_AC:
    if (mixtile_adc_bat_info.bat_info.level == 100 && mixtile_adc_bat_info.bat_info.batt_is_full)
      ret = POWER_SUPPLY_STATUS_FULL;
    else
      ret = POWER_SUPPLY_STATUS_CHARGING;
    break;
  case CHARGER_DISCHARGE:
    ret = POWER_SUPPLY_STATUS_DISCHARGING;
    break;
  default:
    ret = POWER_SUPPLY_STATUS_UNKNOWN;
  }
  dev_dbg(dev, "%s : %s\n", __func__, status_text[ret]);

  return ret;
}

static int mixtile_adc_bat_get_property(struct power_supply *bat_ps, enum power_supply_property psp, union power_supply_propval *val)
{
  dev_dbg(bat_ps->dev, "%s : psp = %d\n", __func__, psp);
  switch (psp) {
  case POWER_SUPPLY_PROP_STATUS:
    val->intval = mixtile_adc_bat_get_charging_status();
    break;
  case POWER_SUPPLY_PROP_HEALTH:
    val->intval = samsung_get_bat_health();
    break;
  case POWER_SUPPLY_PROP_PRESENT:
    val->intval = mixtile_adc_bat_info.present;
    break;
  case POWER_SUPPLY_PROP_TECHNOLOGY:
    val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
    break;
  case POWER_SUPPLY_PROP_CAPACITY:
    val->intval = mixtile_adc_bat_info.bat_info.level;
    dev_dbg(dev, "%s : level = %d\n", __func__, val->intval);
    break;
  case POWER_SUPPLY_PROP_TEMP:
    val->intval = mixtile_adc_bat_info.bat_info.batt_temp;
    dev_dbg(bat_ps->dev, "%s : temp = %d\n", __func__, val->intval);
    break;
  default:
    return -EINVAL;
  }
  return 0;
}

static int samsung_power_get_property(struct power_supply *bat_ps, enum power_supply_property psp, union power_supply_propval *val)
{
  charger_type_t charger;
  dev_dbg(bat_ps->dev, "%s : psp = %d\n", __func__, psp);

  charger = mixtile_adc_bat_info.bat_info.charging_source;

  switch (psp) {
  case POWER_SUPPLY_PROP_ONLINE:
    if (bat_ps->type == POWER_SUPPLY_TYPE_MAINS)
      val->intval = (charger == CHARGER_AC ? 1 : 0);
    else if (bat_ps->type == POWER_SUPPLY_TYPE_USB)
      val->intval = (charger == CHARGER_USB ? 1 : 0);
    else
      val->intval = 0;
    break;
  default:
    return -EINVAL;
  }

  return 0;
}

#define mixtile_adc_bat_ATTR(_name)      \
{              \
  .attr = { .name = #_name, .mode = 0644, },      \
  .show = mixtile_adc_bat_show_property,    \
  .store = mixtile_adc_bat_store_property,  \
}

static struct device_attribute mixtile_adc_bat_attrs[] = {
  mixtile_adc_bat_ATTR(batt_vol),
  mixtile_adc_bat_ATTR(batt_vol_adc),
  mixtile_adc_bat_ATTR(batt_vol_adc_cal),
  mixtile_adc_bat_ATTR(batt_temp),
  mixtile_adc_bat_ATTR(batt_temp_adc),
  mixtile_adc_bat_ATTR(batt_temp_adc_cal),
};

enum {
  BATT_VOL = 0,
  BATT_VOL_ADC,
  BATT_VOL_ADC_CAL,
  BATT_TEMP,
  BATT_TEMP_ADC,
  BATT_TEMP_ADC_CAL,
};

static int mixtile_adc_bat_create_attrs(struct device * dev)
{
  int i, rc;

  for (i = 0; i < ARRAY_SIZE(mixtile_adc_bat_attrs); i++) {
    rc = device_create_file(dev, &mixtile_adc_bat_attrs[i]);
    if (rc)
    goto attrs_failed;
  }
  goto succeed;

attrs_failed:
  while (i--)
    device_remove_file(dev, &mixtile_adc_bat_attrs[i]);
succeed:
  return rc;
}

static ssize_t mixtile_adc_bat_show_property(struct device *dev, struct device_attribute *attr, char *buf)
{
  int i = 0;
  const ptrdiff_t off = attr - mixtile_adc_bat_attrs;

  switch (off) {
  case BATT_VOL:
    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", mixtile_adc_bat_info.bat_info.batt_vol);
  break;
  case BATT_VOL_ADC:
//    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", mixtile_adc_bat_info.bat_info.batt_vol_adc);
    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", real_adc_value);
    break;
  case BATT_VOL_ADC_CAL:
    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", mixtile_adc_bat_info.bat_info.batt_vol_adc_cal);
    break;
  case BATT_TEMP:
    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", mixtile_adc_bat_info.bat_info.batt_temp);
    break;
  case BATT_TEMP_ADC:
    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", mixtile_adc_bat_info.bat_info.batt_temp_adc);
    break;
  case BATT_TEMP_ADC_CAL:
    i += scnprintf(buf + i, PAGE_SIZE - i, "%d\n", mixtile_adc_bat_info.bat_info.batt_temp_adc_cal);
    break;
  default:
    i = -EINVAL;
  }

  return i;
}

static ssize_t mixtile_adc_bat_store_property(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
  int x = 0;
  int ret = 0;
  const ptrdiff_t off = attr - mixtile_adc_bat_attrs;

  switch (off) {
  case BATT_VOL_ADC_CAL:
    if (sscanf(buf, "%d\n", &x) == 1) {
      mixtile_adc_bat_info.bat_info.batt_vol_adc_cal = x;
      ret = count;
    }
    dev_info(dev, "%s : batt_vol_adc_cal = %d\n", __func__, x);
    break;
  case BATT_TEMP_ADC_CAL:
    if (sscanf(buf, "%d\n", &x) == 1) {
      mixtile_adc_bat_info.bat_info.batt_temp_adc_cal = x;
      ret = count;
    }
    dev_info(dev, "%s : batt_temp_adc_cal = %d\n", __func__, x);
    break;
  default:
    ret = -EINVAL;
  }

  return ret;
}

static enum power_supply_property mixtile_adc_bat_properties[] = {
  POWER_SUPPLY_PROP_STATUS,
  POWER_SUPPLY_PROP_HEALTH,
  POWER_SUPPLY_PROP_PRESENT,
  POWER_SUPPLY_PROP_TECHNOLOGY,
  POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property samsung_power_properties[] = {
  POWER_SUPPLY_PROP_ONLINE,
};

static char *supply_list[] = {
  "battery",
};

static struct power_supply samsung_power_supplies[] = {
  {
    .name = "battery",
    .type = POWER_SUPPLY_TYPE_BATTERY,
    .properties = mixtile_adc_bat_properties,
    .num_properties = ARRAY_SIZE(mixtile_adc_bat_properties),
    .get_property = mixtile_adc_bat_get_property,
  }, {
    .name = "usb",
    .type = POWER_SUPPLY_TYPE_USB,
    .supplied_to = supply_list,
    .num_supplicants = ARRAY_SIZE(supply_list),
    .properties = samsung_power_properties,
    .num_properties = ARRAY_SIZE(samsung_power_properties),
    .get_property = samsung_power_get_property,
  }, {
    .name = "ac",
    .type = POWER_SUPPLY_TYPE_MAINS,
    .supplied_to = supply_list,
    .num_supplicants = ARRAY_SIZE(supply_list),
    .properties = samsung_power_properties,
    .num_properties = ARRAY_SIZE(samsung_power_properties),
    .get_property = samsung_power_get_property,
  },
};

static charger_type_t samsung_get_charge_source(charger_type_t *source)
{
  charger_type_t ret_source = CHARGER_DISCHARGE/*CHARGER_BATTERY*/;
  int state1, state2;
  
  state1 = gpio_get_value(GPIO_CHARGE_STATE1);
  state2 = gpio_get_value(GPIO_CHARGE_STATE2);
  
  if((state1==1) && (state2==0))
    ret_source = CHARGER_AC;

  if((state1==0) && (state2==1))
  {
    ret_source = CHARGER_AC;
    *source = CHARGE_FULL;
  }
//printk("stat1=%d stat2=%d\n", state1, state2);

  return ret_source;
}

static void mixtile_adc_bat_status_update(unsigned long unused)
{
  charger_type_t source = CHARGER_BATTERY;
  int adc;

  adc = samsung_get_bat_adc();
  real_adc_value = adc;
  if(adc > 1000)
  {
    mixtile_adc_bat_info.bat_info.batt_temp       = samsung_get_bat_temp();
    mixtile_adc_bat_info.bat_info.batt_vol_adc    = adc;
    mixtile_adc_bat_info.bat_info.batt_vol        = samsung_get_bat_vol();
    mixtile_adc_bat_info.bat_info.level           = samsung_get_bat_level();
  }
  mixtile_adc_bat_info.bat_info.charging_source = samsung_get_charge_source(&source);

  if(mixtile_adc_bat_info.bat_info.level >= 100)
    mixtile_adc_bat_info.bat_info.batt_is_full = 1;
  else
    mixtile_adc_bat_info.bat_info.batt_is_full = 0;

  if(source == CHARGE_FULL)
  {//硬件检测到已充满
    mixtile_adc_bat_info.bat_info.batt_is_full = 1;
    mixtile_adc_bat_info.bat_info.level = 100;
  }

//Apollo + 充电时可休眠
//  if (source == CHARGER_USB || source == CHARGER_AC)
//    wake_lock(&vbus_wake_lock);
//  else
//    wake_lock_timeout(&vbus_wake_lock, HZ / 2);
//Apollo -

  power_supply_changed(&samsung_power_supplies[CHARGER_BATTERY]);
//  power_supply_changed(&s3c_power_supplies[CHARGER_USB]);
//  power_supply_changed(&s3c_power_supplies[CHARGER_AC]);

//  printk("adc=%04d level=%03d vol=%04d source=%d\n", mixtile_adc_bat_info.bat_info.batt_vol_adc, mixtile_adc_bat_info.bat_info.level, mixtile_adc_bat_info.bat_info.batt_vol, mixtile_adc_bat_info.bat_info.charging_source);
}

#ifdef CONFIG_PM
static int mixtile_adc_bat_suspend(struct device *dev)
{
  return 0;
}

static void mixtile_adc_bat_resume(struct device *dev)
{
}
#else
#define mixtile_adc_bat_suspend NULL
#define mixtile_adc_bat_resume NULL
#endif /* CONFIG_PM */

static void s3c_adc_bat_work(struct work_struct *work)
{
  mixtile_adc_bat_status_update(0);
  queue_delayed_work(monitor_wqueue, &bat_work, POLL_MSTIME);

}

static int __devinit mixtile_adc_bat_probe(struct platform_device *pdev)
{
  int i;
  int ret = 0;
  struct s3c_adc_client	*client;

  dev = &pdev->dev;
  dev_info(dev, "%s\n", __func__);

  client = s3c_adc_register(pdev, NULL, NULL, 0);
	if (IS_ERR(client)) {
		dev_err(&pdev->dev, "cannot register adc\n");
		return PTR_ERR(client);
	}
  platform_set_drvdata(pdev, client);
  
  mixtile_adc_bat_info.client = client;
  mixtile_adc_bat_info.present = 1;

  mixtile_adc_bat_info.bat_info.batt_id = 0;
  mixtile_adc_bat_info.bat_info.batt_vol = 0;
  mixtile_adc_bat_info.bat_info.batt_vol_adc = 0;
  mixtile_adc_bat_info.bat_info.batt_vol_adc_cal = 0;
  mixtile_adc_bat_info.bat_info.batt_temp = 0;
  mixtile_adc_bat_info.bat_info.batt_temp_adc = 0;
  mixtile_adc_bat_info.bat_info.batt_temp_adc_cal = 0;
  mixtile_adc_bat_info.bat_info.batt_current = 0;
  mixtile_adc_bat_info.bat_info.level = 0;
  mixtile_adc_bat_info.bat_info.charging_source = CHARGER_BATTERY;
  mixtile_adc_bat_info.bat_info.charging_enabled = 0;
  mixtile_adc_bat_info.bat_info.batt_health = POWER_SUPPLY_HEALTH_GOOD;

  /* init power supplier framework */
  for (i = 0; i < ARRAY_SIZE(samsung_power_supplies); i++) {
    ret = power_supply_register(&pdev->dev, &samsung_power_supplies[i]);
    if (ret) {
      dev_err(dev, "Failed to register power supply %d,%d\n", i, ret);
      goto __end__;
    }
  }

  mixtile_adc_bat_create_attrs(samsung_power_supplies[CHARGER_BATTERY].dev);

  INIT_DELAYED_WORK(&bat_work, s3c_adc_bat_work);
  monitor_wqueue = create_singlethread_workqueue(dev_name(&pdev->dev));
  queue_delayed_work(monitor_wqueue, &bat_work, POLL_MSTIME);

__end__:
  return ret;
}

static int __devexit mixtile_adc_bat_remove(struct platform_device *pdev)
{
  int i;

  for (i = 0; i < ARRAY_SIZE(samsung_power_supplies); i++)
    power_supply_unregister(&samsung_power_supplies[i]);

  return 0;
}

static const struct dev_pm_ops mixtile_adc_bat_pm_ops = {
  .prepare  = mixtile_adc_bat_suspend,
  .complete  = mixtile_adc_bat_resume,
};

static struct platform_driver mixtile_adc_bat_driver = {
  .driver = {
    .name  = "mixtile_adc_bat",
    .owner  = THIS_MODULE,
    .pm  = &mixtile_adc_bat_pm_ops,
  },
  .probe    = mixtile_adc_bat_probe,
  .remove    = __devexit_p(mixtile_adc_bat_remove),
};

static int __init mixtile_adc_bat_init(void)
{
  wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
  return platform_driver_register(&mixtile_adc_bat_driver);
}

static void __exit mixtile_adc_bat_exit(void)
{
  platform_driver_unregister(&mixtile_adc_bat_driver);
}

module_init(mixtile_adc_bat_init);
module_exit(mixtile_adc_bat_exit);

MODULE_AUTHOR("Apollo <Apollo5520@gmail.com>");
MODULE_DESCRIPTION("Mixtile battery controller driver");
MODULE_LICENSE("GPL");
