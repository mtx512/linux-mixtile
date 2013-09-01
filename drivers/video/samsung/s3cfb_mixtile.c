#include "s3cfb.h"

extern struct s3cfb_lcd mixtile_lcd;
extern struct s3cfb_lcd mixtile_lcd_1080p;
extern struct s3cfb_lcd mixtile_lcd_720p;

extern int dev_ver;
/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
	mixtile_lcd.init_ldi = NULL;
//  ctrl->lcd = &mixtile_lcd_1080p;
//  ctrl->lcd = &mixtile_lcd_720p;
  ctrl->lcd = &mixtile_lcd;
}

