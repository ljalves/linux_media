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

#ifndef SI2183_H
#define SI2183_H

#include <linux/dvb/frontend.h>
/*
 * I2C address
 * 0x64
 */
struct si2183_config {
	/*
	 * frontend
	 * returned by driver
	 */
	struct dvb_frontend **fe;

	/*
	 * tuner I2C adapter
	 * returned by driver
	 */
	struct i2c_adapter **i2c_adapter;

	/* TS mode */
#define SI2183_TS_PARALLEL	0x06
#define SI2183_TS_SERIAL	0x03
	u8 ts_mode;

	/* TS clock inverted */
	bool ts_clock_inv;

	/* TS clock gapped */
	bool ts_clock_gapped;
};

#endif
