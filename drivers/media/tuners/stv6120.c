/*
 * Driver for the ST STV6120 tuner
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 only, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <asm/div64.h>

#include "dvb_frontend.h"

#include "stv6120.h"

#define REG_N0		0
#define REG_N1_F0	1
#define REG_F1		2
#define REG_F2_ICP	3
#define REG_CF_PDIV	4
#define REG_CFHF	5
#define REG_CAL		6



static const u8 tuner_init[25] = {
 0x77,
 0x33, 0xce, 0x54, 0x55, 0x0d, 0x32, 0x44, 0x0e,
 0xf9, 0x1b,
 0x33, 0xce, 0x54, 0x55, 0x0d, 0x32, 0x44, 0x0e,
 0x00, 0x00, 0x4c, 0x00, 0x00, 0x4c,
};

LIST_HEAD(stvlist);

static inline u32 MulDiv32(u32 a, u32 b, u32 c)
{
	u64 tmp64;

	tmp64 = (u64)a * (u64)b;
	do_div(tmp64, c);

	return (u32) tmp64;
}


struct stv_base {
	struct list_head     stvlist;

	u8                   adr;
	struct i2c_adapter  *i2c;
	struct mutex         i2c_lock;
	struct mutex         reg_lock;
	int                  count;
};


struct stv {
	struct stv_base     *base;
	struct dvb_frontend *fe;
	int                  nr;

	struct stv6120_cfg *cfg;

	u8 reg[7];
};

static int i2c_read(struct i2c_adapter *adap,
		    u8 adr, u8 *msg, int len, u8 *answ, int alen)
{
	struct i2c_msg msgs[2] = { { .addr = adr, .flags = 0,
				     .buf = msg, .len = len},
				   { .addr = adr, .flags = I2C_M_RD,
				     .buf = answ, .len = alen } };
	if (i2c_transfer(adap, msgs, 2) != 2) {
		pr_err("stv6120: i2c_read error\n");
		return -1;
	}
	return 0;
}

static int i2c_write(struct i2c_adapter *adap, u8 adr, u8 *data, int len)
{
	struct i2c_msg msg = {.addr = adr, .flags = 0,
			      .buf = data, .len = len};

	if (i2c_transfer(adap, &msg, 1) != 1) {
		pr_err("stv6120: i2c_write error\n");
		return -1;
	}
	return 0;
}
#if 0
static int write_regs(struct stv *state, int reg, int len)
{
	u8 d[8];

	u8 base = 0x02 + 0xa * state->nr;

	memcpy(&d[1], &state->base->reg[reg], len);
	d[0] = reg;
	return i2c_write(state->base->i2c, state->base->adr, d, len + 1);
}
#endif
static int write_tuner_regs(struct stv *state)
{
	u8 d[8];
	memcpy(&d[1], state->reg, 7);
	d[0] = 0x02 + 0xa * state->nr;
	return i2c_write(state->base->i2c, state->base->adr, d, 8);
}

#if 0
static int write_reg(struct stv *state, u8 reg, u8 val)
{
	u8 d[2] = {reg, val};

	return i2c_write(state->i2c, state->adr, d, 2);
}
#endif

static int read_reg(struct stv *state, u8 reg, u8 *val)
{
	return i2c_read(state->base->i2c, state->base->adr, &reg, 1, val, 1);
}

static int read_regs(struct stv *state, u8 reg, u8 *val, int len)
{
	return i2c_read(state->base->i2c, state->base->adr, &reg, 1, val, len);
}

#if 0
static void dump_regs(struct stv *state)
{
	u8 d[25], *c = &state->reg[0];

	read_regs(state, state->nr * 0xa + 2, d, 7);
	pr_info("stv6120_regs = %02x %02x %02x %02x %02x %02x %02x\n",
		d[0], d[1], d[2], d[3], d[4], d[5], d[6]);
	pr_info("reg[] =        %02x %02x %02x %02x %02x %02x %02x\n",
		c[0], c[1], c[2], c[3], c[4], c[5], c[6]);


	read_regs(state, 0, d, 14);
	pr_info("global: 0=%02x a=%02x 1=%02x b=%02x\n", d[0], d[0xa], d[1], d[0xb]);
}
#endif

static int wait_for_call_done(struct stv *state, u8 mask)
{
	int status = 0;
	u32 LockRetryCount = 10;

	while (LockRetryCount > 0) {
		u8 Status;

		status = read_reg(state, state->nr * 0xa + 8, &Status);
		if (status < 0)
			return status;

		if ((Status & mask) == 0)
			break;
		usleep_range(4000, 6000);
		LockRetryCount -= 1;

		status = -1;
	}
	return status;
}

static void init_regs(struct stv *state)
{
	u32 clkdiv = 0;
	u32 agcmode = 0;
	u32 agcref = 2;
	u32 agcset = 0xffffffff;
	u32 bbmode = 0xffffffff;

/*
 0f40 00770133 02ce0354 0455050d 06320744  .w.3...T.U...2.D
 0f50 080e09f9 0a030b33 0cce0d54 0e550f0d  .......3...T.U..

 0f60 10321144 120e1300 1400154c 16001700  .2.D.......L....
 0f70 184c0000 00000000 00000000 00000000  .L..............
*/

//i = 0
// rd(i) != (i+1) ?
//	wr(i, i+1)
// repeat until 32
// 
// wr(0xa, 0x1b)

	//memcpy(state->base->reg, tuner_init, 25);
/*
	state->ref_freq = 16000;


	if (clkdiv <= 3)
		state->reg[0x00] |= (clkdiv & 0x03);
	if (agcmode <= 3) {
		state->reg[0x03] |= (agcmode << 5);
		if (agcmode == 0x01)
			state->reg[0x01] |= 0x30;
	}
	if (bbmode <= 3)
		state->reg[0x01] = (state->reg[0x01] & ~0x30) | (bbmode << 4);
	if (agcref <= 7)
		state->reg[0x03] |= agcref;
	if (agcset <= 31)
		state->reg[0x02] = (state->reg[0x02] & ~0x1F) | agcset | 0x40;
*/
}

static int probe(struct stv *state)
{
	struct dvb_frontend *fe = state->fe;
	int ret = 0;

	u8 d[26];

	init_regs(state);
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);

	memcpy(&d[1], tuner_init, 25);
	d[0] = 0;
	ret = i2c_write(state->base->i2c, state->base->adr, d, 25 + 1);


	if (ret < 0)
		goto err;
//	pr_info("attach_init OK\n");
//	dump_regs(state);

err:
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
	return ret;
}

static int sleep(struct dvb_frontend *fe)
{
	/* struct tda_state *state = fe->tuner_priv; */

	//pr_info("tuner sleep\n");
	return 0;
}

static int init(struct dvb_frontend *fe)
{
	/* struct tda_state *state = fe->tuner_priv; */
	//pr_info("init\n");
	return 0;
}

static void release(struct dvb_frontend *fe)
{
	struct stv *state = fe->tuner_priv;

	state->base->count--;
	if (state->base->count == 0) {
		//pr_info("remove STV tuner base\n");
		list_del(&state->base->stvlist);
		kfree(state->base);
	}
	kfree(state);
	fe->tuner_priv = NULL;
	return;
}
#if 0
static int set_bandwidth(struct dvb_frontend *fe, u32 CutOffFrequency)
{
	struct stv *state = fe->tuner_priv;
	u32 index = (CutOffFrequency + 999999) / 1000000;

	if (index < 6)
		index = 6;
	if (index > 50)
		index = 50;
	if ((state->base->reg[0x08] & ~0xFC) == ((index-6) << 2))
		return 0;

	state->base->reg[0x08] = (state->base->reg[0x08] & ~0xFC) | ((index-6) << 2);
	state->base->reg[0x09] = (state->base->reg[0x09] & ~0x0C) | 0x08;
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);
	write_regs(state, 0x08, 2);
	wait_for_call_done(state, 0x08);
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
	return 0;
}

#endif



static int set_lof(struct stv *state, u32 LocalFrequency, u32 CutOffFrequency)
{
	int cf_index = (CutOffFrequency / 1000000) - 5;
	u32 Frequency = (LocalFrequency + 500) / 1000; // Hz -> kHz
	u32 p = 1, psel = 0, fvco, div, frac;
	u8 Icp, tmp;

	u32 freq = Frequency;
	u8 PDIV, P;

	//pr_info("F = %u, CutOff = %u\n", Frequency, CutOffFrequency);


	/* set PDIV and CF */
	if (cf_index < 0)
		cf_index = 0;
	if (cf_index > 31)
		cf_index = 31;

	if (Frequency >= 1191000) {
		PDIV = 0;
		P    = 2;
	} else if (Frequency >= 596000) {
		PDIV = 1;
		P    = 4;
	} else if (Frequency >= 299000) {
		PDIV = 2;
		P    = 8;
	} else {
		PDIV = 3;
		P    = 16;
	}


	fvco = Frequency * P;
	div = (fvco * state->cfg->Rdiv) / state->cfg->xtal;

	/* charge pump current */
	Icp = 0;
	if (fvco < 2472000)
		Icp = 0;
	else if (fvco < 2700000)
		Icp = 1;
	else if (fvco < 3021000)
		Icp = 2;
	else if (fvco < 3387000)
		Icp = 3;
	else if (fvco < 3845000)
		Icp = 5;
	else if (fvco < 4394000)
		Icp = 6;
	else
		Icp = 7;





	frac = (fvco * state->cfg->Rdiv) % state->cfg->xtal;
	frac = MulDiv32(frac, 0x40000, state->cfg->xtal);


	state->reg[REG_N0]    = div & 0xff;
	state->reg[REG_N1_F0] = (((div >> 8) & 0x01) | ((frac & 0x7f) << 1)) & 0xff;
	state->reg[REG_F1]    = (frac >> 7) & 0xff;
	state->reg[REG_F2_ICP] &= 0x88;
	state->reg[REG_F2_ICP] |= (Icp << 4) | ((frac >> 15) & 0x07);
	state->reg[REG_CF_PDIV] &= 0x9f;
	state->reg[REG_CF_PDIV] |= ((PDIV << 5) | cf_index);

	/* Start cal vco,CF */
	state->reg[REG_CAL] &= 0xf8;
	state->reg[REG_CAL] |= 0x06;

	write_tuner_regs(state);

	wait_for_call_done(state, 0x06);

	usleep_range(10000, 12000);

#if 0
	read_reg(state, 0x03, &tmp);
	if (tmp & 0x10)	{
		state->base->reg[0x02] &= ~0x80;   /* LNA NF Mode */
		write_regs(state, 2, 1);
	}
	read_reg(state, 0x08, &tmp);
#endif

//	dump_regs(state);
	return 0;
}

static int set_params(struct dvb_frontend *fe)
{
	struct stv *state = fe->tuner_priv;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	int status;
	u32 freq, symb, cutoff;
	u32 rolloff;

	if (p->delivery_system != SYS_DVBS && p->delivery_system != SYS_DVBS2)
		return -EINVAL;

	switch (p->rolloff) {
	case ROLLOFF_20:
		rolloff = 120;
		break;
	case ROLLOFF_25:
		rolloff = 125;
		break;
	default:
		rolloff = 135;
		break;
	}

	freq = p->frequency * 1000;
	symb = p->symbol_rate;
	cutoff = 5000000 + MulDiv32(p->symbol_rate, rolloff, 200);

	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 1);
	set_lof(state, freq, cutoff);
	if (fe->ops.i2c_gate_ctrl)
		fe->ops.i2c_gate_ctrl(fe, 0);
	return status;
}

static int get_frequency(struct dvb_frontend *fe, u32 *frequency)
{
	*frequency = 0;
	return 0;
}

static u32 AGC_Gain[] = {
	000, /* 0.0 */
	000, /* 0.1 */
	1000, /* 0.2 */
	2000, /* 0.3 */
	3000, /* 0.4 */
	4000, /* 0.5 */
	5000, /* 0.6 */
	6000, /* 0.7 */
	7000, /* 0.8 */
	14000, /* 0.9 */
	20000, /* 1.0 */
	27000, /* 1.1 */
	32000, /* 1.2 */
	37000, /* 1.3 */
	42000, /* 1.4 */
	47000, /* 1.5 */
	50000, /* 1.6 */
	53000, /* 1.7 */
	56000, /* 1.8 */
	58000, /* 1.9 */
	60000, /* 2.0 */
	62000, /* 2.1 */
	63000, /* 2.2 */
	64000, /* 2.3 */
	64500, /* 2.4 */
	65000, /* 2.5 */
	65500, /* 2.6 */
	66000, /* 2.7 */
	66500, /* 2.8 */
	67000, /* 2.9 */
};

static int get_rf_strength(struct dvb_frontend *fe, u16 *st)
{
	*st = 0;
#if 0
	struct stv *state = fe->tuner_priv;
	s32 Gain;
	u32 Index = RFAgc / 100;
	if (Index >= (sizeof(AGC_Gain) / sizeof(AGC_Gain[0]) - 1))
		Gain = AGC_Gain[sizeof(AGC_Gain) / sizeof(AGC_Gain[0]) - 1];
	else
		Gain = AGC_Gain[Index] +
			((AGC_Gain[Index+1] - AGC_Gain[Index]) *
			 (RFAgc % 100)) / 100;
	*st = Gain;
#endif
	return 0;
}

static int get_if(struct dvb_frontend *fe, u32 *frequency)
{
	*frequency = 0;
	return 0;
}

static int get_bandwidth(struct dvb_frontend *fe, u32 *bandwidth)
{
	return 0;
}

static struct dvb_tuner_ops tuner_ops = {
	.info = {
		.name = "STV6120",
		.frequency_min  =  950000,
		.frequency_max  = 2150000,
		.frequency_step =       0
	},
	.init              = init,
	.sleep             = sleep,
	.set_params        = set_params,
	.release           = release,
//	.get_frequency     = get_frequency,
//	.get_if_frequency  = get_if,
//	.get_bandwidth     = get_bandwidth,
//	.get_rf_strength   = get_rf_strength,
//	.set_bandwidth     = set_bandwidth,
};

static struct stv_base *match_base(struct i2c_adapter  *i2c, u8 adr)
{
	struct stv_base *p;

	list_for_each_entry(p, &stvlist, stvlist)
		if (p->i2c == i2c && p->adr == adr)
			return p;
	return NULL;
}


struct dvb_frontend *stv6120_attach(struct dvb_frontend *fe,
		    struct i2c_adapter *i2c, struct stv6120_cfg *cfg)
{
	struct stv *state;
	struct stv_base *base;
	int stat = 0;

	state = kzalloc(sizeof(struct stv), GFP_KERNEL);
	if (!state)
		return NULL;
	memcpy(&fe->ops.tuner_ops, &tuner_ops, sizeof(struct dvb_tuner_ops));
	state->fe = fe;
	memcpy(state->reg, &tuner_init[2], 7);

	base = match_base(i2c, cfg->adr);
	if (base) {
		base->count++;
		state->base = base;
	} else {
		base = kzalloc(sizeof(struct stv_base), GFP_KERNEL);
		if (!base)
			goto fail;
		base->i2c = i2c;
		base->adr = cfg->adr;
		base->count = 1;
		mutex_init(&base->i2c_lock);
		mutex_init(&base->reg_lock);
		state->base = base;
		if (probe(state) < 0) {
			kfree(base);
			goto fail;
		}
		list_add(&base->stvlist, &stvlist);
	}


	if (cfg->xtal == 0) {
		printk("xtal=0!!!\n");
		goto fail;
	}

	state->cfg = cfg;
	state->nr = 1-(base->count-1);

	fe->tuner_priv = state;
	return fe;
fail:
	kfree(state);
	return NULL;
}
EXPORT_SYMBOL_GPL(stv6120_attach);

MODULE_DESCRIPTION("STV6120 driver");
MODULE_AUTHOR("Luis Alves");
MODULE_LICENSE("GPL");

