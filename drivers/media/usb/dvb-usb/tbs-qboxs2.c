/* DVB USB framework compliant Linux driver for the
*	TBS QBOX
*
* Copyright (C) 2008 Bob Liu (Bob@Turbosight.com)
* Igor M. Liplianin (liplianin@me.by)
*
*	This program is free software; you can redistribute it and/or modify it
*	under the terms of the GNU General Public License as published by the
*	Free Software Foundation, version 2.
*
* see Documentation/dvb/README.dvb-usb for more information
*/

/* 
* History:
*
* July 2009 Konstantin Dimitrov <kosio.dimitrov@gmail.com>
* remove QBOX2-DS3000 support
* add QBOXS2-CX24116 support
*/

#include <linux/version.h>
#include "tbs-qboxs2.h"
#include "cx24116.h"

#define TBSQBOX_READ_MSG 0
#define TBSQBOX_WRITE_MSG 1

/* on my own*/
#define TBSQBOX_VOLTAGE_CTRL (0x1800)
#define TBSQBOX_RC_QUERY (0x1a00)
#define TBSQBOX_LED_CTRL (0x1b00)

struct tbsqboxs2_state {
	u32 last_key_pressed;
};
struct tbsqboxs2_rc_keys {
	u32 keycode;
	u32 event;
};

/* debug */
static int dvb_usb_tbsqboxs2_debug;
module_param_named(debug, dvb_usb_tbsqboxs2_debug, int, 0644);
MODULE_PARM_DESC(debug, "set debugging level (1=info 2=xfer (or-able))." DVB_USB_DEBUG_STATUS);

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static int tbsqboxs2_op_rw(struct usb_device *dev, u8 request, u16 value,
			u16 index, u8 * data, u16 len, int flags)
{
	int ret;
	u8 u8buf[len];

	unsigned int pipe = (flags == TBSQBOX_READ_MSG) ?
				usb_rcvctrlpipe(dev, 0) : usb_sndctrlpipe(dev, 0);
	u8 request_type = (flags == TBSQBOX_READ_MSG) ? USB_DIR_IN : USB_DIR_OUT;

	if (flags == TBSQBOX_WRITE_MSG)
		memcpy(u8buf, data, len);
	ret = usb_control_msg(dev, pipe, request, request_type | USB_TYPE_VENDOR,
				value, index, u8buf, len, 2000);

	if (flags == TBSQBOX_READ_MSG)
		memcpy(data, u8buf, len);
	return ret;
}

/* I2C */
static int tbsqboxs2_i2c_transfer(struct i2c_adapter *adap, struct i2c_msg msg[],
		int num)
{
struct dvb_usb_device *d = i2c_get_adapdata(adap);
	int i = 0, len;
	u8 ibuf[1], obuf[3];
	u8 buf6[20];

	if (!d)
		return -ENODEV;
	if (mutex_lock_interruptible(&d->i2c_mutex) < 0)
		return -EAGAIN;

	switch (num) {
	case 2: {
		/* read */
		obuf[0] = msg[0].len;
		obuf[1] = msg[0].addr<<1;
		obuf[2] = msg[0].buf[0];

		tbsqboxs2_op_rw(d->udev, 0x90, 0, 0,
					obuf, 3, TBSQBOX_WRITE_MSG);
		msleep(5);
		tbsqboxs2_op_rw(d->udev, 0x91, 0, 0,
					ibuf, msg[1].len, TBSQBOX_READ_MSG);
		memcpy(msg[1].buf, ibuf, msg[1].len);
		break;
	}
	case 1:
		switch (msg[0].addr) {
		case 0x55: {
			if (msg[0].buf[0] == 0xf7) {
				/* firmware */
				/* Write in small blocks */
				u8 iobuf[19];
				iobuf[0] = 0x12;
				iobuf[1] = 0xaa;
				iobuf[2] = 0xf7;
				len = msg[0].len - 1;
				i = 1;
				do {
					memcpy(iobuf + 3, msg[0].buf + i, (len > 16 ? 16 : len));
					tbsqboxs2_op_rw(d->udev, 0x80, 0, 0,
						iobuf, (len > 16 ? 16 : len) + 3, TBSQBOX_WRITE_MSG);
					i += 16;
					len -= 16;
				} while (len > 0);
			} else {
				/* write to register */
				buf6[0] = msg[0].len+1;//lenth
				buf6[1] = msg[0].addr<<1;//demod addr
				for(i=0;i<msg[0].len;i++) {
				buf6[2+i] = msg[0].buf[i];//register
				}
				tbsqboxs2_op_rw(d->udev, 0x80, 0, 0,
							buf6, msg[0].len+2, TBSQBOX_WRITE_MSG);
				//msleep(3);
			}
			break;
		}
		case 0x60: {
			/* write to register */
			buf6[0] = msg[0].len+1;//lenth
			buf6[1] = msg[0].addr<<1;//demod addr
			for(i=0;i<msg[0].len;i++) {
				buf6[2+i] = msg[0].buf[i];//register
			}
			tbsqboxs2_op_rw(d->udev, 0x80, 0, 0,
						buf6, msg[0].len+2, TBSQBOX_WRITE_MSG);
			msleep(3);

			break;
		}
		case (TBSQBOX_RC_QUERY): {
			tbsqboxs2_op_rw(d->udev, 0xb8, 0, 0,
					buf6, 4, TBSQBOX_READ_MSG);
			msg[0].buf[0] = buf6[2];
			msg[0].buf[1] = buf6[3];
			msleep(3);
			//info("TBSQBOX_RC_QUERY %x %x %x %x\n",buf6[0],buf6[1],buf6[2],buf6[3]);
			break;
		}
		case (TBSQBOX_VOLTAGE_CTRL): {
			buf6[0] = 3;
			buf6[1] = msg[0].buf[0];
			tbsqboxs2_op_rw(d->udev, 0x8a, 0, 0,
					buf6, 2, TBSQBOX_WRITE_MSG);
			break;
		}
		case (TBSQBOX_LED_CTRL): {
			buf6[0] = 5;
			buf6[1] = msg[0].buf[0];
			tbsqboxs2_op_rw(d->udev, 0x8a, 0, 0,
					buf6, 2, TBSQBOX_WRITE_MSG);
			break;
		}
		}

		break;
	}

	mutex_unlock(&d->i2c_mutex);
	return num;
}

static u32 tbsqboxs2_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C;
}

static struct i2c_algorithm tbsqboxs2_i2c_algo = {
	.master_xfer = tbsqboxs2_i2c_transfer,
	.functionality = tbsqboxs2_i2c_func,
};

static void tbsqboxs2_led_ctrl(struct dvb_frontend *fe, int offon)
{
	static u8 led_off[] = { 0 };
	static u8 led_on[] = { 1 };
	struct i2c_msg msg = {
		.addr = TBSQBOX_LED_CTRL,
		.flags = 0,
		.buf = led_off,
		.len = 1
	};
	struct dvb_usb_adapter *udev_adap =
		(struct dvb_usb_adapter *)(fe->dvb->priv);

	if (offon)
		msg.buf = led_on;
	i2c_transfer(&udev_adap->dev->i2c_adap, &msg, 1);
	info("tbsqboxs2_led_ctrl %d",offon);
}

static const struct cx24116_config qbox2_cx24116_config = {
	.demod_address = 0x55,
	.mpg_clk_pos_pol = 0x01,
	.set_lock_led = tbsqboxs2_led_ctrl,
};

static int tbsqboxs2_read_mac_address(struct dvb_usb_device *d, u8 mac[6])
{
	int i,ret;
	u8 ibuf[3] = {0, 0,0};
	u8 eeprom[256], eepromline[16];

	for (i = 0; i < 256; i++) {
		ibuf[0]=1;//lenth
		ibuf[1]=0xa0;//eeprom addr
		ibuf[2]=i;//register
		ret = tbsqboxs2_op_rw(d->udev, 0x90, 0, 0,
					ibuf, 3, TBSQBOX_WRITE_MSG);
		ret = tbsqboxs2_op_rw(d->udev, 0x91, 0, 0,
					ibuf, 1, TBSQBOX_READ_MSG);
			if (ret < 0) {
				err("read eeprom failed");
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

static int tbsqboxs2_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	static u8 command_13v[1] = {0x00};
	static u8 command_18v[1] = {0x01};
	struct i2c_msg msg[] = {
		{.addr = TBSQBOX_VOLTAGE_CTRL, .flags = 0,
			.buf = command_13v, .len = 1},
	};
	
	struct dvb_usb_adapter *udev_adap =
		(struct dvb_usb_adapter *)(fe->dvb->priv);
	if (voltage == SEC_VOLTAGE_18)
		msg[0].buf = command_18v;
	info("tbsqboxs2_set_voltage %d",voltage);
	i2c_transfer(&udev_adap->dev->i2c_adap, msg, 1);
	return 0;
}

static struct dvb_usb_device_properties tbsqboxs2_properties;

static int tbsqboxs2_frontend_attach(struct dvb_usb_adapter *d)
{
	u8 buf[20];
	
	if ((d->fe_adap->fe = dvb_attach(cx24116_attach, &qbox2_cx24116_config,
					&d->dev->i2c_adap)) != NULL) {
			d->fe_adap->fe->ops.set_voltage = tbsqboxs2_set_voltage;
			printk("QBOXS2: CX24116 attached.\n");

			buf[0] = 7;
			buf[1] = 1;
			tbsqboxs2_op_rw(d->dev->udev, 0x8a, 0, 0,
					buf, 2, TBSQBOX_WRITE_MSG);

			return 0;
	}

	return -EIO;
}



static struct rc_map_table tbsqboxs2_rc_keys[] = {
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



static int tbsqboxs2_rc_query(struct dvb_usb_device *d, u32 *event, int *state)
{
	struct rc_map_table *keymap = d->props.rc.legacy.rc_map_table;
	int keymap_size = d->props.rc.legacy.rc_map_size;
	
	struct tbsqboxs2_state *st = d->priv;
	u8 key[2];
	struct i2c_msg msg[] = {
		{.addr = TBSQBOX_RC_QUERY, .flags = I2C_M_RD, .buf = key,
		.len = 2},
	};
	int i;

	*state = REMOTE_NO_KEY_PRESSED;
	if (tbsqboxs2_i2c_transfer(&d->i2c_adap, msg, 1) == 1) {
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

static struct usb_device_id tbsqboxs2_table[] = {
	{USB_DEVICE(0x734c, 0x5928)},
	{ }
};

MODULE_DEVICE_TABLE(usb, tbsqboxs2_table);

static int tbsqboxs2_load_firmware(struct usb_device *dev,
			const struct firmware *frmwr)
{
	u8 *b, *p;
	int ret = 0, i;
	u8 reset;
	const struct firmware *fw;
	switch (dev->descriptor.idProduct) {
	case 0x5928:
		ret = request_firmware(&fw, tbsqboxs2_properties.firmware, &dev->dev);
		if (ret != 0) {
			err("did not find the firmware file. (%s) "
			"Please see linux/Documentation/dvb/ for more details "
			"on firmware-problems.", tbsqboxs2_properties.firmware);
			return ret;
		}
		break;
	default:
		fw = frmwr;
		break;
	}
	info("start downloading TBSQBOX firmware");
	p = kmalloc(fw->size, GFP_KERNEL);
	reset = 1;
	/*stop the CPU*/
	tbsqboxs2_op_rw(dev, 0xa0, 0x7f92, 0, &reset, 1, TBSQBOX_WRITE_MSG);
	tbsqboxs2_op_rw(dev, 0xa0, 0xe600, 0, &reset, 1, TBSQBOX_WRITE_MSG);

	if (p != NULL) {
		memcpy(p, fw->data, fw->size);
		for (i = 0; i < fw->size; i += 0x40) {
			b = (u8 *) p + i;
			if (tbsqboxs2_op_rw(dev, 0xa0, i, 0, b , 0x40,
					TBSQBOX_WRITE_MSG) != 0x40) {
				err("error while transferring firmware");
				ret = -EINVAL;
				break;
			}
		}
		/* restart the CPU */
		reset = 0;
		if (ret || tbsqboxs2_op_rw(dev, 0xa0, 0x7f92, 0, &reset, 1,
					TBSQBOX_WRITE_MSG) != 1) {
			err("could not restart the USB controller CPU.");
			ret = -EINVAL;
		}
		if (ret || tbsqboxs2_op_rw(dev, 0xa0, 0xe600, 0, &reset, 1,
					TBSQBOX_WRITE_MSG) != 1) {
			err("could not restart the USB controller CPU.");
			ret = -EINVAL;
		}

		msleep(100);
		kfree(p);
	}
	return ret;
}

static struct dvb_usb_device_properties tbsqboxs2_properties = {
	.caps = DVB_USB_IS_AN_I2C_ADAPTER,
	.usb_ctrl = DEVICE_SPECIFIC,
	.firmware = "dvb-usb-tbsqbox-id5928.fw",
	.size_of_priv = sizeof(struct tbsqboxs2_state),
	.no_reconnect = 1,

	.i2c_algo = &tbsqboxs2_i2c_algo,
	.rc.legacy = {
		.rc_map_table = tbsqboxs2_rc_keys,
		.rc_map_size = ARRAY_SIZE(tbsqboxs2_rc_keys),
		.rc_interval = 150,
		.rc_query = tbsqboxs2_rc_query,
	},

	.generic_bulk_ctrl_endpoint = 0x81,
	/* parameter for the MPEG2-data transfer */
	.num_adapters = 1,
	.download_firmware = tbsqboxs2_load_firmware,
	.read_mac_address = tbsqboxs2_read_mac_address,
	.adapter = {{
		.num_frontends = 1,
		.fe = {{
			.frontend_attach = tbsqboxs2_frontend_attach,
			.streaming_ctrl = NULL,
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
		{"TBS QBOXS2 DVBS2 USB2.0",
			{&tbsqboxs2_table[0], NULL},
			{NULL},
		}
	}
};

static int tbsqboxs2_probe(struct usb_interface *intf,
		const struct usb_device_id *id)
{
	if (0 == dvb_usb_device_init(intf, &tbsqboxs2_properties,
			THIS_MODULE, NULL, adapter_nr)) {
		return 0;
	}
	return -ENODEV;
}

static struct usb_driver tbsqboxs2_driver = {
	.name = "tbsqboxs2",
	.probe = tbsqboxs2_probe,
	.disconnect = dvb_usb_device_exit,
	.id_table = tbsqboxs2_table,
};

static int __init tbsqboxs2_module_init(void)
{
	int ret =  usb_register(&tbsqboxs2_driver);
	if (ret)
		err("usb_register failed. Error number %d", ret);

	return ret;
}

static void __exit tbsqboxs2_module_exit(void)
{
	usb_deregister(&tbsqboxs2_driver);
}

module_init(tbsqboxs2_module_init);
module_exit(tbsqboxs2_module_exit);

MODULE_AUTHOR("Bob Liu <Bob@turbosight.com>");
MODULE_DESCRIPTION("Driver for TBS QBOXS2-CX24116");
MODULE_VERSION("0.2");
MODULE_LICENSE("GPL");
