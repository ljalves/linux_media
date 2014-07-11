/*
 * Silicon Labs Si2168 DVB-T/T2/C demodulator driver
 *
 * Copyright (C) 2014 Antti Palosaari <crope@iki.fi>
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

#ifndef SI2168_PRIV_H
#define SI2168_PRIV_H

#include "si2168.h"
#include "dvb_frontend.h"
#include <linux/firmware.h>
#include <linux/i2c-mux.h>

#define SI2168_FIRMWARE "dvb-demod-si2168-02.fw"
#define SI2168A20_2_FIRMWARE "dvb-demod-si2168-03.fw"

/* state struct */
struct si2168 {
	struct i2c_client *client;
	struct i2c_adapter *adapter;
	struct mutex i2c_mutex;
	struct dvb_frontend fe;
	fe_delivery_system_t delivery_system;
	fe_status_t fe_status;
	bool active;

	u8 hw_rev;
	u8 fw_rev;
};

/* firmare command struct */
#define SI2157_ARGLEN      30
struct si2168_cmd {
	u8 args[SI2157_ARGLEN];
	unsigned wlen;
	unsigned rlen;
};

struct si2168_prop {
	u16	addr;
	u16	val;
};

struct si2168_prop si2168_demod_init[] = {
	{ 0x1003, 0x0017 },
	{ 0x1002, 0x0015 },
	{ 0x100c, 0x0012 },
	{ 0x1006, 0x0024 },
	{ 0x100b, 0x1388 },
	{ 0x1007, 0x2400 },
	{ 0x100a, 0x0028 },	/* sys | bandwidth */
	{ 0x1004, 0x0015 },
	{ 0x1005, 0x00a1 },
	{ 0x100f, 0x0010 },
	{ 0x100d, 0x02d0 },
	{ 0x1001, 0x0056 },	/* crope : 0x00 / 0x56 */
	{ 0x1009, 0x18e3 },
	{ 0x1008, 0x15d7 },
};

struct si2168_prop si2168_dvbc_init[] = {
	{0x1104, 0x0070},
	{0x1103, 0x0064},
	{0x1101, 0x0000},	/* fec_inner */
	{0x1102, 0x1af4},	/* symbol_rate/1000 */
};

struct si2168_prop si2168_scan_init[] = {
	{ 0x0304, 0x0000 },
	{ 0x0303, 0x0000 },
	{ 0x0308, 0x0000 },
	{ 0x0307, 0x0201 },
	{ 0x0306, 0x0000 },
	{ 0x0305, 0x0000 },
	{ 0x0301, 0x000c },	/* crope : 0x400c / 0x000c */

};

/* mcns - multimedia cable network system */
struct si2168_prop si2168_mcns_init[] = {
	{ 0x1604, 0x0070 },
	{ 0x1603, 0x0064 },
	{ 0x1601, 0x0000 },
	{ 0x1602, 0x1af4 },
};


#endif
