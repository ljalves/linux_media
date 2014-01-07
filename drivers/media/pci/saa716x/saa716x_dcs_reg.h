#ifndef __SAA716x_DCS_REG_H
#define __SAA716x_DCS_REG_H

/* -------------- DCS Registers -------------- */

#define DCSC_CTRL			0x000
#define DCSC_SEL_PLLDI			(0x03ffffff <<  5)
#define DCSC_TOUT_SEL			(0x0000000f <<  1)
#define DCSC_TOUT_OFF			(0x00000001 <<  0)

#define DCSC_ADDR			0x00c
#define DCSC_ERR_TOUT_ADDR		(0x3fffffff <<  2)

#define DCSC_STAT			0x010
#define DCSC_ERR_TOUT_GNT		(0x0000001f << 24)
#define DCSC_ERR_TOUT_SEL		(0x0000007f << 10)
#define DCSC_ERR_TOUT_READ		(0x00000001 <<  8)
#define DCSC_ERR_TOUT_MASK		(0x0000000f <<  4)
#define DCSC_ERR_ACK			(0x00000001 <<  1)

#define DCSC_FEATURES			0x040
#define DCSC_UNIQUE_ID			(0x00000007 << 16)
#define DCSC_SECURITY			(0x00000001 << 14)
#define DCSC_NUM_BASE_REGS		(0x00000003 << 11)
#define DCSC_NUM_TARGETS		(0x0000001f <<  5)
#define DCSC_NUM_INITIATORS		(0x0000001f <<  0)

#define DCSC_BASE_REG0			0x100
#define DCSC_BASE_N_REG			(0x00000fff << 20)

#define DCSC_INT_CLR_ENABLE		0xfd8
#define DCSC_INT_CLR_ENABLE_TOUT	(0x00000001 <<  1)
#define DCSC_INT_CLR_ENABLE_ERROR	(0x00000001 <<  0)

#define DCSC_INT_SET_ENABLE		0xfdc
#define DCSC_INT_SET_ENABLE_TOUT	(0x00000001 <<  1)
#define DCSC_INT_SET_ENABLE_ERROR	(0x00000001 <<  0)

#define DCSC_INT_STATUS			0xfe0
#define DCSC_INT_STATUS_TOUT		(0x00000001 <<  1)
#define DCSC_INT_STATUS_ERROR		(0x00000001 <<  0)

#define DCSC_INT_ENABLE			0xfe4
#define DCSC_INT_ENABLE_TOUT		(0x00000001 <<  1)
#define DCSC_INT_ENABLE_ERROR		(0x00000001 <<  0)

#define DCSC_INT_CLR_STATUS		0xfe8
#define DCSC_INT_CLEAR_TOUT		(0x00000001 <<  1)
#define DCSC_INT_CLEAR_ERROR		(0x00000001 <<  0)

#define DCSC_INT_SET_STATUS		0xfec
#define DCSC_INT_SET_TOUT		(0x00000001 <<  1)
#define DCSC_INT_SET_ERROR		(0x00000001 <<  0)


#endif /* __SAA716x_DCS_REG_H */
