#ifndef __MIXTILE4X12_H__
#define __MIXTILE4X12_H__

extern int s3c_gpio_slp_cfgpin(unsigned int pin, unsigned int config);
extern int s3c_gpio_slp_setpull_updown(unsigned int pin, unsigned int config);

extern void mixtile4x12_config_gpio_table(void);
extern void mixtile4x12_config_sleep_gpio_table(void);

extern void set_gps_uart_op(int onoff);
#endif
