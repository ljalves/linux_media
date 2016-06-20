/*
 * Driver for the Maxlinear MX58x family of tuners/demods
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2, as published by the Free Software Foundation.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
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
#include <linux/mutex.h>
#include <linux/vmalloc.h>
#include <asm/div64.h>
#include <asm/unaligned.h>

#include "dvb_frontend.h"
#include "mxl5xx.h"
#include "mxl5xx_regs.h"
#include "mxl5xx_defs.h"


#define BYTE0(v) ((v >>  0) & 0xff)
#define BYTE1(v) ((v >>  8) & 0xff)
#define BYTE2(v) ((v >> 16) & 0xff)
#define BYTE3(v) ((v >> 24) & 0xff)

#define MXL5XX_DEFAULT_FIRMWARE "dvb-fe-mxl5xx.fw"

static int mode = 0;
module_param(mode, int, 0444);
MODULE_PARM_DESC(mode,
		"Multi-switch mode: 0=quattro/quad");

LIST_HEAD(mxllist);

struct mxl_base {
	struct list_head     mxllist;

	u8                   adr;
	struct i2c_adapter  *i2c;
	u32                  count;
	u32                  type;
	u32                  chipversion;
	u32                  clock;

	struct mutex         i2c_lock;
	struct mutex         status_lock;
	u8                   buf[MXL_HYDRA_OEM_MAX_CMD_BUFF_LEN];

	u32                  cmd_size;
	u8                   cmd_data[MAX_CMD_DATA];

	struct mxl5xx_cfg   *cfg;
};

struct mxl {
	struct mxl_base     *base;
	struct dvb_frontend  fe;
	u32                  demod;
	u32                  rf_in;
};

static void convert_endian(u8 flag, u32 size, u8 *d)
{
	u32 i;

	if (!flag)
		return;
	for (i = 0; i < (size & ~3); i += 4) {
		d[i + 0] ^= d[i + 3];
		d[i + 3] ^= d[i + 0];
		d[i + 0] ^= d[i + 3];

		d[i + 1] ^= d[i + 2];
		d[i + 2] ^= d[i + 1];
		d[i + 1] ^= d[i + 2];
	}
}

static int i2c_write(struct i2c_adapter *adap, u8 adr,
			    u8 *data, u32 len)
{
	struct i2c_msg msg = {.addr = adr, .flags = 0,
			      .buf = data, .len = len};

	return (i2c_transfer(adap, &msg, 1) == 1) ? 0 : -1;
}

static int i2c_read(struct i2c_adapter *adap, u8 adr,
			   u8 *data, u32 len)
{
	struct i2c_msg msg = {.addr = adr, .flags = I2C_M_RD,
			      .buf = data, .len = len};

	return (i2c_transfer(adap, &msg, 1) == 1) ? 0 : -1;
}

static int i2cread(struct mxl *state, u8 *data, int len)
{
	return i2c_read(state->base->i2c, state->base->adr, data, len);
}

static int i2cwrite(struct mxl *state, u8 *data, int len)
{
	return i2c_write(state->base->i2c, state->base->adr, data, len);
}

static int send_command(struct mxl *state, u32 size, u8 *buf)
{
	int stat;

	mutex_lock(&state->base->i2c_lock);
	stat = i2cwrite(state, buf, size);
	mutex_unlock(&state->base->i2c_lock);
	return stat;
}

static int write_register(struct mxl *state, u32 reg, u32 val)
{
	int stat;
	u8 data[MXL_HYDRA_REG_WRITE_LEN] = {
		MXL_HYDRA_PLID_REG_WRITE, 0x08,
		BYTE0(reg), BYTE1(reg), BYTE2(reg), BYTE3(reg),
		BYTE0(val), BYTE1(val), BYTE2(val), BYTE3(val),
	};
	mutex_lock(&state->base->i2c_lock);
	stat = i2cwrite(state, data, sizeof(data));
	mutex_unlock(&state->base->i2c_lock);
	if (stat)
		pr_err("i2c write error\n");
	return stat;
}

static int write_register_block(struct mxl *state, u32 reg, u32 size, u8 *data)
{
	int stat;
	u8 *buf = state->base->buf;

	mutex_lock(&state->base->i2c_lock);

	buf[0] = MXL_HYDRA_PLID_REG_WRITE;
	buf[1] = size + 4;
	buf[2] = GET_BYTE(reg, 0);
	buf[3] = GET_BYTE(reg, 1);
	buf[4] = GET_BYTE(reg, 2);
	buf[5] = GET_BYTE(reg, 3);
	memcpy(&buf[6], data, size);

	convert_endian(MXL_ENABLE_BIG_ENDIAN, size, &buf[6]);
	stat = i2cwrite(state, buf,
			MXL_HYDRA_I2C_HDR_SIZE +
			MXL_HYDRA_REG_SIZE_IN_BYTES + size);
	mutex_unlock(&state->base->i2c_lock);
	return stat;
}

static int write_firmware_block(struct mxl *state,
				u32 reg, u32 size, u8 *regDataPtr)
{
	int stat;
	u8 *buf = state->base->buf;

	mutex_lock(&state->base->i2c_lock);
	buf[0] = MXL_HYDRA_PLID_REG_WRITE;
	buf[1] = size + 4;
	buf[2] = GET_BYTE(reg, 0);
	buf[3] = GET_BYTE(reg, 1);
	buf[4] = GET_BYTE(reg, 2);
	buf[5] = GET_BYTE(reg, 3);
	memcpy(&buf[6], regDataPtr, size);
	stat = i2cwrite(state, buf,
			MXL_HYDRA_I2C_HDR_SIZE +
			MXL_HYDRA_REG_SIZE_IN_BYTES + size);
	mutex_unlock(&state->base->i2c_lock);
	if (stat)
		pr_err("fw block write failed\n");
	return stat;
}

static int read_register(struct mxl *state, u32 reg, u32 *val)
{
	int stat;
	u8 data[MXL_HYDRA_REG_SIZE_IN_BYTES + MXL_HYDRA_I2C_HDR_SIZE] = {
		MXL_HYDRA_PLID_REG_READ, 0x04,
		GET_BYTE(reg, 0), GET_BYTE(reg, 1),
		GET_BYTE(reg, 2), GET_BYTE(reg, 3),
	};

	mutex_lock(&state->base->i2c_lock);
	stat = i2cwrite(state, data,
			MXL_HYDRA_REG_SIZE_IN_BYTES + MXL_HYDRA_I2C_HDR_SIZE);
	if (stat)
		pr_err("i2c read error 1\n");
	if (!stat)
		stat = i2cread(state, (u8 *) val, MXL_HYDRA_REG_SIZE_IN_BYTES);
	mutex_unlock(&state->base->i2c_lock);
	le32_to_cpus(val);
	if (stat)
		pr_err("i2c read error 2\n");
	return stat;
}

static int read_register_block(struct mxl *state, u32 reg, u32 size, u8 *data)
{
	int stat;
	u8 *buf = state->base->buf;

	mutex_lock(&state->base->i2c_lock);

	buf[0] = MXL_HYDRA_PLID_REG_READ;
	buf[1] = size + 4;
	buf[2] = GET_BYTE(reg, 0);
	buf[3] = GET_BYTE(reg, 1);
	buf[4] = GET_BYTE(reg, 2);
	buf[5] = GET_BYTE(reg, 3);
	stat = i2cwrite(state, buf,
			MXL_HYDRA_I2C_HDR_SIZE + MXL_HYDRA_REG_SIZE_IN_BYTES);
	if (!stat) {
		stat = i2cread(state, data, size);
		convert_endian(MXL_ENABLE_BIG_ENDIAN, size, data);
	}
	mutex_unlock(&state->base->i2c_lock);
	return stat;
}

static int read_by_mnemonic(struct mxl *state,
			    u32 reg, u8 lsbloc, u8 numofbits, u32 *val)
{
	u32 data = 0, mask = 0;
	int stat;

	stat = read_register(state, reg, &data);
	if (stat)
		return stat;
	mask = MXL_GET_REG_MASK_32(lsbloc, numofbits);
	data &= mask;
	data >>= lsbloc;
	*val = data;
	return 0;
}


static int update_by_mnemonic(struct mxl *state,
			      u32 reg, u8 lsbloc, u8 numofbits, u32 val)
{
	u32 data, mask;
	int stat;

	stat = read_register(state, reg, &data);
	if (stat)
		return stat;
	mask = MXL_GET_REG_MASK_32(lsbloc, numofbits);
	data = (data & ~mask) | ((val << lsbloc) & mask);
	stat = write_register(state, reg, data);
	return stat;
}

static void extract_from_mnemonic(u32 regAddr, u8 lsbPos, u8 width,
				  u32 *toAddr, u8 *toLsbPos, u8 *toWidth)
{
	if (toAddr)
		*toAddr = regAddr;
	if (toLsbPos)
		*toLsbPos = lsbPos;
	if (toWidth)
		*toWidth = width;
}

static int firmware_is_alive(struct mxl *state)
{
	u32 hb0, hb1;

	if (read_register(state, HYDRA_HEAR_BEAT, &hb0))
		return 0;
	msleep(20);
	if (read_register(state, HYDRA_HEAR_BEAT, &hb1))
		return 0;
	if (hb1 == hb0)
		return 0;
	return 1;
}

static int init(struct dvb_frontend *fe)
{
	return 0;
}

static void release(struct dvb_frontend *fe)
{
	struct mxl *state = fe->demodulator_priv;

	state->base->count--;
	if (state->base->count == 0) {
		list_del(&state->base->mxllist);
		kfree(state->base);
	}
	kfree(state);
}

static int get_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int CfgDemodAbortTune(struct mxl *state)
{
	MXL_HYDRA_DEMOD_ABORT_TUNE_T abortTuneCmd;
	u8 cmdSize = sizeof(abortTuneCmd);
	u8 cmdBuff[MXL_HYDRA_OEM_MAX_CMD_BUFF_LEN];
	
	abortTuneCmd.demodId = state->demod;
	BUILD_HYDRA_CMD(MXL_HYDRA_ABORT_TUNE_CMD, MXL_CMD_WRITE, cmdSize, &abortTuneCmd, cmdBuff);
	return send_command(state, cmdSize + MXL_HYDRA_CMD_HEADER_SIZE, &cmdBuff[0]);
}

static int send_master_cmd(struct dvb_frontend *fe,
			   struct dvb_diseqc_master_cmd *cmd)
{
	struct mxl *state = fe->demodulator_priv;

	return CfgDemodAbortTune(state);
}

static int set_parameters(struct dvb_frontend *fe)
{
	struct mxl *state = fe->demodulator_priv;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;
	int ret;

	MXL_HYDRA_DEMOD_PARAM_T demodChanCfg;
	u8 cmdSize = sizeof(demodChanCfg);
	u8 cmdBuff[MXL_HYDRA_OEM_MAX_CMD_BUFF_LEN];
	//MXL_HYDRA_DEMOD_ID_E demodId = state->demod;
#if 0
	MXL_REG_FIELD_T xpt_enable_dss_input[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_INP_MODE_DSS0}, {XPT_INP_MODE_DSS1},
		{XPT_INP_MODE_DSS2}, {XPT_INP_MODE_DSS3},
		{XPT_INP_MODE_DSS4}, {XPT_INP_MODE_DSS5},
		{XPT_INP_MODE_DSS6}, {XPT_INP_MODE_DSS7} };
#endif
	
	if (p->frequency < 950000 || p->frequency > 2150000)
		return -EINVAL;
	if (p->symbol_rate < 1000000 || p->symbol_rate > 45000000)
		return -EINVAL;

	//CfgDemodAbortTune(state);
	
	switch (p->delivery_system) {
	case SYS_DSS:
		demodChanCfg.standard = MXL_HYDRA_DSS;
		break;
	case SYS_DVBS:
		demodChanCfg.standard = MXL_HYDRA_DVBS;
		demodChanCfg.rollOff = MXL_HYDRA_ROLLOFF_AUTO;
		demodChanCfg.modulationScheme = MXL_HYDRA_MOD_QPSK;
		break;
	case SYS_DVBS2:
		demodChanCfg.standard = MXL_HYDRA_DVBS2;
		demodChanCfg.rollOff = MXL_HYDRA_ROLLOFF_AUTO;
		demodChanCfg.modulationScheme = MXL_HYDRA_MOD_AUTO;
		demodChanCfg.pilots = MXL_HYDRA_PILOTS_AUTO;
		break;
	default:
		return -EINVAL;
	}

	demodChanCfg.tunerIndex = state->rf_in;
	demodChanCfg.demodIndex = state->demod;
	demodChanCfg.frequencyInHz = p->frequency * 1000;
	demodChanCfg.symbolRateInHz = p->symbol_rate;
	demodChanCfg.maxCarrierOffsetInMHz = 10;
	demodChanCfg.spectrumInversion = MXL_HYDRA_SPECTRUM_AUTO;
	demodChanCfg.fecCodeRate = MXL_HYDRA_FEC_AUTO;

	//printk("std %u freq %u\n", demodChanCfg.standard, demodChanCfg.symbolRateInHz);

#if 0
	if (p->delivery_system == SYS_DSS)
		update_by_mnemonic(state,
				   xpt_enable_dss_input[demodId].regAddr,
				   xpt_enable_dss_input[demodId].lsbPos,
				   xpt_enable_dss_input[demodId].numOfBits,
				   MXL_TRUE);
	else
		update_by_mnemonic(state,
				   xpt_enable_dss_input[demodId].regAddr,
				   xpt_enable_dss_input[demodId].lsbPos,
				   xpt_enable_dss_input[demodId].numOfBits,
				   MXL_FALSE);
#endif
	
	BUILD_HYDRA_CMD(MXL_HYDRA_DEMOD_SET_PARAM_CMD, MXL_CMD_WRITE,
			cmdSize, &demodChanCfg, cmdBuff);

	mutex_lock(&state->base->status_lock);
 	ret = send_command(state, cmdSize + MXL_HYDRA_CMD_HEADER_SIZE, &cmdBuff[0]);
	mutex_unlock(&state->base->status_lock);

	return ret;
}

static int read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct mxl *state = fe->demodulator_priv;

	int stat;
	u32 regData = 0;

	mutex_lock(&state->base->status_lock);
	HYDRA_DEMOD_STATUS_LOCK(state, state->demod);
	stat = read_register(state, (HYDRA_DMD_LOCK_STATUS_ADDR_OFFSET +
				     HYDRA_DMD_STATUS_OFFSET(state->demod)),
			     &regData);
	HYDRA_DEMOD_STATUS_UNLOCK(state, state->demod);
	mutex_unlock(&state->base->status_lock);

	*status = (regData == 1) ? 0x1f : 0;

	return 0;
}

static int tune(struct dvb_frontend *fe, bool re_tune,
		unsigned int mode_flags,
		unsigned int *delay, enum fe_status *status)
{
	//struct mxl *state = fe->demodulator_priv;
	int r = 0;

	*delay = HZ / 2;
	if (re_tune) {
		r = set_parameters(fe);
		if (r)
			return r;
	}
	if (*status & FE_HAS_LOCK)
		return 0;

	r = read_status(fe, status);
	if (r)
		return r;

	return 0;
}

static int sleep(struct dvb_frontend *fe)
{
	return 0;
}

static int read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct mxl *state = fe->demodulator_priv;
	int stat;
	u32 regData = 0;

	mutex_lock(&state->base->status_lock);
	HYDRA_DEMOD_STATUS_LOCK(state, state->demod);
	stat = read_register(state, (HYDRA_DMD_SNR_ADDR_OFFSET +
				     HYDRA_DMD_STATUS_OFFSET(state->demod)),
			     &regData);
	HYDRA_DEMOD_STATUS_UNLOCK(state, state->demod);
	mutex_unlock(&state->base->status_lock);
	*snr = (s16) (regData & 0xFFFF)*220;
	//printk("snr dmd%d=%d\n", state->demod, *snr);
	return stat;
}

static int read_ber(struct dvb_frontend *fe, u32 *ber)
{
	*ber = 0;

	return 0;
}

static int read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct mxl *state = fe->demodulator_priv;
	int stat;
	u32 regData = 0;

	mutex_lock(&state->base->status_lock);
	HYDRA_DEMOD_STATUS_LOCK(state, state->demod);
	stat = read_register(state, (HYDRA_DMD_STATUS_INPUT_POWER_ADDR +
				     HYDRA_DMD_STATUS_OFFSET(state->demod)),
			     &regData);
	HYDRA_DEMOD_STATUS_UNLOCK(state, state->demod);
	mutex_unlock(&state->base->status_lock);
	*strength = (u16) (regData & 0xFFFF);
	return stat;
}

static int read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	return 0;
}

static int get_frontend(struct dvb_frontend *fe, struct dtv_frontend_properties *p)
{
	//struct mxl *state = fe->demodulator_priv;
	//struct dtv_frontend_properties *p = &fe->dtv_property_cache;

	switch (p->delivery_system) {
	case SYS_DSS:
		break;
	case SYS_DVBS:
		break;
	case SYS_DVBS2:
		break;
	default:
		return -EINVAL;
	}
	return 0;
}

static int set_input_tone(struct dvb_frontend *fe, enum fe_sec_tone_mode tone, int rf_in)
{
	struct mxl *state = fe->demodulator_priv;
	u8 buf[14] = {
		MXL_HYDRA_PLID_CMD_WRITE,
		12, 8,
		MXL_HYDRA_DISEQC_CONT_TONE_CFG, 0, 0,
		rf_in, 0, 0, 0,
		(tone == SEC_TONE_ON) ? 1 : 0, 0, 0, 0
	};
	return send_command(state, sizeof(buf), buf);
}



static int set_voltage(struct dvb_frontend *fe, enum fe_sec_voltage voltage)
{
	struct mxl *state = fe->demodulator_priv;

	switch (mode) {
	case 0:
	default:
		if (voltage == SEC_VOLTAGE_18)
			state->rf_in &= ~2;
		else
			state->rf_in |= 2;
		break;
	}

	return 0; //state->base->cfg->set_voltage(fe, voltage, 0);
}


static int set_tone(struct dvb_frontend *fe, enum fe_sec_tone_mode tone)
{
	struct mxl *state = fe->demodulator_priv;

	switch (mode) {
	case 0:
	default:
		if (tone == SEC_TONE_ON)
			state->rf_in &= ~1;
		else
			state->rf_in |= 1;
		break;
	}

	return 0;
}

static struct dvb_frontend_ops mxl_ops = {
	.delsys = { SYS_DVBS, SYS_DVBS2, SYS_DSS },
	.info = {
		.name			= "MXL5XX",
		.frequency_min		= 950000,
		.frequency_max		= 2150000,
		.frequency_stepsize	= 0,
		.frequency_tolerance	= 0,
		.symbol_rate_min	= 1000000,
		.symbol_rate_max	= 70000000,
		.caps			= FE_CAN_INVERSION_AUTO |
					  FE_CAN_FEC_AUTO       |
					  FE_CAN_QPSK           |
					  FE_CAN_2G_MODULATION
	},
	.init				= init,
	.release                        = release,
	.get_frontend_algo              = get_algo,
	.tune                           = tune,
	.read_status			= read_status,
	.sleep				= sleep,
	.read_snr			= read_snr,
	.read_ber			= read_ber,
	.read_signal_strength		= read_signal_strength,
	.read_ucblocks			= read_ucblocks,
	.get_frontend                   = get_frontend,
	.diseqc_send_master_cmd		= send_master_cmd,

	.set_tone			= set_tone,
	.set_voltage			= set_voltage,
};

static struct mxl_base *match_base(struct i2c_adapter *i2c, u8 adr)
{
	struct mxl_base *p;

	list_for_each_entry(p, &mxllist, mxllist)
		if (p->i2c == i2c && p->adr == adr)
			return p;
	return NULL;
}

static void cfg_dev_xtal(struct mxl *state, u32 freq, u32 cap, u32 enable)
{
	SET_REG_FIELD_DATA(AFE_REG_D2A_XTAL_EN_CLKOUT_1P8, enable);
	if (freq == 24000000)
		write_register(state, HYDRA_CRYSTAL_SETTING, 0);
	else
		write_register(state, HYDRA_CRYSTAL_SETTING, 1);

	write_register(state, HYDRA_CRYSTAL_CAP, cap);
}

static u32 get_big_endian(u8 numOfBits, const u8 buf[])
{
	u32 retValue = 0;

	switch (numOfBits) {
	case 24:
		retValue = (((u32) buf[0]) << 16) |
			(((u32) buf[1]) << 8) | buf[2];
		break;
	case 32:
		retValue = (((u32) buf[0]) << 24) |
			(((u32) buf[1]) << 16) |
			(((u32) buf[2]) << 8) | buf[3];
		break;
	default:
		break;
	}

	return retValue;
}

static void flip_data_in_dword(u32 size, u8 *d)
{
	u32 i;
	u8 t;

	for (i = 0; i < size; i += 4) {
		t = d[i + 3]; d[i + 3] = d[i]; d[i] = t;
		t = d[i + 2]; d[i + 2] = d[i + 1]; d[i + 1] = t;
	}
}

static int write_fw_segment(struct mxl *state,
			    u32 MemAddr, u32 totalSize, u8 *dataPtr)
{
	int status;
	u32 dataCount = 0;
	u32 size = 0;
	u32 origSize = 0;
	u8 *wBufPtr = NULL;
	u32 blockSize = MXL_HYDRA_OEM_MAX_BLOCK_WRITE_LENGTH;
	u8 wMsgBuffer[MXL_HYDRA_OEM_MAX_BLOCK_WRITE_LENGTH];

	//pr_info("seg %u\n", totalSize);
	do {
		size = origSize = (((u32)(dataCount + blockSize)) > totalSize) ?
			(totalSize - dataCount) : blockSize;
		wBufPtr = dataPtr;

		if (origSize & 3) {
			size = (origSize + 4) & ~3;
			wBufPtr = &wMsgBuffer[0];
			memset((void *)wBufPtr + origSize,
			       0x00, size - origSize);
			memcpy((void *)wBufPtr, (void *)dataPtr, origSize);
		}
		flip_data_in_dword(size, wBufPtr);
		status = write_firmware_block(state, MemAddr, size, wBufPtr);
		if (status)
			return status;
		dataCount += size;
		MemAddr   += size;
		dataPtr   += size;
	} while (dataCount < totalSize);

	return status;
}

static int do_firmware_download(struct mxl *state,
				u32 mbinBufferSize,
				u8 *mbinBufferPtr)
{
	int status;
	u32 index = 0;
	u32 segLength = 0;
	u32 segAddress = 0;
	MBIN_FILE_T *mbinPtr  = (MBIN_FILE_T *)mbinBufferPtr;
	MBIN_SEGMENT_T *segmentPtr;

	if (mbinPtr->header.id != MBIN_FILE_HEADER_ID) {
		pr_err("%s: Invalid file header ID (%c)\n",
		       __func__, mbinPtr->header.id);
		return -EINVAL;
	}
	status = write_register(state, FW_DL_SIGN_ADDR, 0);
	if (status)
		return status;
	segmentPtr = (MBIN_SEGMENT_T *) (&mbinPtr->data[0]);
	for (index = 0; index < mbinPtr->header.numSegments; index++) {
		if (segmentPtr->header.id != MBIN_SEGMENT_HEADER_ID) {
			pr_err("%s: Invalid segment header ID (%c)\n",
			       __func__, segmentPtr->header.id);
			return -EINVAL;
		}
		segLength  = get_big_endian(24, &(segmentPtr->header.len24[0]));
		segAddress = get_big_endian(32, &(segmentPtr->header.address[0]));
		status = -1;
		if (((segAddress & 0x90760000) != 0x90760000) &&
		    ((segAddress & 0x90740000) != 0x90740000))
			status = write_fw_segment(state, segAddress,
						  segLength, (u8 *) segmentPtr->data);
		if (status)
			return status;
		segmentPtr = (MBIN_SEGMENT_T *)
			&(segmentPtr->data[((segLength + 3) / 4) * 4]);
	}
	return status;
}

static int firmware_download(struct mxl *state, u32 mbinBufferSize,
			     u8 *mbinBufferPtr)
{
	int status;
	u32 regData = 0;
	MXL_HYDRA_SKU_COMMAND_T devSkuCfg;
	u8 cmdSize = sizeof(MXL_HYDRA_SKU_COMMAND_T);
	u8 cmdBuff[sizeof(MXL_HYDRA_SKU_COMMAND_T) + 6];

	/* put CPU into reset */
	status = SET_REG_FIELD_DATA(PRCM_PRCM_CPU_SOFT_RST_N, 0);
	if (status)
		return status;
	usleep_range(1000, 2000);

	/* Reset TX FIFO's, BBAND, XBAR */
	status = write_register(state, HYDRA_RESET_TRANSPORT_FIFO_REG,
				HYDRA_RESET_TRANSPORT_FIFO_DATA);
	if (status)
		return status;
	status = write_register(state, HYDRA_RESET_BBAND_REG,
				HYDRA_RESET_BBAND_DATA);
	if (status)
		return status;
	status = write_register(state, HYDRA_RESET_XBAR_REG,
				HYDRA_RESET_XBAR_DATA);
	if (status)
		return status;

	// lja
	/* Disable clock to Baseband, Wideband, SerDes, Alias ext & Transport modules */
	status = write_register(state, HYDRA_MODULES_CLK_2_REG, HYDRA_DISABLE_CLK_2);
	if (status)
		return status;
	//write_register(state, HYDRA_MODULES_CLK_2_REG, 0x0000000b);


	/* Clear Software & Host interrupt status - (Clear on read) */
	status = read_register(state, HYDRA_PRCM_ROOT_CLK_REG, &regData);
	if (status)
		return status;

	status = do_firmware_download(state, mbinBufferSize, mbinBufferPtr);
	if (status)
		return status;
	
	if (state->base->type == MXL_HYDRA_DEVICE_568) {
		msleep(10);
		
		// bring XCPU out of reset
		status = write_register(state, 0x90720000, 1);
		if (status)
			return status;
		msleep(500);
		
		// Enable XCPU UART message processing in MCPU
		status = write_register(state, 0x9076B510, 1);
		if (status)
			return status;
	} else {
		/* Bring CPU out of reset */
		status = SET_REG_FIELD_DATA(PRCM_PRCM_CPU_SOFT_RST_N, 1);
		if (status)
			return status;
		/* Wait until FW boots */
		msleep(150);
	}

	// lja
	// Initilize XPT XBAR
	status = write_register(state, XPT_DMD0_BASEADDR, 0x76543210);
	if (status)
		return status;
	
	if (!firmware_is_alive(state))
		return -1;

	pr_info("Hydra FW alive\n");

	/* sometimes register values are wrong shortly after first heart beats */
	msleep(50);

	devSkuCfg.skuType = state->base->type;
	BUILD_HYDRA_CMD(MXL_HYDRA_DEV_CFG_SKU_CMD, MXL_CMD_WRITE,
			cmdSize, &devSkuCfg, cmdBuff);
	status = send_command(state, cmdSize + MXL_HYDRA_CMD_HEADER_SIZE, &cmdBuff[0]);
	if (status)
		return status;

	status = GET_REG_FIELD_DATA(PAD_MUX_BOND_OPTION, &regData);
	if (status)
		return status;
	pr_info("chipID=%08x\n", regData);

	status = GET_REG_FIELD_DATA(PRCM_AFE_CHIP_MMSK_VER, &regData);
	if (status)
		return status;
	pr_info("chipVer=%08x\n", regData);

	status = read_register(state, HYDRA_FIRMWARE_VERSION, &regData);
	if (status)
		return status;
	pr_info("FWVer=%08x\n", regData);

	return status;
}

static int cfg_ts_pad_mux(struct mxl *state, MXL_BOOL_E enableSerialTS)
{
	int status = 0;
	u32 padMuxValue = 0;

	if (enableSerialTS == MXL_TRUE)
		padMuxValue = 0;
	else
		padMuxValue = 3;

	switch (state->base->type) {
	case MXL_HYDRA_DEVICE_561:
	case MXL_HYDRA_DEVICE_581:
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_14_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_15_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_16_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_17_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_18_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_19_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_20_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_21_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_22_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_23_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_24_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_25_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_26_PINMUX_SEL, padMuxValue);
		break;

	case MXL_HYDRA_DEVICE_584:
	default:
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_09_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_10_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_11_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_12_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_13_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_14_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_15_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_16_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_17_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_18_PINMUX_SEL, padMuxValue);
		status |= SET_REG_FIELD_DATA(PAD_MUX_DIGIO_19_PINMUX_SEL, padMuxValue);
		break;
	}
	return status;
}

static int config_ts(struct mxl *state, MXL_HYDRA_DEMOD_ID_E demodId, MXL_HYDRA_MPEGOUT_PARAM_T *mpegOutParamPtr)
{
	int status = 0;
	u32 ncoCountMin = 0;
	u32 clkType = 0;

	MXL_REG_FIELD_T xpt_sync_polarity[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_SYNC_POLARITY0}, {XPT_SYNC_POLARITY1},
		{XPT_SYNC_POLARITY2}, {XPT_SYNC_POLARITY3},
		{XPT_SYNC_POLARITY4}, {XPT_SYNC_POLARITY5},
		{XPT_SYNC_POLARITY6}, {XPT_SYNC_POLARITY7} };
	MXL_REG_FIELD_T xpt_clock_polarity[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_CLOCK_POLARITY0}, {XPT_CLOCK_POLARITY1},
		{XPT_CLOCK_POLARITY2}, {XPT_CLOCK_POLARITY3},
		{XPT_CLOCK_POLARITY4}, {XPT_CLOCK_POLARITY5},
		{XPT_CLOCK_POLARITY6}, {XPT_CLOCK_POLARITY7} };
	MXL_REG_FIELD_T xpt_valid_polarity[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_VALID_POLARITY0}, {XPT_VALID_POLARITY1},
		{XPT_VALID_POLARITY2}, {XPT_VALID_POLARITY3},
		{XPT_VALID_POLARITY4}, {XPT_VALID_POLARITY5},
		{XPT_VALID_POLARITY6}, {XPT_VALID_POLARITY7} };
	MXL_REG_FIELD_T xpt_ts_clock_phase[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_TS_CLK_PHASE0}, {XPT_TS_CLK_PHASE1},
		{XPT_TS_CLK_PHASE2}, {XPT_TS_CLK_PHASE3},
		{XPT_TS_CLK_PHASE4}, {XPT_TS_CLK_PHASE5},
		{XPT_TS_CLK_PHASE6}, {XPT_TS_CLK_PHASE7} };
	MXL_REG_FIELD_T xpt_lsb_first[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_LSB_FIRST0}, {XPT_LSB_FIRST1}, {XPT_LSB_FIRST2}, {XPT_LSB_FIRST3},
		{XPT_LSB_FIRST4}, {XPT_LSB_FIRST5}, {XPT_LSB_FIRST6}, {XPT_LSB_FIRST7} };
	MXL_REG_FIELD_T xpt_sync_byte[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_SYNC_FULL_BYTE0}, {XPT_SYNC_FULL_BYTE1},
		{XPT_SYNC_FULL_BYTE2}, {XPT_SYNC_FULL_BYTE3},
		{XPT_SYNC_FULL_BYTE4}, {XPT_SYNC_FULL_BYTE5},
		{XPT_SYNC_FULL_BYTE6}, {XPT_SYNC_FULL_BYTE7} };
	MXL_REG_FIELD_T xpt_enable_output[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_ENABLE_OUTPUT0}, {XPT_ENABLE_OUTPUT1},
		{XPT_ENABLE_OUTPUT2}, {XPT_ENABLE_OUTPUT3},
		{XPT_ENABLE_OUTPUT4}, {XPT_ENABLE_OUTPUT5},
		{XPT_ENABLE_OUTPUT6}, {XPT_ENABLE_OUTPUT7} };
	MXL_REG_FIELD_T xpt_enable_dvb_input[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_ENABLE_INPUT0}, {XPT_ENABLE_INPUT1},
		{XPT_ENABLE_INPUT2}, {XPT_ENABLE_INPUT3},
		{XPT_ENABLE_INPUT4}, {XPT_ENABLE_INPUT5},
		{XPT_ENABLE_INPUT6}, {XPT_ENABLE_INPUT7} };
	MXL_REG_FIELD_T xpt_err_replace_sync[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_ERROR_REPLACE_SYNC0}, {XPT_ERROR_REPLACE_SYNC1},
		{XPT_ERROR_REPLACE_SYNC2}, {XPT_ERROR_REPLACE_SYNC3},
		{XPT_ERROR_REPLACE_SYNC4}, {XPT_ERROR_REPLACE_SYNC5},
		{XPT_ERROR_REPLACE_SYNC6}, {XPT_ERROR_REPLACE_SYNC7} };
	MXL_REG_FIELD_T xpt_err_replace_valid[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_ERROR_REPLACE_VALID0}, {XPT_ERROR_REPLACE_VALID1},
		{XPT_ERROR_REPLACE_VALID2}, {XPT_ERROR_REPLACE_VALID3},
		{XPT_ERROR_REPLACE_VALID4}, {XPT_ERROR_REPLACE_VALID5},
		{XPT_ERROR_REPLACE_VALID6}, {XPT_ERROR_REPLACE_VALID7} };
	MXL_REG_FIELD_T xpt_continuous_clock[MXL_HYDRA_DEMOD_MAX] = {
		{XPT_TS_CLK_OUT_EN0}, {XPT_TS_CLK_OUT_EN1},
		{XPT_TS_CLK_OUT_EN2}, {XPT_TS_CLK_OUT_EN3},
		{XPT_TS_CLK_OUT_EN4}, {XPT_TS_CLK_OUT_EN5},
		{XPT_TS_CLK_OUT_EN6}, {XPT_TS_CLK_OUT_EN7} };
	MXL_REG_FIELD_T mxl561_xpt_ts_sync[MXL_HYDRA_DEMOD_ID_6] = {
		{PAD_MUX_DIGIO_25_PINMUX_SEL}, {PAD_MUX_DIGIO_20_PINMUX_SEL},
		{PAD_MUX_DIGIO_17_PINMUX_SEL}, {PAD_MUX_DIGIO_11_PINMUX_SEL},
		{PAD_MUX_DIGIO_08_PINMUX_SEL}, {PAD_MUX_DIGIO_03_PINMUX_SEL} };
	MXL_REG_FIELD_T mxl561_xpt_ts_valid[MXL_HYDRA_DEMOD_ID_6] = {
		{PAD_MUX_DIGIO_26_PINMUX_SEL}, {PAD_MUX_DIGIO_19_PINMUX_SEL},
		{PAD_MUX_DIGIO_18_PINMUX_SEL}, {PAD_MUX_DIGIO_10_PINMUX_SEL},
		{PAD_MUX_DIGIO_09_PINMUX_SEL}, {PAD_MUX_DIGIO_02_PINMUX_SEL} };

	if (MXL_ENABLE == mpegOutParamPtr->enable) {
		cfg_ts_pad_mux(state, MXL_TRUE);
		SET_REG_FIELD_DATA(XPT_ENABLE_PARALLEL_OUTPUT, MXL_FALSE);
	}
	ncoCountMin = (u32)(MXL_HYDRA_NCO_CLK/mpegOutParamPtr->maxMpegClkRate);
	SET_REG_FIELD_DATA(XPT_NCO_COUNT_MIN, ncoCountMin);

	if (mpegOutParamPtr->mpegClkType == MXL_HYDRA_MPEG_CLK_CONTINUOUS)
		clkType = 1;

	if (mpegOutParamPtr->mpegMode < MXL_HYDRA_MPEG_MODE_PARALLEL) {
		status  |= update_by_mnemonic(state,
					      xpt_continuous_clock[demodId].regAddr,
					      xpt_continuous_clock[demodId].lsbPos,
					      xpt_continuous_clock[demodId].numOfBits,
					      clkType);
	} else
		SET_REG_FIELD_DATA(XPT_TS_CLK_OUT_EN_PARALLEL, clkType);

	status |= update_by_mnemonic(state,
				     xpt_sync_polarity[demodId].regAddr,
				     xpt_sync_polarity[demodId].lsbPos,
				     xpt_sync_polarity[demodId].numOfBits,
				     mpegOutParamPtr->mpegSyncPol);

	status |= update_by_mnemonic(state,
				     xpt_valid_polarity[demodId].regAddr,
				     xpt_valid_polarity[demodId].lsbPos,
				     xpt_valid_polarity[demodId].numOfBits,
				     mpegOutParamPtr->mpegValidPol);

	status |= update_by_mnemonic(state,
				     xpt_clock_polarity[demodId].regAddr,
				     xpt_clock_polarity[demodId].lsbPos,
				     xpt_clock_polarity[demodId].numOfBits,
				     mpegOutParamPtr->mpegClkPol);

	status |= update_by_mnemonic(state,
				     xpt_sync_byte[demodId].regAddr,
				     xpt_sync_byte[demodId].lsbPos,
				     xpt_sync_byte[demodId].numOfBits,
				     mpegOutParamPtr->mpegSyncPulseWidth);

	status |= update_by_mnemonic(state,
				     xpt_ts_clock_phase[demodId].regAddr,
				     xpt_ts_clock_phase[demodId].lsbPos,
				     xpt_ts_clock_phase[demodId].numOfBits,
				     mpegOutParamPtr->mpegClkPhase);

	status |= update_by_mnemonic(state,
				     xpt_lsb_first[demodId].regAddr,
				     xpt_lsb_first[demodId].lsbPos,
				     xpt_lsb_first[demodId].numOfBits,
				     mpegOutParamPtr->lsbOrMsbFirst);

	switch (mpegOutParamPtr->mpegErrorIndication) {
	case MXL_HYDRA_MPEG_ERR_REPLACE_SYNC:
		status |= update_by_mnemonic(state,
					     xpt_err_replace_sync[demodId].regAddr,
					     xpt_err_replace_sync[demodId].lsbPos,
					     xpt_err_replace_sync[demodId].numOfBits,
					     MXL_TRUE);

		status |= update_by_mnemonic(state,
					     xpt_err_replace_valid[demodId].regAddr,
					     xpt_err_replace_valid[demodId].lsbPos,
					     xpt_err_replace_valid[demodId].numOfBits,
					     MXL_FALSE);
		break;

	case MXL_HYDRA_MPEG_ERR_REPLACE_VALID:
		status |= update_by_mnemonic(state,
					     xpt_err_replace_sync[demodId].regAddr,
					     xpt_err_replace_sync[demodId].lsbPos,
					     xpt_err_replace_sync[demodId].numOfBits,
					     MXL_FALSE);

		status |= update_by_mnemonic(state,
					     xpt_err_replace_valid[demodId].regAddr,
					     xpt_err_replace_valid[demodId].lsbPos,
					     xpt_err_replace_valid[demodId].numOfBits,
					     MXL_TRUE);
		break;

	case MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED:
	default:
		status |= update_by_mnemonic(state,
					     xpt_err_replace_sync[demodId].regAddr,
					     xpt_err_replace_sync[demodId].lsbPos,
					     xpt_err_replace_sync[demodId].numOfBits,
					     MXL_FALSE);

		status |= update_by_mnemonic(state,
					     xpt_err_replace_valid[demodId].regAddr,
					     xpt_err_replace_valid[demodId].lsbPos,
					     xpt_err_replace_valid[demodId].numOfBits,
					     MXL_FALSE);

		break;

	}

	if (mpegOutParamPtr->mpegMode != MXL_HYDRA_MPEG_MODE_PARALLEL) {
		status |= update_by_mnemonic(state,
					     xpt_enable_output[demodId].regAddr,
					     xpt_enable_output[demodId].lsbPos,
					     xpt_enable_output[demodId].numOfBits,
					     mpegOutParamPtr->enable);

		status |=
			update_by_mnemonic(state,
					   xpt_enable_dvb_input[demodId].regAddr,
					   xpt_enable_dvb_input[demodId].lsbPos,
					   xpt_enable_dvb_input[demodId].numOfBits,
					   mpegOutParamPtr->enable);

	}
	return status;
}

static int config_mux(struct mxl *state)
{
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT0, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT1, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT2, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT3, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT4, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT5, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT6, 0);
	SET_REG_FIELD_DATA(XPT_ENABLE_OUTPUT7, 0);
	SET_REG_FIELD_DATA(XPT_STREAM_MUXMODE0, 1);
	SET_REG_FIELD_DATA(XPT_STREAM_MUXMODE1, 1);
	return 0;
}

static int config_dis(struct mxl *state, u32 id)
{
	MXL_HYDRA_DISEQC_ID_E diseqcId = id;
	MXL_HYDRA_DISEQC_OPMODE_E opMode = MXL_HYDRA_DISEQC_TONE_MODE; //lja MXL_HYDRA_DISEQC_ENVELOPE_MODE;
	MXL_HYDRA_DISEQC_VER_E version = MXL_HYDRA_DISEQC_1_X;
	MXL_HYDRA_DISEQC_CARRIER_FREQ_E carrierFreqInHz =
		MXL_HYDRA_DISEQC_CARRIER_FREQ_22KHZ;
	MXL58x_DSQ_OP_MODE_T diseqcMsg;
	u8 cmdSize = sizeof(diseqcMsg);
	u8 cmdBuff[MXL_HYDRA_OEM_MAX_CMD_BUFF_LEN];

	diseqcMsg.diseqcId = diseqcId;
	diseqcMsg.opMode = opMode;
	diseqcMsg.version = version;
	diseqcMsg.centerFreq = carrierFreqInHz;

	BUILD_HYDRA_CMD(MXL_HYDRA_DISEQC_CFG_MSG_CMD,
			MXL_CMD_WRITE, cmdSize, &diseqcMsg, cmdBuff);
	return send_command(state, cmdSize + MXL_HYDRA_CMD_HEADER_SIZE, &cmdBuff[0]);
}

static int load_fw(struct mxl *state)
{
	struct mxl5xx_cfg *cfg = state->base->cfg;
	int stat = 0;
	u8 *buf;

#if 1
	const struct firmware *fw;

	pr_info("loading firmware, please wait...\n");

	stat = request_firmware(&fw, MXL5XX_DEFAULT_FIRMWARE,
		state->base->i2c->dev.parent);
	if (stat)
		return stat;

	stat = firmware_download(state, fw->size, fw->data);
	
	release_firmware(fw);

	if (stat)
		pr_info("error loading firmware\n");

#else
	if (cfg->fw)
		return firmware_download(state, cfg->fw_len, cfg->fw);
	
#ifdef FW_INCLUDED
	return firmware_download(state, sizeof(hydra_fw), hydra_fw);
#endif
	
	if (!cfg->fw_read)
		return -1;

	buf = vmalloc(0x40000);
	if (!buf)
		return -ENOMEM;
	
	cfg->fw_read(cfg->fw_priv, buf, 0x40000);
	stat = firmware_download(state, 0x40000, buf);
	vfree(buf);
#endif
	return stat;
}

static int init_multisw(struct mxl *state)
{
	struct dvb_frontend *fe = &state->fe;
	struct i2c_adapter *i2c = state->base->i2c;
	struct mxl5xx_cfg *cfg = state->base->cfg;

	switch (mode) {
	case 0:
	default:
		cfg->set_voltage(i2c, SEC_VOLTAGE_13, 3);
		cfg->set_voltage(i2c, SEC_VOLTAGE_13, 2);
		cfg->set_voltage(i2c, SEC_VOLTAGE_18, 1);
		cfg->set_voltage(i2c, SEC_VOLTAGE_18, 0);
		set_input_tone(fe, SEC_TONE_OFF, 3);
		set_input_tone(fe, SEC_TONE_ON, 2);
		set_input_tone(fe, SEC_TONE_OFF, 1);
		set_input_tone(fe, SEC_TONE_ON, 0);
		break;
	}

	return 0;
}

static int probe(struct mxl *state)
{
	struct mxl5xx_cfg *cfg = state->base->cfg;
	u32 chipver;
	int fw, status, j;
	MXL_HYDRA_MPEGOUT_PARAM_T mpegInterfaceCfg;

	fw = firmware_is_alive(state);

	if (!fw)  {
		// lja
		SET_REG_FIELD_DATA(PRCM_AFE_REG_CLOCK_ENABLE, 1);
		//write_register(state, HYDRA_MODULES_CLK_1_REG, 0x0000020b);

		SET_REG_FIELD_DATA(PRCM_PRCM_AFE_REG_SOFT_RST_N, 1);
		status = GET_REG_FIELD_DATA(PRCM_CHIP_VERSION, &chipver);
		if (status)
			state->base->chipversion = 0;
		else
			state->base->chipversion = (chipver == 2) ? 2 : 1;
		pr_info("Hydra chip version %u\n", state->base->chipversion);



	cfg_dev_xtal(state, cfg->clk, cfg->cap, 0);

	status = load_fw(state);
	if (status)
		return status;

	config_dis(state, 0);
	config_dis(state, 1);
	config_dis(state, 2);
	config_dis(state, 3);

#if 0
	config_mux(state);

	mpegInterfaceCfg.enable = MXL_ENABLE;
	mpegInterfaceCfg.lsbOrMsbFirst = MXL_HYDRA_MPEG_SERIAL_MSB_1ST;
	/*  supports only (0-104&139)MHz */
	mpegInterfaceCfg.maxMpegClkRate = 139;
	mpegInterfaceCfg.mpegClkPhase = MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_180_DEG; //MXL_HYDRA_MPEG_CLK_PHASE_SHIFT_0_DEG;
	mpegInterfaceCfg.mpegClkPol = MXL_HYDRA_MPEG_CLK_IN_PHASE;
	/* MXL_HYDRA_MPEG_CLK_GAPPED; */
	mpegInterfaceCfg.mpegClkType = MXL_HYDRA_MPEG_CLK_CONTINUOUS;
	mpegInterfaceCfg.mpegErrorIndication =
		MXL_HYDRA_MPEG_ERR_INDICATION_DISABLED;
	mpegInterfaceCfg.mpegMode = MXL_HYDRA_MPEG_MODE_SERIAL_3_WIRE;
	mpegInterfaceCfg.mpegSyncPol  = MXL_HYDRA_MPEG_ACTIVE_HIGH;
	mpegInterfaceCfg.mpegSyncPulseWidth  = MXL_HYDRA_MPEG_SYNC_WIDTH_BIT;
	mpegInterfaceCfg.mpegValidPol  = MXL_HYDRA_MPEG_ACTIVE_HIGH;

	for (j = 0; j < 8; j++) {
		status = config_ts(state, (MXL_HYDRA_DEMOD_ID_E) j,
				   &mpegInterfaceCfg);
		if (status)
			return status;
	}
#endif

	// lja
	write_register(state, 0x90700008, 0x00000005);
	write_register(state, 0x90000170, 0);
	write_register(state, 0x90000174, 0);
	write_register(state, 0x90700044, 0x00030000);
	write_register(state, 0x90700238, 0x03030303);
	write_register(state, 0x9070023C, 0x00030303);
	write_register(state, 0x907001D4, 0x000000ff);
	write_register(state, 0x90700010, 0x0000ff00);
	write_register(state, 0x90700014, 0x000000ff);
	write_register(state, 0x90700018, 0x22222222);
	write_register(state, 0x9070000C, 0x000000ff);
	write_register(state, 0x90700000, 0x000000ff);

	}
	

	return 0;
}

struct dvb_frontend *mxl5xx_attach(struct i2c_adapter *i2c,
				   struct mxl5xx_cfg *cfg,
				   u32 demod)
{
	struct mxl *state;
	struct mxl_base *base;

	state = kzalloc(sizeof(struct mxl), GFP_KERNEL);
	if (!state)
		return NULL;

	state->demod = demod;
	state->rf_in = 0;
	state->fe.ops = mxl_ops;
	state->fe.demodulator_priv = state;

	base = match_base(i2c, cfg->adr);
	if (base) {
		base->count++;
		state->base = base;
	} else {
		base = kzalloc(sizeof(struct mxl_base), GFP_KERNEL);
		if (!base)
			goto fail;
		base->i2c = i2c;
		base->cfg = cfg;
		base->adr = cfg->adr;
		base->type = cfg->type;
		base->count = 1;
		mutex_init(&base->i2c_lock);
		mutex_init(&base->status_lock);
		state->base = base;
		if (probe(state) < 0) {
			kfree(base);
			goto fail;
		}

		init_multisw(state);

		list_add(&base->mxllist, &mxllist);
	}


	return &state->fe;

fail:
	kfree(state);
	return NULL;
}
EXPORT_SYMBOL_GPL(mxl5xx_attach);

MODULE_DESCRIPTION("MXL5XX driver");
MODULE_AUTHOR("Ralph Metzler");
MODULE_LICENSE("GPL");
