#ifndef __SAA716x_PRIV_H
#define __SAA716x_PRIV_H

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mutex.h>

#include <linux/pci.h>
#include <linux/ioport.h>
#include <linux/i2c.h>
#include "saa716x_i2c.h"
#include "saa716x_boot.h"
#include "saa716x_cgu.h"
#include "saa716x_dma.h"
#include "saa716x_fgpi.h"
#include "saa716x_spi.h"
#include "saa716x_vip.h"

#include "dvbdev.h"
#include "dvb_demux.h"
#include "dmxdev.h"
#include "dvb_frontend.h"
#include "dvb_net.h"

#define SAA716x_ERROR		0
#define SAA716x_NOTICE		1
#define SAA716x_INFO		2
#define SAA716x_DEBUG		3

#define SAA716x_DEV		(saa716x)->num
#define SAA716x_VERBOSE		(saa716x)->verbose
#define SAA716x_MAX_ADAPTERS	4

#define dprintk(__x, __y, __fmt, __arg...) do {								\
	if (__y) {											\
		if	((SAA716x_VERBOSE > SAA716x_ERROR) && (SAA716x_VERBOSE > __x))			\
			printk(KERN_ERR "%s (%d): " __fmt "\n" , __func__ , SAA716x_DEV , ##__arg);	\
		else if	((SAA716x_VERBOSE > SAA716x_NOTICE) && (SAA716x_VERBOSE > __x))			\
			printk(KERN_NOTICE "%s (%d): " __fmt "\n" , __func__ , SAA716x_DEV , ##__arg);	\
		else if ((SAA716x_VERBOSE > SAA716x_INFO) && (SAA716x_VERBOSE > __x))			\
			printk(KERN_INFO "%s (%d): " __fmt "\n" , __func__ , SAA716x_DEV , ##__arg);	\
		else if ((SAA716x_VERBOSE > SAA716x_DEBUG) && (SAA716x_VERBOSE > __x))			\
			printk(KERN_DEBUG "%s (%d): " __fmt "\n" , __func__ , SAA716x_DEV , ##__arg);	\
	} else {											\
		if (SAA716x_VERBOSE > __x)								\
			printk(__fmt , ##__arg);							\
	}												\
} while(0)


#define NXP_SEMICONDUCTOR	0x1131
#define SAA7160			0x7160
#define SAA7161			0x7161
#define SAA7162			0x7162

#define NXP_REFERENCE_BOARD	0x1131

#define MAKE_ENTRY(__subven, __subdev, __chip, __configptr) {		\
		.vendor		= NXP_SEMICONDUCTOR,			\
		.device		= (__chip),				\
		.subvendor	= (__subven),				\
		.subdevice	= (__subdev),				\
		.driver_data	= (unsigned long) (__configptr)		\
}

#define SAA716x_EPWR(__offst, __addr, __data)	writel((__data), (saa716x->mmio + (__offst + __addr)))
#define SAA716x_EPRD(__offst, __addr)		readl((saa716x->mmio + (__offst + __addr)))

#define SAA716x_RCWR(__offst, __addr, __data)	writel((__data), (saa716x->mmio + (__offst + __addr)))
#define SAA716x_RCRD(__offst, __addr)		readl((saa716x->mmio + (__offst + __addr)))


#define SAA716x_MSI_MAX_VECTORS			16

struct saa716x_msix_entry {
	int vector;
	u8 desc[32];
	irqreturn_t (*handler)(int irq, void *dev_id);
};

struct saa716x_dev;
struct saa716x_adapter;
struct saa716x_spi_config;

struct saa716x_adap_config {
	u32				ts_port;
	void				(*worker)(unsigned long);
};

struct saa716x_config {
	char				*model_name;
	char				*dev_type;

	enum saa716x_boot_mode		boot_mode;

	int				adapters;
	int				frontends;

	int (*frontend_attach)(struct saa716x_adapter *adapter, int count);
	irqreturn_t (*irq_handler)(int irq, void *dev_id);

	struct saa716x_adap_config	adap_config[SAA716x_MAX_ADAPTERS];
	enum saa716x_i2c_rate		i2c_rate;
	enum saa716x_i2c_mode		i2c_mode;
};

struct saa716x_adapter {
	struct dvb_adapter		dvb_adapter;
	struct dvb_frontend		*fe;
	struct dvb_demux		demux;
	struct dmxdev			dmxdev;
	struct dmx_frontend		fe_hw;
	struct dmx_frontend		fe_mem;
	struct dvb_net			dvb_net;

	struct saa716x_dev		*saa716x;

	u8				feeds;
	u8				count;
};

struct saa716x_dev {
	struct saa716x_config		*config;
	struct pci_dev			*pdev;

	int				num; /* device count */
	int				verbose;

	u8 				revision;

	/* PCI */
	void __iomem			*mmio;

#define MODE_INTA	0
#define MODE_MSI	1
#define MODE_MSI_X	2
	u8				int_type;

	struct msix_entry		msix_entries[SAA716x_MSI_MAX_VECTORS];
	struct saa716x_msix_entry	saa716x_msix_handler[56];
	u8				handlers; /* no. of active handlers */

	/* I2C */
	struct saa716x_i2c		i2c[2];
	u32				i2c_rate; /* init time */
	u32				I2C_DEV[2];

	struct saa716x_spi_state	*saa716x_spi;
	struct saa716x_spi_config	spi_config;

	struct saa716x_adapter		saa716x_adap[SAA716x_MAX_ADAPTERS];
	struct mutex			adap_lock;
	struct saa716x_cgu		cgu;

	spinlock_t			gpio_lock;
	/* DMA */

	struct saa716x_fgpi_stream_port	fgpi[4];
	struct saa716x_vip_stream_port	vip[2];

	u32				id_offst;
	u32				id_len;
	void				*priv;

	/* remote control */
	void				*ir_priv;
};

/* PCI */
extern int saa716x_pci_init(struct saa716x_dev *saa716x);
extern void saa716x_pci_exit(struct saa716x_dev *saa716x);

/* MSI */
extern int saa716x_msi_init(struct saa716x_dev *saa716x);
extern void saa716x_msi_exit(struct saa716x_dev *saa716x);
extern void saa716x_msiint_disable(struct saa716x_dev *saa716x);

/* DMA */
extern int saa716x_dma_init(struct saa716x_dev *saa716x);
extern void saa716x_dma_exit(struct saa716x_dev *saa716x);

/* AUDIO */
extern int saa716x_audio_init(struct saa716x_dev *saa716x);
extern void saa716x_audio_exit(struct saa716x_dev *saa716x);

/* Boot */
extern int saa716x_core_boot(struct saa716x_dev *saa716x);
extern int saa716x_jetpack_init(struct saa716x_dev *saa716x);

/* Remote control */
extern int saa716x_ir_init(struct saa716x_dev *saa716x);
extern void saa716x_ir_exit(struct saa716x_dev *saa716x);
extern void saa716x_ir_handler(struct saa716x_dev *saa716x, u32 ir_cmd);

#endif /* __SAA716x_PRIV_H */
