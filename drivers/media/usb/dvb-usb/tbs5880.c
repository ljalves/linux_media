/*
 * TurboSight TBS 5880 CI driver
 *
 * Copyright (c) 2011 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, version 2.
 *
 */

#include <linux/version.h>
#include "tbs5880.h"
#include "tda18212.h"
#include "cxd2820r.h"

#include "dvb_ca_en50221.h"

#define TBS5880_READ_MSG 0
#define TBS5880_WRITE_MSG 1

#define TBS5880_RC_QUERY (0x1a00)
#define TBS5880_LED_CTRL (0x1b00)

struct tbs5880_state {
	struct dvb_ca_en50221 ca;
	struct mutex ca_mutex;

	u32 last_key_pressed;
};

/*struct tbs5880_rc_keys {
	u32 keycode;
	u32 event;
};*/

/* debug */
static int dvb_usb_tbs5880_debug;
module_param_named(debug, dvb_usb_tbs5880_debug, int, 0644);
MODULE_PARM_DESC(debug, "set debugging level (1=info 2=xfer (or-able))." 
							DVB_USB_DEBUG_STATUS);

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static int tbs5880_op_rw(struct usb_device *dev, u8 request, u16 value,
				u16 index, u8 * data, u16 len, int flags)
{
	int ret;
	u8 u8buf[len];

	unsigned int pipe = (flags == TBS5880_READ_MSG) ?
			usb_rcvctrlpipe(dev, 0) : usb_sndctrlpipe(dev, 0);
	u8 request_type = (flags == TBS5880_READ_MSG) ? USB_DIR_IN : 
								USB_DIR_OUT;

	if (flags == TBS5880_WRITE_MSG)
		memcpy(u8buf, data, len);
	ret = usb_control_msg(dev, pipe, request, request_type | 
			USB_TYPE_VENDOR, value, index , u8buf, len, 2000);

	if (flags == TBS5880_READ_MSG)
		memcpy(data, u8buf, len);
	return ret;
}

static int tbs5880_read_attribute_mem(struct dvb_ca_en50221 *ca,
                                                	int slot, int address)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[4], rbuf[3];
	int ret;

	if (0 != slot)
		return -EINVAL;

	buf[0] = 1;
	buf[1] = 0;
	buf[2] = (address >> 8) & 0x0f;
	buf[3] = address;

	//msleep(10);

	mutex_lock(&state->ca_mutex);

	ret = tbs5880_op_rw(d->udev, 0xa4, 0, 0,
						buf, 4, TBS5880_WRITE_MSG);

	//msleep(1);

	ret = tbs5880_op_rw(d->udev, 0xa5, 0, 0,
						rbuf, 1, TBS5880_READ_MSG);

	mutex_unlock(&state->ca_mutex);

	if (ret < 0)
		return ret;

	return rbuf[0];
}

static int tbs5880_write_attribute_mem(struct dvb_ca_en50221 *ca,
						int slot, int address, u8 value)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[5];//, rbuf[1];
	int ret;

	if (0 != slot)
		return -EINVAL;

	buf[0] = 1;
	buf[1] = 0;
	buf[2] = (address >> 8) & 0x0f;
	buf[3] = address;
	buf[4] = value;

	mutex_lock(&state->ca_mutex);

	ret = tbs5880_op_rw(d->udev, 0xa2, 0, 0,
						buf, 5, TBS5880_WRITE_MSG);

	//msleep(1);

	//ret = tbs5880_op_rw(d->udev, 0xa5, 0, 0,
	//					rbuf, 1, TBS5880_READ_MSG);

	mutex_unlock(&state->ca_mutex);

	if (ret < 0)
		return ret;

	return 0;
}

static int tbs5880_read_cam_control(struct dvb_ca_en50221 *ca, int slot, 
								u8 address)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[4], rbuf[1];
	int ret;

	if (0 != slot)
		return -EINVAL;

	buf[0] = 1;
	buf[1] = 1;
	buf[2] = (address >> 8) & 0x0f;
	buf[3] = address;

	mutex_lock(&state->ca_mutex);

	ret = tbs5880_op_rw(d->udev, 0xa4, 0, 0,
						buf, 4, TBS5880_WRITE_MSG);

	//msleep(10);

	ret = tbs5880_op_rw(d->udev, 0xa5, 0, 0,
						rbuf, 1, TBS5880_READ_MSG);

	mutex_unlock(&state->ca_mutex);

	if (ret < 0)
		return ret;

	return rbuf[0];
}

static int tbs5880_write_cam_control(struct dvb_ca_en50221 *ca, int slot, 
							u8 address, u8 value)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[5];//, rbuf[1];
	int ret;

	if (0 != slot)
		return -EINVAL;

	buf[0] = 1;
	buf[1] = 1;
	buf[2] = (address >> 8) & 0x0f;
	buf[3] = address;
	buf[4] = value;

	mutex_lock(&state->ca_mutex);

	ret = tbs5880_op_rw(d->udev, 0xa2, 0, 0,
						buf, 5, TBS5880_WRITE_MSG);

	//msleep(1);

	//ret = tbs5880_op_rw(d->udev, 0xa5, 0, 0,
	//					rbuf, 1, TBS5880_READ_MSG);

	mutex_unlock(&state->ca_mutex);

	if (ret < 0)
		return ret;

	return 0;
}

static int tbs5880_set_video_port(struct dvb_ca_en50221 *ca, 
							int slot, int enable)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[2];
	int ret;

	if (0 != slot)
		return -EINVAL;

	buf[0] = 2;
	buf[1] = enable;

	mutex_lock(&state->ca_mutex);

	ret = tbs5880_op_rw(d->udev, 0xa6, 0, 0,
						buf, 2, TBS5880_WRITE_MSG);

	mutex_unlock(&state->ca_mutex);

	if (ret < 0)
		return ret;

	if (enable != buf[1]) {
		err("CI not %sabled.", enable ? "en" : "dis");
		return -EIO;
	}

	info("CI %sabled.", enable ? "en" : "dis");
	return 0;
}

static int tbs5880_slot_shutdown(struct dvb_ca_en50221 *ca, int slot)
{
	return tbs5880_set_video_port(ca, slot, /* enable */ 0);
}

static int tbs5880_slot_ts_enable(struct dvb_ca_en50221 *ca, int slot)
{
	return tbs5880_set_video_port(ca, slot, /* enable */ 1);
}

static int tbs5880_slot_reset(struct dvb_ca_en50221 *ca, int slot)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[2];
	int ret;

	if (0 != slot) {
		return -EINVAL;
	}

	buf[0] = 1;
	buf[1] = 0;

	mutex_lock (&state->ca_mutex);

	ret = tbs5880_op_rw(d->udev, 0xa6, 0, 0,
						buf, 2, TBS5880_WRITE_MSG);

	msleep (5);

	buf[1] = 1;

	ret = tbs5880_op_rw(d->udev, 0xa6, 0, 0,
						buf, 2, TBS5880_WRITE_MSG);

	msleep (1400);

	mutex_unlock (&state->ca_mutex);

	if (ret < 0)
		return ret;

	return 0;
}

static int tbs5880_poll_slot_status(struct dvb_ca_en50221 *ca,
							int slot, int open)
{
	struct dvb_usb_device *d = (struct dvb_usb_device *)ca->data;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	u8 buf[3];

	if (0 != slot)
		return -EINVAL;

	mutex_lock(&state->ca_mutex);

	tbs5880_op_rw(d->udev, 0xa8, 0, 0,
					buf, 3, TBS5880_READ_MSG);

	mutex_unlock(&state->ca_mutex);

	if ((1 == buf[2]) && (1 == buf[1]) && (0xa9 == buf[0])) {
				return (DVB_CA_EN50221_POLL_CAM_PRESENT |
						DVB_CA_EN50221_POLL_CAM_READY);
	} else {
		return 0;
	}
}

static void tbs5880_uninit(struct dvb_usb_device *d)
{
	struct tbs5880_state *state;

	if (NULL == d)
		return;

	state = (struct tbs5880_state *)d->priv;
	if (NULL == state)
		return;

	if (NULL == state->ca.data)
		return;

	/* Error ignored. */
	tbs5880_set_video_port(&state->ca, /* slot */ 0, /* enable */ 0);

	dvb_ca_en50221_release(&state->ca);

	memset(&state->ca, 0, sizeof(state->ca));
}

static int tbs5880_init(struct dvb_usb_adapter *a)
{

	struct dvb_usb_device *d = a->dev;
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	int ret;

	state->ca.owner = THIS_MODULE;
	state->ca.read_attribute_mem = tbs5880_read_attribute_mem;
	state->ca.write_attribute_mem = tbs5880_write_attribute_mem;
	state->ca.read_cam_control = tbs5880_read_cam_control;
	state->ca.write_cam_control = tbs5880_write_cam_control;
	state->ca.slot_reset = tbs5880_slot_reset;
	state->ca.slot_shutdown = tbs5880_slot_shutdown;
	state->ca.slot_ts_enable = tbs5880_slot_ts_enable;
	state->ca.poll_slot_status = tbs5880_poll_slot_status;
	state->ca.data = d;

	ret = dvb_ca_en50221_init (&a->dvb_adap, &state->ca,
						/* flags */ 0, /* n_slots */ 1);

	if (0 != ret) {
		err ("Cannot initialize CI: Error %d.", ret);
		memset (&state->ca, 0, sizeof (state->ca));
		return ret;
	}

	info ("CI initialized.");

	ret = tbs5880_poll_slot_status(&state->ca, 0, 0);
	if (0 == ret)
		tbs5880_set_video_port(&state->ca, /* slot */ 0, /* enable */ 0);

	return 0;
}

/* I2C */
static int tbs5880_i2c_transfer(struct i2c_adapter *adap, 
					struct i2c_msg msg[], int num)
{
	struct dvb_usb_device *d = i2c_get_adapdata(adap);
	struct tbs5880_state *state = (struct tbs5880_state *)d->priv;
	int i = 0;
	u8 buf6[20];
	u8 inbuf[20];

	if (!d)
		return -ENODEV;

	mutex_lock(&state->ca_mutex);

	if (mutex_lock_interruptible(&d->i2c_mutex) < 0)
		return -EAGAIN;

	switch (num) {
	case 2:
		buf6[0]=msg[1].len;//lenth
		buf6[1]=msg[0].addr<<1;//demod addr
		//register
		buf6[2] = msg[0].buf[0];

		tbs5880_op_rw(d->udev, 0x90, 0, 0,
					buf6, 3, TBS5880_WRITE_MSG);
		//msleep(5);
		tbs5880_op_rw(d->udev, 0x91, 0, 0,
					inbuf, msg[1].len, TBS5880_READ_MSG);
		memcpy(msg[1].buf, inbuf, msg[1].len);
		break;
	case 1:
		switch (msg[0].addr) {
		case 0x6c:
		case 0x6e:
		case 0x61:
		case 0x60:
			if (msg[0].flags == 0) {
				buf6[0] = msg[0].len+1;//lenth
				buf6[1] = msg[0].addr<<1;//addr
				for(i=0;i<msg[0].len;i++) {
					buf6[2+i] = msg[0].buf[i];//register
				}
				tbs5880_op_rw(d->udev, 0x80, 0, 0,
					buf6, msg[0].len+2, TBS5880_WRITE_MSG);
			} else {
				buf6[0] = msg[0].len;//length
				buf6[1] = msg[0].addr<<1;//addr
				buf6[2] = 0x00;
				tbs5880_op_rw(d->udev, 0x90, 0, 0,
						buf6, 3, TBS5880_WRITE_MSG);
				//msleep(5);
				tbs5880_op_rw(d->udev, 0x91, 0, 0,
					inbuf, buf6[0], TBS5880_READ_MSG);
				memcpy(msg[0].buf, inbuf, msg[0].len);
			}
			//msleep(3);
			break;
		case (TBS5880_RC_QUERY):
			tbs5880_op_rw(d->udev, 0xb8, 0, 0,
					buf6, 4, TBS5880_READ_MSG);
			msg[0].buf[0] = buf6[2];
			msg[0].buf[1] = buf6[3];
			//msleep(3);
			//info("TBS5880_RC_QUERY %x %x %x %x\n",
			//		buf6[0],buf6[1],buf6[2],buf6[3]);
			break;
		case (TBS5880_LED_CTRL):
			buf6[0] = 5;
			buf6[1] = msg[0].buf[0];
			tbs5880_op_rw(d->udev, 0x8a, 0, 0,
					buf6, 2, TBS5880_WRITE_MSG);
			break;
		}

		break;
	}

	mutex_unlock(&d->i2c_mutex);
	mutex_unlock(&state->ca_mutex);
	return num;
}

static u32 tbs5880_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}

static struct i2c_algorithm tbs5880_i2c_algo = {
	.master_xfer = tbs5880_i2c_transfer,
	.functionality = tbs5880_i2c_func,
};

static int tbs5880_read_mac_address(struct dvb_usb_device *d, u8 mac[6])
{
	int i,ret;
	u8 ibuf[3] = {0, 0,0};
	u8 eeprom[256], eepromline[16];

	for (i = 0; i < 256; i++) {
		ibuf[0]=1;//lenth
		ibuf[1]=0xa0;//eeprom addr
		ibuf[2]=i;//register
		ret = tbs5880_op_rw(d->udev, 0x90, 0, 0,
					ibuf, 3, TBS5880_WRITE_MSG);
		ret = tbs5880_op_rw(d->udev, 0x91, 0, 0,
					ibuf, 1, TBS5880_READ_MSG);
			if (ret < 0) {
				err("read eeprom failed.");
				return -1;
			} else {
				eepromline[i%16] = ibuf[0];
				eeprom[i] = ibuf[0];
			}
			
			if ((i % 16) == 15) {
				deb_xfer("%02x: ", i - 15);
				debug_dump(eepromline, 16, deb_xfer);
			}
	}
	memcpy(mac, eeprom + 16, 6);
	return 0;
};

static void tbs5880_led_ctrl(struct dvb_frontend *fe, int offon)
{
	static u8 led_off[] = { 0 };
	static u8 led_on[] = { 1 };
	struct i2c_msg msg = {
		.addr = TBS5880_LED_CTRL,
		.flags = 0,
		.buf = led_off,
		.len = 1
	};
	struct dvb_usb_adapter *udev_adap =
		(struct dvb_usb_adapter *)(fe->dvb->priv);

	if (offon)
		msg.buf = led_on;
	i2c_transfer(&udev_adap->dev->i2c_adap, &msg, 1);
	info("tbs5880_led_ctrl %d",offon);
}

static struct dvb_usb_device_properties tbs5880_properties;

static struct cxd2820r_config cxd2820r_config = {
	.i2c_address = 0x6c, /* (0xd8 >> 1) */
	.ts_mode = 0x08,
	.set_lock_led = tbs5880_led_ctrl,
};

static struct tda18212_config tda18212_config = {
	.i2c_address = 0x60 /* (0xc0 >> 1) */,
	.if_dvbt_6 = 3550,
	.if_dvbt_7 = 3700,
	.if_dvbt_8 = 4150,
	.if_dvbt2_6 = 3250,
	.if_dvbt2_7 = 4000,
	.if_dvbt2_8 = 4000,
	.if_dvbc = 5000,
};

static int tbs5880_tuner_attach(struct dvb_usb_adapter *adap)
{
	if (!dvb_attach(tda18212_attach, adap->fe_adap->fe, &adap->dev->i2c_adap,
		&tda18212_config))
		return -EIO;

	return 0;
}

static int tbs5880_frontend_attach(struct dvb_usb_adapter *d)
{
	struct dvb_usb_device *u = d->dev;
	struct tbs5880_state *state = (struct tbs5880_state *)u->priv;
	u8 buf[20];

	mutex_init(&state->ca_mutex);

	if (tbs5880_properties.adapter->fe->tuner_attach == &tbs5880_tuner_attach) {
		d->fe_adap->fe = dvb_attach(cxd2820r_attach, &cxd2820r_config,
			&d->dev->i2c_adap,NULL);

		if (d->fe_adap->fe != NULL) {
			info("Attached TBS5880FE!\n");

			buf[0] = 7;
			buf[1] = 1;
			tbs5880_op_rw(d->dev->udev, 0x8a, 0, 0,
					buf, 2, TBS5880_WRITE_MSG);

			tbs5880_init(d);
			return 0;
		}
	}

	return -EIO;
}

static void tbs58802_usb_disconnect (struct usb_interface * intf)
{
	struct dvb_usb_device *d = usb_get_intfdata (intf);
	
	tbs5880_uninit (d);
	dvb_usb_device_exit (intf);
}

static struct rc_map_table tbs5880_rc_keys[] = {
	{ 0xff84, KEY_POWER2},		/* power */
	{ 0xff94, KEY_MUTE},		/* mute */
	{ 0xff87, KEY_1},
	{ 0xff86, KEY_2},
	{ 0xff85, KEY_3},
	{ 0xff8b, KEY_4},
	{ 0xff8a, KEY_5},
	{ 0xff89, KEY_6},
	{ 0xff8f, KEY_7},
	{ 0xff8e, KEY_8},
	{ 0xff8d, KEY_9},
	{ 0xff92, KEY_0},
	{ 0xff96, KEY_CHANNELUP},	/* ch+ */
	{ 0xff91, KEY_CHANNELDOWN},	/* ch- */
	{ 0xff93, KEY_VOLUMEUP},	/* vol+ */
	{ 0xff8c, KEY_VOLUMEDOWN},	/* vol- */
	{ 0xff83, KEY_RECORD},		/* rec */
	{ 0xff98, KEY_PAUSE},		/* pause, yellow */
	{ 0xff99, KEY_OK},		/* ok */
	{ 0xff9a, KEY_CAMERA},		/* snapshot */
	{ 0xff81, KEY_UP},
	{ 0xff90, KEY_LEFT},
	{ 0xff82, KEY_RIGHT},
	{ 0xff88, KEY_DOWN},
	{ 0xff95, KEY_FAVORITES},	/* blue */
	{ 0xff97, KEY_SUBTITLE},	/* green */
	{ 0xff9d, KEY_ZOOM},
	{ 0xff9f, KEY_EXIT},
	{ 0xff9e, KEY_MENU},
	{ 0xff9c, KEY_EPG},
	{ 0xff80, KEY_PREVIOUS},	/* red */
	{ 0xff9b, KEY_MODE},
	{ 0xffdd, KEY_TV },
	{ 0xffde, KEY_PLAY },
	{ 0xffdc, KEY_STOP },
	{ 0xffdb, KEY_REWIND },
	{ 0xffda, KEY_FASTFORWARD },
	{ 0xffd9, KEY_PREVIOUS },	/* replay */
	{ 0xffd8, KEY_NEXT },		/* skip */
	{ 0xffd1, KEY_NUMERIC_STAR },
	{ 0xffd2, KEY_NUMERIC_POUND },
	{ 0xffd4, KEY_DELETE },		/* clear */
};

static int tbs5880_rc_query(struct dvb_usb_device *d, u32 *event, int *state)
{
	struct rc_map_table *keymap = d->props.rc.legacy.rc_map_table;
	int keymap_size = d->props.rc.legacy.rc_map_size;

	struct tbs5880_state *st = d->priv;
	u8 key[2];
	struct i2c_msg msg[] = {
		{.addr = TBS5880_RC_QUERY, .flags = I2C_M_RD, .buf = key,
		.len = 2},
	};
	int i;

	*state = REMOTE_NO_KEY_PRESSED;
	if (tbs5880_i2c_transfer(&d->i2c_adap, msg, 1) == 1) {
		//info("key: %x %x\n",msg[0].buf[0],msg[0].buf[1]); 
		for (i = 0; i < keymap_size; i++) {
			if (rc5_data(&keymap[i]) == msg[0].buf[1]) {
				*state = REMOTE_KEY_PRESSED;
				*event = keymap[i].keycode;
				st->last_key_pressed =
					keymap[i].keycode;
				break;
			}
		st->last_key_pressed = 0;
		}
	}
	 
	return 0;
}

static struct usb_device_id tbs5880_table[] = {
	{USB_DEVICE(0x734c, 0x5880)},
	{ }
};

MODULE_DEVICE_TABLE(usb, tbs5880_table);

static int tbs5880_load_firmware(struct usb_device *dev,
			const struct firmware *frmwr)
{
	u8 *b, *p;
	int ret = 0, i;
	u8 reset;
	const struct firmware *fw;
	switch (dev->descriptor.idProduct) {
	case 0x5880:
		ret = request_firmware(&fw, tbs5880_properties.firmware, &dev->dev);
		if (ret != 0) {
			err("did not find the firmware file. (%s) "
			"Please see linux/Documentation/dvb/ for more details "
			"on firmware-problems.", tbs5880_properties.firmware);
			return ret;
		}
		break;
	default:
		fw = frmwr;
		break;
	}
	info("start downloading TBS5880 CI firmware");
	p = kmalloc(fw->size, GFP_KERNEL);
	reset = 1;
	/*stop the CPU*/
	tbs5880_op_rw(dev, 0xa0, 0x7f92, 0, &reset, 1, TBS5880_WRITE_MSG);
	tbs5880_op_rw(dev, 0xa0, 0xe600, 0, &reset, 1, TBS5880_WRITE_MSG);

	if (p != NULL) {
		memcpy(p, fw->data, fw->size);
		for (i = 0; i < fw->size; i += 0x40) {
			b = (u8 *) p + i;
			if (tbs5880_op_rw(dev, 0xa0, i, 0, b , 0x40,
					TBS5880_WRITE_MSG) != 0x40) {
				err("error while transferring firmware");
				ret = -EINVAL;
				break;
			}
		}
		/* restart the CPU */
		reset = 0;
		if (ret || tbs5880_op_rw(dev, 0xa0, 0x7f92, 0, &reset, 1,
					TBS5880_WRITE_MSG) != 1) {
			err("could not restart the USB controller CPU.");
			ret = -EINVAL;
		}
		if (ret || tbs5880_op_rw(dev, 0xa0, 0xe600, 0, &reset, 1,
					TBS5880_WRITE_MSG) != 1) {
			err("could not restart the USB controller CPU.");
			ret = -EINVAL;
		}

		msleep(100);
		kfree(p);
	}
	return ret;
}

static struct dvb_usb_device_properties tbs5880_properties = {
	.caps = DVB_USB_IS_AN_I2C_ADAPTER,
	.usb_ctrl = DEVICE_SPECIFIC,
	.firmware = "dvb-usb-tbsqbox-id5880.fw",
	.size_of_priv = sizeof(struct tbs5880_state),
	.no_reconnect = 1,

	.i2c_algo = &tbs5880_i2c_algo,
	.rc.legacy = {
		.rc_map_table = tbs5880_rc_keys,
		.rc_map_size = ARRAY_SIZE(tbs5880_rc_keys),
		.rc_interval = 150,
		.rc_query = tbs5880_rc_query,
	},

	.generic_bulk_ctrl_endpoint = 0x81,
	/* parameter for the MPEG2-data transfer */
	.num_adapters = 1,
	.download_firmware = tbs5880_load_firmware,
	.read_mac_address = tbs5880_read_mac_address,
	.adapter = {{
		.num_frontends = 1,
		.fe = {{
			.frontend_attach = tbs5880_frontend_attach,
			.streaming_ctrl = NULL,
			.tuner_attach = tbs5880_tuner_attach,
			.stream = {
				.type = USB_BULK,
				.count = 8,
				.endpoint = 0x82,
				.u = {
					.bulk = {
						.buffersize = 4096,
					}
				}
			},
		} },
	} },

	.num_device_descs = 1,
	.devices = {
		{"TBS 5880 CI USB2.0",
			{&tbs5880_table[0], NULL},
			{NULL},
		}
	}
};

static int tbs5880_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	if (0 == dvb_usb_device_init(intf, &tbs5880_properties,
			THIS_MODULE, NULL, adapter_nr)) {
		return 0;
	}
	return -ENODEV;
}

static struct usb_driver tbs5880_driver = {
	.name = "tbs5880",
	.probe = tbs5880_probe,
	.disconnect = tbs58802_usb_disconnect,
	.id_table = tbs5880_table,
};

static int __init tbs5880_module_init(void)
{
	int ret =  usb_register(&tbs5880_driver);
	if (ret)
		err("usb_register failed. Error number %d", ret);

	return ret;
}

static void __exit tbs5880_module_exit(void)
{
	usb_deregister(&tbs5880_driver);
}

module_init(tbs5880_module_init);
module_exit(tbs5880_module_exit);

MODULE_AUTHOR("Konstantin Dimitrov <kosio.dimitrov@gmail.com>");
MODULE_DESCRIPTION("TurboSight TBS 5880 CI driver");
MODULE_VERSION("1.0");
MODULE_LICENSE("GPL");
