#include <linux/delay.h>

#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/interrupt.h>

#include "saa716x_mod.h"

#include "saa716x_msi_reg.h"
#include "saa716x_msi.h"
#include "saa716x_spi.h"

#include "saa716x_priv.h"

#define SAA716x_MSI_VECTORS		50

static const char *vector_name[] = {
	"TAGACK_VI0_0",
	"TAGACK_VI0_1",
	"TAGACK_VI0_2",
	"TAGACK_VI1_0",
	"TAGACK_VI1_1",
	"TAGACK_VI1_2",
	"TAGACK_FGPI_0",
	"TAGACK_FGPI_1",
	"TAGACK_FGPI_2",
	"TAGACK_FGPI_3",
	"TAGACK_AI_0",
	"TAGACK_AI_1",
	"OVRFLW_VI0_0",
	"OVRFLW_VI0_1",
	"OVRFLW_VI0_2",
	"OVRFLW_VI1_0",
	"OVRFLW_VI1_1",
	"OVRFLW_VI1_2",
	"OVRFLW_FGPI_O",
	"OVRFLW_FGPI_1",
	"OVRFLW_FGPI_2",
	"OVRFLW_FGPI_3",
	"OVRFLW_AI_0",
	"OVRFLW_AI_1",
	"AVINT_VI0",
	"AVINT_VI1",
	"AVINT_FGPI_0",
	"AVINT_FGPI_1",
	"AVINT_FGPI_2",
	"AVINT_FGPI_3",
	"AVINT_AI_0",
	"AVINT_AI_1",
	"UNMAPD_TC_INT",
	"EXTINT_0",
	"EXTINT_1",
	"EXTINT_2",
	"EXTINT_3",
	"EXTINT_4",
	"EXTINT_5",
	"EXTINT_6",
	"EXTINT_7",
	"EXTINT_8",
	"EXTINT_9",
	"EXTINT_10",
	"EXTINT_11",
	"EXTINT_12",
	"EXTINT_13",
	"EXTINT_14",
	"EXTINT_15",
	"I2CINT_0",
	"I2CINT_1"
};

static u32 MSI_CONFIG_REG[51] = {
	MSI_CONFIG0,
	MSI_CONFIG1,
	MSI_CONFIG2,
	MSI_CONFIG3,
	MSI_CONFIG4,
	MSI_CONFIG5,
	MSI_CONFIG6,
	MSI_CONFIG7,
	MSI_CONFIG8,
	MSI_CONFIG9,
	MSI_CONFIG10,
	MSI_CONFIG11,
	MSI_CONFIG12,
	MSI_CONFIG13,
	MSI_CONFIG14,
	MSI_CONFIG15,
	MSI_CONFIG16,
	MSI_CONFIG17,
	MSI_CONFIG18,
	MSI_CONFIG19,
	MSI_CONFIG20,
	MSI_CONFIG21,
	MSI_CONFIG22,
	MSI_CONFIG23,
	MSI_CONFIG24,
	MSI_CONFIG25,
	MSI_CONFIG26,
	MSI_CONFIG27,
	MSI_CONFIG28,
	MSI_CONFIG29,
	MSI_CONFIG30,
	MSI_CONFIG31,
	MSI_CONFIG32,
	MSI_CONFIG33,
	MSI_CONFIG34,
	MSI_CONFIG35,
	MSI_CONFIG36,
	MSI_CONFIG37,
	MSI_CONFIG38,
	MSI_CONFIG39,
	MSI_CONFIG40,
	MSI_CONFIG41,
	MSI_CONFIG42,
	MSI_CONFIG43,
	MSI_CONFIG44,
	MSI_CONFIG45,
	MSI_CONFIG46,
	MSI_CONFIG47,
	MSI_CONFIG48,
	MSI_CONFIG49,
	MSI_CONFIG50
};

int saa716x_msi_event(struct saa716x_dev *saa716x, u32 stat_l, u32 stat_h)
{
	dprintk(SAA716x_DEBUG, 0, "%s: MSI event ", __func__);

	if (stat_l & MSI_INT_TAGACK_VI0_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[0]);

	if (stat_l & MSI_INT_TAGACK_VI0_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[1]);

	if (stat_l & MSI_INT_TAGACK_VI0_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[2]);

	if (stat_l & MSI_INT_TAGACK_VI1_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[3]);

	if (stat_l & MSI_INT_TAGACK_VI1_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[4]);

	if (stat_l & MSI_INT_TAGACK_VI1_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[5]);

	if (stat_l & MSI_INT_TAGACK_FGPI_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[6]);

	if (stat_l & MSI_INT_TAGACK_FGPI_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[7]);

	if (stat_l & MSI_INT_TAGACK_FGPI_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[8]);

	if (stat_l & MSI_INT_TAGACK_FGPI_3)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[9]);

	if (stat_l & MSI_INT_TAGACK_AI_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[10]);

	if (stat_l & MSI_INT_TAGACK_AI_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[11]);

	if (stat_l & MSI_INT_OVRFLW_VI0_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[12]);

	if (stat_l & MSI_INT_OVRFLW_VI0_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[13]);

	if (stat_l & MSI_INT_OVRFLW_VI0_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[14]);

	if (stat_l & MSI_INT_OVRFLW_VI1_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[15]);

	if (stat_l & MSI_INT_OVRFLW_VI1_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[16]);

	if (stat_l & MSI_INT_OVRFLW_VI1_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[17]);

	if (stat_l & MSI_INT_OVRFLW_FGPI_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[18]);

	if (stat_l & MSI_INT_OVRFLW_FGPI_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[19]);

	if (stat_l & MSI_INT_OVRFLW_FGPI_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[20]);

	if (stat_l & MSI_INT_OVRFLW_FGPI_3)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[21]);

	if (stat_l & MSI_INT_OVRFLW_AI_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[22]);

	if (stat_l & MSI_INT_OVRFLW_AI_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[23]);

	if (stat_l & MSI_INT_AVINT_VI0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[24]);

	if (stat_l & MSI_INT_AVINT_VI1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[25]);

	if (stat_l & MSI_INT_AVINT_FGPI_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[26]);

	if (stat_l & MSI_INT_AVINT_FGPI_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[27]);

	if (stat_l & MSI_INT_AVINT_FGPI_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[28]);

	if (stat_l & MSI_INT_AVINT_FGPI_3)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[29]);

	if (stat_l & MSI_INT_AVINT_AI_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[30]);

	if (stat_l & MSI_INT_AVINT_AI_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[31]);

	if (stat_h & MSI_INT_UNMAPD_TC_INT)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[32]);

	if (stat_h & MSI_INT_EXTINT_0)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[33]);

	if (stat_h & MSI_INT_EXTINT_1)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[34]);

	if (stat_h & MSI_INT_EXTINT_2)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[35]);

	if (stat_h & MSI_INT_EXTINT_3)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[36]);

	if (stat_h & MSI_INT_EXTINT_4)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[37]);

	if (stat_h & MSI_INT_EXTINT_5)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[38]);

	if (stat_h & MSI_INT_EXTINT_6)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[39]);

	if (stat_h & MSI_INT_EXTINT_7)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[40]);

	if (stat_h & MSI_INT_EXTINT_8)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[41]);

	if (stat_h & MSI_INT_EXTINT_9)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[42]);

	if (stat_h & MSI_INT_EXTINT_10)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[43]);

	if (stat_h & MSI_INT_EXTINT_11)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[44]);

	if (stat_h & MSI_INT_EXTINT_12)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[45]);

	if (stat_h & MSI_INT_EXTINT_13)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[46]);

	if (stat_h & MSI_INT_EXTINT_14)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[47]);

	if (stat_h & MSI_INT_EXTINT_15)
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[48]);

	if (stat_h & MSI_INT_I2CINT_0) {
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[49]);
		saa716x_i2c_irqevent(saa716x, 0);
	}

	if (stat_h & MSI_INT_I2CINT_1) {
		dprintk(SAA716x_DEBUG, 0, "<%s> ", vector_name[50]);
		saa716x_i2c_irqevent(saa716x, 1);
	}

	dprintk(SAA716x_DEBUG, 0, "\n");

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_msi_event);

int saa716x_msi_init(struct saa716x_dev *saa716x)
{
	u32 ena_l, ena_h, sta_l, sta_h, mid;
	int i;

	dprintk(SAA716x_DEBUG, 1, "Initializing MSI ..");
	saa716x->handlers = 0;

	/* get module id & version */
	mid = SAA716x_EPRD(MSI, MSI_MODULE_ID);
	if (mid != 0x30100)
		dprintk(SAA716x_ERROR, 1, "MSI Id<%04x> is not supported", mid);

	/* let HW take care of MSI race */
	SAA716x_EPWR(MSI, MSI_DELAY_TIMER, 0x0);

	/* INTA Polarity: Active High */
	SAA716x_EPWR(MSI, MSI_INTA_POLARITY, MSI_INTA_POLARITY_HIGH);

	/*
	 * IRQ Edge Rising: 25:24 = 0x01
	 * Traffic Class: 18:16 = 0x00
	 * MSI ID: 4:0 = 0x00
	 */
	for (i = 0; i < SAA716x_MSI_VECTORS; i++)
		SAA716x_EPWR(MSI, MSI_CONFIG_REG[i], MSI_INT_POL_EDGE_RISE);

	/* get Status */
	ena_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	ena_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);
	sta_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	sta_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);

	/* disable and clear enabled and asserted IRQ's */
	if (sta_l)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, sta_l);

	if (sta_h)
		SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_H, sta_h);

	if (ena_l)
		SAA716x_EPWR(MSI, MSI_INT_ENA_CLR_L, ena_l);

	if (ena_h)
		SAA716x_EPWR(MSI, MSI_INT_ENA_CLR_H, ena_h);

	msleep(5);

	/* Check IRQ's really disabled */
	ena_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	ena_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);
	sta_l = SAA716x_EPRD(MSI, MSI_INT_STATUS_L);
	sta_h = SAA716x_EPRD(MSI, MSI_INT_STATUS_H);

	if ((ena_l == 0) && (ena_h == 0) && (sta_l == 0) && (sta_h == 0)) {
		dprintk(SAA716x_DEBUG, 1, "Interrupts ena_l <%02x> ena_h <%02x> sta_l <%02x> sta_h <%02x>",
			ena_l, ena_h, sta_l, sta_h);

		return 0;
	} else {
		dprintk(SAA716x_DEBUG, 1, "I/O error");
		return -EIO;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_msi_init);

void saa716x_msiint_disable(struct saa716x_dev *saa716x)
{
	dprintk(SAA716x_DEBUG, 1, "Disabling Interrupts ...");

	SAA716x_EPWR(MSI, MSI_INT_ENA_L, 0x0);
	SAA716x_EPWR(MSI, MSI_INT_ENA_H, 0x0);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, 0xffffffff);
	SAA716x_EPWR(MSI, MSI_INT_STATUS_CLR_L, 0x0000ffff);
}
EXPORT_SYMBOL_GPL(saa716x_msiint_disable);


/* Map the given vector Id to the hardware bitmask. */
static void saa716x_map_vector(struct saa716x_dev *saa716x, int vector, u32 *mask_l, u32 *mask_h)
{
	u32 tmp = 1;

	if (vector < 32) {
		/* Bits 0 - 31 */
		tmp   <<= vector;
		*mask_l = tmp;
		*mask_h	= 0;
	} else {
		/* Bits 32 - 48 */
		tmp <<= vector - 32;
		*mask_l = 0;
		*mask_h = tmp;
	}
}

int saa716x_add_irqvector(struct saa716x_dev *saa716x,
			  int vector,
			  enum saa716x_edge edge,
			  irqreturn_t (*handler)(int irq, void *dev_id),
			  char *desc)
{
	struct saa716x_msix_entry *msix_handler = NULL;

	u32 config, mask_l, mask_h, ena_l, ena_h;

	BUG_ON(saa716x == NULL);
	BUG_ON(vector > SAA716x_MSI_VECTORS);
	dprintk(SAA716x_DEBUG, 1, "Adding Vector %d <%s>", vector, vector_name[vector]);

	if ((vector > 32) && (vector < 49)) {
		config = SAA716x_EPRD(MSI, MSI_CONFIG_REG[vector]);
		config &= 0xfcffffff; /* clear polarity */

		switch (edge) {
		default:
		case SAA716x_EDGE_RISING:
			SAA716x_EPWR(MSI, MSI_CONFIG_REG[vector], config | 0x01000000);
			break;

		case SAA716x_EDGE_FALLING:
			SAA716x_EPWR(MSI, MSI_CONFIG_REG[vector], config | 0x02000000);
			break;

		case SAA716x_EDGE_ANY:
			SAA716x_EPWR(MSI, MSI_CONFIG_REG[vector], config | 0x03000000);
			break;
		}
	}

	saa716x_map_vector(saa716x, vector, &mask_l, &mask_h);

	/* add callback */
	msix_handler = &saa716x->saa716x_msix_handler[saa716x->handlers];
	strcpy(msix_handler->desc, desc);
	msix_handler->vector = vector;
	msix_handler->handler = handler;
	saa716x->handlers++;

	SAA716x_EPWR(MSI, MSI_INT_ENA_SET_L, mask_l);
	SAA716x_EPWR(MSI, MSI_INT_ENA_SET_H, mask_h);

	ena_l = SAA716x_EPRD(MSI, MSI_INT_ENA_L);
	ena_h = SAA716x_EPRD(MSI, MSI_INT_ENA_H);
	dprintk(SAA716x_DEBUG, 1, "Interrupts ena_l <%02x> ena_h <%02x>", ena_l, ena_h);

	return 0;
}

int saa716x_remove_irqvector(struct saa716x_dev *saa716x, int vector)
{
	struct saa716x_msix_entry *msix_handler;
	int i;
	u32 mask_l, mask_h;

	msix_handler = &saa716x->saa716x_msix_handler[saa716x->handlers];
	BUG_ON(msix_handler == NULL);
	dprintk(SAA716x_DEBUG, 1, "Removing Vector %d <%s>", vector, vector_name[vector]);

	/* loop through the registered handlers */
	for (i = 0; i < saa716x->handlers; i++) {

		/* we found our vector */
		if (msix_handler->vector == vector) {
			BUG_ON(msix_handler->handler == NULL); /* no handler yet */
			dprintk(SAA716x_DEBUG, 1, "Vector %d <%s> removed",
				msix_handler->vector,
				msix_handler->desc);

			/* check whether it is already released */
			if (msix_handler->handler) {
				msix_handler->vector = 0;
				msix_handler->handler = NULL;
				saa716x->handlers--;
			}
		}
	}

	saa716x_map_vector(saa716x, vector, &mask_l, &mask_h);

	/* disable vector */
	SAA716x_EPWR(MSI, MSI_INT_ENA_CLR_L, mask_l);
	SAA716x_EPWR(MSI, MSI_INT_ENA_CLR_H, mask_h);

	return 0;
}
