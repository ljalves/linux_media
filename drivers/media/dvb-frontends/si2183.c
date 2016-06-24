/*
 * Silicon Labs Si2183(2) DVB-T/T2/C/C2/S/S2 demodulator driver
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
 */

#include "si2183.h"
#include "dvb_frontend.h"
#include <linux/firmware.h>
#include <linux/i2c-mux.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 7, 0)
#if IS_ENABLED(CONFIG_I2C_MUX)
#define SI2183_USE_I2C_MUX
#endif
#endif

#define SI2183_B60_FIRMWARE "dvb-demod-si2183-b60-01.fw"

#define SI2183_PROP_MODE	0x100a
#define SI2183_PROP_DVBC_CONST	0x1101
#define SI2183_PROP_DVBC_SR	0x1102
#define SI2183_PROP_DVBT_HIER	0x1201
#define SI2183_PROP_DVBT2_MODE	0x1304
#define SI2183_PROP_DVBS2_SR	0x1401
#define SI2183_PROP_DVBS_SR	0x1501

#define SI2183_ARGLEN      30
struct si2183_cmd {
	u8 args[SI2183_ARGLEN];
	unsigned wlen;
	unsigned rlen;
};

static const struct dvb_frontend_ops si2183_ops;

LIST_HEAD(silist);

struct si_base {
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
	struct i2c_mux_core *muxc;
#endif
	struct list_head     silist;

	u8                   adr;
	struct i2c_adapter  *i2c;
	u32                  count;

	struct i2c_adapter  *tuner_adapter;

#ifndef SI2183_USE_I2C_MUX
	struct i2c_client *i2c_gate_client;
#endif
};

/* state struct */
struct si2183_dev {
	struct dvb_frontend fe;
	enum fe_delivery_system delivery_system;
	bool active;
	bool fw_loaded;
	u8 ts_mode;
	bool ts_clock_inv;
	bool ts_clock_gapped;

	struct si_base *base;

	u8 active_fe;
};

/* Own I2C adapter locking is needed because of I2C gate logic. */
static int si2183_i2c_master_send_unlocked(const struct i2c_client *client,
					   const char *buf, int count)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = 0,
		.len = count,
		.buf = (char *)buf,
	};

	ret = __i2c_transfer(client->adapter, &msg, 1);
	return (ret == 1) ? count : ret;
}

static int si2183_i2c_master_recv_unlocked(const struct i2c_client *client,
					   char *buf, int count)
{
	int ret;
	struct i2c_msg msg = {
		.addr = client->addr,
		.flags = I2C_M_RD,
		.len = count,
		.buf = buf,
	};

	ret = __i2c_transfer(client->adapter, &msg, 1);
	return (ret == 1) ? count : ret;
}

/* execute firmware command */
static int si2183_cmd_execute_unlocked(struct i2c_client *client,
				       struct si2183_cmd *cmd)
{
	int ret;
	unsigned long timeout;

	if (cmd->wlen) {
		/* write cmd and args for firmware */
		ret = si2183_i2c_master_send_unlocked(client, cmd->args,
						      cmd->wlen);
		if (ret < 0) {
			goto err;
		} else if (ret != cmd->wlen) {
			ret = -EREMOTEIO;
			goto err;
		}
	}

	if (cmd->rlen) {
		/* wait cmd execution terminate */
		#define TIMEOUT 70
		timeout = jiffies + msecs_to_jiffies(TIMEOUT);
		while (!time_after(jiffies, timeout)) {
			ret = si2183_i2c_master_recv_unlocked(client, cmd->args,
							      cmd->rlen);
			if (ret < 0) {
				goto err;
			} else if (ret != cmd->rlen) {
				ret = -EREMOTEIO;
				goto err;
			}

			/* firmware ready? */
			if ((cmd->args[0] >> 7) & 0x01)
				break;
		}

		dev_dbg(&client->dev, "cmd execution took %d ms\n",
				jiffies_to_msecs(jiffies) -
				(jiffies_to_msecs(timeout) - TIMEOUT));

		/* error bit set? */
		if ((cmd->args[0] >> 6) & 0x01) {
			ret = -EREMOTEIO;
			goto err;
		}

		if (!((cmd->args[0] >> 7) & 0x01)) {
			ret = -ETIMEDOUT;
			goto err;
		}
	}

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int si2183_cmd_execute(struct i2c_client *client, struct si2183_cmd *cmd)
{
	int ret;

	i2c_lock_adapter(client->adapter);
	ret = si2183_cmd_execute_unlocked(client, cmd);
	i2c_unlock_adapter(client->adapter);

	return ret;
}

static int si2183_set_prop(struct i2c_client *client, u16 prop, u16 *val)
{
	struct si2183_cmd cmd;
	int ret;

	cmd.args[0] = 0x14;
	cmd.args[1] = 0x00;
	cmd.args[2] = (u8) prop;
	cmd.args[3] = (u8) (prop >> 8);
	cmd.args[4] = (u8) (*val);
	cmd.args[5] = (u8) (*val >> 8);
	cmd.wlen = 6;
	cmd.rlen = 4;
	ret = si2183_cmd_execute(client, &cmd);
	*val = (cmd.args[2] | (cmd.args[3] << 8));
	return ret;
}
#if 0
static int si2183_get_prop(struct i2c_client *client, u16 prop, u16 *val)
{
	struct si2183_cmd cmd;
	int ret;

	cmd.args[0] = 0x15;
	cmd.args[1] = 0x00;
	cmd.args[2] = (u8) prop;
	cmd.args[3] = (u8) (prop >> 8);
	cmd.wlen = 4;
	cmd.rlen = 4;
	ret = si2183_cmd_execute(client, &cmd);
	*val = (cmd.args[2] | (cmd.args[3] << 8));
	return ret;
}
#endif
static int si2183_read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_dev *dev = i2c_get_clientdata(client);
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret;
	struct si2183_cmd cmd;

	*status = 0;

	if (!dev->active) {
		ret = -EAGAIN;
		goto err;
	}

	if ((dev->delivery_system != c->delivery_system) || (dev->delivery_system == 0))
		return 0;

	switch (c->delivery_system) {
	case SYS_DVBT:
		memcpy(cmd.args, "\xa0\x01", 2);
		cmd.wlen = 2;
		cmd.rlen = 13;
		break;
	case SYS_DVBC_ANNEX_A:
		memcpy(cmd.args, "\x90\x01", 2);
		cmd.wlen = 2;
		cmd.rlen = 9;
		break;
	case SYS_DVBT2:
		memcpy(cmd.args, "\x50\x01", 2);
		cmd.wlen = 2;
		cmd.rlen = 14;
		break;
	case SYS_ISDBT:
		memcpy(cmd.args, "\xa4\x01", 2);
		cmd.wlen = 2;
		cmd.rlen = 14;
		break;
	case SYS_DVBS:
		memcpy(cmd.args, "\x60\x01", 2);
		cmd.wlen = 2;
		cmd.rlen = 10;
		break;
	case SYS_DVBS2:
		memcpy(cmd.args, "\x70\x01", 2);
		cmd.wlen = 2;
		cmd.rlen = 13;
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	ret = si2183_cmd_execute(client, &cmd);
	if (ret) {
		dev_err(&client->dev, "read_status fe%d cmd_exec failed=%d\n", fe->id, ret);
		goto err;
	}

	switch ((cmd.args[2] >> 1) & 0x03) {
	case 0x03:
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER | FE_HAS_VITERBI |
				FE_HAS_SYNC | FE_HAS_LOCK;
		c->cnr.stat[0].svalue = cmd.args[3] * 1000 / 4;
		break;
	case 0x01:
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER;
	default:
		c->cnr.stat[0].svalue = 0;
		break;
	}

	dev_dbg(&client->dev, "status=%02x args=%*ph\n",
			*status, cmd.rlen, cmd.args);
	return 0;
err:
	dev_err(&client->dev, "read_status failed=%d\n", ret);
	return ret;
}


static int si2183_set_dvbc(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct i2c_client *client = fe->demodulator_priv;
	int ret;
	u16 prop;

	/* dvb-c mode */
	prop = 0x38;
	ret = si2183_set_prop(client, SI2183_PROP_MODE, &prop);
	if (ret) {
		dev_err(&client->dev, "err set dvb-c mode\n");
		return ret;
	}

	switch (c->modulation) {
	default:
	case QAM_AUTO:
		prop = 0;
		break;
	case QAM_16:
		prop = 7;
		break;
	case QAM_32:
		prop = 8;
		break;
	case QAM_64:
		prop = 9;
		break;
	case QAM_128:
		prop = 10;
		break;
	case QAM_256:
		prop = 11;
		break;
	}
	ret = si2183_set_prop(client, SI2183_PROP_DVBC_CONST, &prop);
	if (ret) {
		dev_err(&client->dev, "err set dvb-c constelation\n");
		return ret;
	}

	/* symbol rate */
	prop = c->symbol_rate / 1000;
	ret = si2183_set_prop(client, SI2183_PROP_DVBC_SR, &prop);
	if (ret) {
		dev_err(&client->dev, "err set dvb-c symbol rate\n");
		return ret;
	}

	return 0;
}


static int si2183_set_dvbs(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_cmd cmd;
	int ret;
	u16 prop;
	
	/* set mode */
	prop = 0x8;
	switch (c->delivery_system) {
	default:
	case SYS_DVBS:
		prop |= 0x80;
		break;
	case SYS_DVBS2:
		prop |= 0x90;
		break;
	case SYS_DSS:
		prop |= 0xa0;
		break;
	}
	if (c->inversion)
		prop |= 0x100;
	ret = si2183_set_prop(client, SI2183_PROP_MODE, &prop);
	if (ret) {
		dev_err(&client->dev, "err set dvb-s/s2 mode\n");
		return ret;
	}

	/* symbol rate */
	prop = c->symbol_rate / 1000;
	switch (c->delivery_system) {
	default:
	case SYS_DSS:
	case SYS_DVBS:
		ret = si2183_set_prop(client, SI2183_PROP_DVBS_SR, &prop);
		break;
	case SYS_DVBS2:
		ret = si2183_set_prop(client, SI2183_PROP_DVBS2_SR, &prop);
		/* stream_id selection */
		cmd.args[0] = 0x71;
		cmd.args[1] = (u8) c->stream_id;
		cmd.args[2] = c->stream_id == NO_STREAM_ID_FILTER ? 0 : 1;
		cmd.wlen = 3;
		cmd.rlen = 1;
		ret = si2183_cmd_execute(client, &cmd);
		if (ret)
			dev_warn(&client->dev, "dvb-s2: err selecting stream_id\n");
		break;
	}

	return 0;
}

static int si2183_set_dvbt(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_cmd cmd;
	int ret;
	u16 prop;

	if (c->bandwidth_hz == 0) {
		return -EINVAL;
	} else if (c->bandwidth_hz <= 2000000)
		prop = 0x02;
	else if (c->bandwidth_hz <= 5000000)
		prop = 0x05;
	else if (c->bandwidth_hz <= 6000000)
		prop = 0x06;
	else if (c->bandwidth_hz <= 7000000)
		prop = 0x07;
	else if (c->bandwidth_hz <= 8000000)
		prop = 0x08;
	else if (c->bandwidth_hz <= 9000000)
		prop = 0x09;
	else if (c->bandwidth_hz <= 10000000)
		prop = 0x0a;
	else
		prop = 0x0f;

	switch (c->delivery_system) {
	default:
	case SYS_DVBT:
		prop |= 0x20;
		break;
	case SYS_DVBT2:
		prop |= 0x70;
		break;
	}
	ret = si2183_set_prop(client, SI2183_PROP_MODE, &prop);
	if (ret) {
		dev_err(&client->dev, "err set dvb-t mode\n");
		return ret;
	}

	/* hierarchy - HP = 0 / LP = 1 */
	prop = c->hierarchy == HIERARCHY_1 ? 1 : 0;
	ret = si2183_set_prop(client, SI2183_PROP_DVBT_HIER, &prop);
	if (ret)
		dev_warn(&client->dev, "dvb-t: err set hierarchy\n");

	if (c->delivery_system == SYS_DVBT2) {
		/* stream_id selection */
		cmd.args[0] = 0x52;
		cmd.args[1] = (u8) c->stream_id;
		cmd.args[2] = c->stream_id == NO_STREAM_ID_FILTER ? 0 : 1;
		cmd.wlen = 3;
		cmd.rlen = 1;
		ret = si2183_cmd_execute(client, &cmd);
		if (ret)
			dev_warn(&client->dev, "dvb-t2: err selecting stream_id\n");

		/* dvb-t2 mode - any=0 / base=1 / lite=2 */
		prop = 0;
		ret = si2183_set_prop(client, SI2183_PROP_DVBT2_MODE, &prop);
		if (ret)
			dev_warn(&client->dev, "dvb-t2: err set mode\n");
	}

	return 0;
}

static int si2183_set_isdbt(struct dvb_frontend *fe)
{
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	struct i2c_client *client = fe->demodulator_priv;
	int ret;
	u16 prop;

	if (c->bandwidth_hz == 0) {
		return -EINVAL;
	} else if (c->bandwidth_hz <= 2000000)
		prop = 0x02;
	else if (c->bandwidth_hz <= 5000000)
		prop = 0x05;
	else if (c->bandwidth_hz <= 6000000)
		prop = 0x06;
	else if (c->bandwidth_hz <= 7000000)
		prop = 0x07;
	else if (c->bandwidth_hz <= 8000000)
		prop = 0x08;
	else if (c->bandwidth_hz <= 9000000)
		prop = 0x09;
	else if (c->bandwidth_hz <= 10000000)
		prop = 0x0a;
	else
		prop = 0x0f;

	/* ISDB-T mode */
	prop |= 0x40;
	ret = si2183_set_prop(client, SI2183_PROP_MODE, &prop);
	if (ret) {
		dev_err(&client->dev, "err set dvb-t mode\n");
		return ret;
	}
	return 0;
}

static int si2183_set_frontend(struct dvb_frontend *fe)
{
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_dev *dev = i2c_get_clientdata(client);
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret;
	struct si2183_cmd cmd;

	dev_dbg(&client->dev,
			"delivery_system=%u modulation=%u frequency=%u bandwidth_hz=%u symbol_rate=%u inversion=%u stream_id=%u\n",
			c->delivery_system, c->modulation, c->frequency,
			c->bandwidth_hz, c->symbol_rate, c->inversion,
			c->stream_id);

	if (!dev->active) {
		ret = -EAGAIN;
		goto err;
	}

	if (fe->ops.tuner_ops.set_params) {
#ifndef SI2183_USE_I2C_MUX
		if (fe->ops.i2c_gate_ctrl)
			fe->ops.i2c_gate_ctrl(fe, 1);
#endif
		ret = fe->ops.tuner_ops.set_params(fe);
#ifndef SI2183_USE_I2C_MUX
		if (fe->ops.i2c_gate_ctrl)
			fe->ops.i2c_gate_ctrl(fe, 0);
#endif
		if (ret) {
			dev_err(&client->dev, "err setting tuner params\n");
			goto err;
		}
	}

	switch (c->delivery_system) {
	case SYS_DVBT:
	case SYS_DVBT2:
		ret = si2183_set_dvbt(fe);
		break;
	case SYS_DVBC_ANNEX_A:
		ret = si2183_set_dvbc(fe);
		break;
	case SYS_ISDBT:
		ret = si2183_set_isdbt(fe);
		break;
	case SYS_DVBS:
	case SYS_DVBS2:
	case SYS_DSS:
		ret = si2183_set_dvbs(fe);
		break;
	default:
		ret = -EINVAL;
		goto err;
	}

	/* dsp restart */
	memcpy(cmd.args, "\x85", 1);
	cmd.wlen = 1;
	cmd.rlen = 1;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret) {
		dev_err(&client->dev, "err restarting dsp\n");
		return ret;
	}

	dev->delivery_system = c->delivery_system;
	return 0;
err:
	dev_err(&client->dev, "set_params failed=%d\n", ret);
	return ret;
}

static int si2183_init(struct dvb_frontend *fe)
{
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_dev *dev = i2c_get_clientdata(client);
	struct dtv_frontend_properties *c = &fe->dtv_property_cache;
	int ret = 0, len, remaining;
	const struct firmware *fw;
	const char *fw_name;
	struct si2183_cmd cmd;
	unsigned int chip_id;
	u16 prop;

	dev_dbg(&client->dev, "\n");

	if (dev->active_fe) {
		dev->active_fe |= (1 << fe->id);
		return 0;
	}

	c->cnr.len = 1;
	c->cnr.stat[0].scale = FE_SCALE_DECIBEL;

	/* initialize */
	memcpy(cmd.args, "\xc0\x12\x00\x0c\x00\x0d\x16\x00\x00\x00\x00\x00\x00", 13);
	cmd.wlen = 13;
	cmd.rlen = 0;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;

	if (dev->fw_loaded) {
		/* resume */
		memcpy(cmd.args, "\xc0\x06\x08\x0f\x00\x20\x21\x01", 8);
		cmd.wlen = 8;
		cmd.rlen = 1;
		ret = si2183_cmd_execute(client, &cmd);
		if (ret)
			goto err;

		memcpy(cmd.args, "\x85", 1);
		cmd.wlen = 1;
		cmd.rlen = 1;
		ret = si2183_cmd_execute(client, &cmd);
		if (ret)
			goto err;

		goto warm;
	}

	/* power up */
	memcpy(cmd.args, "\xc0\x06\x01\x0f\x00\x20\x20\x01", 8);
	cmd.wlen = 8;
	cmd.rlen = 1;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;

	/* query chip revision */
	memcpy(cmd.args, "\x02", 1);
	cmd.wlen = 1;
	cmd.rlen = 13;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;

	chip_id = cmd.args[1] << 24 | cmd.args[2] << 16 | cmd.args[3] << 8 |
			cmd.args[4] << 0;

	#define SI2183_B60 ('B' << 24 | 83 << 16 | '6' << 8 | '0' << 0)

	switch (chip_id) {
	case SI2183_B60:
		fw_name = SI2183_B60_FIRMWARE;
		break;
	default:
		dev_err(&client->dev, "unknown chip version Si21%d-%c%c%c\n",
				cmd.args[2], cmd.args[1],
				cmd.args[3], cmd.args[4]);
		ret = -EINVAL;
		goto err;
	}

	dev_info(&client->dev, "found a 'Silicon Labs Si21%d-%c%c%c'\n",
			cmd.args[2], cmd.args[1], cmd.args[3], cmd.args[4]);

	ret = request_firmware(&fw, fw_name, &client->dev);
	if (ret) {
		dev_err(&client->dev,
				"firmware file '%s' not found\n",
				fw_name);
		goto err;
	}

	dev_info(&client->dev, "downloading firmware from file '%s'\n",
			fw_name);

	for (remaining = fw->size; remaining > 0; remaining -= 17) {
		len = fw->data[fw->size - remaining];
		if (len > SI2183_ARGLEN) {
			ret = -EINVAL;
			break;
		}
		memcpy(cmd.args, &fw->data[(fw->size - remaining) + 1], len);
		cmd.wlen = len;
		cmd.rlen = 1;
		ret = si2183_cmd_execute(client, &cmd);
		if (ret)
			break;
	}
	release_firmware(fw);

	if (ret) {
		dev_err(&client->dev, "firmware download failed %d\n", ret);
		goto err;
	}

	memcpy(cmd.args, "\x01\x01", 2);
	cmd.wlen = 2;
	cmd.rlen = 1;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;

	/* query firmware version */
	memcpy(cmd.args, "\x11", 1);
	cmd.wlen = 1;
	cmd.rlen = 10;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;

	dev_info(&client->dev, "firmware version: %c.%c.%d\n",
			cmd.args[6], cmd.args[7], cmd.args[8]);

	/* set ts mode */
	memcpy(cmd.args, "\x14\x00\x01\x10\x10\x00", 6);
	cmd.args[4] |= dev->ts_mode;
	if (dev->ts_clock_gapped)
		cmd.args[4] |= 0x40;
	cmd.wlen = 6;
	cmd.rlen = 4;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;


	/* FER resol */
	memcpy(cmd.args, "\x14\x00\x0c\x10\x12\x00", 6);
	cmd.wlen = 6;
	cmd.rlen = 4;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret) {
		dev_err(&client->dev, "err set FER resol\n");
		return ret;
	}

	/* DD IEN */
	memcpy(cmd.args, "\x14\x00\x06\x10\x24\x00", 6);
	cmd.wlen = 6;
	cmd.rlen = 4;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret) {
		dev_err(&client->dev, "err set dd ien\n");
		return ret;
	}

	/* int sense */
	memcpy(cmd.args, "\x14\x00\x07\x10\x00\x24", 6);
	cmd.wlen = 6;
	cmd.rlen = 4;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret) {
		dev_err(&client->dev, "err set int sense\n");
		return ret;
	}

/*
	memcpy(cmd.args, "\x88\x02\x02\x02\x02", 5);
	cmd.wlen = 5;
	cmd.rlen = 5;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret) {
		dev_err(&client->dev, "err set x88\n");
	}
*/

	prop = 0x10;
	ret = si2183_set_prop(client, 0x100f, &prop);
	if (ret) {
		dev_err(&client->dev, "err set 0x100f\n");
		return ret;
	}

	prop = 0x08e3 | (dev->ts_clock_inv ? 0x0000 : 0x1000);
	ret = si2183_set_prop(client, 0x1009, &prop);
	if (ret) {
		dev_err(&client->dev, "err set par_ts\n");
		return ret;
	}

	prop = 0x05d7 | (dev->ts_clock_inv ? 0x0000 : 0x1000);
	ret = si2183_set_prop(client, 0x1008, &prop);
	if (ret) {
		dev_err(&client->dev, "err set ser_ts\n");
		return ret;
	}

	dev->fw_loaded = true;
warm:
	dev->active = true;
	dev->active_fe |= (1 << fe->id);
	return 0;

err:
	dev_dbg(&client->dev, "init failed=%d\n", ret);
	return ret;
}

static int si2183_sleep(struct dvb_frontend *fe)
{
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_dev *dev = i2c_get_clientdata(client);
	int ret;
	struct si2183_cmd cmd;

	dev_dbg(&client->dev, "\n");

	dev->active_fe &= ~(1 << fe->id);
	if (dev->active_fe)
		return 0;

	dev->active = false;

	memcpy(cmd.args, "\x13", 1);
	cmd.wlen = 1;
	cmd.rlen = 0;
	ret = si2183_cmd_execute(client, &cmd);
	if (ret)
		goto err;

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

static int si2183_get_tune_settings(struct dvb_frontend *fe,
	struct dvb_frontend_tune_settings *s)
{
	s->min_delay_ms = 900;

	return 0;
}

#ifdef SI2183_USE_I2C_MUX
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
static int si2183_select(struct i2c_mux_core *muxc, u32 chan)
{
	struct i2c_client *client = i2c_mux_priv(muxc);
#else
static int si2183_select(struct i2c_adapter *adap, void *mux_priv, u32 chan)
{
	struct i2c_client *client = mux_priv;
#endif
	int ret;
	struct si2183_cmd cmd;

	/* open I2C gate */
	memcpy(cmd.args, "\xc0\x0d\x01", 3);
	cmd.wlen = 3;
	cmd.rlen = 0;
	ret = si2183_cmd_execute_unlocked(client, &cmd);
	if (ret)
		goto err;

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
static int si2183_deselect(struct i2c_mux_core *muxc, u32 chan)
{
	struct i2c_client *client = i2c_mux_priv(muxc);
#else
static int si2183_deselect(struct i2c_adapter *adap, void *mux_priv, u32 chan)
{
	struct i2c_client *client = mux_priv;
#endif
	int ret;
	struct si2183_cmd cmd;

	/* close I2C gate */
	memcpy(cmd.args, "\xc0\x0d\x00", 3);
	cmd.wlen = 3;
	cmd.rlen = 0;
	ret = si2183_cmd_execute_unlocked(client, &cmd);
	if (ret)
		goto err;

	return 0;
err:
	dev_dbg(&client->dev, "failed=%d\n", ret);
	return ret;
}
#else
static int i2c_gate_ctrl(struct dvb_frontend* fe, int enable)
{
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_dev *dev = i2c_get_clientdata(client);
	struct si2183_cmd cmd;

	memcpy(cmd.args, "\xc0\x0d\x00", 3);
	if (enable)
		cmd.args[2] = 1;
	cmd.wlen = 3;
	cmd.rlen = 0;
	return si2183_cmd_execute(dev->base->i2c_gate_client, &cmd);
}
#endif

static int si2183_tune(struct dvb_frontend *fe, bool re_tune,
	unsigned int mode_flags, unsigned int *delay, enum fe_status *status)
{
	*delay = HZ / 5;
	if (re_tune) {
		int ret = si2183_set_frontend(fe);
		if (ret)
			return ret;
	}
	return si2183_read_status(fe, status);
}

static int si2183_get_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int si2183_set_property(struct dvb_frontend *fe,
		struct dtv_property *p)
{
	int ret = 0;

	switch (p->cmd) {
	case DTV_DELIVERY_SYSTEM:
		switch (p->u.data) {
		case SYS_DVBS:
		case SYS_DVBS2:
		case SYS_DSS:
			fe->ops.info.frequency_min = 950000;
			fe->ops.info.frequency_max = 2150000;
			fe->ops.info.frequency_stepsize = 0;
			break;
		case SYS_ISDBT:
			fe->ops.info.frequency_min = 42000000;
			fe->ops.info.frequency_max = 1002000000;
			fe->ops.info.frequency_stepsize = 0;
			break;
		case SYS_DVBC_ANNEX_A:
			fe->ops.info.frequency_min = 47000000;
			fe->ops.info.frequency_max = 862000000;
			fe->ops.info.frequency_stepsize = 62500;
			break;
		case SYS_DVBT:
		case SYS_DVBT2:
		default:
			fe->ops.info.frequency_min = 174000000;
			fe->ops.info.frequency_max = 862000000;
			fe->ops.info.frequency_stepsize = 250000;
			break;
		}
		break;
	default:
		break;
	}

	return ret;
}


static int send_diseqc_cmd(struct dvb_frontend *fe,
	u8 cont_tone, u8 tone_burst, u8 burst_sel,
	u8 end_seq, u8 msg_len, u8 *msg)
{
	struct i2c_client *client = fe->demodulator_priv;
	struct si2183_cmd cmd;
	u8 enable = 1;

	cmd.args[0] = 0x8c;
	cmd.args[1] |= enable | (cont_tone << 1)
		    | (tone_burst << 2) | (burst_sel << 3)
		    | (end_seq << 4) | (msg_len << 5);

	if (msg_len > 0)
		memcpy(&cmd.args[2], msg, msg_len);

	cmd.wlen = 8;
	cmd.rlen = 1;
	return si2183_cmd_execute(client, &cmd);
}

static int set_tone(struct dvb_frontend *fe, enum fe_sec_tone_mode tone)
{
	struct i2c_client *client = fe->demodulator_priv;
	int ret;
	u8 cont_tone;

	switch (tone) {
	case SEC_TONE_ON:
		cont_tone = 1;
		break;
	case SEC_TONE_OFF:
		cont_tone = 0;
		break;
	default:
		return -EINVAL;
	}

	ret = send_diseqc_cmd(fe, cont_tone, 0, 0, 1, 0, NULL);
	if (ret)
		goto err;

	return 0;
err:
	dev_err(&client->dev, "set_tone failed=%d\n", ret);
	return ret;
}

static int diseqc_send_burst(struct dvb_frontend *fe,
	enum fe_sec_mini_cmd burst)
{
	struct i2c_client *client = fe->demodulator_priv;
	int ret;
	u8 burst_sel;

	switch (burst) {
	case SEC_MINI_A:
		burst_sel = 0;
		break;
	case SEC_MINI_B:
		burst_sel = 1;
		break;
	default:
		return -EINVAL;
	}

	ret = send_diseqc_cmd(fe, 0, 1, burst_sel, 1, 0, NULL);
	if (ret)
		goto err;

	return 0;
err:
	dev_err(&client->dev, "set_tone failed=%d\n", ret);
	return ret;
}

static int send_diseqc_msg(struct dvb_frontend *fe,
	struct dvb_diseqc_master_cmd *d)
{
	struct i2c_client *client = fe->demodulator_priv;
	int ret;
	u8 remaining = d->msg_len;
	u8 *p = d->msg;
	u8 len = 0;

	while (remaining > 0) {
		p += len;
		len = (remaining > 6) ? 6 : remaining;
		remaining -= len;
		ret = send_diseqc_cmd(fe, 0, 0, 0,
			(remaining == 0) ? 1 : 0, len, p);
		if (ret)
			goto err;
		msleep(50);
	}

	return 0;
err:
	dev_err(&client->dev, "set_tone failed=%d\n", ret);
	return ret;
}

static const struct dvb_frontend_ops si2183_ops = {
	.delsys = {SYS_DVBT, SYS_DVBT2,
		   SYS_ISDBT,
		   SYS_DVBC_ANNEX_A,
		   SYS_DVBS, SYS_DVBS2, SYS_DSS},
	.info = {
		.name = "Silicon Labs Si2183",
		.symbol_rate_min = 1000000,
		.symbol_rate_max = 45000000,
		.caps =	FE_CAN_FEC_1_2 |
			FE_CAN_FEC_2_3 |
			FE_CAN_FEC_3_4 |
			FE_CAN_FEC_5_6 |
			FE_CAN_FEC_7_8 |
			FE_CAN_FEC_AUTO |
			FE_CAN_QPSK |
			FE_CAN_QAM_16 |
			FE_CAN_QAM_32 |
			FE_CAN_QAM_64 |
			FE_CAN_QAM_128 |
			FE_CAN_QAM_256 |
			FE_CAN_QAM_AUTO |
			FE_CAN_TRANSMISSION_MODE_AUTO |
			FE_CAN_GUARD_INTERVAL_AUTO |
			FE_CAN_HIERARCHY_AUTO |
			FE_CAN_MUTE_TS |
			FE_CAN_2G_MODULATION |
			FE_CAN_MULTISTREAM
	},

	.get_tune_settings = si2183_get_tune_settings,

	.init = si2183_init,
	.sleep = si2183_sleep,

	.set_frontend = si2183_set_frontend,

	.read_status = si2183_read_status,

	.get_frontend_algo = si2183_get_algo,
	.tune = si2183_tune,

	.set_property			= si2183_set_property,

	.set_tone			= set_tone,
	.diseqc_send_burst		= diseqc_send_burst,
	.diseqc_send_master_cmd		= send_diseqc_msg,
#ifndef SI2183_USE_I2C_MUX
	.i2c_gate_ctrl			= i2c_gate_ctrl,
#endif
};


static struct si_base *match_base(struct i2c_adapter *i2c, u8 adr)
{
	struct si_base *p;

	list_for_each_entry(p, &silist, silist)
		if (p->i2c == i2c)// && p->adr == adr) lja: TO FIX
			return p;
	return NULL;
}

static int si2183_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct si2183_config *config = client->dev.platform_data;
	struct si2183_dev *dev;
	struct si_base *base;
	int ret;

	dev_dbg(&client->dev, "\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		ret = -ENOMEM;
		dev_err(&client->dev, "kzalloc() failed\n");
		goto err;
	}

	base = match_base(client->adapter, client->addr);
	if (base) {
		base->count++;
		dev->base = base;
	} else {
		base = kzalloc(sizeof(struct si_base), GFP_KERNEL);
		if (!base)
			goto err_kfree;
		base->i2c = client->adapter;
		base->adr = client->addr;
		base->count = 1;
		dev->base = base;
		list_add(&base->silist, &silist);

#ifdef SI2183_USE_I2C_MUX
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		/* create mux i2c adapter for tuner */
		base->muxc = i2c_mux_alloc(client->adapter, &client->adapter->dev,
					  1, 0, I2C_MUX_LOCKED,
					  si2183_select, si2183_deselect);
		if (!base->muxc) {
			ret = -ENOMEM;
			goto err_base_kfree;
		}
		base->muxc->priv = client;
		ret = i2c_mux_add_adapter(base->muxc, 0, 0, 0);
		if (ret)
			goto err_base_kfree;
		base->tuner_adapter = base->muxc->adapter[0];
#else
		/* create mux i2c adapter for tuners */
		base->tuner_adapter = i2c_add_mux_adapter(client->adapter, &client->adapter->dev,
				client, 0, 0, 0, si2183_select, si2183_deselect);
		if (base->tuner_adapter == NULL) {
			ret = -ENODEV;
			goto err_base_kfree;
		}
#endif
#else
		base->tuner_adapter = client->adapter;
		base->i2c_gate_client = client;
#endif
	}

	/* create dvb_frontend */
	memcpy(&dev->fe.ops, &si2183_ops, sizeof(struct dvb_frontend_ops));
	dev->fe.demodulator_priv = client;
	*config->i2c_adapter = base->tuner_adapter;
	*config->fe = &dev->fe;
	dev->ts_mode = config->ts_mode;
	dev->ts_clock_inv = config->ts_clock_inv;
	dev->ts_clock_gapped = config->ts_clock_gapped;
	dev->fw_loaded = false;

	dev->active_fe = 0;

	i2c_set_clientdata(client, dev);

#ifndef SI2183_USE_I2C_MUX
	/* leave gate open for tuner to init */
	i2c_gate_ctrl(&dev->fe, 1);
#endif
	dev_info(&client->dev, "Silicon Labs Si2183 successfully attached\n");
	return 0;
err_base_kfree:
	kfree(base);
err_kfree:
	kfree(dev);
err:
	dev_dbg(&client->dev, "probe failed=%d\n", ret);
	return ret;
}

static int si2183_remove(struct i2c_client *client)
{
	struct si2183_dev *dev = i2c_get_clientdata(client);

	dev_dbg(&client->dev, "\n");

	dev->base->count--;
	if (dev->base->count == 0) {
#ifdef SI2183_USE_I2C_MUX
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 7, 0)
		i2c_mux_del_adapters(dev->base->muxc);
#else
		i2c_del_mux_adapter(dev->base->tuner_adapter);
#endif
#endif
		list_del(&dev->base->silist);
		kfree(dev->base);
	}

	dev->fe.ops.release = NULL;
	dev->fe.demodulator_priv = NULL;

	kfree(dev);

	return 0;
}

static const struct i2c_device_id si2183_id_table[] = {
	{"si2183", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, si2183_id_table);

static struct i2c_driver si2183_driver = {
	.driver = {
		.name	= "si2183",
	},
	.probe		= si2183_probe,
	.remove		= si2183_remove,
	.id_table	= si2183_id_table,
};

module_i2c_driver(si2183_driver);

MODULE_AUTHOR("Luis Alves <ljalvs@gmail.com>");
MODULE_DESCRIPTION("Silicon Labs Si2183 DVB-T/T2/C/C2/S/S2 demodulator driver");
MODULE_LICENSE("GPL");
