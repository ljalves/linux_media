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

#include "tbsecp3.h"

#include "tas2101.h"
#include "av201x.h"

#include "si2168.h"
#include "si2157.h"
#if 0
#include "mxl5xx.h"

#include "stv0910.h"
#include "stv6120.h"
#endif

DVB_DEFINE_MOD_OPT_ADAPTER_NR(adapter_nr);

static int start_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;
	struct tbsecp3_adapter *adapter = dvbdmx->priv;

	if (!adapter->feeds)
		tbsecp3_dma_enable(adapter);

	return ++adapter->feeds;
}

static int stop_feed(struct dvb_demux_feed *dvbdmxfeed)
{
	struct dvb_demux *dvbdmx = dvbdmxfeed->demux;
	struct tbsecp3_adapter *adapter = dvbdmx->priv;

	if (--adapter->feeds)
		return adapter->feeds;

	tbsecp3_dma_disable(adapter);
	return 0;
}

static void reset_demod(struct tbsecp3_adapter *adapter)
{
	struct tbsecp3_dev *dev = adapter->dev;
	struct tbsecp3_gpio_config *cfg = &adapter->cfg->gpio;
	int rst;

	if (cfg->demod_reset_lvl == 0)
		return;

	if (cfg->demod_reset_lvl == TBSECP3_GPIODEF_LOW)
		rst = 0;
	else
		rst = 1;

	tbsecp3_gpio_set_pin(dev, cfg->demod_reset_pin, rst);
	usleep_range(10000, 20000);

	tbsecp3_gpio_set_pin(dev, cfg->demod_reset_pin, 1 - rst);
	usleep_range(50000, 100000);
}


#define TBS6904_LNBPWR_PIN	(2)

static void tbs6904_lnb_power(struct dvb_frontend *fe,
	int gpio, int onoff)
{
	struct i2c_adapter *adapter = tas2101_get_i2c_adapter(fe, 0);
        struct tbsecp3_i2c *i2c = i2c_get_adapdata(adapter);
	struct tbsecp3_dev *dev = i2c->dev;

	/* lnb power, active low */
	if (onoff)
		tbsecp3_gpio_set_pin(dev, TBSECP3_GPIO_PIN(gpio, TBS6904_LNBPWR_PIN), 0);
	else
		tbsecp3_gpio_set_pin(dev, TBSECP3_GPIO_PIN(gpio, TBS6904_LNBPWR_PIN), 1);
}

static void tbs6904_lnb0_power(struct dvb_frontend *fe, int onoff)
{
	tbs6904_lnb_power(fe, 0, onoff);
}

static void tbs6904_lnb1_power(struct dvb_frontend *fe, int onoff)
{
	tbs6904_lnb_power(fe, 1, onoff);
}

static void tbs6904_lnb2_power(struct dvb_frontend *fe, int onoff)
{
	tbs6904_lnb_power(fe, 2, onoff);
}

static void tbs6904_lnb3_power(struct dvb_frontend *fe, int onoff)
{
	tbs6904_lnb_power(fe, 3, onoff);
}

static struct tas2101_config tbs6904_demod_cfg[] = {
	{
		.i2c_address   = 0x68,
		.id            = ID_TAS2101,
		.lnb_power     = tbs6904_lnb0_power,
		.init          = {0xb0, 0x32, 0x81, 0x57, 0x64, 0x9a, 0x33}, // 0xb1
		.init2         = 0,
	},
	{
		.i2c_address   = 0x60,
		.id            = ID_TAS2101,
		.lnb_power     = tbs6904_lnb1_power,
		.init          = {0xb0, 0x32, 0x81, 0x57, 0x64, 0x9a, 0x33},
		.init2         = 0,
	},
	{
		.i2c_address   = 0x68,
		.id            = ID_TAS2101,
		.lnb_power     = tbs6904_lnb2_power,
		.init          = {0xb0, 0x32, 0x81, 0x57, 0x64, 0x9a, 0x33},
		.init2         = 0,
	},
	{
		.i2c_address   = 0x60,
		.id            = ID_TAS2101,
		.lnb_power     = tbs6904_lnb3_power,
		.init          = {0xb0, 0x32, 0x81, 0x57, 0x64, 0x9a, 0x33},
		.init2         = 0,
	}
};

static struct av201x_config tbs6904_av201x_cfg = {
	.i2c_address = 0x63,
	.id          = ID_AV2012,
	.xtal_freq   = 27000,		/* kHz */
};

#if 0
static int max_set_voltage(struct i2c_adapter *i2c,
		enum fe_sec_voltage voltage, u8 rf_in)
{
	struct tbsecp3_i2c *i2c_adap = i2c_get_adapdata(i2c);
	struct tbsecp3_dev *dev = i2c_adap->dev;

	u32 val, reg;

	//printk("set voltage on %u = %d\n", rf_in, voltage);
	
	if (rf_in > 3)
		return -EINVAL;

	reg = rf_in * 4;
	val = tbs_read(TBSECP3_GPIO_BASE, reg) & ~4;

	switch (voltage) {
	case SEC_VOLTAGE_13:
		val &= ~2;
		break;
	case SEC_VOLTAGE_18:
		val |= 2;
		break;
	case SEC_VOLTAGE_OFF:
	default:
		val |= 4;
		break;
	}

	tbs_write(TBSECP3_GPIO_BASE, reg, val);
	return 0;
}

static int max_send_master_cmd(struct dvb_frontend *fe, struct dvb_diseqc_master_cmd *cmd)
{
	//printk("send master cmd\n");
	return 0;
}
static int max_send_burst(struct dvb_frontend *fe, enum fe_sec_mini_cmd burst)
{
	//printk("send burst: %d\n", burst);
	return 0;
}

static struct mxl5xx_cfg tbs6909_mxl5xx_cfg = {
	.adr		= 0x60,
	.type		= 0x01,
	.clk		= 24000000,
	.cap		= 12,
	.fw_read	= NULL,

	.set_voltage	= max_set_voltage,
};

static struct stv0910_cfg tbs6903_stv0910_cfg = {
	.adr      = 0x68,
	.parallel = 1,
	.rptlvl   = 4,
	.clk      = 30000000,

	.set_voltage = max_set_voltage,
};

struct stv6120_cfg tbs6903_stv6120_cfg = {
	.adr      = 0x60,
	.Rdiv     = 2,
	.xtal     = 30000,
};
#endif

static int tbsecp3_set_mac(struct tbsecp3_adapter *adap)
{
	struct tbsecp3_dev *dev = adap->dev;
	int eeprom_bus_nr = dev->info->eeprom_i2c;
	struct i2c_adapter *i2c = &dev->i2c_bus[eeprom_bus_nr].i2c_adap;
	u8 eep_addr;
	int ret;

	struct i2c_msg msg[] = {
		{ .addr = 0x50, .flags = 0,
		  .buf = &eep_addr, .len = 1 },
		{ .addr = 0x50, .flags = I2C_M_RD,
		  .buf = adap->dvb_adapter.proposed_mac, .len = 6 }
	};

	eep_addr = 0xa0 + 0x10 * adap->nr;
	ret = i2c_transfer(i2c, msg, 2);
	if (ret != 2) {
		dev_warn(&dev->pci_dev->dev,
			"error reading MAC address for adapter %d\n",
			adap->nr);
	} else {
		dev_info(&dev->pci_dev->dev,
			"MAC address %pM\n", adap->dvb_adapter.proposed_mac);
	}
	return 0;
};


static int tbsecp3_frontend_attach(struct tbsecp3_adapter *adapter)
{
	struct tbsecp3_dev *dev = adapter->dev;
	struct pci_dev *pci = dev->pci_dev;

	struct si2168_config si2168_config;
	struct si2157_config si2157_config;

	//struct av201x_config av201x_config;

	struct i2c_board_info info;
	struct i2c_adapter *i2c = &adapter->i2c->i2c_adap;
	struct i2c_client *client_demod, *client_tuner;


	adapter->fe = NULL;
	adapter->i2c_client_demod = NULL;
	adapter->i2c_client_tuner = NULL;

	reset_demod(adapter);

	tbsecp3_set_mac(adapter);

	switch (pci->subsystem_vendor) {
	case 0x6205:
		/* attach demod */
		memset(&si2168_config, 0, sizeof(si2168_config));
		si2168_config.i2c_adapter = &i2c;
		si2168_config.fe = &adapter->fe;
		si2168_config.ts_mode = SI2168_TS_PARALLEL;
		si2168_config.ts_clock_gapped = true;

		memset(&info, 0, sizeof(struct i2c_board_info));
		strlcpy(info.type, "si2168", I2C_NAME_SIZE);
		info.addr = 0x64;
		info.platform_data = &si2168_config;
		request_module(info.type);
		client_demod = i2c_new_device(i2c, &info);
		if (client_demod == NULL ||
				client_demod->dev.driver == NULL)
			goto frontend_atach_fail;
		if (!try_module_get(client_demod->dev.driver->owner)) {
			i2c_unregister_device(client_demod);
			goto frontend_atach_fail;
		}
		adapter->i2c_client_demod = client_demod;

		/* attach tuner */
		memset(&si2157_config, 0, sizeof(si2157_config));
		si2157_config.fe = adapter->fe;
		si2157_config.if_port = 1;

		memset(&info, 0, sizeof(struct i2c_board_info));
		strlcpy(info.type, "si2157", I2C_NAME_SIZE);
		info.addr = 0x60;
		info.platform_data = &si2157_config;
		request_module(info.type);
		client_tuner = i2c_new_device(i2c, &info);
		if (client_tuner == NULL ||
				client_tuner->dev.driver == NULL)
			goto frontend_atach_fail;

		if (!try_module_get(client_tuner->dev.driver->owner)) {
			i2c_unregister_device(client_tuner);
			goto frontend_atach_fail;
		}
		adapter->i2c_client_tuner = client_tuner;
		break;
#if 0
	case 0x6903:
		adapter->fe = dvb_attach(stv0910_attach, i2c,
				&tbs6903_stv0910_cfg, adapter->nr & 1);
		if (adapter->fe == NULL)
			goto frontend_atach_fail;

		if (dvb_attach(stv6120_attach, adapter->fe, i2c, &tbs6903_stv6120_cfg) == NULL) {
			pr_err("No STV6120 found at 0x%02x!\n", 0x60);
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			dev_err(&dev->pci_dev->dev,
				"TBS_PCIE frontend %d tuner attach failed\n",
				adapter->nr);
			goto frontend_atach_fail;
		}

		break;
#endif
	case 0x6904:
		adapter->fe = dvb_attach(tas2101_attach, &tbs6904_demod_cfg[adapter->nr], i2c);
		if (adapter->fe == NULL)
			goto frontend_atach_fail;

		if (dvb_attach(av201x_attach, adapter->fe, &tbs6904_av201x_cfg,
				tas2101_get_i2c_adapter(adapter->fe, 2)) == NULL) {
			dvb_frontend_detach(adapter->fe);
			adapter->fe = NULL;
			dev_err(&dev->pci_dev->dev,
				"TBS_PCIE frontend %d tuner attach failed\n",
				adapter->nr);
			goto frontend_atach_fail;
		}


#if 0
		/* attach tuner */
		memset(&av201x_config, 0, sizeof(av201x_config));
		av201x_config.fe = adapter->fe;
		av201x_config.xtal_freq = 27000,

		memset(&info, 0, sizeof(struct i2c_board_info));
		strlcpy(info.type, "av2012", I2C_NAME_SIZE);
		info.addr = 0x63;
		info.platform_data = &av201x_config;
		request_module(info.type);
		client_tuner = i2c_new_device(i2c, &info);
		printk("cli_tu=%p\n", client_tuner);
		if (client_tuner != NULL)
			printk("cli_tu.drv=%p\n", client_tuner->dev.driver);
		if (client_tuner == NULL ||
				client_tuner->dev.driver == NULL) {
			printk("client tunner fail 1\n");
			goto frontend_atach_fail;
		}

		if (!try_module_get(client_tuner->dev.driver->owner)) {
			i2c_unregister_device(client_tuner);
			printk("client tunner fail 2\n");
			goto frontend_atach_fail;
		}
		adapter->i2c_client_tuner = client_tuner;
#endif
		break;
#if 0
	case 0x6909:
/*
		tmp = tbs_read(TBS_GPIO_BASE, 0x20);
		printk("RD 0x20 = %x\n", tmp);
		tbs_write(TBS_GPIO_BASE, 0x20, tmp & 0xfffe);
		tmp = tbs_read(TBS_GPIO_BASE, 0x20);
		printk("RD 0x20 = %x\n", tmp);

		tmp = tbs_read(TBS_GPIO_BASE, 0x24);
		printk("RD 0x24 = %x\n", tmp);
		tbs_write(TBS_GPIO_BASE, 0x24, tmp & 0xfffc);
		tmp = tbs_read(TBS_GPIO_BASE, 0x24);
		printk("RD 0x24 = %x\n", tmp);
*/

		adapter->fe = dvb_attach(mxl5xx_attach, i2c,
				&tbs6909_mxl5xx_cfg, adapter->nr);
		if (adapter->fe == NULL)
			goto frontend_atach_fail;

		adapter->fe->ops.diseqc_send_master_cmd = max_send_master_cmd;
		adapter->fe->ops.diseqc_send_burst = max_send_burst;

		break;
#endif
	default:
		dev_warn(&dev->pci_dev->dev, "unknonw card\n");
		return -ENODEV;
		break;
	}

	return 0;

frontend_atach_fail:
	tbsecp3_i2c_remove_clients(adapter);
	if (adapter->fe != NULL)
		dvb_frontend_detach(adapter->fe);
	adapter->fe = NULL;
	dev_err(&dev->pci_dev->dev, "TBSECP3 frontend %d attach failed\n",
		adapter->nr);

	return -ENODEV;
}

int tbsecp3_dvb_init(struct tbsecp3_adapter *adapter)
{
	struct tbsecp3_dev *dev = adapter->dev;
	struct dvb_adapter *adap = &adapter->dvb_adapter;
	struct dvb_demux *dvbdemux = &adapter->demux;
	struct dmxdev *dmxdev;
	struct dmx_frontend *fe_hw;
	struct dmx_frontend *fe_mem;
	int ret;

	ret = dvb_register_adapter(adap, "TBSECP3 DVB Adapter",
					THIS_MODULE,
					&adapter->dev->pci_dev->dev,
					adapter_nr);
	if (ret < 0) {
		dev_err(&dev->pci_dev->dev, "error registering adapter\n");
		if (ret == -ENFILE)
			dev_err(&dev->pci_dev->dev,
				"increase DVB_MAX_ADAPTERS (%d)\n",
				DVB_MAX_ADAPTERS);
		return ret;
	}

	adap->priv = adapter;
	dvbdemux->priv = adapter;
	dvbdemux->filternum = 256;
	dvbdemux->feednum = 256;
	dvbdemux->start_feed = start_feed;
	dvbdemux->stop_feed = stop_feed;
	dvbdemux->write_to_decoder = NULL;
	dvbdemux->dmx.capabilities = (DMX_TS_FILTERING |
				      DMX_SECTION_FILTERING |
				      DMX_MEMORY_BASED_FILTERING);

	ret = dvb_dmx_init(dvbdemux);
	if (ret < 0) {
		dev_err(&dev->pci_dev->dev, "dvb_dmx_init failed\n");
		goto err0;
	}

	dmxdev = &adapter->dmxdev;

	dmxdev->filternum = 256;
	dmxdev->demux = &dvbdemux->dmx;
	dmxdev->capabilities = 0;

	ret = dvb_dmxdev_init(dmxdev, adap);
	if (ret < 0) {
		dev_err(&dev->pci_dev->dev, "dvb_dmxdev_init failed\n");
		goto err1;
	}

	fe_hw = &adapter->fe_hw;
	fe_mem = &adapter->fe_mem;

	fe_hw->source = DMX_FRONTEND_0;
	ret = dvbdemux->dmx.add_frontend(&dvbdemux->dmx, fe_hw);
	if ( ret < 0) {
		dev_err(&dev->pci_dev->dev, "dvb_dmx_init failed");
		goto err2;
	}

	fe_mem->source = DMX_MEMORY_FE;
	ret = dvbdemux->dmx.add_frontend(&dvbdemux->dmx, fe_mem);
	if (ret  < 0) {
		dev_err(&dev->pci_dev->dev, "dvb_dmx_init failed");
		goto err3;
	}

	ret = dvbdemux->dmx.connect_frontend(&dvbdemux->dmx, fe_hw);
	if (ret < 0) {
		dev_err(&dev->pci_dev->dev, "dvb_dmx_init failed");
		goto err4;
	}

	ret = dvb_net_init(adap, &adapter->dvbnet, adapter->dmxdev.demux);
	if (ret < 0) {
		dev_err(&dev->pci_dev->dev, "dvb_net_init failed");
		goto err5;
	}

	tbsecp3_frontend_attach(adapter);
	if (adapter->fe == NULL) {
		dev_err(&dev->pci_dev->dev, "frontend attach failed\n");
		ret = -ENODEV;
		goto err6;
	}

	ret = dvb_register_frontend(adap, adapter->fe);
	if (ret < 0) {
		dev_err(&dev->pci_dev->dev, "frontend register failed\n");
		goto err7;
	}

	return ret;

err7:
	dvb_frontend_detach(adapter->fe);
err6:
	dvb_net_release(&adapter->dvbnet);
err5:
	dvbdemux->dmx.close(&dvbdemux->dmx);
err4:
	dvbdemux->dmx.remove_frontend(&dvbdemux->dmx, fe_mem);
err3:
	dvbdemux->dmx.remove_frontend(&dvbdemux->dmx, fe_hw);
err2:
	dvb_dmxdev_release(dmxdev);
err1:
	dvb_dmx_release(dvbdemux);
err0:
	dvb_unregister_adapter(adap);
	return ret;
}

void tbsecp3_dvb_exit(struct tbsecp3_adapter *adapter)
{
	struct dvb_adapter *adap = &adapter->dvb_adapter;
	struct dvb_demux *dvbdemux = &adapter->demux;

	if (adapter->fe) {
		dvb_unregister_frontend(adapter->fe);
		dvb_frontend_detach(adapter->fe);
		adapter->fe = NULL;
	}
	dvb_net_release(&adapter->dvbnet);
	dvbdemux->dmx.close(&dvbdemux->dmx);
	dvbdemux->dmx.remove_frontend(&dvbdemux->dmx, &adapter->fe_mem);
	dvbdemux->dmx.remove_frontend(&dvbdemux->dmx, &adapter->fe_hw);
	dvb_dmxdev_release(&adapter->dmxdev);
	dvb_dmx_release(&adapter->demux);
	dvb_unregister_adapter(adap);
}
