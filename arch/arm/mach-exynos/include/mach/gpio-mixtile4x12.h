#ifndef __ASM_ARCH_GPIO_MIXTILE4X12_H
#define __ASM_ARCH_GPIO_MIXTILE4X12_H __FILE__

#define GPIO_LCD_PWM            EXYNOS4_GPD0(0)     // ���� PWM �ź�
#define GPIO_LCD_POWER          EXYNOS4_GPX2(4)     // LCD ��Դ����
#define GPIO_LVDS_POWER         EXYNOS4_GPM3(3)     // LVDS оƬ��Դ����
#define GPIO_LCD_BACKLED        EXYNOS4_GPM3(2)     // LCD �����Դ����

#define GPIO_WIFI_POWER         EXYNOS4_GPM4(5)     // wifi �ⲿ��Դ����

#define GPIO_CAMERA_POWER       EXYNOS4_GPM1(4)     // camera ��Դ����
#define GPIO_CAMERA_F_RST       EXYNOS4_GPM1(2)     // ǰ������ͷ��λ����
#define GPIO_CAMERA_F_PWDN      EXYNOS4_GPM1(3)     // ǰ������ͷ��Դ����
#define GPIO_CAMERA_B_RST       EXYNOS4_GPM2(3)     // ��������ͷ��λ����
#define GPIO_CAMERA_B_PWDN      EXYNOS4_GPM2(4)     // ��������ͷ��Դ����
#define GPIO_TVP5151_RST        EXYNOS4_GPM4(3)     // TVP5151 ��λ����
#define GPIO_TVP5151_PWDN       EXYNOS4_GPM4(7)     // TVP5151 ��Դ����
#define GPIO_FLASH_EN           EXYNOS4_GPD0(2)     // ����Ƶ�Դ����
#define GPIO_FLASH_MODE         EXYNOS4_GPM4(1)     // ����ƹ���ģʽ����

#define GPIO_TP_POWER           EXYNOS4_GPM4(2)     // TP ��Դ����
#define GPIO_TP_INT             EXYNOS4_GPX1(7)     // TP �ж��ź���
#define GPIO_TP_RST             EXYNOS4_GPX1(6)     // TP ��λ�ź���

#define GPIO_GPS_POWER          EXYNOS4_GPM4(0)     // GPS ��Դ����
#define GPIO_GPS_EN             EXYNOS4_GPX0(7)     // GPS ʹ�ܽ�
#define GPIO_GPS_RXD            EXYNOS4_GPA0(4)     // GPS �������ݽ�

#define GPIO_3G_POWER           EXYNOS4_GPM3(0)     // 3G ��Դ����

#define GPIO_USBHUB_POWER       EXYNOS4_GPM3(7)     // USB_HUB оƬ��Դ����
#define GPIO_USB5V_POWER        EXYNOS4_GPX3(4)     // USB_HOST 5V ��Դ����
#define GPIO_USBOTG_ID          EXYNOS4_GPX0(0)     // USB OTG ID
#define GPIO_USBOTG_POWER       EXYNOS4_GPM3(4)     // USB OTG 5V ��Դ����
#define GPIO_USBOTG_VBUS        EXYNOS4_GPX1(1)     // USB VBUS �жϼ��

#define GPIO_DM9000_CS          EXYNOS4_GPY0(1)     // DM9000 Ƭѡ�źŽ�
#define GPIO_DM9000_IRQ         IRQ_EINT(21)        // DM9000 �ж��ź�
#define GPIO_DM9000_POWER				EXYNOS4_GPX3(5)     // DM9000 ��Դ����
#define GPIO_DM9000_CS0					EXYNOS4_GPY0(0)     // DM9000 ��ƽת������
#define GPIO_DM9000_OE          EXYNOS4_GPY0(4)     // DM9000 OE �ź�
#define GPIO_DM9000_WE          EXYNOS4_GPY0(5)     // DM9000 WE �ź�

#define GPIO_MOTOR_POWER        EXYNOS4_GPM3(5)     // ������Դ����
#define GPIO_VOLUMEUP_KEY       EXYNOS4_GPX2(1)     // volume+ ����
#define GPIO_VOLUMEDOWN_KEY     EXYNOS4_GPX2(2)     // volume- ����
#define GPIO_MEDIA_KEY          EXYNOS4_GPX2(3)     // ���� hook ����
#define GPIO_HOME_KEY           EXYNOS4_GPX1(2)     // HOME ����
#define GPIO_MENU_KEY           EXYNOS4_GPX1(4)     // MENU ����
#define GPIO_BACK_KEY           EXYNOS4_GPX1(5)     // BACK ����
#define GPIO_POWER_KEY          EXYNOS4_GPX0(1)     // POWER ����
#define POWER_KEY_EINT          0
#define IRQ_POWER_WAKEUP        1
#define IRQ_RTC_WAKEUP          1
//#define GPIO_KEY_LED            EXYNOS4_GPX2(0)     // �����ƿ����ź�

#define GPIO_HEADPHONE          EXYNOS4_GPX0(3)     // ������������

//#define GPIO_RFID_PWR_CTRL      EXYNOS4_GPX2(7)     // RFID ��Դ����

#define GPIO_CHARGE_STATE1      EXYNOS4_GPX3(0)     // ���״ָ̬ʾ1
#define GPIO_CHARGE_STATE2      EXYNOS4_GPX3(1)     // ���״ָ̬ʾ2

#endif /* __ASM_ARCH_GPIO_MIXTILE4X12_H */
