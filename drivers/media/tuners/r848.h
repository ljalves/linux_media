/*
 * Rafael R848 silicon tuner driver
 *
 * Copyright (C) 2015 Luis Alves <ljalvs@gmail.com>
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
 */

#ifndef R848_H
#define R848_H

#include <linux/kconfig.h>
#include "dvb_frontend.h"








struct r848_config {
	/* tuner i2c address */
	u8 i2c_address;

	u32 xtal;

	// tuner
	u8 R848_DetectTfType ;

	u8 R848_Xtal_Pwr ;
	u8 R848_Xtal_Pwr_tmp ;

	/* dvbc/t */
	u8 R848_SetTfType;

};

#if IS_ENABLED(CONFIG_MEDIA_TUNER_R848)
extern struct dvb_frontend *r848_attach(struct dvb_frontend *fe,
		struct r848_config *cfg, struct i2c_adapter *i2c);
#else
static inline struct dvb_frontend *r848_attach(struct dvb_frontend *fe,
		struct r848_config *cfg, struct i2c_adapter *i2c)
{
	printk(KERN_WARNING "%s: driver disabled by Kconfig\n", __func__);
	return NULL;
}
#endif

#endif /* R848_H */
