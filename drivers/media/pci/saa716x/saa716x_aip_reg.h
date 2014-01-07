#ifndef __SAA716x_AIP_REG_H
#define __SAA716x_AIP_REG_H

/* -------------- AI Registers ---------------- */

#define AI_STATUS			0x000
#define AI_BUF1_ACTIVE			(0x00000001 <<  4)
#define AI_OVERRUN			(0x00000001 <<  3)
#define AI_HBE				(0x00000001 <<  2)
#define AI_BUF2_FULL			(0x00000001 <<  1)
#define AI_BUF1_FULL			(0x00000001 <<  0)

#define AI_CTL				0x004
#define AI_RESET			(0x00000001 <<  31)
#define AI_CAP_ENABLE			(0x00000001 <<  30)
#define AI_CAP_MODE			(0x00000003 <<  28)
#define AI_SIGN_CONVERT			(0x00000001 <<  27)
#define AI_EARLYMODE			(0x00000001 <<  26)
#define AI_DIAGMODE			(0x00000001 <<  25)
#define AI_RAWMODE			(0x00000001 <<  24)
#define AI_OVR_INTEN			(0x00000001 <<   7)
#define AI_HBE_INTEN			(0x00000001 <<   6)
#define AI_BUF2_INTEN			(0x00000001 <<   5)
#define AI_BUF1_INTEN			(0x00000001 <<   4)
#define AI_ACK_OVR			(0x00000001 <<   3)
#define AI_ACK_HBE			(0x00000001 <<   2)
#define AI_ACK2				(0x00000001 <<   1)
#define AI_ACK1				(0x00000001 <<   0)

#define AI_SERIAL			0x008
#define AI_SER_MASTER			(0x00000001 <<  31)
#define AI_DATAMODE			(0x00000001 <<  30)
#define AI_FRAMEMODE			(0x00000003 <<  28)
#define AI_CLOCK_EDGE			(0x00000001 <<  27)
#define AI_SSPOS4			(0x00000001 <<  19)
#define AI_NR_CHAN			(0x00000003 <<  17)
#define AI_WSDIV			(0x000001ff <<   8)
#define AI_SCKDIV			(0x000000ff <<   0)

#define AI_FRAMING			0x00c
#define AI_VALIDPOS			(0x000001ff << 22)
#define AI_LEFTPOS			(0x000001ff << 13)
#define AI_RIGHTPOS			(0x000001ff <<  4)
#define AI_SSPOS_3_0			(0x0000000f <<  0)

#define AI_BASE1			0x014
#define AI_BASE2			0x018
#define AI_BASE				(0x03ffffff <<  6)

#define AI_SIZE				0x01c
#define AI_SAMPLE_SIZE			(0x03ffffff <<  6)

#define AI_INT_ACK			0x020
#define AI_ACK_OVR			(0x00000001 <<  3)
#define AI_ACK_HBE			(0x00000001 <<  2)
#define AI_ACK2				(0x00000001 <<  1)
#define AI_ACK1				(0x00000001 <<  0)

#define AI_PWR_DOWN			0xff4
#define AI_PWR_DWN			(0x00000001 <<  0)

#endif /* __SAA716x_AIP_REG_H */
