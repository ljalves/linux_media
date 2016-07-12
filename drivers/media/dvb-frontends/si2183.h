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

#define SI2183_MP_A		2
#define SI2183_MP_B		3
#define SI2183_MP_C		4
#define SI2183_MP_D		5

#define SI2183_TS_PARALLEL	0x06
#define SI2183_TS_SERIAL	0x03

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

	/* Tuner AGC config */
	u8 ter_agc_pin;
	u8 sat_agc_pin;
	bool ter_agc_inv;
	bool sat_agc_inv;

	/* DVB-T2 FEF flag pin */
	u8 fef_pin;

	/* TS mode */
	u8 ts_mode;

	/* TS clock inverted */
	bool ts_clock_inv;

	/* TS clock gapped */
	bool ts_clock_gapped;
};

#endif
