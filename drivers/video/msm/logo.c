/* drivers/video/msm/logo.c
 *
 * Show Logo in RLE 565 format
 *
 * Copyright (C) 2008 Google Incorporated
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
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fb.h>
#include <linux/vt_kern.h>
#include <linux/unistd.h>
#include <linux/syscalls.h>

#include <linux/irq.h>
#include <asm/system.h>

#define fb_width(fb)	((fb)->var.xres)
#define fb_height(fb)	((fb)->var.yres)
#define fb_size(fb)	((fb)->var.xres * (fb)->var.yres * 2)

#if defined(CONFIG_MACH_ACER_A9)
static void cp_to_fb32(unsigned short *bits, unsigned short *ptr, unsigned max,
			unsigned count, struct fb_info *info)
{
	uint32_t rgb32, red, green, blue, alpha, n;
	uint32_t fb_cnt = 0;
	uint32_t *fb = (uint32_t *)bits;
	uint32_t actual_line_bytes = 0, dummy = 0;
	uint32_t line_base = 0;

	if (info) {
		actual_line_bytes = info->fix.line_length / (info->var.bits_per_pixel / 8);
		dummy = actual_line_bytes - info->var.xres;
	}

	while (count > 3) {
		n = ptr[0];
		if (n > max)
			break;

		/* convert 16 bits to 32 bits */
		rgb32 = ((ptr[1] >> 11) & 0x1F);
		red = (rgb32 << 3) | (rgb32 >> 2);
		rgb32 = ((ptr[1] >> 5) & 0x3F);
		green = (rgb32 << 2) | (rgb32 >> 4);
		rgb32 = ((ptr[1]) & 0x1F);
		blue = (rgb32 << 3) | (rgb32 >> 2);
		alpha = 0xff;
		rgb32 = (alpha << 24) | (blue << 16) | (green << 8) | (red);

		max -= n;
		/* fix->line_length is not the same as actual line length
		 * Fix it here */
		while (n--) {
			*(fb + fb_cnt++)  = rgb32;
			if (fb_cnt == (line_base + info->var.xres)) {
				fb_cnt += dummy;
				line_base = (fb_cnt / actual_line_bytes) * actual_line_bytes;
			}
		}
		ptr += 2;
		count -= 4;
	}
}
#else
static void memset16(void *_ptr, unsigned short val, unsigned count)
{
	unsigned short *ptr = _ptr;
	count >>= 1;
	while (count--)
		*ptr++ = val;
}
#endif


/* 565RLE image format: [count(2 bytes), rle(2 bytes)] */
int load_565rle_image(char *filename, bool bf_supported)
{
	struct fb_info *info;
	int fd, count, err = 0;
	unsigned max;
	unsigned short *data, *bits, *ptr;

	info = registered_fb[0];
	if (!info) {
		printk(KERN_WARNING "%s: Can not access framebuffer\n",
			__func__);
		return -ENODEV;
	}

	fd = sys_open(filename, O_RDONLY, 0);
	if (fd < 0) {
		printk(KERN_WARNING "%s: Can not open %s\n",
			__func__, filename);
		return -ENOENT;
	}
	count = sys_lseek(fd, (off_t)0, 2);
	if (count <= 0) {
		err = -EIO;
		goto err_logo_close_file;
	}
	sys_lseek(fd, (off_t)0, 0);
	data = kmalloc(count, GFP_KERNEL);
	if (!data) {
		printk(KERN_WARNING "%s: Can not alloc data\n", __func__);
		err = -ENOMEM;
		goto err_logo_close_file;
	}
	if (sys_read(fd, (char *)data, count) != count) {
		err = -EIO;
		goto err_logo_free_data;
	}

	max = fb_width(info) * fb_height(info);
	ptr = data;
	if (bf_supported && (info->node == 1 || info->node == 2)) {
		err = -EPERM;
		pr_err("%s:%d no info->creen_base on fb%d!\n",
		       __func__, __LINE__, info->node);
		goto err_logo_free_data;
	}
	bits = (unsigned short *)(info->screen_base);
#if !defined(CONFIG_MACH_ACER_A9)
	while (count > 3) {
		unsigned n = ptr[0];
		if (n > max)
			break;
		memset16(bits, ptr[1], n << 1);
		bits += n;
		max -= n;
		ptr += 2;
		count -= 4;
	}
#else
	cp_to_fb32(bits, ptr, max, count, info);
#endif

err_logo_free_data:
	kfree(data);
err_logo_close_file:
	sys_close(fd);
	return err;
}
EXPORT_SYMBOL(load_565rle_image);
