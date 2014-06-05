#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/pci.h>
#include <linux/mutex.h>

#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/device.h>

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include <linux/i2c.h>

#include "saa716x_mod.h"

#include "saa716x_gpio_reg.h"
#include "saa716x_greg_reg.h"
#include "saa716x_msi_reg.h"

#include "saa716x_adap.h"
#include "saa716x_i2c.h"
#include "saa716x_msi.h"
#include "saa716x_budget.h"
#include "saa716x_gpio.h"
#include "saa716x_rom.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

#include "mb86a16.h"
#include "stv6110x.h"
#include "stv090x.h"
#include "tas2101.h"
#include "av201x.h"
#include "cx24117.h"
#include "isl6422.h"
#include "stb6100.h"
#include "stb6100_cfg.h"
#include "tda18212.h"
#include "cxd2820r.h"

unsigned int verbose;
module_param(verbose, int, 0644);
MODULE_PARM_DESC(verbose, "verbose startup messages, default is 1 (yes)");

unsigned int int_type;
module_param(int_type, int, 0644);
MODULE_PARM_DESC(int_type, "force Interrupt Handler type: 0=INT-A, 1=MSI, 2=MSI-X. default INT-A mode");

#define DRIVER_NAME	"SAA716x Budget"

static int saa716x_budget_pci_probe(struct pci_dev *pdev, const struct pci_device_id *pci_id)
{
	struct saa716x_dev *saa716x;
	int err = 0;

	saa716x = kzalloc(sizeof (struct saa716x_dev), GFP_KERNEL);
	if (saa716x == NULL) {
		printk(KERN_ERR "saa716x_budget_pci_probe ERROR: out of memory\n");
		err = -ENOMEM;
		goto fail0;
	}

	saa716x->verbose	= verbose;
	saa716x->int_type	= int_type;
	saa716x->pdev		= pdev;
	saa716x->config		= (struct saa716x_config *) pci_id->driver_data;

	err = saa716x_pci_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x PCI Initialization failed");
		goto fail1;
	}

	err = saa716x_cgu_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x CGU Init failed");
		goto fail1;
	}

	err = saa716x_core_boot(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x Core Boot failed");
		goto fail2;
	}
	dprintk(SAA716x_DEBUG, 1, "SAA716x Core Boot Success");

	err = saa716x_msi_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x MSI Init failed");
		goto fail2;
	}

	err = saa716x_jetpack_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x Jetpack core initialization failed");
		goto fail1;
	}

	err = saa716x_i2c_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x I2C Initialization failed");
		goto fail3;
	}

	saa716x_gpio_init(saa716x);
#if 0
	err = saa716x_dump_eeprom(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x EEPROM dump failed");
	}

	err = saa716x_eeprom_data(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x EEPROM read failed");
	}

	/* set default port mapping */
	SAA716x_EPWR(GREG, GREG_VI_CTRL, 0x04080FA9);
	/* enable FGPI3 and FGPI1 for TS input from Port 2 and 6 */
	SAA716x_EPWR(GREG, GREG_FGPI_CTRL, 0x321);
#endif

	/* set default port mapping */
	SAA716x_EPWR(GREG, GREG_VI_CTRL, 0x2C688F0A);
	/* enable FGPI3, FGPI2, FGPI1 and FGPI0 for TS input from Port 2 and 6 */
	SAA716x_EPWR(GREG, GREG_FGPI_CTRL, 0x322);

	err = saa716x_dvb_init(saa716x);
	if (err) {
		dprintk(SAA716x_ERROR, 1, "SAA716x DVB initialization failed");
		goto fail4;
	}

	return 0;

fail4:
	saa716x_dvb_exit(saa716x);
fail3:
	saa716x_i2c_exit(saa716x);
fail2:
	saa716x_pci_exit(saa716x);
fail1:
	kfree(saa716x);
fail0:
	return err;
}

static void saa716x_budget_pci_remove(struct pci_dev *pdev)
{
	struct saa716x_dev *saa716x = pci_get_drvdata(pdev);

	saa716x_dvb_exit(saa716x);
	saa716x_i2c_exit(saa716x);
	saa716x_pci_exit(saa716x);
	kfree(saa716x);
}

static irqreturn_t saa716x_budget_pci_irq(int irq, void *dev_id)
{
	struct saa716x_dev *saa716x	= (struct saa716x_dev *) dev_id;

	u32 stat_h, stat_l, mask_h, mask_l;

	if (unlikely(saa716x == NULL)) {
		printk("%s: saa716x=NULL", __func__);
		return IRQ_NONE;
	}

	stat_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	stat_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);
	mask_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	mask_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);

	dprintk(SAA716x_DEBUG, 1, "MSI STAT L=<%02x> H=<%02x>, CTL L=<%02x> H=<%02x>",
		stat_l, stat_h, mask_l, mask_h);

	if (!((stat_l & mask_l) || (stat_h & mask_h)))
		return IRQ_NONE;

	if (stat_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, stat_l);

	if (stat_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, stat_h);

	saa716x_msi_event(saa716x, stat_l, stat_h);
#if 0
	dprintk(SAA716x_DEBUG, 1, "VI STAT 0=<%02x> 1=<%02x>, CTL 1=<%02x> 2=<%02x>",
		SAA716x_EPRD(VI0, INT_STATUS),
		SAA716x_EPRD(VI1, INT_STATUS),
		SAA716x_EPRD(VI0, INT_ENABLE),
		SAA716x_EPRD(VI1, INT_ENABLE));

	dprintk(SAA716x_DEBUG, 1, "FGPI STAT 0=<%02x> 1=<%02x>, CTL 1=<%02x> 2=<%02x>",
		SAA716x_EPRD(FGPI0, INT_STATUS),
		SAA716x_EPRD(FGPI1, INT_STATUS),
		SAA716x_EPRD(FGPI0, INT_ENABLE),
		SAA716x_EPRD(FGPI0, INT_ENABLE));

	dprintk(SAA716x_DEBUG, 1, "FGPI STAT 2=<%02x> 3=<%02x>, CTL 2=<%02x> 3=<%02x>",
		SAA716x_EPRD(FGPI2, INT_STATUS),
		SAA716x_EPRD(FGPI3, INT_STATUS),
		SAA716x_EPRD(FGPI2, INT_ENABLE),
		SAA716x_EPRD(FGPI3, INT_ENABLE));

	dprintk(SAA716x_DEBUG, 1, "AI STAT 0=<%02x> 1=<%02x>, CTL 0=<%02x> 1=<%02x>",
		SAA716x_EPRD(AI0, AI_STATUS),
		SAA716x_EPRD(AI1, AI_STATUS),
		SAA716x_EPRD(AI0, AI_CTL),
		SAA716x_EPRD(AI1, AI_CTL));

	dprintk(SAA716x_DEBUG, 1, "I2C STAT 0=<%02x> 1=<%02x>, CTL 0=<%02x> 1=<%02x>",
		SAA716x_EPRD(I2C_A, INT_STATUS),
		SAA716x_EPRD(I2C_B, INT_STATUS),
		SAA716x_EPRD(I2C_A, INT_ENABLE),
		SAA716x_EPRD(I2C_B, INT_ENABLE));

	dprintk(SAA716x_DEBUG, 1, "DCS STAT=<%02x>, CTL=<%02x>",
		SAA716x_EPRD(DCS, DCSC_INT_STATUS),
		SAA716x_EPRD(DCS, DCSC_INT_ENABLE));
#endif

	if (stat_l) {
		if (stat_l & MSI_INT_TAGACK_FGPI_0) {
			tasklet_schedule(&saa716x->fgpi[0].tasklet);
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_1) {
			tasklet_schedule(&saa716x->fgpi[1].tasklet);
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_2) {
			tasklet_schedule(&saa716x->fgpi[2].tasklet);
		}
		if (stat_l & MSI_INT_TAGACK_FGPI_3) {
			tasklet_schedule(&saa716x->fgpi[3].tasklet);
		}
	}

	return IRQ_HANDLED;
}

static void demux_worker(unsigned long data)
{
	struct saa716x_fgpi_stream_port *fgpi_entry = (struct saa716x_fgpi_stream_port *)data;
	struct saa716x_dev *saa716x = fgpi_entry->saa716x;
	struct dvb_demux *demux;
	u32 fgpi_index;
	u32 i;
	u32 write_index;

	fgpi_index = fgpi_entry->dma_channel - 6;
	demux = NULL;
	for (i = 0; i < saa716x->config->adapters; i++) {
		if (saa716x->config->adap_config[i].ts_port == fgpi_index) {
			demux = &saa716x->saa716x_adap[i].demux;
			break;
		}
	}
	if (demux == NULL) {
		printk(KERN_ERR "%s: unexpected channel %u\n",
		       __func__, fgpi_entry->dma_channel);
		return;
	}

	write_index = saa716x_fgpi_get_write_index(saa716x, fgpi_index);
	if (write_index < 0)
		return;

	dprintk(SAA716x_DEBUG, 1, "dma buffer = %d", write_index);

	if (write_index == fgpi_entry->read_index) {
		printk(KERN_DEBUG "%s: called but nothing to do\n", __func__);
		return;
	}

	do {
		u8 *data = (u8 *)fgpi_entry->dma_buf[fgpi_entry->read_index].mem_virt;

		pci_dma_sync_sg_for_cpu(saa716x->pdev,
			fgpi_entry->dma_buf[fgpi_entry->read_index].sg_list,
			fgpi_entry->dma_buf[fgpi_entry->read_index].list_len,
			PCI_DMA_FROMDEVICE);

		dvb_dmx_swfilter(demux, data, 348 * 188);

		fgpi_entry->read_index = (fgpi_entry->read_index + 1) & 7;
	} while (write_index != fgpi_entry->read_index);
}


#define SAA716x_MODEL_TWINHAN_VP3071	"Twinhan/Azurewave VP-3071"
#define SAA716x_DEV_TWINHAN_VP3071	"2x DVB-T"

static int saa716x_vp3071_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	dprintk(SAA716x_DEBUG, 1, "Adapter (%d) SAA716x frontend Init", count);
	dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Device ID=%02x", count, saa716x->pdev->subsystem_device);

	return -ENODEV;
}

static struct saa716x_config saa716x_vp3071_config = {
	.model_name		= SAA716x_MODEL_TWINHAN_VP3071,
	.dev_type		= SAA716x_DEV_TWINHAN_VP3071,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 2,
	.frontend_attach	= saa716x_vp3071_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
};


#define SAA716x_MODEL_TWINHAN_VP1028	"Twinhan/Azurewave VP-1028"
#define SAA716x_DEV_TWINHAN_VP1028	"DVB-S"

static int vp1028_dvbs_set_voltage(struct dvb_frontend *fe, fe_sec_voltage_t voltage)
{
	struct saa716x_dev *saa716x = fe->dvb->priv;

	switch (voltage) {
	case SEC_VOLTAGE_13:
		dprintk(SAA716x_ERROR, 1, "Polarization=[13V]");
		break;
	case SEC_VOLTAGE_18:
		dprintk(SAA716x_ERROR, 1, "Polarization=[18V]");
		break;
	case SEC_VOLTAGE_OFF:
		dprintk(SAA716x_ERROR, 1, "Frontend (dummy) POWERDOWN");
		break;
	default:
		dprintk(SAA716x_ERROR, 1, "Invalid = (%d)", (u32 ) voltage);
		return -EINVAL;
	}

	return 0;
}

struct mb86a16_config vp1028_mb86a16_config = {
	.demod_address	= 0x08,
	.set_voltage	= vp1028_dvbs_set_voltage,
};

static int saa716x_vp1028_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[1];

	if (count == 0) {

		mutex_lock(&saa716x->adap_lock);

		dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Power ON", count);
		saa716x_gpio_set_output(saa716x, 10);
		msleep(1);

		/* VP-1028 has inverted power supply control */
		saa716x_gpio_write(saa716x, 10, 1); /* set to standby */
		saa716x_gpio_write(saa716x, 10, 0); /* switch it on */
		msleep(100);

		dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Reset", count);
		saa716x_gpio_set_output(saa716x, 12);
		msleep(1);

		/* reset demodulator (Active LOW) */
		saa716x_gpio_write(saa716x, 12, 1);
		msleep(100);
		saa716x_gpio_write(saa716x, 12, 0);
		msleep(100);
		saa716x_gpio_write(saa716x, 12, 1);
		msleep(100);

		mutex_unlock(&saa716x->adap_lock);

		dprintk(SAA716x_ERROR, 1, "Probing for MB86A16 (DVB-S/DSS)");
		adapter->fe = mb86a16_attach(&vp1028_mb86a16_config, &i2c->i2c_adapter);
		if (adapter->fe) {
			dprintk(SAA716x_ERROR, 1, "found MB86A16 DVB-S/DSS frontend @0x%02x",
				vp1028_mb86a16_config.demod_address);

		} else {
			goto exit;
		}
		dprintk(SAA716x_ERROR, 1, "Done!");
	}

	return 0;
exit:
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_vp1028_config = {
	.model_name		= SAA716x_MODEL_TWINHAN_VP1028,
	.dev_type		= SAA716x_DEV_TWINHAN_VP1028,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 1,
	.frontend_attach	= saa716x_vp1028_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
};


#define SAA716x_MODEL_TWINHAN_VP6002	"Twinhan/Azurewave VP-6002"
#define SAA716x_DEV_TWINHAN_VP6002	"DVB-S"

static int saa716x_vp6002_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;

	dprintk(SAA716x_DEBUG, 1, "Adapter (%d) SAA716x frontend Init", count);
	dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Device ID=%02x", count, saa716x->pdev->subsystem_device);

	return -ENODEV;
}

static struct saa716x_config saa716x_vp6002_config = {
	.model_name		= SAA716x_MODEL_TWINHAN_VP6002,
	.dev_type		= SAA716x_DEV_TWINHAN_VP6002,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 1,
	.frontend_attach	= saa716x_vp6002_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
};


#define SAA716x_MODEL_KNC1_DUALS2	"KNC One Dual S2"
#define SAA716x_DEV_KNC1_DUALS2		"1xDVB-S + 1xDVB-S/S2"

static int saa716x_knc1_duals2_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;

	dprintk(SAA716x_DEBUG, 1, "Adapter (%d) SAA716x frontend Init", count);
	dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Device ID=%02x", count, saa716x->pdev->subsystem_device);

	return -ENODEV;
}

static struct saa716x_config saa716x_knc1_duals2_config = {
	.model_name		= SAA716x_MODEL_KNC1_DUALS2,
	.dev_type		= SAA716x_DEV_KNC1_DUALS2,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 2,
	.frontend_attach	= saa716x_knc1_duals2_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
};


#define SAA716x_MODEL_SKYSTAR2_EXPRESS_HD	"SkyStar 2 eXpress HD"
#define SAA716x_DEV_SKYSTAR2_EXPRESS_HD		"DVB-S/S2"

static struct stv090x_config skystar2_stv090x_config = {
	.device			= STV0903,
	.demod_mode		= STV090x_SINGLE,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 8000000,
	.address		= 0x68,

	.ts1_mode		= STV090x_TSMODE_DVBCI,
	.ts2_mode		= STV090x_TSMODE_SERIAL_CONTINUOUS,

	.repeater_level		= STV090x_RPTLEVEL_16,

	.tuner_init		= NULL,
	.tuner_sleep		= NULL,
	.tuner_set_mode		= NULL,
	.tuner_set_frequency	= NULL,
	.tuner_get_frequency	= NULL,
	.tuner_set_bandwidth	= NULL,
	.tuner_get_bandwidth	= NULL,
	.tuner_set_bbgain	= NULL,
	.tuner_get_bbgain	= NULL,
	.tuner_set_refclk	= NULL,
	.tuner_get_status	= NULL,
};

static int skystar2_set_voltage(struct dvb_frontend *fe,
				enum fe_sec_voltage voltage)
{
	int err;
	u8 en = 0;
	u8 sel = 0;

	switch (voltage) {
	case SEC_VOLTAGE_OFF:
		en = 0;
		break;

	case SEC_VOLTAGE_13:
		en = 1;
		sel = 0;
		break;

	case SEC_VOLTAGE_18:
		en = 1;
		sel = 1;
		break;

	default:
		break;
	}

	err = stv090x_set_gpio(fe, 2, 0, en, 0);
	if (err < 0)
		goto exit;
	err = stv090x_set_gpio(fe, 3, 0, sel, 0);
	if (err < 0)
		goto exit;

	return 0;
exit:
	return err;
}

static int skystar2_voltage_boost(struct dvb_frontend *fe, long arg)
{
	int err;
	u8 value;

	if (arg)
		value = 1;
	else
		value = 0;

	err = stv090x_set_gpio(fe, 4, 0, value, 0);
	if (err < 0)
		goto exit;

	return 0;
exit:
	return err;
}

static struct stv6110x_config skystar2_stv6110x_config = {
	.addr			= 0x60,
	.refclk			= 16000000,
	.clk_div		= 2,
};

static int skystar2_express_hd_frontend_attach(struct saa716x_adapter *adapter,
					       int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[SAA716x_I2C_BUS_B];
	struct stv6110x_devctl *ctl;

	if (count < saa716x->config->adapters) {
		dprintk(SAA716x_DEBUG, 1, "Adapter (%d) SAA716x frontend Init",
			count);
		dprintk(SAA716x_DEBUG, 1, "Adapter (%d) Device ID=%02x", count,
			saa716x->pdev->subsystem_device);

		saa716x_gpio_set_output(saa716x, 26);

		/* Reset the demodulator */
		saa716x_gpio_write(saa716x, 26, 1);
		saa716x_gpio_write(saa716x, 26, 0);
		msleep(10);
		saa716x_gpio_write(saa716x, 26, 1);
		msleep(10);

		adapter->fe = dvb_attach(stv090x_attach,
					 &skystar2_stv090x_config,
					 &i2c->i2c_adapter,
					 STV090x_DEMODULATOR_0);

		if (adapter->fe) {
			dprintk(SAA716x_NOTICE, 1, "found STV0903 @0x%02x",
				skystar2_stv090x_config.address);
		} else {
			goto exit;
		}

		adapter->fe->ops.set_voltage = skystar2_set_voltage;
		adapter->fe->ops.enable_high_lnb_voltage = skystar2_voltage_boost;

		ctl = dvb_attach(stv6110x_attach,
				 adapter->fe,
				 &skystar2_stv6110x_config,
				 &i2c->i2c_adapter);

		if (ctl) {
			dprintk(SAA716x_NOTICE, 1, "found STV6110(A) @0x%02x",
				skystar2_stv6110x_config.addr);

			skystar2_stv090x_config.tuner_init	    = ctl->tuner_init;
			skystar2_stv090x_config.tuner_sleep	    = ctl->tuner_sleep;
			skystar2_stv090x_config.tuner_set_mode	    = ctl->tuner_set_mode;
			skystar2_stv090x_config.tuner_set_frequency = ctl->tuner_set_frequency;
			skystar2_stv090x_config.tuner_get_frequency = ctl->tuner_get_frequency;
			skystar2_stv090x_config.tuner_set_bandwidth = ctl->tuner_set_bandwidth;
			skystar2_stv090x_config.tuner_get_bandwidth = ctl->tuner_get_bandwidth;
			skystar2_stv090x_config.tuner_set_bbgain    = ctl->tuner_set_bbgain;
			skystar2_stv090x_config.tuner_get_bbgain    = ctl->tuner_get_bbgain;
			skystar2_stv090x_config.tuner_set_refclk    = ctl->tuner_set_refclk;
			skystar2_stv090x_config.tuner_get_status    = ctl->tuner_get_status;

			/* call the init function once to initialize
			   tuner's clock output divider and demod's
			   master clock */
			if (adapter->fe->ops.init)
				adapter->fe->ops.init(adapter->fe);
		} else {
			goto exit;
		}

		dprintk(SAA716x_ERROR, 1, "Done!");
		return 0;
	}
exit:
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config skystar2_express_hd_config = {
	.model_name		= SAA716x_MODEL_SKYSTAR2_EXPRESS_HD,
	.dev_type		= SAA716x_DEV_SKYSTAR2_EXPRESS_HD,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 1,
	.frontend_attach	= skystar2_express_hd_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.adap_config		= {
		{
			/* Adapter 0 */
			.ts_port = 1, /* using FGPI 1 */
			.worker = demux_worker
		}
	}
};


#define SAA716x_MODEL_TBS6284		"TurboSight TBS 6284"
#define SAA716x_DEV_TBS6284		"DVB-T/T2/C"

static struct cxd2820r_config cxd2820r_config[] = {
	{
		.i2c_address = 0x6c, /* (0xd8 >> 1) */
		.ts_mode = 0x38,
	},
	{
		.i2c_address = 0x6d, /* (0xda >> 1) */
		.ts_mode = 0x38,
	}
};

static struct tda18212_config tda18212_config[] = {
	{
		.i2c_address = 0x60 /* (0xc0 >> 1) */,
		.if_dvbt_6 = 3550,
		.if_dvbt_7 = 3700,
		.if_dvbt_8 = 4150,
		.if_dvbt2_6 = 3250,
		.if_dvbt2_7 = 4000,
		.if_dvbt2_8 = 4000,
		.if_dvbc = 5000,
		.loop_through = 1,
		.xtout = 1
	},
	{
		.i2c_address = 0x63 /* (0xc6 >> 1) */,
		.if_dvbt_6 = 3550,
		.if_dvbt_7 = 3700,
		.if_dvbt_8 = 4150,
		.if_dvbt2_6 = 3250,
		.if_dvbt2_7 = 4000,
		.if_dvbt2_8 = 4000,
		.if_dvbc = 5000,
		.loop_through = 0,
		.xtout = 0
	}
};

static int saa716x_tbs6284_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[SAA716x_I2C_BUS_A];
	struct saa716x_i2c *i2c1 = &saa716x->i2c[SAA716x_I2C_BUS_B];

	switch (count) {
	case 0:
		/* reset */
		saa716x_gpio_set_output(saa716x, 22);
		saa716x_gpio_write(saa716x, 22, 0);
		msleep(200);
		saa716x_gpio_write(saa716x, 22, 1);
		msleep(400);
	case 1:
		dprintk(SAA716x_ERROR, 1, "Probing for cxd2820r (%d)", count);
		adapter->fe = cxd2820r_attach(&cxd2820r_config[count],
					&i2c1->i2c_adapter, NULL);
		if (!adapter->fe)
			goto err;

		if (!dvb_attach(tda18212_attach, adapter->fe,
			&i2c1->i2c_adapter, &tda18212_config[count])) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			goto err;
		}
		break;
	case 2:
		/* reset */
		saa716x_gpio_set_output(saa716x, 12);
		saa716x_gpio_write(saa716x, 12, 0);
		msleep(200);
		saa716x_gpio_write(saa716x, 12, 1);
		msleep(400);
	case 3:
		dprintk(SAA716x_ERROR, 1, "Probing for cxd2820r (%d)", count);
		adapter->fe = cxd2820r_attach(&cxd2820r_config[count - 2],
					&i2c0->i2c_adapter, NULL);
		if (!adapter->fe)
			goto err;

		if (!dvb_attach(tda18212_attach, adapter->fe,
			&i2c0->i2c_adapter, &tda18212_config[count - 2])) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			goto err;
		}
		break;
	default:
		goto err;
	}

	dprintk(SAA716x_ERROR, 1, "Done!");
	return 0;
err:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6284_config = {
	.model_name		= SAA716x_MODEL_TBS6284,
	.dev_type		= SAA716x_DEV_TBS6284,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6284_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3,
			.worker = demux_worker
		},
		{
			/* adapter 1 */
			.ts_port = 2,
			.worker = demux_worker
		},
		{
			/* adapter 2 */
			.ts_port = 1,
			.worker = demux_worker
		},
		{
			/* adapter 3 */
			.ts_port = 0,
			.worker = demux_worker
		}
	},
};

#define SAA716x_MODEL_TBS6280		"TurboSight TBS 6280"
#define SAA716x_DEV_TBS6280		"DVB-T/T2/C"

static int saa716x_tbs6280_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c0 = &saa716x->i2c[SAA716x_I2C_BUS_A];

	switch (count) {
	case 0:
		/* reset */
		saa716x_gpio_set_output(saa716x, 2);
		saa716x_gpio_write(saa716x, 2, 0);
		msleep(200);
		saa716x_gpio_write(saa716x, 2, 1);
		msleep(400);
	case 1:
		dprintk(SAA716x_ERROR, 1, "Probing for cxd2820r (%d)", count);
		adapter->fe = cxd2820r_attach(&cxd2820r_config[count],
					&i2c0->i2c_adapter, NULL);
		if (!adapter->fe)
			goto err;

		if (!dvb_attach(tda18212_attach, adapter->fe,
			&i2c0->i2c_adapter, &tda18212_config[count])) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			goto err;
		}
		break;
	default:
		goto err;
	}

	dprintk(SAA716x_ERROR, 1, "Done!");
	return 0;
err:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6280_config = {
	.model_name		= SAA716x_MODEL_TBS6280,
	.dev_type		= SAA716x_DEV_TBS6280,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6280_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 1, /* using FGPI 1 */
			.worker = demux_worker
		},
		{
			/* adapter 1 */
			.ts_port = 3, /* using FGPI 3 */
			.worker = demux_worker
		},
	},
};

#define SAA716x_MODEL_TBS6220		"TurboSight TBS 6220"
#define SAA716x_DEV_TBS6220		"DVB-T/T2/C"

static int saa716x_tbs6220_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[SAA716x_I2C_BUS_A];

	if (count == 0) {
		dprintk(SAA716x_ERROR, 1, "Probing for cxd2820r (%d)", count);
		adapter->fe = cxd2820r_attach(&cxd2820r_config[count],
					&i2c->i2c_adapter, NULL);
		if (!adapter->fe)
			goto err;

		if (!dvb_attach(tda18212_attach, adapter->fe,
			&i2c->i2c_adapter, &tda18212_config[count])) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			goto err;
		}
		dprintk(SAA716x_ERROR, 1, "Done!");
		return 0;
	}

err:
	printk(KERN_ERR "%s: frontend initialization failed\n",
					adapter->saa716x->config->model_name);
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6220_config = {
	.model_name		= SAA716x_MODEL_TBS6220,
	.dev_type		= SAA716x_DEV_TBS6220,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6220_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3, /* using FGPI 3 */
			.worker = demux_worker
		},
	},
};


#define SAA716x_MODEL_TBS6922		"TurboSight TBS 6922"
#define SAA716x_DEV_TBS6922		"DVB-S/S2"

static void tbs6922_reset_fe(struct dvb_frontend *fe)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;
	int reset_pin = 0;

	/* reset frontend, active low */
/*	saa716x_gpio_set_output(dev, reset_pin);
	saa716x_gpio_write(dev, reset_pin, 0);
	msleep(60);
	saa716x_gpio_write(dev, reset_pin, 1);
	msleep(120);*/
}

static void tbs6922_lnb_power(struct dvb_frontend *fe, int onoff)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;
	int enpwr_pin = 17;

	/* lnb power, active high */
	saa716x_gpio_set_output(dev, enpwr_pin);
	if (onoff)
		saa716x_gpio_write(dev, enpwr_pin, 1);
	else
		saa716x_gpio_write(dev, enpwr_pin, 0);
}


static struct tas2101_config tbs6922_cfg = {
	.i2c_address   = 0x68,
	.reset_demod   = tbs6922_reset_fe,
	.lnb_power     = tbs6922_lnb_power,
	.init          = {0x10, 0x32, 0x54, 0x76, 0xb8, 0x9a},
};

static struct av201x_config tbs6922_av201x_cfg = {
	.i2c_address = 0x63,
	.id          = ID_AV2011,
	.xtal_freq   = 27000,		/* kHz */
};

static int saa716x_tbs6922_frontend_attach(
	struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *dev = adapter->saa716x;

	dev_dbg(&dev->pdev->dev, "%s frontend %d attaching\n",
		dev->config->model_name, count);
	if (count > 0)
		goto err;

	saa716x_gpio_set_output(dev, 2);
	saa716x_gpio_write(dev, 2, 0);
	msleep(60);
	saa716x_gpio_write(dev, 2, 1);
	msleep(120);

	adapter->fe = dvb_attach(tas2101_attach, &tbs6922_cfg,
				&dev->i2c[SAA716x_I2C_BUS_A].i2c_adapter);
	if (adapter->fe == NULL)
		goto err;

	if (dvb_attach(av201x_attach, adapter->fe, &tbs6922_av201x_cfg,
			tas2101_get_i2c_adapter(adapter->fe, 2)) == NULL) {
		dvb_frontend_detach(adapter->fe);
		adapter->fe = NULL;
		dev_dbg(&dev->pdev->dev,
			"%s frontend %d tuner attach failed\n",
			dev->config->model_name, count);
		goto err;
	}

	dev_dbg(&dev->pdev->dev, "%s frontend %d attached\n",
		dev->config->model_name, count);

	return 0;
err:
	dev_err(&dev->pdev->dev, "%s frontend %d attach failed\n",
		dev->config->model_name, count);
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6922_config = {
	.model_name		= SAA716x_MODEL_TBS6922,
	.dev_type		= SAA716x_DEV_TBS6922,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 1,
	.frontend_attach	= saa716x_tbs6922_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3, /* using FGPI 3 */
			.worker = demux_worker
		},
	},
};


#define SAA716x_MODEL_TBS6925		"TurboSight TBS 6925"
#define SAA716x_DEV_TBS6925		"DVB-S/S2"

static struct stv090x_config tbs6925_stv090x_cfg = {
	.device			= STV0900,
	.demod_mode		= STV090x_SINGLE,
	.clk_mode		= STV090x_CLK_EXT,

	.xtal			= 27000000,
	.address		= 0x68,

	.ts1_mode		= STV090x_TSMODE_PARALLEL_PUNCTURED,
	.ts2_mode		= STV090x_TSMODE_PARALLEL_PUNCTURED,

	.repeater_level		= STV090x_RPTLEVEL_16,
	.adc1_range		= STV090x_ADC_1Vpp,
	.tuner_bbgain		= 6,

	.tuner_get_frequency	= stb6100_get_frequency,
	.tuner_set_frequency	= stb6100_set_frequency,
	.tuner_set_bandwidth	= stb6100_set_bandwidth,
	.tuner_get_bandwidth	= stb6100_get_bandwidth,
};

static struct stb6100_config tbs6925_stb6100_cfg = {
	.tuner_address	= 0x60,
	.refclock	= 27000000
};

static int tbs6925_set_voltage(struct dvb_frontend *fe, enum fe_sec_voltage voltage)
{
	struct saa716x_adapter *adapter = fe->dvb->priv;
	struct saa716x_dev *saa716x = adapter->saa716x;

	saa716x_gpio_set_output(saa716x, 16);
	msleep(1);
	switch (voltage) {
	case SEC_VOLTAGE_13:
			dprintk(SAA716x_ERROR, 1, "Polarization=[13V]");
			saa716x_gpio_write(saa716x, 16, 0);
			break;
	case SEC_VOLTAGE_18:
			dprintk(SAA716x_ERROR, 1, "Polarization=[18V]");
			saa716x_gpio_write(saa716x, 16, 1);
			break;
	case SEC_VOLTAGE_OFF:
			dprintk(SAA716x_ERROR, 1, "Frontend (dummy) POWERDOWN");
			break;
	default:
			dprintk(SAA716x_ERROR, 1, "Invalid = (%d)", (u32 ) voltage);
			return -EINVAL;
	}
	msleep(100);

	return 0;
}

static int tbs6925_frontend_attach(struct saa716x_adapter *adapter,
					       int count)
{
	struct saa716x_dev *saa716x = adapter->saa716x;
	struct saa716x_i2c *i2c = &saa716x->i2c[SAA716x_I2C_BUS_A];
	struct stv6110x_devctl *ctl;

	if (count > 0)
		goto err;

	dprintk(SAA716x_DEBUG, 1,
		"Adapter (%d) SAA716x frontend init Device ID=%02x",
		count, saa716x->pdev->subsystem_device);

	/* Reset the demodulator */
	saa716x_gpio_set_output(saa716x, 2);
	saa716x_gpio_write(saa716x, 2, 0);
	msleep(50);
	saa716x_gpio_write(saa716x, 2, 1);
	msleep(100);

	adapter->fe = dvb_attach(stv090x_attach, &tbs6925_stv090x_cfg,
				&i2c->i2c_adapter, STV090x_DEMODULATOR_0);
	if (adapter->fe)
		dprintk(SAA716x_NOTICE, 1, "found STV0900 @0x%02x",
			tbs6925_stv090x_cfg.address);
	else
		goto err;

	adapter->fe->ops.set_voltage   = tbs6925_set_voltage;

	ctl = dvb_attach(stb6100_attach, adapter->fe,
			&tbs6925_stb6100_cfg, &i2c->i2c_adapter);
	if (ctl) {
		dprintk(SAA716x_NOTICE, 1, "found STB6100");
		/* call the init function once to initialize
		   tuner's clock output divider and demod's
		   master clock */
		if (adapter->fe->ops.init)
			adapter->fe->ops.init(adapter->fe);
	} else {
		dvb_frontend_detach(adapter->fe);
		adapter->fe = NULL;
		goto err;
	}

	dprintk(SAA716x_ERROR, 1, "Done!");
	return 0;

err:
	dprintk(SAA716x_ERROR, 1, "Frontend attach failed");
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6925_config = {
	.model_name		= SAA716x_MODEL_TBS6925,
	.dev_type		= SAA716x_DEV_TBS6925,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 1,
	.frontend_attach	= tbs6925_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* Adapter 0 */
			.ts_port = 3, /* using FGPI 1 */
			.worker = demux_worker
		}
	}
};


#define SAA716x_MODEL_TBS6982		"TurboSight TBS 6982"
#define SAA716x_DEV_TBS6982		"DVB-S/S2"

static void tbs6982_reset_fe(struct dvb_frontend *fe, int reset_pin)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;

	/* reset frontend, active low */
	saa716x_gpio_set_output(dev, reset_pin);
	saa716x_gpio_write(dev, reset_pin, 0);
	msleep(60);
	saa716x_gpio_write(dev, reset_pin, 1);
	msleep(120);
}

static void tbs6982_reset_fe0(struct dvb_frontend *fe)
{
	tbs6982_reset_fe(fe, 2);
}

static void tbs6982_reset_fe1(struct dvb_frontend *fe)
{
	tbs6982_reset_fe(fe, 17);
}

static void tbs6982_lnb_power(struct dvb_frontend *fe,
	int enpwr_pin, int onoff)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;

	/* lnb power, active low */
	saa716x_gpio_set_output(dev, enpwr_pin);
	if (onoff)
		saa716x_gpio_write(dev, enpwr_pin, 0);
	else
		saa716x_gpio_write(dev, enpwr_pin, 1);
}

static void tbs6982_lnb0_power(struct dvb_frontend *fe, int onoff)
{
	tbs6982_lnb_power(fe, 5, onoff);
}

static void tbs6982_lnb1_power(struct dvb_frontend *fe, int onoff)
{
	tbs6982_lnb_power(fe, 3, onoff);
}

static struct tas2101_config tbs6982_cfg[] = {
	{
		.i2c_address   = 0x68,
		.reset_demod   = tbs6982_reset_fe0,
		.lnb_power     = tbs6982_lnb0_power,
		.init          = {}
	},
	{
		.i2c_address   = 0x68,
		.reset_demod   = tbs6982_reset_fe1,
		.lnb_power     = tbs6982_lnb1_power,
	}
};

static struct av201x_config tbs6982_av201x_cfg = {
	.i2c_address = 0x63,
	.id          = ID_AV2012,
	.xtal_freq   = 27000,		/* kHz */
};

static int saa716x_tbs6982_frontend_attach(
	struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *dev = adapter->saa716x;

	dev_dbg(&dev->pdev->dev, "%s frontend %d attaching\n",
		dev->config->model_name, count);
	if (count > 1)
		goto err;

	saa716x_gpio_set_output(dev, 16);
	saa716x_gpio_write(dev, 16, 0);
	msleep(60);
	saa716x_gpio_write(dev, 16, 1);
	msleep(120);

	adapter->fe = dvb_attach(tas2101_attach, &tbs6982_cfg[count],
				&dev->i2c[1 - count].i2c_adapter);
	if (adapter->fe == NULL)
		goto err;

	if (dvb_attach(av201x_attach, adapter->fe, &tbs6982_av201x_cfg,
			tas2101_get_i2c_adapter(adapter->fe, 2)) == NULL) {
		dvb_frontend_detach(adapter->fe);
		adapter->fe = NULL;
		dev_dbg(&dev->pdev->dev,
			"%s frontend %d tuner attach failed\n",
			dev->config->model_name, count);
		goto err;
	}

	dev_dbg(&dev->pdev->dev, "%s frontend %d attached\n",
		dev->config->model_name, count);

	return 0;
err:
	dev_err(&dev->pdev->dev, "%s frontend %d attach failed\n",
		dev->config->model_name, count);
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6982_config = {
	.model_name		= SAA716x_MODEL_TBS6982,
	.dev_type		= SAA716x_DEV_TBS6982,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6982_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_400,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3, /* using FGPI 3 */
			.worker = demux_worker
		},
		{
			/* adapter 1 */
			.ts_port = 1, /* using FGPI 1 */
			.worker = demux_worker
		},
	},
};


#define SAA716x_MODEL_TBS6982SE		"TurboSight TBS 6982SE"
#define SAA716x_DEV_TBS6982SE		"DVB-S/S2"

static void tbs6982se_reset_fe(struct dvb_frontend *fe, int reset_pin)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;

	/* reset frontend, active low */
	saa716x_gpio_set_output(dev, reset_pin);
	saa716x_gpio_write(dev, reset_pin, 0);
	msleep(60);
	saa716x_gpio_write(dev, reset_pin, 1);
	msleep(120);
}

static void tbs6982se_reset_fe0(struct dvb_frontend *fe)
{
	tbs6982se_reset_fe(fe, 2);
}

static void tbs6982se_reset_fe1(struct dvb_frontend *fe)
{
	tbs6982se_reset_fe(fe, 17);
}

static void tbs6982se_lnb_power(struct dvb_frontend *fe,
	int enpwr_pin, int onoff)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;

	/* lnb power, active low */
	saa716x_gpio_set_output(dev, enpwr_pin);
	if (onoff)
		saa716x_gpio_write(dev, enpwr_pin, 0);
	else
		saa716x_gpio_write(dev, enpwr_pin, 1);
}

static void tbs6982se_lnb0_power(struct dvb_frontend *fe, int onoff)
{
	tbs6982se_lnb_power(fe, 3, onoff);
}

static void tbs6982se_lnb1_power(struct dvb_frontend *fe, int onoff)
{
	tbs6982se_lnb_power(fe, 16, onoff);
}

static struct tas2101_config tbs6982se_cfg[] = {
	{
		.i2c_address   = 0x60,
		.reset_demod   = tbs6982se_reset_fe0,
		.lnb_power     = tbs6982se_lnb0_power,
		.init          = {0x10, 0x32, 0x54, 0x76, 0xb8, 0x9a},
	},
	{
		.i2c_address   = 0x68,
		.reset_demod   = tbs6982se_reset_fe1,
		.lnb_power     = tbs6982se_lnb1_power,
		.init          = {0x8a, 0x6b, 0x13, 0x70, 0x45, 0x92},
	}
};

static struct av201x_config tbs6982se_av201x_cfg = {
	.i2c_address = 0x63,
	.id          = ID_AV2012,
	.xtal_freq   = 27000,		/* kHz */
};

static int saa716x_tbs6982se_frontend_attach(
	struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *dev = adapter->saa716x;

	dev_dbg(&dev->pdev->dev, "%s frontend %d attaching\n",
		dev->config->model_name, count);
	if (count > 1)
		goto err;

	adapter->fe = dvb_attach(tas2101_attach, &tbs6982se_cfg[count],
				&dev->i2c[count].i2c_adapter);
	if (adapter->fe == NULL)
		goto err;

	if (dvb_attach(av201x_attach, adapter->fe, &tbs6982se_av201x_cfg,
			tas2101_get_i2c_adapter(adapter->fe, 2)) == NULL) {
		dvb_frontend_detach(adapter->fe);
		adapter->fe = NULL;
		dev_dbg(&dev->pdev->dev,
			"%s frontend %d tuner attach failed\n",
			dev->config->model_name, count);
		goto err;
	}

	dev_dbg(&dev->pdev->dev, "%s frontend %d attached\n",
		dev->config->model_name, count);

	return 0;
err:
	dev_err(&dev->pdev->dev, "%s frontend %d attach failed\n",
		dev->config->model_name, count);
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6982se_config = {
	.model_name		= SAA716x_MODEL_TBS6982SE,
	.dev_type		= SAA716x_DEV_TBS6982SE,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 2,
	.frontend_attach	= saa716x_tbs6982se_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_400,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 3, /* using FGPI 3 */
			.worker = demux_worker
		},
		{
			/* adapter 1 */
			.ts_port = 1, /* using FGPI 1 */
			.worker = demux_worker
		},
	},
};


#define SAA716x_MODEL_TBS6984		"TurboSight TBS 6984"
#define SAA716x_DEV_TBS6984		"DVB-S/S2"

static int saa716x_tbs6984_init(struct saa716x_dev *saa716x)
{
	int i;
	const u8 buf[] = {
		0xe0, 0x06, 0x66, 0x33, 0x65,
		0x01, 0x17, 0x06, 0xde};

#define TBS_CK 7
#define TBS_CS 8
#define TBS_DT 11

	/* send init bitstream through a bitbanged spi */
	/* set pins as output */
	saa716x_gpio_set_output(saa716x, TBS_CK);
	saa716x_gpio_set_output(saa716x, TBS_CS);
	saa716x_gpio_set_output(saa716x, TBS_DT);

	/* set all pins high */
	saa716x_gpio_write(saa716x, TBS_CK, 1);
	saa716x_gpio_write(saa716x, TBS_CS, 1);
	saa716x_gpio_write(saa716x, TBS_DT, 1);
	msleep(20);

	/* CS low */
	saa716x_gpio_write(saa716x, TBS_CS, 0);
	msleep(20);
	/* send bitstream */
	for (i = 0; i < 9 * 8; i++) {
		/* clock low */
		saa716x_gpio_write(saa716x, TBS_CK, 0);
		msleep(20);
		/* set data pin */
		saa716x_gpio_write(saa716x, TBS_DT, 
			((buf[i >> 3] >> (7 - (i & 7))) & 1));
		/* clock high */
		saa716x_gpio_write(saa716x, TBS_CK, 1);
		msleep(20);
	}
	/* raise cs, clk and data */
	saa716x_gpio_write(saa716x, TBS_CS, 1);
	saa716x_gpio_write(saa716x, TBS_CK, 1);
	saa716x_gpio_write(saa716x, TBS_DT, 1);

	/* power up LNB supply and control chips */
	saa716x_gpio_set_output(saa716x, 19);	/* a0 */
	saa716x_gpio_set_output(saa716x, 2);	/* a1 */
	saa716x_gpio_set_output(saa716x, 5);	/* a2 */
	saa716x_gpio_set_output(saa716x, 3);	/* a3 */

	/* power on */
	saa716x_gpio_write(saa716x, 19, 0); /* a0 */
	saa716x_gpio_write(saa716x, 2, 0); /* a1 */
	saa716x_gpio_write(saa716x, 5, 0); /* a2 */
	saa716x_gpio_write(saa716x, 3, 0); /* a3 */

	/* done */
	return 0;
}

static struct cx24117_config tbs6984_cx24117_cfg[] = {
	{
		.demod_address = 0x55,
	},
	{
		.demod_address = 0x05,
	},
};

static struct isl6422_config tbs6984_isl6422_cfg[] = {
	{
		.current_max		= SEC_CURRENT_570m,
		.curlim			= SEC_CURRENT_LIM_ON,
		.mod_extern		= 1,
		.addr			= 0x08,
		.id			= 0,
	},
	{
		.current_max		= SEC_CURRENT_570m,
		.curlim			= SEC_CURRENT_LIM_ON,
		.mod_extern		= 1,
		.addr			= 0x08,
		.id			= 1,
	}

};

static int saa716x_tbs6984_frontend_attach(
	struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *dev = adapter->saa716x;

	dev_dbg(&dev->pdev->dev, "%s frontend %d attaching\n",
		dev->config->model_name, count);

	switch (count) {
	case 0:
		saa716x_tbs6984_init(dev);
	case 1:
		/* first and second FE attaching */
		adapter->fe = dvb_attach(cx24117_attach, &tbs6984_cx24117_cfg[0],
				&dev->i2c[SAA716x_I2C_BUS_A].i2c_adapter);
		if (adapter->fe == NULL)
			goto err;
		if (dvb_attach(isl6422_attach, adapter->fe,
				&dev->i2c[SAA716x_I2C_BUS_A].i2c_adapter,
				&tbs6984_isl6422_cfg[count]) == NULL) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			dev_dbg(&dev->pdev->dev,
				"%s frontend %d SEC attach failed\n",
				dev->config->model_name, count);
			goto err;
		}
		break;
	case 2:
	case 3:
		/* third and forth FE attaching */
		adapter->fe = dvb_attach(cx24117_attach, &tbs6984_cx24117_cfg[1],
				&dev->i2c[SAA716x_I2C_BUS_B].i2c_adapter);
		if (adapter->fe == NULL)
			goto err;
		if (dvb_attach(isl6422_attach, adapter->fe,
				&dev->i2c[SAA716x_I2C_BUS_B].i2c_adapter,
				&tbs6984_isl6422_cfg[count - 2]) == NULL) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			dev_dbg(&dev->pdev->dev,
				"%s frontend %d SEC attach failed\n",
				dev->config->model_name, count);
			goto err;
		}
		break;
	default:
		goto err;
		break;
	}

	return 0;
err:
	dev_err(&dev->pdev->dev, "%s frontend %d attach failed\n",
		dev->config->model_name, count);
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6984_config = {
	.model_name		= SAA716x_MODEL_TBS6984,
	.dev_type		= SAA716x_DEV_TBS6984,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6984_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_100,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 2,
			.worker = demux_worker
		},
		{
			/* adapter 1 */
			.ts_port = 3,
			.worker = demux_worker
		},
		{
			/* adapter 2 */
			.ts_port = 0,
			.worker = demux_worker
		},
		{
			/* adapter 3 */
			.ts_port = 1,
			.worker = demux_worker
		},
	},
};


#define SAA716x_MODEL_TBS6985 "TurboSight TBS 6985"
#define SAA716x_DEV_TBS6985   "DVB-S/S2"


static void tbs6985_reset_fe(struct dvb_frontend *fe, int reset_pin)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;

	/* reset frontend, active low */
	saa716x_gpio_set_output(dev, reset_pin);
	saa716x_gpio_write(dev, reset_pin, 0);
	msleep(60);
	saa716x_gpio_write(dev, reset_pin, 1);
	msleep(120);
}

static void tbs6985_reset_fe0(struct dvb_frontend *fe)
{
	tbs6985_reset_fe(fe, 5);
}

static void tbs6985_reset_fe1(struct dvb_frontend *fe)
{
	tbs6985_reset_fe(fe, 2);
}

static void tbs6985_reset_fe2(struct dvb_frontend *fe)
{
	tbs6985_reset_fe(fe, 13);
}

static void tbs6985_reset_fe3(struct dvb_frontend *fe)
{
	tbs6985_reset_fe(fe, 3);
}

static void tbs6985_lnb_power(struct dvb_frontend *fe,
	int enpwr_pin, int onoff)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct saa716x_i2c *i2c = i2c_get_adapdata(adapter);
        struct saa716x_dev *dev = i2c->saa716x;

	/* lnb power, active low */
	saa716x_gpio_set_output(dev, enpwr_pin);
	if (onoff)
		saa716x_gpio_write(dev, enpwr_pin, 0);
	else
		saa716x_gpio_write(dev, enpwr_pin, 1);
}

static void tbs6985_lnb0_power(struct dvb_frontend *fe, int onoff)
{
	tbs6985_lnb_power(fe, 27, onoff);
}

static void tbs6985_lnb1_power(struct dvb_frontend *fe, int onoff)
{
	tbs6985_lnb_power(fe, 22, onoff);
}

static void tbs6985_lnb2_power(struct dvb_frontend *fe, int onoff)
{
	tbs6985_lnb_power(fe, 19, onoff);
}

static void tbs6985_lnb3_power(struct dvb_frontend *fe, int onoff)
{
	tbs6985_lnb_power(fe, 15, onoff);
}

static struct tas2101_config tbs6985_cfg[] = {
	{
		.i2c_address   = 0x60,
		.reset_demod   = tbs6985_reset_fe0,
		.lnb_power     = tbs6985_lnb0_power,
		.init          = {0x01, 0x32, 0x65, 0x74, 0xab, 0x98},
	},
	{
		.i2c_address   = 0x68,
		.reset_demod   = tbs6985_reset_fe1,
		.lnb_power     = tbs6985_lnb1_power,
		.init          = {0x10, 0x32, 0x54, 0xb7, 0x86, 0x9a},
	},
	{
		.i2c_address   = 0x60,
		.reset_demod   = tbs6985_reset_fe2,
		.lnb_power     = tbs6985_lnb2_power,
		.init          = {0x25, 0x36, 0x40, 0xb1, 0x87, 0x9a},
	},
	{
		.i2c_address   = 0x68,
		.reset_demod   = tbs6985_reset_fe3,
		.lnb_power     = tbs6985_lnb3_power,
		.init          = {0x80, 0xba, 0x21, 0x53, 0x74, 0x96},
	}
};

static struct av201x_config tbs6985_av201x_cfg = {
	.i2c_address = 0x63,
	.id          = ID_AV2012,
	.xtal_freq   = 27000,		/* kHz */
};

static int saa716x_tbs6985_frontend_attach(struct saa716x_adapter *adapter, int count)
{
	struct saa716x_dev *dev = adapter->saa716x;

	if (count > 3)
		goto err;

	adapter->fe = dvb_attach(tas2101_attach, &tbs6985_cfg[count],
				&dev->i2c[1 - (count >> 1)].i2c_adapter);
	if (adapter->fe == NULL)
		goto err;

	if (dvb_attach(av201x_attach, adapter->fe, &tbs6985_av201x_cfg,
			tas2101_get_i2c_adapter(adapter->fe, 2)) == NULL) {
		dvb_frontend_detach(adapter->fe);
		adapter->fe = NULL;
		dev_dbg(&dev->pdev->dev,
			"%s frontend %d tuner attach failed\n",
			dev->config->model_name, count);
		goto err;
	}

	return 0;
err:
	dev_err(&dev->pdev->dev, "%s frontend %d attach failed\n",
		dev->config->model_name, count);
	return -ENODEV;
}

static struct saa716x_config saa716x_tbs6985_config = {
	.model_name		= SAA716x_MODEL_TBS6985,
	.dev_type		= SAA716x_DEV_TBS6985,
	.boot_mode		= SAA716x_EXT_BOOT,
	.adapters		= 4,
	.frontend_attach	= saa716x_tbs6985_frontend_attach,
	.irq_handler		= saa716x_budget_pci_irq,
	.i2c_rate		= SAA716x_I2C_RATE_400,
	.i2c_mode		= SAA716x_I2C_MODE_POLLING,
	.adap_config		= {
		{
			/* adapter 0 */
			.ts_port = 2,
			.worker = demux_worker
		},
		{
			/* adapter 1 */
			.ts_port = 3,
			.worker = demux_worker
		},
		{
			/* adapter 2 */
			.ts_port = 0,
			.worker = demux_worker
		},
		{
			/* adapter 3 */
			.ts_port = 1,
			.worker = demux_worker
		}
	},
};


static struct pci_device_id saa716x_budget_pci_table[] = {
	MAKE_ENTRY(TWINHAN_TECHNOLOGIES, TWINHAN_VP_1028, SAA7160, &saa716x_vp1028_config), /* VP-1028 */
	MAKE_ENTRY(TWINHAN_TECHNOLOGIES, TWINHAN_VP_3071, SAA7160, &saa716x_vp3071_config), /* VP-3071 */
	MAKE_ENTRY(TWINHAN_TECHNOLOGIES, TWINHAN_VP_6002, SAA7160, &saa716x_vp6002_config), /* VP-6002 */
	MAKE_ENTRY(KNC_One, KNC_Dual_S2, SAA7160, &saa716x_knc1_duals2_config),
	MAKE_ENTRY(TECHNISAT, SKYSTAR2_EXPRESS_HD, SAA7160, &skystar2_express_hd_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6284, TBS6284,   SAA7160, &saa716x_tbs6284_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6280, TBS6280,   SAA7160, &saa716x_tbs6280_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6220, TBS6220,   SAA7160, &saa716x_tbs6220_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6922, TBS6922,   SAA7160, &saa716x_tbs6922_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6925, TBS6925,   SAA7160, &saa716x_tbs6925_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6982, TBS6982,   SAA7160, &saa716x_tbs6982_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6982, TBS6982SE, SAA7160, &saa716x_tbs6982se_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6984, TBS6984,   SAA7160, &saa716x_tbs6984_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6985, TBS6985,   SAA7160, &saa716x_tbs6985_config),
	MAKE_ENTRY(TURBOSIGHT_TBS6985, TBS6985+1, SAA7160, &saa716x_tbs6985_config),
	MAKE_ENTRY(TECHNOTREND,        TT4100,    SAA7160, &saa716x_tbs6922_config),
	{ }
};
MODULE_DEVICE_TABLE(pci, saa716x_budget_pci_table);

static struct pci_driver saa716x_budget_pci_driver = {
	.name			= DRIVER_NAME,
	.id_table		= saa716x_budget_pci_table,
	.probe			= saa716x_budget_pci_probe,
	.remove			= saa716x_budget_pci_remove,
};

static int __init saa716x_budget_init(void)
{
	return pci_register_driver(&saa716x_budget_pci_driver);
}

static void __exit saa716x_budget_exit(void)
{
	return pci_unregister_driver(&saa716x_budget_pci_driver);
}

module_init(saa716x_budget_init);
module_exit(saa716x_budget_exit);

MODULE_DESCRIPTION("SAA716x Budget driver");
MODULE_AUTHOR("Manu Abraham");
MODULE_LICENSE("GPL");
