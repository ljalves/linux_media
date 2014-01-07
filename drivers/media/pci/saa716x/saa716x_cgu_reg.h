#ifndef __SAA716x_CGU_REG_H
#define __SAA716x_CGU_REG_H

/* -------------- CGU Registers -------------- */

#define CGU_SCR_0			0x000
#define CGU_SCR_1			0x004
#define CGU_SCR_2			0x008
#define CGU_SCR_3			0x00c
#define CGU_SCR_4			0x010
#define CGU_SCR_5			0x014
#define CGU_SCR_6			0x018
#define CGU_SCR_7			0x01c
#define CGU_SCR_8			0x020
#define CGU_SCR_9			0x024
#define CGU_SCR_10			0x028
#define CGU_SCR_11			0x02c
#define CGU_SCR_12			0x030
#define CGU_SCR_13			0x034
#define CGU_SCR_STOP			(0x00000001 <<  3)
#define CGU_SCR_RESET			(0x00000001 <<  2)
#define CGU_SCR_ENF2			(0x00000001 <<  1)
#define CGU_SCR_ENF1			(0x00000001 <<  0)

#define CGU_FS1_0			0x038
#define CGU_FS1_1			0x03c
#define CGU_FS1_2			0x040
#define CGU_FS1_3			0x044
#define CGU_FS1_4			0x048
#define CGU_FS1_5			0x04c
#define CGU_FS1_6			0x050
#define CGU_FS1_7			0x054
#define CGU_FS1_8			0x058
#define CGU_FS1_9			0x05c
#define CGU_FS1_10			0x060
#define CGU_FS1_11			0x064
#define CGU_FS1_12			0x068
#define CGU_FS1_13			0x06c
#define CGU_FS1_PLL			(0x00000000 <<  0)


#define CGU_FS2_0			0x070
#define CGU_FS2_1			0x074
#define CGU_FS2_2			0x078
#define CGU_FS2_3			0x07c
#define CGU_FS2_4			0x080
#define CGU_FS2_5			0x084
#define CGU_FS2_6			0x088
#define CGU_FS2_7			0x08c
#define CGU_FS2_8			0x090
#define CGU_FS2_9			0x094
#define CGU_FS2_10			0x098
#define CGU_FS2_11			0x09c
#define CGU_FS2_12			0x0a0
#define CGU_FS2_13			0x0a4

#define CGU_SSR_0			0x0a8
#define CGU_SSR_1			0x0ac
#define CGU_SSR_2			0x0b0
#define CGU_SSR_3			0x0b4
#define CGU_SSR_4			0x0b8
#define CGU_SSR_5			0x0bc
#define CGU_SSR_6			0x0c0
#define CGU_SSR_7			0x0c4
#define CGU_SSR_8			0x0c8
#define CGU_SSR_9			0x0cc
#define CGU_SSR_10			0x0d0
#define CGU_SSR_11			0x0d4
#define CGU_SSR_12			0x0d8
#define CGU_SSR_13			0x0dc

#define CGU_PCR_0_0			0x0e0
#define CGU_PCR_0_1			0x0e4
#define CGU_PCR_0_2			0x0e8
#define CGU_PCR_0_3			0x0ec
#define CGU_PCR_0_4			0x0f0
#define CGU_PCR_0_5			0x0f4
#define CGU_PCR_0_6			0x0f8
#define CGU_PCR_0_7			0x0fc
#define CGU_PCR_1_0			0x100
#define CGU_PCR_1_1			0x104
#define CGU_PCR_2_0			0x108
#define CGU_PCR_2_1			0x10c
#define CGU_PCR_3_0			0x110
#define CGU_PCR_3_1			0x114
#define CGU_PCR_3_2			0x118
#define CGU_PCR_4_0			0x11c
#define CGU_PCR_4_1			0x120
#define CGU_PCR_5			0x124
#define CGU_PCR_6			0x128
#define CGU_PCR_7			0x12c
#define CGU_PCR_8			0x130
#define CGU_PCR_9			0x134
#define CGU_PCR_10			0x138
#define CGU_PCR_11			0x13c
#define CGU_PCR_12			0x140
#define CGU_PCR_13			0x144
#define CGU_PCR_WAKE_EN			(0x00000001 <<  2)
#define CGU_PCR_AUTO			(0x00000001 <<  1)
#define CGU_PCR_RUN			(0x00000001 <<  0)


#define CGU_PSR_0_0			0x148
#define CGU_PSR_0_1			0x14c
#define CGU_PSR_0_2			0x150
#define CGU_PSR_0_3			0x154
#define CGU_PSR_0_4			0x158
#define CGU_PSR_0_5			0x15c
#define CGU_PSR_0_6			0x160
#define CGU_PSR_0_7			0x164
#define CGU_PSR_1_0			0x168
#define CGU_PSR_1_1			0x16c
#define CGU_PSR_2_0			0x170
#define CGU_PSR_2_1			0x174
#define CGU_PSR_3_0			0x178
#define CGU_PSR_3_1			0x17c
#define CGU_PSR_3_2			0x180
#define CGU_PSR_4_0			0x184
#define CGU_PSR_4_1			0x188
#define CGU_PSR_5			0x18c
#define CGU_PSR_6			0x190
#define CGU_PSR_7			0x194
#define CGU_PSR_8			0x198
#define CGU_PSR_9			0x19c
#define CGU_PSR_10			0x1a0
#define CGU_PSR_11			0x1a4
#define CGU_PSR_12			0x1a8
#define CGU_PSR_13			0x1ac

#define CGU_ESR_0_0			0x1b0
#define CGU_ESR_0_1			0x1b4
#define CGU_ESR_0_2			0x1b8
#define CGU_ESR_0_3			0x1bc
#define CGU_ESR_0_4			0x1c0
#define CGU_ESR_0_5			0x1c4
#define CGU_ESR_0_6			0x1c8
#define CGU_ESR_0_7			0x1cc
#define CGU_ESR_1_0			0x1d0
#define CGU_ESR_1_1			0x1d4
#define CGU_ESR_2_0			0x1d8
#define CGU_ESR_2_1			0x1dc
#define CGU_ESR_3_0			0x1e0
#define CGU_ESR_3_1			0x1e4
#define CGU_ESR_3_2			0x1e8
#define CGU_ESR_4_0			0x1ec
#define CGU_ESR_4_1			0x1f0
#define CGU_ESR_5			0x1f4
#define CGU_ESR_6			0x1f8
#define CGU_ESR_7			0x1fc
#define CGU_ESR_8			0x200
#define CGU_ESR_9			0x204
#define CGU_ESR_10			0x208
#define CGU_ESR_11			0x20c
#define CGU_ESR_12			0x210
#define CGU_ESR_13			0x214
#define CGU_ESR_FD_EN			(0x00000001 <<  0)

#define CGU_FDC_0			0x218
#define CGU_FDC_1			0x21c
#define CGU_FDC_2			0x220
#define CGU_FDC_3			0x224
#define CGU_FDC_4			0x228
#define CGU_FDC_5			0x22c
#define CGU_FDC_6			0x230
#define CGU_FDC_7			0x234
#define CGU_FDC_8			0x238
#define CGU_FDC_9			0x23c
#define CGU_FDC_10			0x240
#define CGU_FDC_11			0x244
#define CGU_FDC_12			0x248
#define CGU_FDC_13			0x24c
#define CGU_FDC_STRETCH			(0x00000001 <<  0)
#define CGU_FDC_RESET			(0x00000001 <<  1)
#define CGU_FDC_RUN1			(0x00000001 <<  2)
#define CGU_FDC_MADD			(0x000000ff <<  3)
#define CGU_FDC_MSUB			(0x000000ff << 11)

#endif /* __SAA716x_CGU_REG_H */
