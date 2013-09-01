#ifndef __ASM_ARCH_GPIO_MIXTILE4X12_H
#define __ASM_ARCH_GPIO_MIXTILE4X12_H __FILE__

#define GPIO_LCD_PWM            EXYNOS4_GPD0(0)     // 背光 PWM 信号
#define GPIO_LCD_POWER          EXYNOS4_GPX2(4)     // LCD 电源控制
#define GPIO_LVDS_POWER         EXYNOS4_GPM3(3)     // LVDS 芯片电源控制
#define GPIO_LCD_BACKLED        EXYNOS4_GPM3(2)     // LCD 背光电源控制

#define GPIO_WIFI_POWER         EXYNOS4_GPM4(5)     // wifi 外部电源控制

#define GPIO_CAMERA_POWER       EXYNOS4_GPM1(4)     // camera 电源控制
#define GPIO_CAMERA_F_RST       EXYNOS4_GPM1(2)     // 前置摄像头复位控制
#define GPIO_CAMERA_F_PWDN      EXYNOS4_GPM1(3)     // 前置摄像头电源控制
#define GPIO_CAMERA_B_RST       EXYNOS4_GPM2(3)     // 后置摄像头复位控制
#define GPIO_CAMERA_B_PWDN      EXYNOS4_GPM2(4)     // 后置摄像头电源控制
#define GPIO_TVP5151_RST        EXYNOS4_GPM4(3)     // TVP5151 复位控制
#define GPIO_TVP5151_PWDN       EXYNOS4_GPM4(7)     // TVP5151 电源控制
#define GPIO_FLASH_EN           EXYNOS4_GPD0(2)     // 闪光灯电源控制
#define GPIO_FLASH_MODE         EXYNOS4_GPM4(1)     // 闪光灯工作模式控制

#define GPIO_TP_POWER           EXYNOS4_GPM4(2)     // TP 电源控制
#define GPIO_TP_INT             EXYNOS4_GPX1(7)     // TP 中断信号线
#define GPIO_TP_RST             EXYNOS4_GPX1(6)     // TP 复位信号线

#define GPIO_GPS_POWER          EXYNOS4_GPM4(0)     // GPS 电源控制
#define GPIO_GPS_EN             EXYNOS4_GPX0(7)     // GPS 使能脚
#define GPIO_GPS_RXD            EXYNOS4_GPA0(4)     // GPS 接收数据脚

#define GPIO_3G_POWER           EXYNOS4_GPM3(0)     // 3G 电源控制

#define GPIO_USBHUB_POWER       EXYNOS4_GPM3(7)     // USB_HUB 芯片电源控制
#define GPIO_USB5V_POWER        EXYNOS4_GPX3(4)     // USB_HOST 5V 电源控制
#define GPIO_USBOTG_ID          EXYNOS4_GPX0(0)     // USB OTG ID
#define GPIO_USBOTG_POWER       EXYNOS4_GPM3(4)     // USB OTG 5V 电源控制
#define GPIO_USBOTG_VBUS        EXYNOS4_GPX1(1)     // USB VBUS 中断检测

#define GPIO_DM9000_CS          EXYNOS4_GPY0(1)     // DM9000 片选信号脚
#define GPIO_DM9000_IRQ         IRQ_EINT(21)        // DM9000 中断信号
#define GPIO_DM9000_POWER				EXYNOS4_GPX3(5)     // DM9000 电源控制
#define GPIO_DM9000_CS0					EXYNOS4_GPY0(0)     // DM9000 电平转换控制
#define GPIO_DM9000_OE          EXYNOS4_GPY0(4)     // DM9000 OE 信号
#define GPIO_DM9000_WE          EXYNOS4_GPY0(5)     // DM9000 WE 信号

#define GPIO_MOTOR_POWER        EXYNOS4_GPM3(5)     // 震动马达电源控制
#define GPIO_VOLUMEUP_KEY       EXYNOS4_GPX2(1)     // volume+ 按键
#define GPIO_VOLUMEDOWN_KEY     EXYNOS4_GPX2(2)     // volume- 按键
#define GPIO_MEDIA_KEY          EXYNOS4_GPX2(3)     // 耳机 hook 按键
#define GPIO_HOME_KEY           EXYNOS4_GPX1(2)     // HOME 按键
#define GPIO_MENU_KEY           EXYNOS4_GPX1(4)     // MENU 按键
#define GPIO_BACK_KEY           EXYNOS4_GPX1(5)     // BACK 按键
#define GPIO_POWER_KEY          EXYNOS4_GPX0(1)     // POWER 按键
#define POWER_KEY_EINT          0
#define IRQ_POWER_WAKEUP        1
#define IRQ_RTC_WAKEUP          1
//#define GPIO_KEY_LED            EXYNOS4_GPX2(0)     // 按键灯控制信号

#define GPIO_HEADPHONE          EXYNOS4_GPX0(3)     // 耳机插入检测线

//#define GPIO_RFID_PWR_CTRL      EXYNOS4_GPX2(7)     // RFID 电源控制

#define GPIO_CHARGE_STATE1      EXYNOS4_GPX3(0)     // 充电状态指示1
#define GPIO_CHARGE_STATE2      EXYNOS4_GPX3(1)     // 充电状态指示2

#endif /* __ASM_ARCH_GPIO_MIXTILE4X12_H */
