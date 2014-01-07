#ifndef __SAA716x_MSI_H
#define __SAA716x_MSI_H

#define	TAGACK_VI0_0		0x000
#define	TAGACK_VI0_1		0x001
#define	TAGACK_VI0_2		0x002
#define	TAGACK_VI1_0		0x003
#define	TAGACK_VI1_1		0x004
#define	TAGACK_VI1_2		0x005
#define	TAGACK_FGPI_0		0x006
#define	TAGACK_FGPI_1		0x007
#define	TAGACK_FGPI_2		0x008
#define	TAGACK_FGPI_3		0x009
#define	TAGACK_AI_0		0x00a
#define	TAGACK_AI_1		0x00b
#define	OVRFLW_VI0_0		0x00c
#define	OVRFLW_VI0_1		0x00d
#define	OVRFLW_VI0_2		0x00e
#define	OVRFLW_VI1_0		0x00f
#define	OVRFLW_VI1_1		0x010
#define	OVRFLW_VI1_2		0x011
#define	OVRFLW_FGPI_O		0x012
#define	OVRFLW_FGPI_1		0x013
#define	OVRFLW_FGPI_2		0x014
#define	OVRFLW_FGPI_3		0x015
#define	OVRFLW_AI_0		0x016
#define	OVRFLW_AI_1		0x017
#define	AVINT_VI0		0x018
#define	AVINT_VI1		0x019
#define	AVINT_FGPI_0		0x01a
#define	AVINT_FGPI_1		0x01b
#define	AVINT_FGPI_2		0x01c
#define	AVINT_FGPI_3		0x01d
#define	AVINT_AI_0		0x01e
#define	AVINT_AI_1		0x01f
#define	UNMAPD_TC_INT		0x020
#define	EXTINT_0		0x021
#define	EXTINT_1		0x022
#define	EXTINT_2		0x023
#define	EXTINT_3		0x024
#define	EXTINT_4		0x025
#define	EXTINT_5		0x026
#define EXTINT_6		0x027
#define	EXTINT_7		0x028
#define	EXTINT_8		0x029
#define	EXTINT_9		0x02a
#define	EXTINT_10		0x02b
#define	EXTINT_11		0x02c
#define	EXTINT_12		0x02d
#define	EXTINT_13		0x02e
#define	EXTINT_14		0x02f
#define	EXTINT_15		0x030
#define	I2CINT_0		0x031
#define	I2CINT_1		0x032

#define SAA716x_TC0		0x000
#define SAA716x_TC1		0x001
#define SAA716x_TC2		0x002
#define SAA716x_TC3		0x003
#define SAA716x_TC4		0x004
#define SAA716x_TC5		0x005
#define SAA716x_TC6		0x006
#define SAA716x_TC7		0x007


enum saa716x_edge {
	SAA716x_EDGE_RISING	= 1,
	SAA716x_EDGE_FALLING	= 2,
	SAA716x_EDGE_ANY	= 3
};

struct saa716x_dev;

extern int saa716x_msi_event(struct saa716x_dev *saa716x, u32 stat_l, u32 stat_h);

extern int saa716x_msi_init(struct saa716x_dev *saa716x);
extern void saa716x_msiint_disable(struct saa716x_dev *saa716x);

extern int saa716x_add_irqvector(struct saa716x_dev *saa716x,
				 int vector,
				 enum saa716x_edge edge,
				 irqreturn_t (*handler)(int irq, void *dev_id),
				 char *desc);

extern int saa716x_remove_irqvector(struct saa716x_dev *saa716x, int vector);

#endif /* __SAA716x_MSI_H */
