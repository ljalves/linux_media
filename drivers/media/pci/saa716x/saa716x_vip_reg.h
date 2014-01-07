#ifndef __SAA716x_VIP_REG_H
#define __SAA716x_VIP_REG_H

/* -------------- VIP Registers -------------- */

#define VI_MODE				0x000
#define VID_CFEN			(0x00000003 << 30)
#define VID_CFEN_ODD			(0x00000001 << 30)
#define VID_CFEN_EVEN			(0x00000002 << 30)
#define VID_CFEN_BOTH			(0x00000003 << 30)
#define VID_OSM				(0x00000001 << 29)
#define VID_FSEQ			(0x00000001 << 28)
#define AUX_CFEN			(0x00000003 << 26)
#define AUX_OSM				(0x00000001 << 25)
#define AUX_FSEQ			(0x00000001 << 24)
#define AUX_ANC_DATA			(0x00000003 << 22)
#define AUX_ANC_RAW			(0x00000001 << 21)
#define RST_ON_ERR			(0x00000001 << 17)
#define SOFT_RESET			(0x00000001 << 16)
#define IFF_CLAMP			(0x00000001 << 14)
#define IFF_MODE			(0x00000003 << 12)
#define DFF_CLAMP			(0x00000001 << 10)
#define DFF_MODE			(0x00000003 <<  8)
#define HSP_CLAMP			(0x00000001 <<  3)
#define HSP_RGB				(0x00000001 <<  2)
#define HSP_MODE			(0x00000003 <<  0)

#define ANC_DID_FIELD0			0x020
#define VI_ID_MASK_0			(0x000000ff <<  8)
#define VI_DATA_ID_0			(0x000000ff <<  0)

#define ANC_DID_FIELD1			0x024
#define VI_ID_MASK_1			(0x000000ff <<  8)
#define VI_DATA_ID_1			(0x000000ff <<  0)

#define VI_LINE_THRESH			0x040
#define VI_LCTHR			(0x000007ff <<  0)

#define VIN_FORMAT			0x100
#define VI_VSRA				(0x00000003 << 30)
#define VI_SYNCHD			(0x00000001 << 25)
#define VI_DUAL_STREAM			(0x00000001 << 24)
#define VI_NHDAUX			(0x00000001 << 20)
#define VI_NPAR				(0x00000001 << 19)
#define VI_VSEL				(0x00000003 << 14)
#define VI_TWOS				(0x00000001 << 13)
#define VI_TPG				(0x00000001 << 12)
#define VI_FREF				(0x00000001 << 10)
#define VI_FTGL				(0x00000001 <<  9)
#define VI_SF				(0x00000001 <<  3)
#define VI_FZERO			(0x00000001 <<  2)
#define VI_REVS				(0x00000001 <<  1)
#define VI_REHS				(0x00000001 <<  0)

#define VIN_TESTPGEN			0x104

#define WIN_XYSTART			0x140
#define WIN_XYEND			0x144

#define PRE_DIT_CTRL			0x160
#define POST_DIT_CTRL			0x164

#define AUX_XYSTART			0x180
#define AUX_XYEND			0x184

#define CSM_CKEY			0x284

#define PSU_FORMAT			0x300
#define PSU_WINDOW			0x304
#define PSU_BASE1			0x340
#define PSU_PITCH1			0x344
#define PSU_BASE2			0x348
#define PSU_PITCH2			0x34c
#define PSU_BASE3			0x350
#define PSU_BASE4			0x354
#define PSU_BASE5			0x358
#define PSU_BASE6			0x35c

#define AUX_FORMAT			0x380
#define AUX_BASE			0x390
#define AUX_PITCH			0x394

#define INT_STATUS			0xfe0
#define VI_STAT_FID_AUX			(0x00000001 << 31)
#define VI_STAT_FID_VID			(0x00000001 << 30)
#define VI_STAT_FID_VPI			(0x00000001 << 29)
#define VI_STAT_LINE_COUNT		(0x00000fff << 16)
#define VI_STAT_AUX_OVRFLW		(0x00000001 <<  9)
#define VI_STAT_VID_OVRFLW		(0x00000001 <<  8)
#define VI_STAT_WIN_SEQBRK		(0x00000001 <<  7)
#define VI_STAT_FID_SEQBRK		(0x00000001 <<  6)
#define VI_STAT_LINE_THRESH		(0x00000001 <<  5)
#define VI_STAT_AUX_WRAP		(0x00000001 <<  4)
#define VI_STAT_AUX_START_IN		(0x00000001 <<  3)
#define VI_STAT_AUX_END_OUT		(0x00000001 <<  2)
#define VI_STAT_VID_START_IN		(0x00000001 <<  1)
#define VI_STAT_VID_END_OUT		(0x00000001 <<  0)

#define INT_ENABLE			0xfe4
#define VI_ENABLE_AUX_OVRFLW		(0x00000001 <<  9)
#define VI_ENABLE_VID_OVRFLW		(0x00000001 <<  8)
#define VI_ENABLE_WIN_SEQBRK		(0x00000001 <<  7)
#define VI_ENABLE_FID_SEQBRK		(0x00000001 <<  6)
#define VI_ENABLE_LINE_THRESH		(0x00000001 <<  5)
#define VI_ENABLE_AUX_WRAP		(0x00000001 <<  4)
#define VI_ENABLE_AUX_START_IN		(0x00000001 <<  3)
#define VI_ENABLE_AUX_END_OUT		(0x00000001 <<  2)
#define VI_ENABLE_VID_START_IN		(0x00000001 <<  1)
#define VI_ENABLE_VID_END_OUT		(0x00000001 <<  0)

#define INT_CLR_STATUS			0xfe8
#define VI_CLR_STATUS_AUX_OVRFLW	(0x00000001 <<  9)
#define VI_CLR_STATUS_VID_OVRFLW	(0x00000001 <<  8)
#define VI_CLR_STATUS_WIN_SEQBRK	(0x00000001 <<  7)
#define VI_CLR_STATUS_FID_SEQBRK	(0x00000001 <<  6)
#define VI_CLR_STATUS_LINE_THRESH	(0x00000001 <<  5)
#define VI_CLR_STATUS_AUX_WRAP		(0x00000001 <<  4)
#define VI_CLR_STATUS_AUX_START_IN	(0x00000001 <<  3)
#define VI_CLR_STATUS_AUX_END_OUT	(0x00000001 <<  2)
#define VI_CLR_STATUS_VID_START_IN	(0x00000001 <<  1)
#define VI_CLR_STATUS_VID_END_OUT	(0x00000001 <<  0)

#define INT_SET_STATUS			0xfec
#define VI_SET_STATUS_AUX_OVRFLW	(0x00000001 <<  9)
#define VI_SET_STATUS_VID_OVRFLW	(0x00000001 <<  8)
#define VI_SET_STATUS_WIN_SEQBRK	(0x00000001 <<  7)
#define VI_SET_STATUS_FID_SEQBRK	(0x00000001 <<  6)
#define VI_SET_STATUS_LINE_THRESH	(0x00000001 <<  5)
#define VI_SET_STATUS_AUX_WRAP		(0x00000001 <<  4)
#define VI_SET_STATUS_AUX_START_IN	(0x00000001 <<  3)
#define VI_SET_STATUS_AUX_END_OUT	(0x00000001 <<  2)
#define VI_SET_STATUS_VID_START_IN	(0x00000001 <<  1)
#define VI_SET_STATUS_VID_END_OUT	(0x00000001 <<  0)

#define VIP_POWER_DOWN			0xff4
#define VI_PWR_DWN			(0x00000001 << 31)

#define VI_MODULE_ID			0xffc


#endif /* __SAA716x_VIP_REG_H */
