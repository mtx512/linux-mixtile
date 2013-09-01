/* 
 * linux/drivers/media/video/gc2015.c
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/version.h>
#include <linux/videodev2.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <linux/gpio.h>
#include <media/mixtile_camera.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio-mixtile4x12.h>

#ifdef CONFIG_VIDEO_SAMSUNG_V4L2
#include <linux/videodev2_samsung.h>
#endif

#include "gc2015.h"
#define GC2015_DRIVER_NAME	"GC2015"

extern int mixtilev310_cam_f_power(int onoff);
static int initted = 0;

#define PRINTK(x,y...) do{if(x<1) v4l_info(client, y);}while(0)
//#define PRINTK(x,y...) do{if(x<1) printk(y);}while(0)

/* Camera functional setting values configured by user concept */
struct gc2015_userset {
	signed int exposure_bias;	/* V4L2_CID_EXPOSURE */
	unsigned int ae_lock;
	unsigned int awb_lock;
	unsigned int auto_wb;	/* V4L2_CID_AUTO_WHITE_BALANCE */
	unsigned int manual_wb;	/* V4L2_CID_WHITE_BALANCE_PRESET */
	unsigned int wb_temp;	/* V4L2_CID_WHITE_BALANCE_TEMPERATURE */
	unsigned int effect;	/* Color FX (AKA Color tone) */
	unsigned int contrast;	/* V4L2_CID_CONTRAST */
	unsigned int saturation;	/* V4L2_CID_SATURATION */
	unsigned int sharpness;		/* V4L2_CID_SHARPNESS */
	unsigned int brightness;	/* V4L2_CID_CAMERA_BRIGHTNESS */
	unsigned int glamour;
};
static struct gc2015_userset *cur_userset;

struct gc2015_state {
	struct mixtile_camera_data *pdata;
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	struct gc2015_userset userset;
	int freq;	/* MCLK in KHz */
	int is_mipi;
	int isize;
	int ver;
	int fps;
	int framesize_index;
};

/* define supported resolution */
static struct cam_res {
	const char		*name;
	unsigned long		width;
	unsigned long		height;
} cam_res_list[] = {
	{
		.name		= "320x240, QVGA",
		.width		= 320,
		.height		= 240
	}, {
		.name		= "640x480, VGA",
		.width		= 640,
		.height		= 480
	}, {
		.name		= "800x600, SVGA",
		.width		= 800,
		.height		= 600
	}, {
		.name		= "1280x720, HD720P",
		.width		= 1280,
		.height		= 720
	}, {
		.name		= "1280x960, SXGA",
		.width		= 1280,
		.height		= 960
	}, {
		.name		= "1600x1200, UXGA",
		.width		= 1600,
		.height		= 1200
	}
};
#define N_RESOLUTIONS ARRAY_SIZE(cam_res_list)


static inline struct gc2015_state *to_state(struct v4l2_subdev *sd)
{
	return container_of(sd, struct gc2015_state, sd);
}

//static inline int gc2015_write(struct v4l2_subdev *sd, u8 addr, u8 val)
//{
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	struct i2c_msg msg[1];
//	unsigned char reg[2];
//	int err = 0;
//	int retry = 0;
//
//
//	if (!client->adapter)
//		return -ENODEV;
//
//again:
//	msg->addr = client->addr;
//	msg->flags = 0;
//	msg->len = 2;
//	msg->buf = reg;
//
//	reg[0] = addr & 0xff;
//	reg[1] = val & 0xff;
//
//	err = i2c_transfer(client->adapter, msg, 1);
//	if (err >= 0)
//		return err;	/* Returns here on success */
//
//	/* abnormal case: retry 5 times */
//	if (retry < 5) {
//		dev_err(&client->dev, "%s: address: 0x%02x%02x, value: 0x%02x%02x\n", 
//		        __func__, reg[0], reg[1], reg[2], reg[3]);
//		retry++;
//		goto again;
//	}
//	return err;
//}

static int gc2015_i2c_write(struct v4l2_subdev *sd, unsigned char i2c_data[], unsigned char length)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return i2c_smbus_write_byte_data(client, i2c_data[0], i2c_data[1]);
//	struct i2c_client *client = v4l2_get_subdevdata(sd);
//	unsigned char buf[length], i;
//	struct i2c_msg msg = {client->addr, 0, length, buf};
//
//	for (i = 0; i < length; i++)
//		buf[i] = i2c_data[i];
//
//	return i2c_transfer(client->adapter, &msg, 1) == 1 ? 0 : -EIO;
}

static int gc2015_write_regs(struct v4l2_subdev *sd, unsigned char regs[], int size)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int i, err;

	for (i = 0; i < size; i++) {
		err = gc2015_i2c_write(sd, &regs[i], sizeof(regs[i]));
		if (err < 0)
		{
			v4l_info(client, "%s: register set failed\n", __func__);
printk("gc2015_write_regs failed\n");
	  }
	}

	return 0;	/* FIXME */
}

static s32 gc2015_read_reg(struct v4l2_subdev *sd, unsigned char reg) //可正常工作
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	return i2c_smbus_read_byte_data(client, reg);
}

static int i2c_read_reg(struct v4l2_subdev *sd, uint8_t *buf, int len)  //可正常工作
{
	struct i2c_msg msgs[2];
	int ret=-1;
	int retries = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	msgs[0].flags=!I2C_M_RD;
	msgs[0].addr=client->addr;
	msgs[0].len=1;
	msgs[0].buf=&buf[0];

	msgs[1].flags=I2C_M_RD;
	msgs[1].addr=client->addr;
	msgs[1].len=len-1;
	msgs[1].buf=&buf[1];

	while(retries<5)
	{
		ret=i2c_transfer(client->adapter,msgs, 2);
		if(ret == 2)break;
		retries++;
	}
	if(retries == 5)
	  printk("read reg error\n");
	return ret;
}


static int gc2015_detect(struct v4l2_subdev *sd)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
  unsigned char gc2015_check_id[3] = {0,0xFF,0xFF};

//  mixtilev310_cam_f_power(1);
  gc2015_check_id[1] = gc2015_read_reg(sd, 0)&0xFF;
  gc2015_check_id[2] = gc2015_read_reg(sd, 1)&0x0F;
//  i2c_read_reg(sd, gc2015_check_id, 3);

//  mixtilev310_cam_f_power(0);
  dev_info(&client->dev, "gc2015_id = 0x%02x 0x%02x\n", gc2015_check_id[1], gc2015_check_id[2]);

  if((gc2015_check_id[1] != 0x20) || (gc2015_check_id[2] != 0x05))
    return -1;
  return 0;
}

void gc2015_brightness_preset(struct v4l2_subdev *sd, int which)
{
	int err, i;
	unsigned char ctrlreg[4][2];

	ctrlreg[0][0] = 0xFE; ctrlreg[0][1] = 0x01;
	ctrlreg[2][0] = 0xFE; ctrlreg[2][1] = 0x00;
	switch(which)
	{
	  case EV_MINUS_4:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x40;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0xC0;
      break;
	  case EV_MINUS_3:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x48;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0xD0;
      break;
	  case EV_MINUS_2:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x50;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0xE0;
      break;
	  case EV_MINUS_1:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x58;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0xF0;
      break;
	  case EV_DEFAULT:
    default:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x60;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0x00;
      break;
	  case EV_PLUS_1:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x68;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0x10;
      break;
	  case EV_PLUS_2:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x70;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0x20;
      break;
	  case EV_PLUS_3:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x78;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0x30;
      break;
	  case EV_PLUS_4:
   		ctrlreg[1][0] = 0x13; ctrlreg[1][1] = 0x80;
   		ctrlreg[3][0] = 0xD5; ctrlreg[3][1] = 0x40;
      break;
	}
	for (i=0; i<4; i++)
		err = gc2015_i2c_write(sd, ctrlreg[i], 2);
}

void gc2015_white_balance_preset(struct v4l2_subdev *sd, int which)
{
	int i;
	int num;
	unsigned char ctrlreg[2][2];
	
	ctrlreg[0][0] = 0xfe;
	ctrlreg[0][1] = 0x00;
  gc2015_i2c_write(sd, ctrlreg[0], 2);
  
	switch (which) {
	case 1:		//WHITE_BALANCE_AUTO
		ctrlreg[1][0]=0x42; 
		ctrlreg[1][1]=gc2015_read_reg(sd, 0x42)|0x02;
		gc2015_i2c_write(sd, ctrlreg[1], 2);
		
	break;
	case 2:		//WHITE_BALANCE_SUNNY
		ctrlreg[1][0]=0x42; 
		ctrlreg[1][1]=gc2015_read_reg(sd, 0x42) & (~0x02);
		gc2015_i2c_write(sd, ctrlreg[1], 2);
		
		num = ((sizeof(GC2015_white_balance_daylight)/sizeof(GC2015_white_balance_daylight[0])));			
		for (i = 0; i < num ; i++)
		  gc2015_i2c_write(sd, GC2015_white_balance_daylight[i], 2);
	break;
	case 3:		//WHITE_BALANCE_CLOUDY
		ctrlreg[1][0]=0x22; 
		ctrlreg[1][1]=gc2015_read_reg(sd, 0x42) & (~0x02);
		gc2015_i2c_write(sd, ctrlreg[1], 2);
		
		num = ((sizeof(GC2015_white_balance_cloud)/sizeof(GC2015_white_balance_cloud[0])));			
		for (i = 0; i < num ; i++)
			gc2015_i2c_write(sd, GC2015_white_balance_cloud[i], 2);
	break;
	case 4:		
		ctrlreg[1][0]=0x22; //WHITE_BALANCE_TUNGSTEN
		ctrlreg[1][1]=gc2015_read_reg(sd, 0x42) & (~0x02);
		gc2015_i2c_write(sd, ctrlreg[1], 2);
		
		num = ((sizeof(GC2015_white_balance_tungsten)/sizeof(GC2015_white_balance_tungsten[0])));			
		for (i = 0; i < num ; i++)
			gc2015_i2c_write(sd, GC2015_white_balance_tungsten[i], 2);
	break;
	case 5:	
		ctrlreg[1][0]=0x22; //WHITE_BALANCE_FLUORESCENT
		ctrlreg[1][1]=gc2015_read_reg(sd, 0x42) & (~0x02);
		gc2015_i2c_write(sd, ctrlreg[1], 2);
		
		num = ((sizeof(GC2015_white_balance_fluorescent)/sizeof(GC2015_white_balance_fluorescent[0])));			
		for (i = 0; i < num ; i++)
			gc2015_i2c_write(sd, GC2015_white_balance_fluorescent[i], 2);
	break;
	}
//	mdelay(100);
}

static int gc2015_enum_framesizes(struct v4l2_subdev *sd, struct v4l2_frmsizeenum *fsize)
{
	int err = 0;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
 	struct gc2015_state *state = to_state(sd);

	fsize->discrete.width = cam_res_list[state->framesize_index].width;
	fsize->discrete.height = cam_res_list[state->framesize_index].height;
  PRINTK(1, "%s width=%d height=%d\n", __func__, fsize->discrete.width, fsize->discrete.height);
	return err;
}

static int gc2015_try_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *f)
{
	int i;
 	struct i2c_client *client = v4l2_get_subdevdata(sd);
 	struct gc2015_state *state = to_state(sd);

  PRINTK(1, "%s\n", __func__);
 	PRINTK(1, "requested res(%d, %d)\n", f->width, f->height);

	for(i = 0; i < N_RESOLUTIONS; i++) {
		if (f->width > cam_res_list[i].width - 20 && f->width < cam_res_list[i].width + 20 &&
  			f->height > cam_res_list[i].height - 20 && f->height < cam_res_list[i].height + 20)
			break;
	}

	if(i == N_RESOLUTIONS)
    i = N_RESOLUTIONS-1;

	f->width = cam_res_list[i].width;
	f->height = cam_res_list[i].height;
  state->framesize_index = i;
	return i;
}

/*
  V4L2_CID_CAMERA_AUTO_FOCUS_RESULT
  V4L2_CID_CAMERA_EXIF_ISO
  V4L2_CID_CAMERA_EXIF_TV
  V4L2_CID_CAMERA_EXIF_BV
  V4L2_CID_CAMERA_EXIF_EBV
*/
static int gc2015_g_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	int err = -EINVAL;

  PRINTK(1, "%s %d\n", __func__, ctrl->id - V4L2_CID_PRIVATE_BASE);

	switch (ctrl->id) {
	case V4L2_CID_EXPOSURE:
		//ctrl->value = userset.exposure_bias;
		ctrl->value = 0;
		err = 0;
		break;

	case V4L2_CID_AUTO_WHITE_BALANCE:
		//ctrl->value = userset.auto_wb;
		ctrl->value = 0;
		err = 0;
		break;

	case V4L2_CID_WHITE_BALANCE_PRESET:
		//ctrl->value = userset.manual_wb;
		ctrl->value = 0;
		err = 0;
		break;

	case V4L2_CID_COLORFX:
		//ctrl->value = userset.effect;
		ctrl->value = 0;
		err = 0;
		break;

	case V4L2_CID_CONTRAST:
		//ctrl->value = userset.contrast;
		ctrl->value = 0;
		err = 0;
		break;

	case V4L2_CID_SATURATION:
		//ctrl->value = userset.saturation;
		ctrl->value = 0;
		err = 0;
		break;

	case V4L2_CID_SHARPNESS:
		//ctrl->value = userset.saturation;
	  ctrl->value = 0;	
		err = 0;
		break;


	default:
		dev_err(&client->dev, "%s: no such ctrl process %d\n", __func__, ctrl->id - V4L2_CID_PRIVATE_BASE);
		break;
	}

	return err;
}

/*
  V4L2_CID_CAMERA_ZOOM
  V4L2_CID_CAM_JPEG_QUALITY
  V4L2_CID_CAMERA_BRIGHTNESS
  V4L2_CID_CAMERA_SCENE_MODE
  V4L2_CID_CAMERA_FLASH_MODE
  V4L2_CID_CAMERA_WHITE_BALANCE
*/
static int gc2015_s_ctrl(struct v4l2_subdev *sd, struct v4l2_control *ctrl)
{
	int err = -EINVAL;
	struct i2c_client *client = v4l2_get_subdevdata(sd);

  PRINTK(1, "%s %d\n", __func__, ctrl->id - V4L2_CID_PRIVATE_BASE);

	switch (ctrl->id) {
	case V4L2_CID_CAMERA_BRIGHTNESS:
	  if(cur_userset->brightness == ctrl->value)
	    return 0;
		dev_dbg(&client->dev, "%s: V4L2_CID_CAMERA_BRIGHTNESS\n", __func__);
    gc2015_brightness_preset(sd, ctrl->value);
		cur_userset->brightness = ctrl->value;
		err = 0;
		break;

	case V4L2_CID_CAMERA_WHITE_BALANCE:
	  if(cur_userset->manual_wb == ctrl->value)
	    return 0;
		dev_dbg(&client->dev, "%s: V4L2_CID_AUTO_WHITE_BALANCE\n", __func__);
		gc2015_white_balance_preset(sd, ctrl->value);
    cur_userset->manual_wb = ctrl->value;
		err = 0;
		break;
	case V4L2_CID_CAMERA_FLASH_MODE:
		if(3 == ctrl->value )
		{
			gpio_direction_output(GPIO_FLASH_MODE, 0); 
			gpio_direction_output(GPIO_FLASH_EN, 1); 
	  }else if(1 == ctrl->value){
	  	msleep(400);
			gpio_direction_output(GPIO_FLASH_MODE, 0); 
			gpio_direction_output(GPIO_FLASH_EN, 0); 
	  }
		break;
	case V4L2_CID_CAM_JPEG_QUALITY:
		err = 0;
		break;	
	default:
		dev_err(&client->dev, "%s: no such control no process %d\n", __func__, ctrl->id - V4L2_CID_PRIVATE_BASE);
//		err = 0;
		break;
	}
	return err;
}

static int gc2015_init(struct v4l2_subdev *sd, u32 val)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	struct gc2015_state *state = to_state(sd);
	int err = -EINVAL, i;
	unsigned char initreg[2] = {0x12, 0x80};

//	v4l_info(client, "%s: camera initialization start\n", __func__);
  PRINTK(1, "%s\n", __func__);

//  if(gc2015_detect(sd) == 0)
//	  dev_info(&client->dev, "gc2015 has been probed\n");
//	else
//	{
//	  dev_info(&client->dev, "gc2015 has not found\n");
//	  return -EIO;
//	}

  if(initted == 0)
  {
  	err = gc2015_i2c_write(sd, initreg, 2);
  	if (err < 0)
  		v4l_info(client, "%s: init register set failed\n", __func__);
//  	msleep(100);
  
  	for (i = 0; i < GC2015_INIT_REGS; i++) {
  		err = gc2015_i2c_write(sd, GC2015_init_reg[i], sizeof(GC2015_init_reg[i]));
  		if (err < 0)
  			v4l_info(client, "%s: %d register set failed\n", __func__, i);
  	}
  
  	if (err < 0) {
  		v4l_err(client, "%s: camera initialization failed\n",	__func__);
  		return -EIO;	/* FIXME */
  	}

  	gc2015_brightness_preset(sd, cur_userset->brightness);
  	gc2015_white_balance_preset(sd, cur_userset->manual_wb);
	  initted = 1;
	  state->framesize_index = 2;
//	  msleep(100);
	}
//	else
//	  msleep(150);
	return 0;
}

static int gc2015_s_stream(struct v4l2_subdev *sd, int enable)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);
  PRINTK(1, "%s %d\n", __func__, enable);
	return 0;
}

static int gc2015_s_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
  int index, i, num;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
 	struct gc2015_state *state = to_state(sd);

  PRINTK(1, "%s %d %d\n", __func__, fmt->width, fmt->height);

  if((fmt->height == cam_res_list[state->framesize_index].height) && (fmt->width == cam_res_list[state->framesize_index].width))
    return 0;

  index = gc2015_try_fmt(sd, fmt);
  if(index == -EINVAL)
    return -EINVAL;

  switch(fmt->width)
  {
    case 640:
  		num = ((sizeof(GC2015_capture_vga)/sizeof(GC2015_capture_vga[0])));			
  		for(i=0; i<num; i++)
  			gc2015_i2c_write(sd, GC2015_capture_vga[i], 2);
      break;
    case 1280:
  		num = ((sizeof(GC2015_capture_sxga)/sizeof(GC2015_capture_sxga[0])));			
  		for(i=0; i<num; i++)
  			gc2015_i2c_write(sd, GC2015_capture_sxga[i], 2);
      break;
    case 1600:
  		num = ((sizeof(GC2015_capture_uxga)/sizeof(GC2015_capture_uxga[0])));			
  		for(i=0; i<num; i++)
  			gc2015_i2c_write(sd, GC2015_capture_uxga[i], 2);
      break;
    case 800:
    default:
  		num = ((sizeof(GC2015_capture_svga)/sizeof(GC2015_capture_svga[0])));			
  		for(i=0; i<num; i++)
  			gc2015_i2c_write(sd, GC2015_capture_svga[i], 2);
      break;
  }

//  msleep(100);
	return 0;
}

static const struct v4l2_subdev_core_ops gc2015_core_ops = {
	.init = gc2015_init,	/* initializing API */
	.g_ctrl = gc2015_g_ctrl,
	.s_ctrl = gc2015_s_ctrl,
};

static const struct v4l2_subdev_video_ops gc2015_video_ops = {
	.enum_framesizes = gc2015_enum_framesizes,
	.s_mbus_fmt = gc2015_s_fmt,
	.s_stream = gc2015_s_stream,
};

static const struct v4l2_subdev_ops gc2015_ops = {
	.core = &gc2015_core_ops,
	.video = &gc2015_video_ops,
};

/*
 * gc2015_probe
 * Fetching platform data is being done with s_config subdev call.
 * In probe routine, we just register subdev device
 */
static int gc2015_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct gc2015_state *state;
	struct v4l2_subdev *sd;
	int ret = 0;

	state = kzalloc(sizeof(struct gc2015_state), GFP_KERNEL);
	if (state == NULL)
		return -ENOMEM;

  cur_userset = &state->userset;
	sd = &state->sd;
	strcpy(sd->name, GC2015_DRIVER_NAME);

	/* Registering subdev */
	v4l2_i2c_subdev_init(sd, client, &gc2015_ops);
	printk("%s success\n", __func__);
  
	cur_userset->manual_wb = 1;
	cur_userset->brightness = EV_DEFAULT;
  initted = 0;
	return ret;
}

static int gc2015_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);

	v4l2_device_unregister_subdev(sd);
	kfree(to_state(sd));
	return 0;
}

static const struct i2c_device_id gc2015_id[] = {
	{ GC2015_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, gc2015_id);

static struct i2c_driver gc2015_i2c_driver = {
	.driver = {
		.name	= GC2015_DRIVER_NAME,
	},
	.probe		= gc2015_probe,
	.remove		= gc2015_remove,
	.id_table	= gc2015_id,
};

static int __init gc2015_mod_init(void)
{
	return i2c_add_driver(&gc2015_i2c_driver);
}

static void __exit gc2015_mod_exit(void)
{
	i2c_del_driver(&gc2015_i2c_driver);
}
module_init(gc2015_mod_init);
module_exit(gc2015_mod_exit);

MODULE_DESCRIPTION("GC2015 camera driver");
MODULE_AUTHOR("Apollo Yang <Apollo5520@gmail.como>");
MODULE_LICENSE("GPL");
