/*
    TBS ECP3 FPGA based cards PCIe driver

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TBSECP3_H_
#define _TBSECP3_H_

#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/dma-mapping.h>
#include <linux/slab.h>

#include "demux.h"
#include "dmxdev.h"
#include "dvb_demux.h"
#include "dvb_frontend.h"
#include "dvb_net.h"
#include "dvbdev.h"
#include "tbsecp3-regs.h"
#include "dvb_ca_en50221.h"


#define TBSECP3_VID		0x544d
#define TBSECP3_PID		0x6178

#define TBSECP3_BOARD_TBS6205 	0x6205
#define TBSECP3_BOARD_TBS6903	0x6903
#define TBSECP3_BOARD_TBS6904	0x6904
#define TBSECP3_BOARD_TBS6909	0x6909

#define TBSECP3_MAX_ADAPTERS	(8)
#define TBSECP3_MAX_I2C_BUS	(4)

#define TBSECP3_GPIODEF_NONE	(0)
#define TBSECP3_GPIODEF_HIGH	(1)
#define TBSECP3_GPIODEF_LOW	(2)

#define TBSECP3_DMA_BUFFERS	8


struct tbsecp3_dev;

struct tbsecp3_gpio_config {
	u8 voltage_onoff_pin;
	u8 voltage_onoff_lvl;

	u8 voltage_1318_pin;
	u8 voltage_1318_lvl;

	u8 demod_reset_pin;
	u8 demod_reset_lvl;
};

struct tbsecp3_adap_config {
        u32 ts_in;
	u8 i2c_bus_nr;
	struct tbsecp3_gpio_config gpio;
};

struct tbsecp3_board {
	char *name;
	int adapters;
	u32 i2c_speed;
	u8 eeprom_i2c;
	struct tbsecp3_adap_config adap_config[8];
};

struct tbsecp3_i2c {
	struct tbsecp3_dev *dev;
	u32 base;

	struct i2c_adapter i2c_adap;
	struct i2c_client i2c_client;

	struct mutex lock;
	wait_queue_head_t wq;
	bool done;
};

struct tbsecp3_dma_channel {
	u32 base;
	dma_addr_t dma_addr;
	u8 *buf[TBSECP3_DMA_BUFFERS + 1];
	u8 offset;
	u8 cnt;
};

struct tbsecp3_ca {
	int nr;
	u32 base;
	struct dvb_ca_en50221 ca;

	struct tbsecp3_adapter *adapter;
	struct mutex lock;
	int status;
};

struct tbsecp3_adapter {
	int nr;
	struct tbsecp3_adap_config *cfg;

	/* parent device */
	struct tbsecp3_dev *dev;

	/* i2c */
	struct tbsecp3_i2c *i2c;
	struct i2c_client *i2c_client_demod;
	struct i2c_client *i2c_client_tuner;

	/* dvb */
	struct dvb_adapter dvb_adapter;
	struct dvb_frontend *fe;
	struct dvb_demux demux;
	struct dmxdev dmxdev;
	struct dvb_net dvbnet;
	struct dmx_frontend fe_hw;
	struct dmx_frontend fe_mem;
	int feeds;

	/* dma */
	spinlock_t adap_lock;
	struct tasklet_struct tasklet;
	struct tbsecp3_dma_channel dma;

	/* ca interface */
	struct tbsecp3_ca *tbsca;
};

struct tbsecp3_dev {
	struct tbsecp3_board *info;

	/* pcie */
	struct pci_dev *pci_dev;
	void __iomem *lmmio;
	bool msi;

	/* dvb adapters */
	struct tbsecp3_adapter adapter[TBSECP3_MAX_ADAPTERS];

	/* i2c */
	struct tbsecp3_i2c i2c_bus[TBSECP3_MAX_I2C_BUS];
};

#define tbs_read(_b, _o)	readl(dev->lmmio + (_b + _o))
#define tbs_write(_b, _o, _v)	writel((_v), dev->lmmio + (_b + _o))


/* tbsecp3-core.c */
void tbsecp3_gpio_set_pin(struct tbsecp3_dev *dev, int pin, int state);

/* tbspcie-i2c.c */
extern int tbsecp3_i2c_init(struct tbsecp3_dev *dev);
extern void tbsecp3_i2c_exit(struct tbsecp3_dev *dev);
extern void tbsecp3_i2c_reg_init(struct tbsecp3_dev *dev);
extern void tbsecp3_i2c_remove_clients(struct tbsecp3_adapter *adapter);

/* tbspcie-cards.c */
extern struct tbsecp3_board tbsecp3_boards[];

/* tbspcie-dvb.c */
extern int tbsecp3_dvb_init(struct tbsecp3_adapter *adapter);
extern void tbsecp3_dvb_exit(struct tbsecp3_adapter *adapter);

/* tbsecp3-dma.c */
extern int tbsecp3_dma_init(struct tbsecp3_dev *dev);
extern void tbsecp3_dma_free(struct tbsecp3_dev *dev);
extern void tbsecp3_dma_reg_init(struct tbsecp3_dev *dev);
extern void tbsecp3_dma_enable(struct tbsecp3_adapter *adap);
extern void tbsecp3_dma_disable(struct tbsecp3_adapter *adap);

#endif
