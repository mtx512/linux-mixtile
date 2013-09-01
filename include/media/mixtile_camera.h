/* linux/include/media/mixtile_camera.h
*/

#include <linux/device.h>
#include <media/v4l2-mediabus.h>

struct mixtile_camera_data {
	unsigned int default_width;
	unsigned int default_height;
	unsigned int pixelformat;
	int freq;	/* MCLK in KHz */
	/* This SoC supports Parallel & CSI-2 */
	int is_mipi;
};

