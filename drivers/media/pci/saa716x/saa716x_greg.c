#include <linux/kernel.h>

#include "saa716x_mod.h"

#include "saa716x_greg_reg.h"
#include "saa716x_greg.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

static u32 g_save[12];

void saa716x_greg_save(struct saa716x_dev *saa716x)
{
	g_save[0] = SAA716x_EPRD(GREG, GREG_SUBSYS_CONFIG);
	g_save[1] = SAA716x_EPRD(GREG, GREG_MSI_BAR_PMCSR);
	g_save[2] = SAA716x_EPRD(GREG, GREG_PMCSR_DATA_1);
	g_save[3] = SAA716x_EPRD(GREG, GREG_PMCSR_DATA_2);
	g_save[4] = SAA716x_EPRD(GREG, GREG_VI_CTRL);
	g_save[5] = SAA716x_EPRD(GREG, GREG_FGPI_CTRL);
	g_save[6] = SAA716x_EPRD(GREG, GREG_RSTU_CTRL);
	g_save[7] = SAA716x_EPRD(GREG, GREG_I2C_CTRL);
	g_save[8] = SAA716x_EPRD(GREG, GREG_OVFLW_CTRL);
	g_save[9] = SAA716x_EPRD(GREG, GREG_TAG_ACK_FLEN);

	g_save[10] = SAA716x_EPRD(GREG, GREG_VIDEO_IN_CTRL);
}

void saa716x_greg_restore(struct saa716x_dev *saa716x)
{
	SAA716x_EPWR(GREG, GREG_SUBSYS_CONFIG, g_save[0]);
	SAA716x_EPWR(GREG, GREG_MSI_BAR_PMCSR, g_save[1]);
	SAA716x_EPWR(GREG, GREG_PMCSR_DATA_1, g_save[2]);
	SAA716x_EPWR(GREG, GREG_PMCSR_DATA_2, g_save[3]);
	SAA716x_EPWR(GREG, GREG_VI_CTRL, g_save[4]);
	SAA716x_EPWR(GREG, GREG_FGPI_CTRL, g_save[5]);
	SAA716x_EPWR(GREG, GREG_RSTU_CTRL, g_save[6]);
	SAA716x_EPWR(GREG, GREG_I2C_CTRL, g_save[7]);
	SAA716x_EPWR(GREG, GREG_OVFLW_CTRL, g_save[8]);
	SAA716x_EPWR(GREG, GREG_TAG_ACK_FLEN, g_save[9]);

	SAA716x_EPWR(GREG, GREG_VIDEO_IN_CTRL, g_save[10]);
}
