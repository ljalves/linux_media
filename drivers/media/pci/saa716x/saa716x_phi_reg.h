#ifndef __SAA716x_PHI_REG_H
#define __SAA716x_PHI_REG_H

/* -------------- PHI_0 Registers -------------- */

#define PHI_0_MODE			0x0000
#define PHI_0_0_CONFIG			0x0008
#define PHI_0_1_CONFIG			0x000c
#define PHI_0_2_CONFIG			0x0010
#define PHI_0_3_CONFIG			0x0014

#define PHI_POLARITY			0x0038
#define PHI_TIMEOUT			0x003c
#define PHI_SW_RST			0x0ff0

#define PHI_0_0_RW_0			0x1000
#define PHI_0_0_RW_511			0x17fc

#define PHI_0_1_RW_0			0x1800
#define PHI_0_1_RW_511			0x1ffc

#define PHI_0_2_RW_0			0x2000
#define PHI_0_2_RW_511			0x27fc

#define PHI_0_3_RW_0			0x2800
#define PHI_0_3_RW_511			0x2ffc

#define PHI_CSN_DEASSERT		(0x00000001 <<  2)
#define PHI_AUTO_INCREMENT		(0x00000001 <<  1)
#define PHI_FIFO_MODE			(0x00000001 <<  0)

#define PHI_DELAY_RD_WR			(0x0000001f << 27)
#define PHI_EXTEND_RDY3			(0x00000003 << 25)
#define PHI_EXTEND_RDY2			(0x00000003 << 23)
#define PHI_EXTEND_RDY1			(0x00000003 << 21)
#define PHI_EXTEND_RDY0			(0x00000003 << 19)
#define PHI_RDY3_OD			(0x00000001 << 18)
#define PHI_RDY2_OD			(0x00000001 << 17)
#define PHI_RDY1_OD			(0x00000001 << 16)
#define PHI_RDY0_OD			(0x00000001 << 15)
#define PHI_ALE_POL			(0x00000001 << 14)
#define PHI_WRN_POL			(0x00000001 << 13)
#define PHI_RDN_POL			(0x00000001 << 12)
#define PHI_RDY3_POL			(0x00000001 << 11)
#define PHI_RDY2_POL			(0x00000001 << 10)
#define PHI_RDY1_POL			(0x00000001 <<  9)
#define PHI_RDY0_POL			(0x00000001 <<  8)
#define PHI_CSN7_POL			(0x00000001 <<  7)
#define PHI_CSN6_POL			(0x00000001 <<  6)
#define PHI_CSN5_POL			(0x00000001 <<  5)
#define PHI_CSN4_POL			(0x00000001 <<  4)
#define PHI_CSN3_POL			(0x00000001 <<  3)
#define PHI_CSN2_POL			(0x00000001 <<  2)
#define PHI_CSN1_POL			(0x00000001 <<  1)
#define PHI_CSN0_POL			(0x00000001 <<  0)

/* -------------- PHI_1 Registers -------------- */

#define PHI_1_MODE			0x00004
#define PHI_1_0_CONFIG			0x00018
#define PHI_1_1_CONFIG			0x0001c
#define PHI_1_2_CONFIG			0x00020
#define PHI_1_3_CONFIG			0x00024
#define PHI_1_4_CONFIG			0x00028
#define PHI_1_5_CONFIG			0x0002c
#define PHI_1_6_CONFIG			0x00030
#define PHI_1_7_CONFIG			0x00034

#define PHI_1_0_RW_0			0x00000
#define PHI_1_0_RW_16383		0x0fffc

#define PHI_1_1_RW_0			0x1000
#define PHI_1_1_RW_16383		0x1ffc

#define PHI_1_2_RW_0			0x2000
#define PHI_1_2_RW_16383		0x2ffc

#define PHI_1_3_RW_0			0x3000
#define PHI_1_3_RW_16383		0x3ffc

#define PHI_1_4_RW_0			0x4000
#define PHI_1_4_RW_16383		0x4ffc

#define PHI_1_5_RW_0			0x5000
#define PHI_1_5_RW_16383		0x5ffc

#define PHI_1_6_RW_0			0x6000
#define PHI_1_6_RW_16383		0x6ffc

#define PHI_1_7_RW_0			0x7000
#define PHI_1_7_RW_16383		0x7ffc


#endif /* __SAA716x_PHI_REG_H */
