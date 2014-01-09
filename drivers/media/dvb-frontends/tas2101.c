/*
    Tmax TAS2101 - DVBS/S2 Satellite demod/tuner driver

    Copyright (C) 2014 Luis Alves <ljalvs@gmail.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>

#include "dvb_frontend.h"

#include "tas2101.h"
#include "tas2101_priv.h"

/* return i2c adapter from this frontend */
struct i2c_adapter* tas2101_get_i2c_adapter(struct dvb_frontend *fe)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	return priv->i2c;
}
EXPORT_SYMBOL_GPL(tas2101_get_i2c_adapter);

/* write multiple (continuous) registers */
/* the first value is the starting address */
static int tas2101_wrm(struct tas2101_priv *priv, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg = {
		.addr = priv->cfg->demod_address,
		.flags = 0, .buf = buf, .len = len };

	dev_dbg(&priv->i2c->dev, "%s() i2c wrm @0x%02x (len=%d)\n",
		__func__, buf[0], len);

	ret = i2c_transfer(priv->i2c, &msg, 1);
	if (ret < 0) {
		dev_warn(&priv->i2c->dev,
			"%s: i2c wrm err(%i) @0x%02x (len=%d)\n",
			KBUILD_MODNAME, ret, buf[0], len);
		return ret;
	}
	return 0;
}

/* write one register */
static int tas2101_wr(struct tas2101_priv *priv, u8 addr, u8 data)
{
	u8 buf[] = { addr, data };
	return tas2101_wrm(priv, buf, 2);
}

/* read multiple (continuous) registers starting at addr */
static int tas2101_rdm(struct tas2101_priv *priv, u8 addr, u8 *buf, int len)
{
	int ret;
	struct i2c_msg msg[] = {
		{ .addr = priv->cfg->demod_address, .flags = 0,
			.buf = &addr, .len = 1 },
		{ .addr = priv->cfg->demod_address, .flags = I2C_M_RD,
			.buf = buf, .len = len }
	};

	dev_dbg(&priv->i2c->dev, "%s() i2c rdm @0x%02x (len=%d)\n",
		__func__, buf[0], len);

	ret = i2c_transfer(priv->i2c, msg, 2);
	if (ret < 0) {
		dev_warn(&priv->i2c->dev,
			"%s: i2c rdm err(%i) @0x%02x (len=%d)\n",
			KBUILD_MODNAME, ret, addr, len);
		return ret;
	}
	return 0;
}

/* read one register */
static int tas2101_rd(struct tas2101_priv *priv, u8 addr, u8 *data)
{
	return tas2101_rdm(priv, addr, data, 1);
}

static int tas2101_regmask(struct tas2101_priv *priv,
	u8 reg, u8 setmask, u8 clrmask)
{
	int ret;
	u8 b;

	ret = tas2101_rd(priv, reg, &b);
	if (ret)
		return ret;
	return tas2101_wr(priv, reg, (b & ~clrmask) | setmask);
}

static int tas2101_wrtable(struct tas2101_priv *priv,
	struct tas2101_regtable *regtable, int len)
{
	int ret, i;

	for (i = 0; i < len; i++) {
		ret = tas2101_regmask(priv, regtable[i].addr,
			regtable[i].setmask, regtable[i].clrmask);
		if (ret)
			return ret;
		if (regtable[i].sleep)
			msleep(regtable[i].sleep);
	}
	return 0;
}

/* unimplemented */
static int tas2101_read_status(struct dvb_frontend *fe, fe_status_t *status)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	int ret;
	u8 reg;

	*status = 0;

	ret = tas2101_rd(priv, DEMOD_STATUS, &reg);
	if (ret)
		return ret;

	reg &= DEMOD_STATUS_MASK;
	if (reg == DEMOD_LOCKED) {
		*status = FE_HAS_SIGNAL | FE_HAS_CARRIER |
			FE_HAS_VITERBI | FE_HAS_SYNC | FE_HAS_LOCK;

		ret = tas2101_rd(priv, REG_04, &reg);
		if (ret)
			return ret;
		if (reg & 0x08)
			ret = tas2101_wr(priv, REG_04, reg & ~0x08);
	}

	dev_dbg(&priv->i2c->dev, "%s() status = 0x%02x\n", __func__, *status);
	return ret;
}

/* unimplemented */
static int tas2101_read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	*ber = 0;
	dev_dbg(&priv->i2c->dev, "%s() ber = %d\n", __func__, *ber);
	return 0;
}

/* unimplemented */
static int tas2101_read_signal_strength(struct dvb_frontend *fe,
	u16 *signal_strength)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	*signal_strength = 0xf000;
	dev_dbg(&priv->i2c->dev, "%s() strength = 0x%04x\n",
		__func__, *signal_strength);
	return 0;
}

/* unimplemented */
static int tas2101_read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	*snr = 0;
	dev_dbg(&priv->i2c->dev, "%s() snr = 0x%04x\n",
		__func__, *snr);
	return 0;
}

/* unimplemented */
static int tas2101_read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	dev_dbg(&priv->i2c->dev, "%s()\n", __func__);
	*ucblocks = 0;
	return 0;
}

static int tas2101_set_voltage(struct dvb_frontend *fe,
	fe_sec_voltage_t voltage)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	int ret = 0;
	
	dev_dbg(&priv->i2c->dev, "%s() %s\n", __func__,
		voltage == SEC_VOLTAGE_13 ? "SEC_VOLTAGE_13" :
		voltage == SEC_VOLTAGE_18 ? "SEC_VOLTAGE_18" :
		"SEC_VOLTAGE_OFF");

	switch (voltage) {
		case SEC_VOLTAGE_13:
			priv->cfg->lnb_power(fe, LNB_ON);
			ret = tas2101_regmask(priv, LNB_CTRL,
				0, VSEL13_18);
			break;
		case SEC_VOLTAGE_18:
			priv->cfg->lnb_power(fe, LNB_ON);
			ret = tas2101_regmask(priv, LNB_CTRL,
				VSEL13_18, 0);
			break;
		default: /* OFF */
			priv->cfg->lnb_power(fe, LNB_OFF);
			break;
	}
	return ret;
}

static int tas2101_set_tone(struct dvb_frontend *fe,
	fe_sec_tone_mode_t tone)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	int ret = -EINVAL;

	dev_dbg(&priv->i2c->dev, "%s() %s\n", __func__,
		tone == SEC_TONE_ON ? "SEC_TONE_ON" : "SEC_TONE_OFF");

	switch (tone) {
	case SEC_TONE_ON:
		ret = tas2101_regmask(priv, LNB_CTRL,
			TONE_ON, DISEQC_CMD_MASK);
		break;
	case SEC_TONE_OFF:
		ret = tas2101_regmask(priv, LNB_CTRL,
			TONE_OFF, DISEQC_CMD_MASK);
		break;
	default:
		dev_warn(&priv->i2c->dev, "%s() invalid tone (%d)\n",
			__func__, tone);
		break;
	}
	return ret;
}

static int tas2101_diseqc_send_burst(struct dvb_frontend *fe,
	fe_sec_mini_cmd_t burst)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	int ret, i;
	u8 bck, r;

	if ((burst != SEC_MINI_A) && (burst != SEC_MINI_B)) {
		dev_err(&priv->i2c->dev, "%s() invalid burst(%d)\n",
			__func__, burst);
		return -EINVAL;
	}

	dev_dbg(&priv->i2c->dev, "%s() %s\n", __func__,
		burst == SEC_MINI_A ? "SEC_MINI_A" : "SEC_MINI_B");

	/* backup LNB tone state */
	ret = tas2101_rd(priv, LNB_CTRL, &bck);
	if (ret)
		return ret;

	ret = tas2101_regmask(priv, REG_34, 0, 0x40);
	if (ret)
		goto exit;

	/* set tone burst cmd */
	r = (bck & ~DISEQC_CMD_MASK) |
		(burst == SEC_MINI_A) ? DISEQC_BURST_A : DISEQC_BURST_B;

	ret = tas2101_wr(priv, LNB_CTRL, r);
	if (ret)
		goto exit;

	/* spec = around 12.5 ms for the burst */
	for (i = 0; i < 10; i++) {
		tas2101_rd(priv, LNB_STATUS, &r);
		if (r & DISEQC_BUSY)
			goto exit;
		msleep(20);
	}

	/* try to restore the tone setting but return a timeout error */
	ret = tas2101_wr(priv, LNB_CTRL, bck);
	dev_warn(&priv->i2c->dev, "%s() timeout sending burst\n", __func__);
	return -ETIMEDOUT;
exit:
	/* restore tone setting */
	return tas2101_wr(priv, LNB_CTRL, bck);
}



static void tas2101_release(struct dvb_frontend *fe)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	dev_dbg(&priv->i2c->dev, "%s\n", __func__);
	kfree(priv);
}

static struct dvb_frontend_ops tas2101_ops;

struct dvb_frontend *tas2101_attach(const struct tas2101_config *cfg,
	struct i2c_adapter *i2c)
{
	struct tas2101_priv *priv = NULL;
	int ret;
	u8 id[2];

	dev_dbg(&i2c->dev, "%s: Attaching frontend\n", KBUILD_MODNAME);

	/* allocate memory for the priv data */
	priv = kzalloc(sizeof(struct tas2101_priv), GFP_KERNEL);
	if (priv == NULL)
		goto err;

	priv->i2c = i2c;
	priv->cfg = cfg;

	/* create dvb_frontend */
	memcpy(&priv->fe.ops, &tas2101_ops,
		sizeof(struct dvb_frontend_ops));
	priv->fe.demodulator_priv = priv;

	/* reset demod */
	cfg->reset_demod(&priv->fe);

	/* check if demod is alive */
	ret = tas2101_rdm(priv, ID_0, id, 2);
	if (ret)
		goto err1;
	if ((id[0] != 0x44) || (id[1] != 0x4c))
		goto err1;

	return &priv->fe;

err1:
	kfree(priv);
err:
	dev_err(&i2c->dev, "%s: Error attaching frontend\n", KBUILD_MODNAME);
	return NULL;
}
EXPORT_SYMBOL_GPL(tas2101_attach);

static int tas2101_initfe(struct dvb_frontend *fe)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	int ret;

	dev_dbg(&priv->i2c->dev, "%s()\n", __func__);

	ret = tas2101_wrtable(priv, tas2101_initfe0,
		ARRAY_SIZE(tas2101_initfe0));
	if (ret)
		return ret;

	switch (priv->cfg->id) {
	case 0:
		ret = tas2101_wrtable(priv, tas2101_initfe1a,
			ARRAY_SIZE(tas2101_initfe1a));
		break;
	case 1:
		ret = tas2101_wrtable(priv, tas2101_initfe1b,
			ARRAY_SIZE(tas2101_initfe1b));
		break;
	default:
		dev_warn(&priv->i2c->dev, "%s: no init for frontend id=%d\n",
			__func__, priv->cfg->id);
		ret = -EINVAL;
		break;
	}
	if (ret)
		return ret;

	ret = tas2101_wrtable(priv, tas2101_initfe2,
		ARRAY_SIZE(tas2101_initfe2));
	if (ret)
		return ret;

	return 0;
}

static int tas2101_sleep(struct dvb_frontend *fe)
{
	struct tas2101_priv *priv = fe->demodulator_priv;
	dev_dbg(&priv->i2c->dev, "%s()\n", __func__);
	return 0;
}

static int tas2101_get_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static struct dvb_frontend_ops tas2101_ops = {
	.delsys = { SYS_DVBS, SYS_DVBS2 },
	.info = {
		.name = "Tmax TAS2101",
		.frequency_min = 950000,
		.frequency_max = 2150000,
		.frequency_stepsize = 1011, /* kHz for QPSK frontends */
		.frequency_tolerance = 5000,
		.symbol_rate_min = 1000000,
		.symbol_rate_max = 45000000,
		.caps = FE_CAN_INVERSION_AUTO |
			FE_CAN_FEC_1_2 | FE_CAN_FEC_2_3 | FE_CAN_FEC_3_4 |
			FE_CAN_FEC_4_5 | FE_CAN_FEC_5_6 | FE_CAN_FEC_6_7 |
			FE_CAN_FEC_7_8 | FE_CAN_FEC_AUTO |
			FE_CAN_2G_MODULATION |
			FE_CAN_QPSK | FE_CAN_RECOVER
	},
	.release = tas2101_release,
	.init = tas2101_initfe,
	.sleep = tas2101_sleep,
	.read_status = tas2101_read_status,
	.read_ber = tas2101_read_ber,
	.read_signal_strength = tas2101_read_signal_strength,
	.read_snr = tas2101_read_snr,
	.read_ucblocks = tas2101_read_ucblocks,
	.set_tone = tas2101_set_tone,
	.set_voltage = tas2101_set_voltage,
//	.diseqc_send_master_cmd = tas2101_send_diseqc_msg,
	.diseqc_send_burst = tas2101_diseqc_send_burst,
	.get_frontend_algo = tas2101_get_algo,
//	.tune = tas2101_tune,
//	.set_frontend = tas2101_set_frontend,
//	.get_frontend = tas2101_get_frontend,
};

MODULE_DESCRIPTION("DVB Frontend module for Tmax TAS2101");
MODULE_AUTHOR("Luis Alves (ljalvs@gmail.com)");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

