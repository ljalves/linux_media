/*
 * Realtek RTL2832U SDR driver
 *
 * Copyright (C) 2013 Antti Palosaari <crope@iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * GNU Radio plugin "gr-kernel" for device usage will be on:
 * http://git.linuxtv.org/anttip/gr-kernel.git
 *
 * TODO:
 * Help is very highly welcome for these + all the others you could imagine:
 * - move controls to V4L2 API
 * - use libv4l2 for stream format conversions
 * - gr-kernel: switch to v4l2_mmap (current read eats a lot of cpu)
 * - SDRSharp support
 */

#include "dvb_frontend.h"
#include "rtl2832_sdr.h"
#include "dvb_usb.h"

#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-event.h>
#include <media/videobuf2-vmalloc.h>

/* TODO: These should be moved to V4L2 API */
#define RTL2832_SDR_CID_SAMPLING_MODE       ((V4L2_CID_USER_BASE | 0xf000) +  0)
#define RTL2832_SDR_CID_SAMPLING_RATE       ((V4L2_CID_USER_BASE | 0xf000) +  1)
#define RTL2832_SDR_CID_SAMPLING_RESOLUTION ((V4L2_CID_USER_BASE | 0xf000) +  2)
#define RTL2832_SDR_CID_TUNER_RF            ((V4L2_CID_USER_BASE | 0xf000) + 10)
#define RTL2832_SDR_CID_TUNER_BW            ((V4L2_CID_USER_BASE | 0xf000) + 11)
#define RTL2832_SDR_CID_TUNER_IF            ((V4L2_CID_USER_BASE | 0xf000) + 12)
#define RTL2832_SDR_CID_TUNER_GAIN          ((V4L2_CID_USER_BASE | 0xf000) + 13)

#define MAX_BULK_BUFS            (8)
#define BULK_BUFFER_SIZE         (8 * 512)

/* intermediate buffers with raw data from the USB device */
struct rtl2832_sdr_frame_buf {
	struct vb2_buffer vb;   /* common v4l buffer stuff -- must be first */
	struct list_head list;
};

struct rtl2832_sdr_state {
#define POWER_ON           (1 << 1)
#define URB_BUF            (1 << 2)
	unsigned long flags;

	const struct rtl2832_config *cfg;
	struct dvb_frontend *fe;
	struct dvb_usb_device *d;
	struct i2c_adapter *i2c;
	u8 bank;

	struct video_device vdev;
	struct v4l2_device v4l2_dev;

	/* videobuf2 queue and queued buffers list */
	struct vb2_queue vb_queue;
	struct list_head queued_bufs;
	spinlock_t queued_bufs_lock; /* Protects queued_bufs */

	/* Note if taking both locks v4l2_lock must always be locked first! */
	struct mutex v4l2_lock;      /* Protects everything else */
	struct mutex vb_queue_lock;  /* Protects vb_queue and capt_file */

	/* Pointer to our usb_device, will be NULL after unplug */
	struct usb_device *udev; /* Both mutexes most be hold when setting! */

	unsigned int vb_full; /* vb is full and packets dropped */

	struct urb     *urb_list[MAX_BULK_BUFS];
	int            buf_num;
	unsigned long  buf_size;
	u8             *buf_list[MAX_BULK_BUFS];
	dma_addr_t     dma_addr[MAX_BULK_BUFS];
	int urbs_initialized;
	int urbs_submitted;

	/* Controls */
	struct v4l2_ctrl_handler ctrl_handler;
	struct v4l2_ctrl *ctrl_sampling_rate;
	struct v4l2_ctrl *ctrl_tuner_rf;
	struct v4l2_ctrl *ctrl_tuner_bw;
	struct v4l2_ctrl *ctrl_tuner_if;
	struct v4l2_ctrl *ctrl_tuner_gain;

	/* for sample rate calc */
	unsigned int sample;
	unsigned int sample_measured;
	unsigned long jiffies;
};

/* write multiple hardware registers */
static int rtl2832_sdr_wr(struct rtl2832_sdr_state *s, u8 reg, const u8 *val,
		int len)
{
	int ret;
	u8 buf[1 + len];
	struct i2c_msg msg[1] = {
		{
			.addr = s->cfg->i2c_addr,
			.flags = 0,
			.len = 1 + len,
			.buf = buf,
		}
	};

	buf[0] = reg;
	memcpy(&buf[1], val, len);

	ret = i2c_transfer(s->i2c, msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		dev_err(&s->i2c->dev,
			"%s: I2C wr failed=%d reg=%02x len=%d\n",
			KBUILD_MODNAME, ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

#if 0
/* read multiple hardware registers */
static int rtl2832_sdr_rd(struct rtl2832_sdr_state *s, u8 reg, u8 *val, int len)
{
	int ret;
	struct i2c_msg msg[2] = {
		{
			.addr = s->cfg->i2c_addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		}, {
			.addr = s->cfg->i2c_addr,
			.flags = I2C_M_RD,
			.len = len,
			.buf = val,
		}
	};

	ret = i2c_transfer(s->i2c, msg, 2);
	if (ret == 2) {
		ret = 0;
	} else {
		dev_err(&s->i2c->dev,
				"%s: I2C rd failed=%d reg=%02x len=%d\n",
				KBUILD_MODNAME, ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}
#endif

/* write multiple registers */
static int rtl2832_sdr_wr_regs(struct rtl2832_sdr_state *s, u16 reg,
		const u8 *val, int len)
{
	int ret;
	u8 reg2 = (reg >> 0) & 0xff;
	u8 bank = (reg >> 8) & 0xff;

	/* switch bank if needed */
	if (bank != s->bank) {
		ret = rtl2832_sdr_wr(s, 0x00, &bank, 1);
		if (ret)
			return ret;

		s->bank = bank;
	}

	return rtl2832_sdr_wr(s, reg2, val, len);
}

#if 0
/* read multiple registers */
static int rtl2832_sdr_rd_regs(struct rtl2832_sdr_state *s, u16 reg, u8 *val,
		int len)
{
	int ret;
	u8 reg2 = (reg >> 0) & 0xff;
	u8 bank = (reg >> 8) & 0xff;

	/* switch bank if needed */
	if (bank != s->bank) {
		ret = rtl2832_sdr_wr(s, 0x00, &bank, 1);
		if (ret)
			return ret;

		s->bank = bank;
	}

	return rtl2832_sdr_rd(s, reg2, val, len);
}
#endif

/* write single register */
static int rtl2832_sdr_wr_reg(struct rtl2832_sdr_state *s, u16 reg, u8 val)
{
	return rtl2832_sdr_wr_regs(s, reg, &val, 1);
}

#if 0
/* read single register */
static int rtl2832_sdr_rd_reg(struct rtl2832_sdr_state *s, u16 reg, u8 *val)
{
	return rtl2832_sdr_rd_regs(s, reg, val, 1);
}

/* write single register with mask */
static int rtl2832_sdr_wr_reg_mask(struct rtl2832_sdr_state *s, u16 reg,
		u8 val, u8 mask)
{
	int ret;
	u8 tmp;

	/* no need for read if whole reg is written */
	if (mask != 0xff) {
		ret = rtl2832_sdr_rd_regs(s, reg, &tmp, 1);
		if (ret)
			return ret;

		val &= mask;
		tmp &= ~mask;
		val |= tmp;
	}

	return rtl2832_sdr_wr_regs(s, reg, &val, 1);
}

/* read single register with mask */
static int rtl2832_sdr_rd_reg_mask(struct rtl2832_sdr_state *s, u16 reg,
		u8 *val, u8 mask)
{
	int ret, i;
	u8 tmp;

	ret = rtl2832_sdr_rd_regs(s, reg, &tmp, 1);
	if (ret)
		return ret;

	tmp &= mask;

	/* find position of the first bit */
	for (i = 0; i < 8; i++) {
		if ((mask >> i) & 0x01)
			break;
	}
	*val = tmp >> i;

	return 0;
}
#endif

/* Private functions */
static struct rtl2832_sdr_frame_buf *rtl2832_sdr_get_next_fill_buf(
		struct rtl2832_sdr_state *s)
{
	unsigned long flags = 0;
	struct rtl2832_sdr_frame_buf *buf = NULL;

	spin_lock_irqsave(&s->queued_bufs_lock, flags);
	if (list_empty(&s->queued_bufs))
		goto leave;

	buf = list_entry(s->queued_bufs.next,
			struct rtl2832_sdr_frame_buf, list);
	list_del(&buf->list);
leave:
	spin_unlock_irqrestore(&s->queued_bufs_lock, flags);
	return buf;
}

/*
 * Integer to 32-bit IEEE floating point representation routine is taken
 * from Radeon R600 driver (drivers/gpu/drm/radeon/r600_blit_kms.c).
 *
 * TODO: Currently we do conversion here in Kernel, but in future that will
 * be moved to the libv4l2 library as video format conversions are.
 */
#define I2F_FRAC_BITS  23
#define I2F_MASK ((1 << I2F_FRAC_BITS) - 1)

/*
 * Converts signed 8-bit integer into 32-bit IEEE floating point
 * representation.
 */
static u32 rtl2832_sdr_convert_sample(struct rtl2832_sdr_state *s, u16 x)
{
	u32 msb, exponent, fraction, sign;

	/* Zero is special */
	if (!x)
		return 0;

	/* Negative / positive value */
	if (x & (1 << 7)) {
		x = -x;
		x &= 0x7f; /* result is 7 bit ... + sign */
		sign = 1 << 31;
	} else {
		sign = 0 << 31;
	}

	/* Get location of the most significant bit */
	msb = __fls(x);

	fraction = ror32(x, (msb - I2F_FRAC_BITS) & 0x1f) & I2F_MASK;
	exponent = (127 + msb) << I2F_FRAC_BITS;

	return (fraction + exponent) | sign;
}

static unsigned int rtl2832_sdr_convert_stream(struct rtl2832_sdr_state *s,
		u32 *dst, const u8 *src, unsigned int src_len)
{
	unsigned int i, dst_len = 0;

	for (i = 0; i < src_len; i++)
		*dst++ = rtl2832_sdr_convert_sample(s, src[i]);

	/* 8-bit to 32-bit IEEE floating point */
	dst_len = src_len * 4;

	/* calculate samping rate and output it in 10 seconds intervals */
	if ((s->jiffies + msecs_to_jiffies(10000)) <= jiffies) {
		unsigned long jiffies_now = jiffies;
		unsigned long msecs = jiffies_to_msecs(jiffies_now) - jiffies_to_msecs(s->jiffies);
		unsigned int samples = s->sample - s->sample_measured;
		s->jiffies = jiffies_now;
		s->sample_measured = s->sample;
		dev_dbg(&s->udev->dev,
				"slen=%d samples=%u msecs=%lu sampling rate=%lu\n",
				src_len, samples, msecs,
				samples * 1000UL / msecs);
	}

	/* total number of I+Q pairs */
	s->sample += src_len / 2;

	return dst_len;
}

/*
 * This gets called for the bulk stream pipe. This is done in interrupt
 * time, so it has to be fast, not crash, and not stall. Neat.
 */
static void rtl2832_sdr_urb_complete(struct urb *urb)
{
	struct rtl2832_sdr_state *s = urb->context;
	struct rtl2832_sdr_frame_buf *fbuf;

	dev_dbg_ratelimited(&s->udev->dev,
			"%s: status=%d length=%d/%d errors=%d\n",
			__func__, urb->status, urb->actual_length,
			urb->transfer_buffer_length, urb->error_count);

	switch (urb->status) {
	case 0:             /* success */
	case -ETIMEDOUT:    /* NAK */
		break;
	case -ECONNRESET:   /* kill */
	case -ENOENT:
	case -ESHUTDOWN:
		return;
	default:            /* error */
		dev_err_ratelimited(&s->udev->dev, "urb failed=%d\n",
				urb->status);
		break;
	}

	if (urb->actual_length > 0) {
		void *ptr;
		unsigned int len;
		/* get free framebuffer */
		fbuf = rtl2832_sdr_get_next_fill_buf(s);
		if (fbuf == NULL) {
			s->vb_full++;
			dev_notice_ratelimited(&s->udev->dev,
					"videobuf is full, %d packets dropped\n",
					s->vb_full);
			goto skip;
		}

		/* fill framebuffer */
		ptr = vb2_plane_vaddr(&fbuf->vb, 0);
		len = rtl2832_sdr_convert_stream(s, ptr, urb->transfer_buffer,
				urb->actual_length);
		vb2_set_plane_payload(&fbuf->vb, 0, len);
		vb2_buffer_done(&fbuf->vb, VB2_BUF_STATE_DONE);
	}
skip:
	usb_submit_urb(urb, GFP_ATOMIC);
}

static int rtl2832_sdr_kill_urbs(struct rtl2832_sdr_state *s)
{
	int i;

	for (i = s->urbs_submitted - 1; i >= 0; i--) {
		dev_dbg(&s->udev->dev, "%s: kill urb=%d\n", __func__, i);
		/* stop the URB */
		usb_kill_urb(s->urb_list[i]);
	}
	s->urbs_submitted = 0;

	return 0;
}

static int rtl2832_sdr_submit_urbs(struct rtl2832_sdr_state *s)
{
	int i, ret;

	for (i = 0; i < s->urbs_initialized; i++) {
		dev_dbg(&s->udev->dev, "%s: submit urb=%d\n", __func__, i);
		ret = usb_submit_urb(s->urb_list[i], GFP_ATOMIC);
		if (ret) {
			dev_err(&s->udev->dev,
					"Could not submit urb no. %d - get them all back\n",
					i);
			rtl2832_sdr_kill_urbs(s);
			return ret;
		}
		s->urbs_submitted++;
	}

	return 0;
}

static int rtl2832_sdr_free_stream_bufs(struct rtl2832_sdr_state *s)
{
	if (s->flags & USB_STATE_URB_BUF) {
		while (s->buf_num) {
			s->buf_num--;
			dev_dbg(&s->udev->dev, "%s: free buf=%d\n",
					__func__, s->buf_num);
			usb_free_coherent(s->udev, s->buf_size,
					  s->buf_list[s->buf_num],
					  s->dma_addr[s->buf_num]);
		}
	}
	s->flags &= ~USB_STATE_URB_BUF;

	return 0;
}

static int rtl2832_sdr_alloc_stream_bufs(struct rtl2832_sdr_state *s)
{
	s->buf_num = 0;
	s->buf_size = BULK_BUFFER_SIZE;

	dev_dbg(&s->udev->dev,
			"%s: all in all I will use %u bytes for streaming\n",
			__func__,  MAX_BULK_BUFS * BULK_BUFFER_SIZE);

	for (s->buf_num = 0; s->buf_num < MAX_BULK_BUFS; s->buf_num++) {
		s->buf_list[s->buf_num] = usb_alloc_coherent(s->udev,
				BULK_BUFFER_SIZE, GFP_ATOMIC,
				&s->dma_addr[s->buf_num]);
		if (!s->buf_list[s->buf_num]) {
			dev_dbg(&s->udev->dev, "%s: alloc buf=%d failed\n",
					__func__, s->buf_num);
			rtl2832_sdr_free_stream_bufs(s);
			return -ENOMEM;
		}

		dev_dbg(&s->udev->dev, "%s: alloc buf=%d %p (dma %llu)\n",
				__func__, s->buf_num,
				s->buf_list[s->buf_num],
				(long long)s->dma_addr[s->buf_num]);
		s->flags |= USB_STATE_URB_BUF;
	}

	return 0;
}

static int rtl2832_sdr_free_urbs(struct rtl2832_sdr_state *s)
{
	int i;

	rtl2832_sdr_kill_urbs(s);

	for (i = s->urbs_initialized - 1; i >= 0; i--) {
		if (s->urb_list[i]) {
			dev_dbg(&s->udev->dev, "%s: free urb=%d\n",
					__func__, i);
			/* free the URBs */
			usb_free_urb(s->urb_list[i]);
		}
	}
	s->urbs_initialized = 0;

	return 0;
}

static int rtl2832_sdr_alloc_urbs(struct rtl2832_sdr_state *s)
{
	int i, j;

	/* allocate the URBs */
	for (i = 0; i < MAX_BULK_BUFS; i++) {
		dev_dbg(&s->udev->dev, "%s: alloc urb=%d\n", __func__, i);
		s->urb_list[i] = usb_alloc_urb(0, GFP_ATOMIC);
		if (!s->urb_list[i]) {
			dev_dbg(&s->udev->dev, "%s: failed\n", __func__);
			for (j = 0; j < i; j++)
				usb_free_urb(s->urb_list[j]);
			return -ENOMEM;
		}
		usb_fill_bulk_urb(s->urb_list[i],
				s->udev,
				usb_rcvbulkpipe(s->udev, 0x81),
				s->buf_list[i],
				BULK_BUFFER_SIZE,
				rtl2832_sdr_urb_complete, s);

		s->urb_list[i]->transfer_flags = URB_NO_TRANSFER_DMA_MAP;
		s->urb_list[i]->transfer_dma = s->dma_addr[i];
		s->urbs_initialized++;
	}

	return 0;
}

/* Must be called with vb_queue_lock hold */
static void rtl2832_sdr_cleanup_queued_bufs(struct rtl2832_sdr_state *s)
{
	unsigned long flags = 0;
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	spin_lock_irqsave(&s->queued_bufs_lock, flags);
	while (!list_empty(&s->queued_bufs)) {
		struct rtl2832_sdr_frame_buf *buf;
		buf = list_entry(s->queued_bufs.next,
				struct rtl2832_sdr_frame_buf, list);
		list_del(&buf->list);
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
	}
	spin_unlock_irqrestore(&s->queued_bufs_lock, flags);
}

/* The user yanked out the cable... */
static void rtl2832_sdr_release_sec(struct dvb_frontend *fe)
{
	struct rtl2832_sdr_state *s = fe->sec_priv;
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	mutex_lock(&s->vb_queue_lock);
	mutex_lock(&s->v4l2_lock);
	/* No need to keep the urbs around after disconnection */
	s->udev = NULL;

	v4l2_device_disconnect(&s->v4l2_dev);
	video_unregister_device(&s->vdev);
	mutex_unlock(&s->v4l2_lock);
	mutex_unlock(&s->vb_queue_lock);

	v4l2_device_put(&s->v4l2_dev);

	fe->sec_priv = NULL;
}

static int rtl2832_sdr_querycap(struct file *file, void *fh,
		struct v4l2_capability *cap)
{
	struct rtl2832_sdr_state *s = video_drvdata(file);
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	strlcpy(cap->driver, KBUILD_MODNAME, sizeof(cap->driver));
	strlcpy(cap->card, s->vdev.name, sizeof(cap->card));
	usb_make_path(s->udev, cap->bus_info, sizeof(cap->bus_info));
	cap->device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING |
			V4L2_CAP_READWRITE;
	cap->device_caps = V4L2_CAP_TUNER;
	cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS;
	return 0;
}

/* Videobuf2 operations */
static int rtl2832_sdr_queue_setup(struct vb2_queue *vq,
		const struct v4l2_format *fmt, unsigned int *nbuffers,
		unsigned int *nplanes, unsigned int sizes[], void *alloc_ctxs[])
{
	struct rtl2832_sdr_state *s = vb2_get_drv_priv(vq);
	dev_dbg(&s->udev->dev, "%s: *nbuffers=%d\n", __func__, *nbuffers);

	/* Absolute min and max number of buffers available for mmap() */
	*nbuffers = 32;
	*nplanes = 1;
	sizes[0] = PAGE_ALIGN(BULK_BUFFER_SIZE * 4); /* 8 * 512 * 4 = 16384 */
	dev_dbg(&s->udev->dev, "%s: nbuffers=%d sizes[0]=%d\n",
			__func__, *nbuffers, sizes[0]);
	return 0;
}

static int rtl2832_sdr_buf_prepare(struct vb2_buffer *vb)
{
	struct rtl2832_sdr_state *s = vb2_get_drv_priv(vb->vb2_queue);

	/* Don't allow queing new buffers after device disconnection */
	if (!s->udev)
		return -ENODEV;

	return 0;
}

static void rtl2832_sdr_buf_queue(struct vb2_buffer *vb)
{
	struct rtl2832_sdr_state *s = vb2_get_drv_priv(vb->vb2_queue);
	struct rtl2832_sdr_frame_buf *buf =
			container_of(vb, struct rtl2832_sdr_frame_buf, vb);
	unsigned long flags = 0;

	/* Check the device has not disconnected between prep and queuing */
	if (!s->udev) {
		vb2_buffer_done(&buf->vb, VB2_BUF_STATE_ERROR);
		return;
	}

	spin_lock_irqsave(&s->queued_bufs_lock, flags);
	list_add_tail(&buf->list, &s->queued_bufs);
	spin_unlock_irqrestore(&s->queued_bufs_lock, flags);
}

static int rtl2832_sdr_set_adc(struct rtl2832_sdr_state *s)
{
	struct dvb_frontend *fe = s->fe;
	int ret;
	unsigned int f_sr, f_if;
	u8 buf[3], tmp;
	u64 u64tmp;
	u32 u32tmp;

	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	if (!test_bit(POWER_ON, &s->flags))
		return 0;

	f_sr = s->ctrl_sampling_rate->val64;

	ret = rtl2832_sdr_wr_regs(s, 0x13e, "\x00\x00", 2);
	ret = rtl2832_sdr_wr_regs(s, 0x115, "\x00", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x116, "\x00\x00", 2);
	ret = rtl2832_sdr_wr_regs(s, 0x118, "\x00", 1);

	/* get IF from tuner */
	if (fe->ops.tuner_ops.get_if_frequency)
		ret = fe->ops.tuner_ops.get_if_frequency(fe, &f_if);
	else
		ret = -EINVAL;

	if (ret)
		goto err;

	/* program IF */
	u64tmp = f_if % s->cfg->xtal;
	u64tmp *= 0x400000;
	u64tmp = div_u64(u64tmp, s->cfg->xtal);
	u64tmp = -u64tmp;
	u32tmp = u64tmp & 0x3fffff;

	dev_dbg(&s->udev->dev, "%s: f_if=%u if_ctl=%08x\n",
			__func__, f_if, u32tmp);

	buf[0] = (u32tmp >> 16) & 0xff;
	buf[1] = (u32tmp >>  8) & 0xff;
	buf[2] = (u32tmp >>  0) & 0xff;

	ret = rtl2832_sdr_wr_regs(s, 0x119, buf, 3);
	if (ret)
		goto err;

	/* program BB / IF mode */
	if (f_if)
		tmp = 0x00;
	else
		tmp = 0x01;

	ret = rtl2832_sdr_wr_reg(s, 0x1b1, tmp);
	if (ret)
		goto err;

	ret = rtl2832_sdr_wr_regs(s, 0x19f, "\x03\x84", 2);
	ret = rtl2832_sdr_wr_regs(s, 0x1a1, "\x00\x00", 2);
	ret = rtl2832_sdr_wr_regs(s, 0x11c, "\xca", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x11d, "\xdc", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x11e, "\xd7", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x11f, "\xd8", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x120, "\xe0", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x121, "\xf2", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x122, "\x0e", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x123, "\x35", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x124, "\x06", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x125, "\x50", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x126, "\x9c", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x127, "\x0d", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x128, "\x71", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x129, "\x11", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x12a, "\x14", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x12b, "\x71", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x12c, "\x74", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x12d, "\x19", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x12e, "\x41", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x12f, "\xa5", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x017, "\x11", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x018, "\x10", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x019, "\x21", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x01f, "\xff", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x01e, "\x01", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x01d, "\x06", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x01c, "\x0d", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x01b, "\x16", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x01a, "\x1b", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x192, "\x00", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x193, "\xf0", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x194, "\x0f", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x061, "\x60", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x006, "\x80", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x112, "\x5a", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x102, "\x40", 1);

	if (s->cfg->tuner == RTL2832_TUNER_R820T) {
		ret = rtl2832_sdr_wr_regs(s, 0x115, "\x01", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x103, "\x80", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1c7, "\x24", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x104, "\xcc", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x105, "\xbe", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1c8, "\x14", 1);
	} else {
		ret = rtl2832_sdr_wr_regs(s, 0x103, "\x5a", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1c7, "\x30", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x104, "\xd0", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x105, "\xbe", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1c8, "\x18", 1);
	}

	ret = rtl2832_sdr_wr_regs(s, 0x106, "\x35", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x1c9, "\x21", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x1ca, "\x21", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x1cb, "\x00", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x107, "\x40", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x1cd, "\x10", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x1ce, "\x10", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x108, "\x80", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x109, "\x7f", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x10a, "\x80", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x10b, "\x7f", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x00e, "\xfc", 1);
	ret = rtl2832_sdr_wr_regs(s, 0x00e, "\xfc", 1);

	if (s->cfg->tuner == RTL2832_TUNER_R820T) {
		ret = rtl2832_sdr_wr_regs(s, 0x011, "\xf4", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x101, "\x14", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x101, "\x10", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x019, "\x21", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x116, "\x00\x00", 2);
		ret = rtl2832_sdr_wr_regs(s, 0x118, "\x00", 1);
	} else {
		ret = rtl2832_sdr_wr_regs(s, 0x011, "\xd4", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1e5, "\xf0", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1d9, "\x00", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1db, "\x00", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1dd, "\x14", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1de, "\xec", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1d8, "\x0c", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1e6, "\x02", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x1d7, "\x09", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x00d, "\x83", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x010, "\x49", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x00d, "\x87", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x00d, "\x85", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x013, "\x02", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x008, "\xcd", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x10c, "\x00", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x101, "\x14", 1);
		ret = rtl2832_sdr_wr_regs(s, 0x101, "\x10", 1);
	}

err:
	return ret;
};

static int rtl2832_sdr_set_tuner(struct rtl2832_sdr_state *s)
{
	struct dvb_frontend *fe = s->fe;
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;

	unsigned int f_rf = s->ctrl_tuner_rf->val64;

	/*
	 * bandwidth (Hz)
	 */
	unsigned int bandwidth = s->ctrl_tuner_bw->val;

	/*
	 * intermediate frequency (Hz)
	 */
	unsigned int f_if = s->ctrl_tuner_if->val;

	/*
	 * gain (dB)
	 */
	int gain = s->ctrl_tuner_gain->val;

	dev_dbg(&s->udev->dev,
			"%s: f_rf=%u bandwidth=%d f_if=%u gain=%d\n",
			__func__, f_rf, bandwidth, f_if, gain);

	if (!test_bit(POWER_ON, &s->flags))
		return 0;

	if (fe->ops.tuner_ops.init)
		fe->ops.tuner_ops.init(fe);

	c->bandwidth_hz = bandwidth;
	c->frequency = f_rf;

	if (fe->ops.tuner_ops.set_params)
		fe->ops.tuner_ops.set_params(fe);

	return 0;
};

static void rtl2832_sdr_unset_tuner(struct rtl2832_sdr_state *s)
{
	struct dvb_frontend *fe = s->fe;

	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	if (fe->ops.tuner_ops.sleep)
		fe->ops.tuner_ops.sleep(fe);

	return;
};

static int rtl2832_sdr_start_streaming(struct vb2_queue *vq, unsigned int count)
{
	struct rtl2832_sdr_state *s = vb2_get_drv_priv(vq);
	int ret;
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	if (!s->udev)
		return -ENODEV;

	if (mutex_lock_interruptible(&s->v4l2_lock))
		return -ERESTARTSYS;

	if (s->d->props->power_ctrl)
		s->d->props->power_ctrl(s->d, 1);

	set_bit(POWER_ON, &s->flags);

	ret = rtl2832_sdr_set_tuner(s);
	if (ret)
		goto err;

	ret = rtl2832_sdr_set_adc(s);
	if (ret)
		goto err;

	ret = rtl2832_sdr_alloc_stream_bufs(s);
	if (ret)
		goto err;

	ret = rtl2832_sdr_alloc_urbs(s);
	if (ret)
		goto err;

	ret = rtl2832_sdr_submit_urbs(s);
	if (ret)
		goto err;

err:
	mutex_unlock(&s->v4l2_lock);

	return ret;
}

static int rtl2832_sdr_stop_streaming(struct vb2_queue *vq)
{
	struct rtl2832_sdr_state *s = vb2_get_drv_priv(vq);
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	if (mutex_lock_interruptible(&s->v4l2_lock))
		return -ERESTARTSYS;

	rtl2832_sdr_kill_urbs(s);
	rtl2832_sdr_free_urbs(s);
	rtl2832_sdr_free_stream_bufs(s);
	rtl2832_sdr_cleanup_queued_bufs(s);
	rtl2832_sdr_unset_tuner(s);

	clear_bit(POWER_ON, &s->flags);

	if (s->d->props->power_ctrl)
		s->d->props->power_ctrl(s->d, 0);

	mutex_unlock(&s->v4l2_lock);

	return 0;
}

static struct vb2_ops rtl2832_sdr_vb2_ops = {
	.queue_setup            = rtl2832_sdr_queue_setup,
	.buf_prepare            = rtl2832_sdr_buf_prepare,
	.buf_queue              = rtl2832_sdr_buf_queue,
	.start_streaming        = rtl2832_sdr_start_streaming,
	.stop_streaming         = rtl2832_sdr_stop_streaming,
	.wait_prepare           = vb2_ops_wait_prepare,
	.wait_finish            = vb2_ops_wait_finish,
};

static int rtl2832_sdr_enum_input(struct file *file, void *fh, struct v4l2_input *i)
{
	if (i->index != 0)
		return -EINVAL;

	strlcpy(i->name, "SDR data", sizeof(i->name));
	i->type = V4L2_INPUT_TYPE_CAMERA;

	return 0;
}

static int rtl2832_sdr_g_input(struct file *file, void *fh, unsigned int *i)
{
	*i = 0;

	return 0;
}

static int rtl2832_sdr_s_input(struct file *file, void *fh, unsigned int i)
{
	return i ? -EINVAL : 0;
}

static int vidioc_s_tuner(struct file *file, void *priv,
		const struct v4l2_tuner *v)
{
	struct rtl2832_sdr_state *s = video_drvdata(file);
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	return 0;
}

static int vidioc_g_tuner(struct file *file, void *priv, struct v4l2_tuner *v)
{
	struct rtl2832_sdr_state *s = video_drvdata(file);
	dev_dbg(&s->udev->dev, "%s:\n", __func__);

	strcpy(v->name, "SDR RX");
	v->capability = V4L2_TUNER_CAP_LOW;

	return 0;
}

static int vidioc_s_frequency(struct file *file, void *priv,
		const struct v4l2_frequency *f)
{
	struct rtl2832_sdr_state *s = video_drvdata(file);
	dev_dbg(&s->udev->dev, "%s: frequency=%lu Hz (%u)\n",
			__func__, f->frequency * 625UL / 10UL, f->frequency);

	return v4l2_ctrl_s_ctrl_int64(s->ctrl_tuner_rf,
			f->frequency * 625UL / 10UL);
}

static const struct v4l2_ioctl_ops rtl2832_sdr_ioctl_ops = {
	.vidioc_querycap          = rtl2832_sdr_querycap,

	.vidioc_enum_input        = rtl2832_sdr_enum_input,
	.vidioc_g_input           = rtl2832_sdr_g_input,
	.vidioc_s_input           = rtl2832_sdr_s_input,

	.vidioc_reqbufs           = vb2_ioctl_reqbufs,
	.vidioc_create_bufs       = vb2_ioctl_create_bufs,
	.vidioc_prepare_buf       = vb2_ioctl_prepare_buf,
	.vidioc_querybuf          = vb2_ioctl_querybuf,
	.vidioc_qbuf              = vb2_ioctl_qbuf,
	.vidioc_dqbuf             = vb2_ioctl_dqbuf,

	.vidioc_streamon          = vb2_ioctl_streamon,
	.vidioc_streamoff         = vb2_ioctl_streamoff,

	.vidioc_g_tuner           = vidioc_g_tuner,
	.vidioc_s_tuner           = vidioc_s_tuner,
	.vidioc_s_frequency       = vidioc_s_frequency,

	.vidioc_subscribe_event   = v4l2_ctrl_subscribe_event,
	.vidioc_unsubscribe_event = v4l2_event_unsubscribe,
	.vidioc_log_status        = v4l2_ctrl_log_status,
};

static const struct v4l2_file_operations rtl2832_sdr_fops = {
	.owner                    = THIS_MODULE,
	.open                     = v4l2_fh_open,
	.release                  = vb2_fop_release,
	.read                     = vb2_fop_read,
	.poll                     = vb2_fop_poll,
	.mmap                     = vb2_fop_mmap,
	.unlocked_ioctl           = video_ioctl2,
};

static struct video_device rtl2832_sdr_template = {
	.name                     = "Realtek RTL2832U SDR",
	.release                  = video_device_release_empty,
	.fops                     = &rtl2832_sdr_fops,
	.ioctl_ops                = &rtl2832_sdr_ioctl_ops,
};

static int rtl2832_sdr_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct rtl2832_sdr_state *s =
			container_of(ctrl->handler, struct rtl2832_sdr_state,
					ctrl_handler);
	int ret;
	dev_dbg(&s->udev->dev,
			"%s: id=%d name=%s val=%d min=%d max=%d step=%d\n",
			__func__, ctrl->id, ctrl->name, ctrl->val,
			ctrl->minimum, ctrl->maximum, ctrl->step);

	switch (ctrl->id) {
	case RTL2832_SDR_CID_SAMPLING_MODE:
	case RTL2832_SDR_CID_SAMPLING_RATE:
	case RTL2832_SDR_CID_SAMPLING_RESOLUTION:
		ret = rtl2832_sdr_set_adc(s);
		break;
	case RTL2832_SDR_CID_TUNER_RF:
	case RTL2832_SDR_CID_TUNER_BW:
	case RTL2832_SDR_CID_TUNER_IF:
	case RTL2832_SDR_CID_TUNER_GAIN:
		ret = rtl2832_sdr_set_tuner(s);
		break;
	default:
		ret = -EINVAL;
	}

	return ret;
}

static const struct v4l2_ctrl_ops rtl2832_sdr_ctrl_ops = {
	.s_ctrl = rtl2832_sdr_s_ctrl,
};

static void rtl2832_sdr_video_release(struct v4l2_device *v)
{
	struct rtl2832_sdr_state *s =
			container_of(v, struct rtl2832_sdr_state, v4l2_dev);

	v4l2_ctrl_handler_free(&s->ctrl_handler);
	v4l2_device_unregister(&s->v4l2_dev);
	kfree(s);
}

struct dvb_frontend *rtl2832_sdr_attach(struct dvb_frontend *fe,
		struct i2c_adapter *i2c, const struct rtl2832_config *cfg)
{
	int ret;
	struct rtl2832_sdr_state *s;
	struct dvb_usb_device *d = i2c_get_adapdata(i2c);
	static const char * const ctrl_sampling_mode_qmenu_strings[] = {
		"Quadrature Sampling",
		NULL,
	};
	static const struct v4l2_ctrl_config ctrl_sampling_mode = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_SAMPLING_MODE,
		.type   = V4L2_CTRL_TYPE_MENU,
		.flags  = V4L2_CTRL_FLAG_INACTIVE,
		.name	= "Sampling Mode",
		.qmenu  = ctrl_sampling_mode_qmenu_strings,
	};
	static const struct v4l2_ctrl_config ctrl_sampling_rate = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_SAMPLING_RATE,
		.type	= V4L2_CTRL_TYPE_INTEGER64,
		.name	= "Sampling Rate",
		.min	=  500000,
		.max	= 4000000,
		.def    = 2048000,
		.step	= 1,
	};
	static const struct v4l2_ctrl_config ctrl_sampling_resolution = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_SAMPLING_RESOLUTION,
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.flags  = V4L2_CTRL_FLAG_INACTIVE,
		.name	= "Sampling Resolution",
		.min	= 8,
		.max	= 8,
		.def    = 8,
		.step	= 1,
	};
	static const struct v4l2_ctrl_config ctrl_tuner_rf = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_TUNER_RF,
		.type   = V4L2_CTRL_TYPE_INTEGER64,
		.name	= "Tuner RF",
		.min	=   40000000,
		.max	= 2000000000,
		.def    =  100000000,
		.step	= 1,
	};
	static const struct v4l2_ctrl_config ctrl_tuner_bw = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_TUNER_BW,
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.name	= "Tuner BW",
		.min	=  200000,
		.max	= 8000000,
		.def    =  600000,
		.step	= 1,
	};
	static const struct v4l2_ctrl_config ctrl_tuner_if = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_TUNER_IF,
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.flags  = V4L2_CTRL_FLAG_INACTIVE,
		.name	= "Tuner IF",
		.min	= 0,
		.max	= 10,
		.def    = 0,
		.step	= 1,
	};
	static const struct v4l2_ctrl_config ctrl_tuner_gain = {
		.ops	= &rtl2832_sdr_ctrl_ops,
		.id	= RTL2832_SDR_CID_TUNER_GAIN,
		.type	= V4L2_CTRL_TYPE_INTEGER,
		.name	= "Tuner Gain",
		.min	= 0,
		.max	= 102,
		.def    = 0,
		.step	= 1,
	};

	s = kzalloc(sizeof(struct rtl2832_sdr_state), GFP_KERNEL);
	if (s == NULL) {
		dev_err(&d->udev->dev,
				"Could not allocate memory for rtl2832_sdr_state\n");
		return ERR_PTR(-ENOMEM);
	}

	/* setup the state */
	s->fe = fe;
	s->d = d;
	s->udev = d->udev;
	s->i2c = i2c;
	s->cfg = cfg;

	mutex_init(&s->v4l2_lock);
	mutex_init(&s->vb_queue_lock);
	spin_lock_init(&s->queued_bufs_lock);
	INIT_LIST_HEAD(&s->queued_bufs);

	/* Init videobuf2 queue structure */
	s->vb_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	s->vb_queue.io_modes = VB2_MMAP | VB2_USERPTR | VB2_READ;
	s->vb_queue.drv_priv = s;
	s->vb_queue.buf_struct_size = sizeof(struct rtl2832_sdr_frame_buf);
	s->vb_queue.ops = &rtl2832_sdr_vb2_ops;
	s->vb_queue.mem_ops = &vb2_vmalloc_memops;
	s->vb_queue.timestamp_type = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC;
	ret = vb2_queue_init(&s->vb_queue);
	if (ret < 0) {
		dev_err(&s->udev->dev, "Could not initialize vb2 queue\n");
		goto err_free_mem;
	}

	/* Init video_device structure */
	s->vdev = rtl2832_sdr_template;
	s->vdev.queue = &s->vb_queue;
	s->vdev.queue->lock = &s->vb_queue_lock;
	set_bit(V4L2_FL_USE_FH_PRIO, &s->vdev.flags);
	video_set_drvdata(&s->vdev, s);

	/* Register controls */
	v4l2_ctrl_handler_init(&s->ctrl_handler, 7);
	v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_sampling_mode, NULL);
	s->ctrl_sampling_rate = v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_sampling_rate, NULL);
	v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_sampling_resolution, NULL);
	s->ctrl_tuner_rf = v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_tuner_rf, NULL);
	s->ctrl_tuner_bw = v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_tuner_bw, NULL);
	s->ctrl_tuner_if = v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_tuner_if, NULL);
	s->ctrl_tuner_gain = v4l2_ctrl_new_custom(&s->ctrl_handler, &ctrl_tuner_gain, NULL);
	if (s->ctrl_handler.error) {
		ret = s->ctrl_handler.error;
		dev_err(&s->udev->dev, "Could not initialize controls\n");
		goto err_free_controls;
	}

	/* Register the v4l2_device structure */
	s->v4l2_dev.release = rtl2832_sdr_video_release;
	ret = v4l2_device_register(&s->udev->dev, &s->v4l2_dev);
	if (ret) {
		dev_err(&s->udev->dev,
				"Failed to register v4l2-device (%d)\n", ret);
		goto err_free_controls;
	}

	s->v4l2_dev.ctrl_handler = &s->ctrl_handler;
	s->vdev.v4l2_dev = &s->v4l2_dev;
	s->vdev.lock = &s->v4l2_lock;

	ret = video_register_device(&s->vdev, VFL_TYPE_GRABBER, -1);
	if (ret < 0) {
		dev_err(&s->udev->dev,
				"Failed to register as video device (%d)\n",
				ret);
		goto err_unregister_v4l2_dev;
	}
	dev_info(&s->udev->dev, "Registered as %s\n",
			video_device_node_name(&s->vdev));

	fe->sec_priv = s;
	fe->ops.release_sec = rtl2832_sdr_release_sec;

	dev_info(&s->i2c->dev, "%s: Realtek RTL2832 SDR attached\n",
			KBUILD_MODNAME);
	return fe;

err_unregister_v4l2_dev:
	v4l2_device_unregister(&s->v4l2_dev);
err_free_controls:
	v4l2_ctrl_handler_free(&s->ctrl_handler);
err_free_mem:
	kfree(s);
	return ERR_PTR(ret);
}
EXPORT_SYMBOL(rtl2832_sdr_attach);

MODULE_AUTHOR("Antti Palosaari <crope@iki.fi>");
MODULE_DESCRIPTION("Realtek RTL2832 SDR driver");
MODULE_LICENSE("GPL");
