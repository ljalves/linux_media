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
		msleep(10);
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


static struct pci_device_id saa716x_budget_pci_table[] = {

	MAKE_ENTRY(TWINHAN_TECHNOLOGIES, TWINHAN_VP_1028, SAA7160, &saa716x_vp1028_config), /* VP-1028 */
	MAKE_ENTRY(TWINHAN_TECHNOLOGIES, TWINHAN_VP_3071, SAA7160, &saa716x_vp3071_config), /* VP-3071 */
	MAKE_ENTRY(TWINHAN_TECHNOLOGIES, TWINHAN_VP_6002, SAA7160, &saa716x_vp6002_config), /* VP-6002 */
	MAKE_ENTRY(KNC_One, KNC_Dual_S2, SAA7160, &saa716x_knc1_duals2_config),
	MAKE_ENTRY(TECHNISAT, SKYSTAR2_EXPRESS_HD, SAA7160, &skystar2_express_hd_config),
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
