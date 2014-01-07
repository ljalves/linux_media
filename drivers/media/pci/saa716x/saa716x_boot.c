#include <linux/delay.h>

#include "saa716x_mod.h"

#include "saa716x_greg_reg.h"
#include "saa716x_cgu_reg.h"
#include "saa716x_vip_reg.h"
#include "saa716x_aip_reg.h"
#include "saa716x_msi_reg.h"
#include "saa716x_dma_reg.h"
#include "saa716x_gpio_reg.h"
#include "saa716x_fgpi_reg.h"
#include "saa716x_dcs_reg.h"

#include "saa716x_boot.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

static int saa716x_ext_boot(struct saa716x_dev *saa716x)
{
	/* Write GREG boot_ready to 0
	 * DW_0 = 0x0001_2018
	 * DW_1 = 0x0000_0000
	 */
	SAA716x_EPWR(GREG, GREG_RSTU_CTRL, 0x00000000);

	/* Clear VI0 interrupt
	 * DW_2 = 0x0000_0fe8
	 * DW_3 = 0x0000_03ff
	 */
	SAA716x_EPWR(VI0, INT_CLR_STATUS, 0x000003ff);

	/* Clear VI1 interrupt
	 * DW_4 = 0x0000_1fe8
	 * DW_5 = 0x0000_03ff
	 */
	SAA716x_EPWR(VI1, INT_CLR_STATUS, 0x000003ff);

	/* CLear FGPI0 interrupt
	 * DW_6 = 0x0000_2fe8
	 * DW_7 = 0x0000_007f
	 */
	SAA716x_EPWR(FGPI0, INT_CLR_STATUS, 0x0000007f);

	/* Clear FGPI1 interrupt
	 * DW_8 = 0x0000_3fe8
	 * DW_9 = 0x0000_007f
	 */
	SAA716x_EPWR(FGPI1, INT_CLR_STATUS, 0x0000007f);

	/* Clear FGPI2 interrupt
	 * DW_10 = 0x0000_4fe8
	 * DW_11 = 0x0000_007f
	 */
	SAA716x_EPWR(FGPI2, INT_CLR_STATUS, 0x0000007f);

	/* Clear FGPI3 interrupt
	 * DW_12 = 0x0000_5fe8
	 * DW_13 = 0x0000_007f
	 */
	SAA716x_EPWR(FGPI3, INT_CLR_STATUS, 0x0000007f);

	/* Clear AI0 interrupt
	 * DW_14 = 0x0000_6020
	 * DW_15 = 0x0000_000f
	 */
	SAA716x_EPWR(AI0, AI_INT_ACK, 0x0000000f);

	/* Clear AI1 interrupt
	 * DW_16 = 0x0000_7020
	 * DW_17 = 0x0000_200f
	 */
	SAA716x_EPWR(AI1, AI_INT_ACK, 0x0000000f);

	/* Set GREG boot_ready bit to 1
	 * DW_18 = 0x0001_2018
	 * DW_19 = 0x0000_2000
	 */
	SAA716x_EPWR(GREG, GREG_RSTU_CTRL, 0x00002000);
#if 0
	/* End of Boot script command
	 * DW_20 = 0x0000_0006
	 * Where to write this value ??
	 * This seems very odd an address to trigger the
	 * Boot Control State Machine !
	 */
	SAA716x_EPWR(VI0, 0x00000006, 0xffffffff);
#endif
	return 0;
}

/* Internal Bootscript configuration */
static void saa716x_int_boot(struct saa716x_dev *saa716x)
{
	/* #1 Configure PCI COnfig space
	 * GREG_JETSTR_CONFIG_0
	 */
	SAA716x_EPWR(GREG, GREG_SUBSYS_CONFIG, saa716x->pdev->subsystem_vendor);

	/* GREG_JETSTR_CONFIG_1
	 * pmcsr_scale:7 = 0x00
	 * pmcsr_scale:6 = 0x00
	 * pmcsr_scale:5 = 0x00
	 * pmcsr_scale:4 = 0x00
	 * pmcsr_scale:3 = 0x00
	 * pmcsr_scale:2 = 0x00
	 * pmcsr_scale:1 = 0x00
	 * pmcsr_scale:0 = 0x00
	 * BAR mask = 20 bit
	 * BAR prefetch = no
	 * MSI capable = 32 messages
	 */
	SAA716x_EPWR(GREG, GREG_MSI_BAR_PMCSR, 0x00001005);

	/* GREG_JETSTR_CONFIG_2
	 * pmcsr_data:3 = 0x0
	 * pmcsr_data:2 = 0x0
	 * pmcsr_data:1 = 0x0
	 * pmcsr_data:0 = 0x0
	 */
	SAA716x_EPWR(GREG, GREG_PMCSR_DATA_1, 0x00000000);

	/* GREG_JETSTR_CONFIG_3
	 * pmcsr_data:7 = 0x0
	 * pmcsr_data:6 = 0x0
	 * pmcsr_data:5 = 0x0
	 * pmcsr_data:4 = 0x0
	 */
	SAA716x_EPWR(GREG, GREG_PMCSR_DATA_2, 0x00000000);

	/* #2 Release GREG resets
	 * ip_rst_an
	 * dpa1_rst_an
	 * jetsream_reset_an
	 */
	SAA716x_EPWR(GREG, GREG_RSTU_CTRL, 0x00000e00);

	/* #3 GPIO Setup
	 * GPIO 25:24 = Output
	 * GPIO Output "0" after Reset
	 */
	SAA716x_EPWR(GPIO, GPIO_OEN, 0xfcffffff);

	/* #4 Custom stuff goes in here */

	/* #5 Disable CGU Clocks
	 * except for PHY, Jetstream, DPA1, DCS, Boot, GREG
	 * CGU_PCR_0_3: pss_mmu_clk:0 = 0x0
	 */
	SAA716x_EPWR(CGU, CGU_PCR_0_3, 0x00000006);

	/* CGU_PCR_0_4: pss_dtl2mtl_mmu_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_0_4, 0x00000006);

	/* CGU_PCR_0_5: pss_msi_ck:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_0_5, 0x00000006);

	/* CGU_PCR_0_7: pss_gpio_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_0_7, 0x00000006);

	/* CGU_PCR_2_1: spi_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_2_1, 0x00000006);

	/* CGU_PCR_3_2: i2c_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_3_2, 0x00000006);

	/* CGU_PCR_4_1: phi_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_4_1, 0x00000006);

	/* CGU_PCR_5: vip0_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_5, 0x00000006);

	/* CGU_PCR_6: vip1_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_6, 0x00000006);

	/* CGU_PCR_7: fgpi0_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_7, 0x00000006);

	/* CGU_PCR_8: fgpi1_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_8, 0x00000006);

	/* CGU_PCR_9: fgpi2_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_9, 0x00000006);

	/* CGU_PCR_10: fgpi3_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_10, 0x00000006);

	/* CGU_PCR_11: ai0_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_11, 0x00000006);

	/* CGU_PCR_12: ai1_clk:0 = 0x0 */
	SAA716x_EPWR(CGU, CGU_PCR_12, 0x00000006);

	/* #6 Set GREG boot_ready = 0x1 */
	SAA716x_EPWR(GREG, GREG_RSTU_CTRL, 0x00002000);

	/* #7 Disable GREG CGU Clock */
	SAA716x_EPWR(CGU, CGU_PCR_0_6, 0x00000006);

	/* End of Bootscript command ?? */
}

int saa716x_core_boot(struct saa716x_dev *saa716x)
{
	struct saa716x_config *config = saa716x->config;

	switch (config->boot_mode) {
	case SAA716x_EXT_BOOT:
		dprintk(SAA716x_DEBUG, 1, "Using External Boot from config");
		saa716x_ext_boot(saa716x);
		break;
	case SAA716x_INT_BOOT:
		dprintk(SAA716x_DEBUG, 1, "Using Internal Boot from config");
		saa716x_int_boot(saa716x);
		break;
	default:
		dprintk(SAA716x_ERROR, 1, "Unknown configuration %d", config->boot_mode);
		break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_core_boot);

static void saa716x_bus_report(struct pci_dev *pdev, int enable)
{
	u32 reg;

	pci_read_config_dword(pdev, 0x04, &reg);
	if (enable)
		reg |= 0x00000100; /* enable SERR */
	else
		reg &= 0xfffffeff; /* disable SERR */
	pci_write_config_dword(pdev, 0x04, reg);

	pci_read_config_dword(pdev, 0x58, &reg);
	reg &= 0xfffffffd;
	pci_write_config_dword(pdev, 0x58, reg);
}

int saa716x_jetpack_init(struct saa716x_dev *saa716x)
{
	/*
	 * configure PHY through config space not to report
	 * non-fatal error messages to avoid problems with
	 * quirky BIOS'es
	 */
	saa716x_bus_report(saa716x->pdev, 0);

	/*
	 * create time out for blocks that have no clock
	 * helps with lower bitrates on FGPI
	 */
	SAA716x_EPWR(DCS, DCSC_CTRL, ENABLE_TIMEOUT);

	/* Reset all blocks */
	SAA716x_EPWR(MSI, MSI_SW_RST, MSI_SW_RESET);
	SAA716x_EPWR(MMU, MMU_SW_RST, MMU_SW_RESET);
	SAA716x_EPWR(BAM, BAM_SW_RST, BAM_SW_RESET);

	switch (saa716x->pdev->device) {
	case SAA7162:
		dprintk(SAA716x_DEBUG, 1, "SAA%02x Decoder disable", saa716x->pdev->device);
		SAA716x_EPWR(GPIO, GPIO_OEN, 0xfcffffff);
		SAA716x_EPWR(GPIO, GPIO_WR,  0x00000000); /* Disable decoders */
		msleep(10);
		SAA716x_EPWR(GPIO, GPIO_WR,  0x03000000); /* Enable decoders */
		break;
	case SAA7161:
		dprintk(SAA716x_DEBUG, 1, "SAA%02x Decoder disable", saa716x->pdev->device);
		SAA716x_EPWR(GPIO, GPIO_OEN, 0xfeffffff);
		SAA716x_EPWR(GPIO, GPIO_WR,  0x00000000); /* Disable decoders */
		msleep(10);
		SAA716x_EPWR(GPIO, GPIO_WR,  0x01000000); /* Enable decoder */
		break;
	case SAA7160:
		saa716x->i2c_rate = SAA716x_I2C_RATE_100;
		break;
	default:
		dprintk(SAA716x_ERROR, 1, "Unknown device (0x%02x)", saa716x->pdev->device);
		return -ENODEV;
	}

	/* General setup for MMU */
	SAA716x_EPWR(MMU, MMU_MODE, 0x14);
	dprintk(SAA716x_DEBUG, 1, "SAA%02x Jetpack Successfully initialized", saa716x->pdev->device);

	return 0;
}
EXPORT_SYMBOL(saa716x_jetpack_init);

void saa716x_core_reset(struct saa716x_dev *saa716x)
{
	dprintk(SAA716x_DEBUG, 1, "RESET Modules");

	/* VIP */
	SAA716x_EPWR(VI0, VI_MODE, SOFT_RESET);
	SAA716x_EPWR(VI1, VI_MODE, SOFT_RESET);

	/* FGPI */
	SAA716x_EPWR(FGPI0, FGPI_SOFT_RESET, FGPI_SOFTWARE_RESET);
	SAA716x_EPWR(FGPI1, FGPI_SOFT_RESET, FGPI_SOFTWARE_RESET);
	SAA716x_EPWR(FGPI2, FGPI_SOFT_RESET, FGPI_SOFTWARE_RESET);
	SAA716x_EPWR(FGPI3, FGPI_SOFT_RESET, FGPI_SOFTWARE_RESET);

	/* AIP */
	SAA716x_EPWR(AI0, AI_CTL, AI_RESET);
	SAA716x_EPWR(AI1, AI_CTL, AI_RESET);

	/* BAM */
	SAA716x_EPWR(BAM, BAM_SW_RST, BAM_SW_RESET);

	/* MMU */
	SAA716x_EPWR(MMU, MMU_SW_RST, MMU_SW_RESET);

	/* MSI */
	SAA716x_EPWR(MSI, MSI_SW_RST, MSI_SW_RESET);
}
EXPORT_SYMBOL_GPL(saa716x_core_reset);
