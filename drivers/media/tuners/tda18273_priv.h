/*
	tda18273-priv.h - private header for the NXP TDA18273 silicon tuner
	Copyright (C) 2014 CrazyCat <crazycat69@narod.ru>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __TDA18273_PRIV
#define __TDA18273_PRIV

#define TDA18273_TUNER_RESET		0

#define TDA_FREQ_LOW_BAND		1
#define TDA_FREQ_MID_BAND		2
#define TDA_FREQ_HIGH_BAND		4

#define TDA_AGC_115dBuV			1
#define TDA_AGC_112dBuV			2
#define TDA_AGC_109dBuV			3
#define TDA_AGC_106dBuV			4
#define TDA_AGC_103dBuV			5
#define TDA_AGC_EXTERNAL		6
#define TDA_AGC_DISABLE			7

#define TDA_PLL_CP_50uA			6
#define TDA_PLL_CP_125uA		7
#define TDA_PLL_CP_250uA		0x0e
#define TDA_PLL_CP_650uA		0x0f

#define TDA_PLLref_DIVIDER_64		3
#define TDA_PLLref_DIVIDER_24		2

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef unsigned char		UInt8;
typedef unsigned int		UInt32;
typedef int			Int32;


/* Result codes */
#define TDA_RESULT_SUCCESS		0
#define TDA_RESULT_RESET_FAILURE	1
#define TDA_RESULT_I2C_WRITE_FAILURE	2
#define TDA_RESULT_I2C_READ_FAILURE	3
#define TDA_RESULT_OUT_OF_RANGE		5


/* Product id */
#define TDA_DEVICE_TYPE		18273

/* Register Bit-Field Definition */
typedef struct _TDA18273_BitField_t {
	UInt8 Address;
	UInt8 PositionInBits;
	UInt8 WidthInBits;
	UInt8 Attributes;
} TDA18273_BitField_t, *pTDA18273_BitField_t;

/* Bus Access */
typedef enum _tmbslFrontEndBusAccess_t {
    Bus_RW = 0x00,						/* RW Bus access */
    Bus_NoRead = 0x01,					/* No Read Bus access */
    Bus_NoWrite = 0x02,					/* No Write Bus access */
    Bus_None = Bus_NoRead|Bus_NoWrite,	/* No Bus access. Update RegMap only */
    Bus_Max
} tmbslFrontEndBusAccess_t;

typedef enum tmPowerState
{
    tmPowerOn = 0,                      /* Device powered on      (D0 state) */
    tmPowerStandby,                     /* Device power standby   (D1 state) */
    tmPowerSuspend,                     /* Device power suspended (D2 state) */
    tmPowerOff,                         /* Device powered off     (D3 state) */
    tmPowerMax                          /* Device power max */
}   tmPowerState_t, *ptmPowerState_t;

typedef enum _TDA18273PowerState_t {
    TDA18273_PowerNormalMode = 0,                                 /* Device normal mode */
    TDA18273_PowerStandbyWithXtalOn,                              /* Device standby mode with Xtal Output */
    TDA18273_PowerStandby,                                        /* Device standby mode */
    TDA18273_PowerMax
} TDA18273PowerState_t, *pTDA18273PowerState_t;

typedef enum _TDA18273StandardMode_t {
    TDA18273_StandardMode_Unknown = 0,                  /* Unknown standard */
    TDA18273_QAM_6MHz,                                  /* Digital TV QAM 6MHz */
    TDA18273_QAM_8MHz,                                  /* Digital TV QAM 8MHz */
    TDA18273_ATSC_6MHz,                                 /* Digital TV ATSC 6MHz */
    TDA18273_ISDBT_6MHz,                                /* Digital TV ISDBT 6MHz */
    TDA18273_DVBT_1_7MHz,                               /* Digital TV DVB-T/T2 6MHz */
    TDA18273_DVBT_6MHz,                                 /* Digital TV DVB-T/T2 6MHz */
    TDA18273_DVBT_7MHz,                                 /* Digital TV DVB-T/T2 7MHz */
    TDA18273_DVBT_8MHz,                                 /* Digital TV DVB-T/T2 8MHz */
    TDA18273_DVBT_10MHz,                                /* Digital TV DVB-T/T2 10MHz */     
    TDA18273_DMBT_8MHz,                                 /* Digital TV DMB-T 8MHz */
    TDA18273_FM_Radio,                                  /* Analog FM Radio */
    TDA18273_ANLG_MN,                                   /* Analog TV M/N */
    TDA18273_ANLG_B,                                    /* Analog TV B */
    TDA18273_ANLG_GH,                                   /* Analog TV G/H */
    TDA18273_ANLG_I,                                    /* Analog TV I */
    TDA18273_ANLG_DK,                                   /* Analog TV D/K */
    TDA18273_ANLG_L,                                    /* Analog TV L */
    TDA18273_ANLG_LL,                                   /* Analog TV L' */
    TDA18273_Scanning,                                  /* Analog Preset Blind Scanning */
    TDA18273_ScanXpress,                                /* ScanXpress */
    TDA18273_StandardMode_Max
} TDA18273StandardMode_t, *pTDA18273StandardMode_t;

/* TDA18273 Driver State Machine */
typedef enum _TDA18273HwState_t {
    TDA18273_HwState_Unknown = 0,   /* Hw State Unknown */
    TDA18273_HwState_InitNotDone,   /* Hw Init Not Done */
    TDA18273_HwState_InitPending,   /* Hw Init Pending */
    TDA18273_HwState_InitDone,      /* Hw Init Done */
    TDA18273_HwState_SetStdDone,    /* Set Standard Done */
    TDA18273_HwState_SetRFDone,     /* Set RF Done */
    TDA18273_HwState_SetFineRFDone, /* Set Fine RF Done */
    TDA18273_HwState_Max
} TDA18273HwState_t, *pTDA18273HwState_t;

typedef enum _TDA18273HwStateCaller_t {
    TDA18273_HwStateCaller_Unknown = 0, /* Caller Unknown */
    TDA18273_HwStateCaller_SetPower,    /* Caller SetPowerState */
    TDA18273_HwStateCaller_HwInit,      /* Caller HwInit */
    TDA18273_HwStateCaller_SetStd,      /* Caller SetStandardMode */
    TDA18273_HwStateCaller_SetRF,       /* Caller SetRF */
    TDA18273_HwStateCaller_SetFineRF,   /* Caller SetFineRF */
    TDA18273_HwStateCaller_GetRSSI,     /* Caller GetRSSI */
    TDA18273_HwStateCaller_SetRawRF,    /* Caller SetRawRF */
    TDA18273_HwStateCaller_Max
} TDA18273HwStateCaller_t, *pTDA18273HwStateCaller_t;

/* TDA18273 specific powerstate bits: */
typedef enum _TDA18273SM_Reg_t {
    TDA18273_SM_NONE    = 0x00, /* No SM bit to set */
    TDA18273_SM_XT      = 0x01, /* SM_XT bit to set */
    TDA18273_SM         = 0x02  /* SM bit to set */
} TDA18273SM_Reg_t, *pTDA18273SM_Reg_t;

/* TDA18273 specific MSM: */
typedef enum _TDA18273MSM_t {
    TDA18273_MSM_Calc_PLL       = 0x01, /* Calc_PLL bit */
    TDA18273_MSM_RC_Cal         = 0x02, /* RC_Cal bit */
    TDA18273_MSM_IR_CAL_Wanted  = 0x04, /* IR_CAL_Wanted bit */
    TDA18273_MSM_IR_Cal_Image   = 0x08, /* IR_Cal_Image bit */
    TDA18273_MSM_IR_CAL_Loop    = 0x10, /* IR_CAL_Loop bit */
    TDA18273_MSM_RF_CAL         = 0x20, /* RF_CAL bit */
    TDA18273_MSM_RF_CAL_AV      = 0x40, /* RF_CAL_AV bit */
    TDA18273_MSM_RSSI_Meas      = 0x80, /* RSSI_Meas bit */
    /* Performs all CALs except IR_CAL */
    TDA18273_MSM_HwInit         = TDA18273_MSM_Calc_PLL\
                                    |TDA18273_MSM_RC_Cal\
                                    |TDA18273_MSM_RF_CAL,
    /* Performs all IR_CAL */
    TDA18273_MSM_IrCal          = TDA18273_MSM_IR_Cal_Image\
                                    |TDA18273_MSM_IR_CAL_Loop,
    TDA18273_MSM_SetRF          = TDA18273_MSM_Calc_PLL\
                                    |TDA18273_MSM_RF_CAL_AV,
    TDA18273_MSM_GetPowerLevel  = TDA18273_MSM_RSSI_Meas
} TDA18273MSM_t, *pTDA18273MSM_t;

/* TDA18273 specific IRQ clear: */
typedef enum _TDA18273IRQ_t {
    TDA18273_IRQ_MSM_RCCal      = 0x01, /* MSM_RCCal bit */
    TDA18273_IRQ_MSM_IRCAL      = 0x02, /* MSM_IRCAL bit */
    TDA18273_IRQ_MSM_RFCal      = 0x04, /* MSM_RFCal bit */
    TDA18273_IRQ_MSM_LOCalc     = 0x08, /* MSM_LOCalc bit */
    TDA18273_IRQ_MSM_RSSI       = 0x10, /* MSM_RSSI bit */
    TDA18273_IRQ_XtalCal        = 0x20, /* XtalCal bit */
    TDA18273_IRQ_Global         = 0x80, /* IRQ_status bit */
    TDA18273_IRQ_HwInit         = TDA18273_IRQ_MSM_RCCal\
                                    |TDA18273_IRQ_MSM_RFCal\
                                    |TDA18273_IRQ_MSM_LOCalc\
                                    |TDA18273_IRQ_MSM_RSSI,
    TDA18273_IRQ_IrCal          = TDA18273_IRQ_MSM_IRCAL\
                                    |TDA18273_IRQ_MSM_LOCalc\
                                    |TDA18273_IRQ_MSM_RSSI,
    TDA18273_IRQ_SetRF          = TDA18273_IRQ_MSM_RFCal\
                                    |TDA18273_IRQ_MSM_LOCalc,
    TDA18273_IRQ_GetPowerLevel  = TDA18273_IRQ_MSM_RSSI
} TDA18273IRQ_t, *pTDA18273IRQ_t;

/* TDA18273 Standard settings: */
typedef enum _TDA18273LPF_t {
    TDA18273_LPF_6MHz = 0,  /* 6MHz LPFc */
    TDA18273_LPF_7MHz,      /* 7MHz LPFc */
    TDA18273_LPF_8MHz,      /* 8MHz LPFc */
    TDA18273_LPF_9MHz,      /* 9MHz LPFc */
    TDA18273_LPF_1_5MHz,    /* 1.5MHz LPFc */
    TDA18273_LPF_Max
} TDA18273LPF_t, *pTDA18273LPF_t;

typedef enum _TDA18273LPFOffset_t {
    TDA18273_LPFOffset_0pc = 0,     /* LPFc 0% */
    TDA18273_LPFOffset_min_4pc,     /* LPFc -4% */
    TDA18273_LPFOffset_min_8pc,     /* LPFc -8% */
    TDA18273_LPFOffset_min_12pc,    /* LPFc -12% */
    TDA18273_LPFOffset_Max
} TDA18273LPFOffset_t, *pTDA18273LPFOffset_t;

typedef enum TDA18273DC_Notch_IF_PPF_t {
    TDA18273_DC_Notch_IF_PPF_Disabled = 0,  /* IF Notch Disabled */
    TDA18273_DC_Notch_IF_PPF_Enabled,       /* IF Notch Enabled */
    TDA18273_DC_Notch_IF_PPF_Max
} TDA18273DC_Notch_IF_PPF_t, *pTDA18273DC_Notch_IF_PPF_t;

typedef enum _TDA18273IF_HPF_t {
    TDA18273_IF_HPF_Disabled = 0,   /* IF HPF disabled */
    TDA18273_IF_HPF_0_4MHz,         /* IF HPF 0.4MHz */
    TDA18273_IF_HPF_0_85MHz,        /* IF HPF 0.85MHz */
    TDA18273_IF_HPF_1MHz,           /* IF HPF 1MHz */
    TDA18273_IF_HPF_1_5MHz,         /* IF HPF 1.5MHz */
    TDA18273_IF_HPF_Max
} TDA18273IF_HPF_t, *pTDA18273IF_HPF_t;

typedef enum _TDA18273IF_Notch_t {
    TDA18273_IF_Notch_Disabled = 0, /* IF Notch Disabled */
    TDA18273_IF_Notch_Enabled,      /* IF Notch Enabled */
    TDA18273_IF_Notch_Max
} TDA18273IF_Notch_t, *pTDA18273IF_Notch_t;

typedef enum _TDA18273IFnotchToRSSI_t {
    TDA18273_IFnotchToRSSI_Disabled = 0,    /* IFnotchToRSSI Disabled */
    TDA18273_IFnotchToRSSI_Enabled,         /* IFnotchToRSSI Enabled */
    TDA18273_IFnotchToRSSI_Max
} TDA18273IFnotchToRSSI_t, *pTDA18273IFnotchToRSSI_t;

typedef enum _TDA18273AGC1_TOP_I2C_DN_UP_t {
    TDA18273_AGC1_TOP_I2C_DN_UP_d88_u82dBuV = 0,    /* AGC1 TOP I2C DN/UP down 88 up 82 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d90_u84dBuV,        /* AGC1 TOP I2C DN/UP down 90 up 84 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89wdBuV,       /* AGC1 TOP I2C DN/UP down 95 up 89 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d93_u87dBuV,        /* AGC1 TOP I2C DN/UP down 93 up 87 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,        /* AGC1 TOP I2C DN/UP down 95 up 89 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d99_u84dBuV,        /* AGC1 TOP I2C DN/UP down 99 up 84 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d100_u82dBuV,       /* AGC1 TOP I2C DN/UP down 100 up 82 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d100_u94bisdBuV,    /* AGC1 TOP I2C DN/UP down 100 up 94 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d102_u82dBuV,       /* AGC1 TOP I2C DN/UP down 102 up 82 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d102_u84dBuV,       /* AGC1 TOP I2C DN/UP down 102 up 84 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_d100_u94dBuV,       /* AGC1 TOP I2C DN/UP down 100 up 94 dBuV */
    TDA18273_AGC1_TOP_I2C_DN_UP_Max
} TDA18273AGC1_TOP_I2C_DN_UP_t, *pTDA18273AGC1_TOP_I2C_DN_UP_t;

typedef enum _TDA18273AGC1_Adapt_TOP_DN_UP_t {
    TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step = 0,   /* AGC1 Adapt TOP DN/UP 0 Step */
    TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,       /* AGC1 Adapt TOP DN/UP 1 Step */
    TDA18273_AGC1_Adapt_TOP_DN_UP_2_Step,       /* AGC1 Adapt TOP DN/UP 2 Step */
    TDA18273_AGC1_Adapt_TOP_DN_UP_3_Step,       /* AGC1 Adapt TOP DN/UP 3 Step */
    TDA18273_AGC1_Adapt_TOP_DN_UP_Max
} TDA18273AGC1_Adapt_TOP_DN_UP_t, *pTDA18273AGC1_Adapt_TOP_DN_UP_t;

typedef enum _TDA18273AGC1_Mode_t {
    TDA18273_AGC1_Mode_No_Mode = 0,         /* AGC1 Mode */
    TDA18273_AGC1_Mode_TOP_ADAPT,           /* AGC1 Mode: TOP ADAPT */
    TDA18273_AGC1_Mode_LNA_ADAPT,           /* AGC1 Mode: LNA ADAPT */
    TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT, /* AGC1 Mode: LNA ADAPT & TOP ADAPT */
    TDA18273_AGC1_Mode_FREEZE,              /* AGC1 Mode: FREEZE */
    TDA18273_AGC1_Mode_WIDE,                /* AGC1 Mode: WIDE */
    TDA18273_AGC1_Mode_LNA_ADAPT_FREEZE,    /* AGC1 Mode: LNA ADAPT & FREEZE */
    TDA18273_AGC1_Mode_LNA_ADAPT_WIDE,      /* AGC1 Mode: LNA ADAPT & WIDE */
    TDA18273_AGC1_Mode_Max
} TDA18273AGC1_Mode_t, *pTDA18273AGC1_Mode_t;

typedef enum _TDA18273Range_LNA_Adapt_t {
    TDA18273_Range_LNA_Adapt_20dB_8dB = 0,  /* Range LNA Adapt 20dB-8dB */
    TDA18273_Range_LNA_Adapt_20dB_11dB,     /* Range LNA Adapt 20dB-11dB */
    TDA18273_Range_LNA_Adapt_Max
} TDA18273Range_LNA_Adapt_t, *pTDA18273Range_LNA_Adapt_t;

typedef enum _TDA18273LNA_Adapt_RFAGC_Gv_Threshold {
    TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB = 0,  /* 18.25dB */
    TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_16_75dB,      /* 16.75dB */
    TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_15_25dB,      /* 15.25dB */
    TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_13_75dB,      /* 13.75dB */
    TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_Max
} TDA18273LNA_Adapt_RFAGC_Gv_Threshold, *pTDA18273LNA_Adapt_RFAGC_Gv_Threshold;

typedef enum _TDA18273AGC1_Top_Adapt_RFAGC_Gv_Threshold {
    TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB = 0, /* 16.75dB */
    TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,     /* 15.25dB */
    TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_13_75dB,     /* 13.75dB */
    TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_12_25dB,     /* 12.25dB */
    TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_Max
} TDA18273AGC1_Top_Adapt_RFAGC_Gv_Threshold, *pTDA18273AGC1_Top_Adapt_RFAGC_Gv_Threshold;

typedef enum _TDA18273AGC1_DN_Time_Constant_t {
    TDA18273_AGC1_DN_Time_Constant_32_752ms = 0, /* 32.752 ms */
	TDA18273_AGC1_DN_Time_Constant_16_376ms,     /* 16.376 ms */
	TDA18273_AGC1_DN_Time_Constant_8_188ms,      /* 8.188 ms  */
	TDA18273_AGC1_DN_Time_Constant_4_094ms       /* 4.094 ms  */
} TDA18273AGC1_DN_Time_Constant_t, *pTDA18273AGC1_DN_Time_Constant_t;

typedef enum _TDA18273AGC2_TOP_DN_UP_t {
    TDA18273_AGC2_TOP_DN_UP_d88_u81dBuV = 0,    /* AGC2 TOP DN/UP down 88 up 81 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d90_u83dBuV,        /* AGC2 TOP DN/UP down 90 up 83 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d93_u86dBuV,        /* AGC2 TOP DN/UP down 93 up 86 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d95_u88dBuV,        /* AGC2 TOP DN/UP down 95 up 88 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d88_u82dBuV,        /* AGC2 TOP DN/UP down 88 up 82 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,        /* AGC2 TOP DN/UP down 90 up 84 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d93_u87dBuV,        /* AGC2 TOP DN/UP down 93 up 87 dBuV */
    TDA18273_AGC2_TOP_DN_UP_d95_u89dBuV,        /* AGC2 TOP DN/UP down 95 up 89 dBuV */
    TDA18273_AGC2_TOP_DN_UP_Max
} TDA18273AGC2_TOP_DN_UP_t, *pTDA18273AGC2_TOP_DN_UP_t;

typedef enum _TDA18273AGC2_DN_Time_Constant_t {
	TDA18273_AGC2_DN_Time_Constant_16_376ms = 0, /* 16.376 ms */
	TDA18273_AGC2_DN_Time_Constant_8_188ms,      /* 8.188 ms  */
	TDA18273_AGC2_DN_Time_Constant_4_094ms,      /* 4.094 ms  */
	TDA18273_AGC2_DN_Time_Constant_2_047ms,      /* 2.047 ms  */
} TDA18273AGC2_DN_Time_Constant_t, *pTDA18273AGC2_DN_Time_Constant_t;

typedef enum _TDA18273AGC3_TOP_I2C_t {
    TDA18273_AGC3_TOP_I2C_94dBuV = 0,   /* AGC3 TOP I2C 94 dBuV */
    TDA18273_AGC3_TOP_I2C_96dBuV,       /* AGC3 TOP I2C 96 dBuV */
    TDA18273_AGC3_TOP_I2C_98dBuV,       /* AGC3 TOP I2C 98 dBuV */
    TDA18273_AGC3_TOP_I2C_100dBuV,      /* AGC3 TOP I2C 100 dBuV */
    TDA18273_AGC3_TOP_I2C_102dBuV,      /* AGC3 TOP I2C 102 dBuV */
    TDA18273_AGC3_TOP_I2C_104dBuV,      /* AGC3 TOP I2C 104 dBuV */
    TDA18273_AGC3_TOP_I2C_106dBuV,      /* AGC3 TOP I2C 106 dBuV */
    TDA18273_AGC3_TOP_I2C_107dBuV,      /* AGC3 TOP I2C 107 dBuV */
    TDA18273_AGC3_TOP_I2C_Max
} TDA18273AGC3_TOP_I2C_t, *pTDA18273AGC3_TOP_I2C_t;

typedef enum _TDA18273AGC4_TOP_DN_UP_t {
    TDA18273_AGC4_TOP_DN_UP_d105_u99dBuV = 0,   /* AGC4 TOP DN/UP down 105 up 99 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,      /* AGC4 TOP DN/UP down 105 up 100 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d105_u101dBuV,      /* AGC4 TOP DN/UP down 105 up 101 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d107_u101dBuV,      /* AGC4 TOP DN/UP down 107 up 101 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d107_u102dBuV,      /* AGC4 TOP DN/UP down 107 up 102 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d107_u103dBuV,      /* AGC4 TOP DN/UP down 107 up 103 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d108_u102dBuV,      /* AGC4 TOP DN/UP down 108 up 102 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d109_u103dBuV,      /* AGC4 TOP DN/UP down 109 up 103 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d109_u104dBuV,      /* AGC4 TOP DN/UP down 109 up 104 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d109_u105dBuV,      /* AGC4 TOP DN/UP down 109 up 105 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d110_u104dBuV,      /* AGC4 TOP DN/UP down 110 up 104 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,      /* AGC4 TOP DN/UP down 110 up 105 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d110_u106dBuV,      /* AGC4 TOP DN/UP down 110 up 106 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d112_u106dBuV,      /* AGC4 TOP DN/UP down 112 up 106 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d112_u107dBuV,      /* AGC4 TOP DN/UP down 112 up 107 dBuV */
    TDA18273_AGC4_TOP_DN_UP_d112_u108dBuV,      /* AGC4 TOP DN/UP down 112 up 108 dBuV */
    TDA18273_AGC4_TOP_DN_UP_Max
} TDA18273AGC4_TOP_DN_UP_t, *pTDA18273AGC4_TOP_DN_UP_t;

typedef enum _TDA18273AGC5_TOP_DN_UP_t {
    TDA18273_AGC5_TOP_DN_UP_d105_u99dBuV = 0,   /* AGC5 TOP DN/UP down 105 up 99 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,      /* AGC5 TOP DN/UP down 105 up 100 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d105_u101dBuV,      /* AGC5 TOP DN/UP down 105 up 101 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d107_u101dBuV,      /* AGC5 TOP DN/UP down 107 up 101 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d107_u102dBuV,      /* AGC5 TOP DN/UP down 107 up 102 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d107_u103dBuV,      /* AGC5 TOP DN/UP down 107 up 103 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d108_u102dBuV,      /* AGC5 TOP DN/UP down 108 up 102 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d109_u103dBuV,      /* AGC5 TOP DN/UP down 109 up 103 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d109_u104dBuV,      /* AGC5 TOP DN/UP down 109 up 104 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d109_u105dBuV,      /* AGC5 TOP DN/UP down 109 up 105 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d110_u104dBuV,      /* AGC5 TOP DN/UP down 108 up 104 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,      /* AGC5 TOP DN/UP down 108 up 105 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d110_u106dBuV,      /* AGC5 TOP DN/UP down 108 up 106 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d112_u106dBuV,      /* AGC5 TOP DN/UP down 108 up 106 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d112_u107dBuV,      /* AGC5 TOP DN/UP down 108 up 107 dBuV */
    TDA18273_AGC5_TOP_DN_UP_d112_u108dBuV,      /* AGC5 TOP DN/UP down 108 up 108 dBuV */
    TDA18273_AGC5_TOP_DN_UP_Max
} TDA18273AGC5_TOP_DN_UP_t, *pTDA18273AGC5_TOP_DN_UP_t;

typedef enum _TDA18273AGC3_Top_Adapt_Algorithm {
    TDA18273_Top_Adapt_NO_TOP_ADAPT = 0,    /* NO TOP ADAPT */
    TDA18273_Top_Adapt_TOP_ADAPT35,         /* TOP ADAPT35  */
    TDA18273_Top_Adapt_TOP_ADAPT34,         /* TOP ADAPT34  */
    TDA18273_Top_Adapt_Max
} TDA18273AGC3_Top_Adapt_Algorithm, *pTDA18273AGC3_Top_Adapt_Algorithm;

typedef enum _TDA18273AGC3_Adapt_TOP_t {
    TDA18273_AGC3_Adapt_TOP_0_Step = 0, /* same level as AGC3 TOP  */
    TDA18273_AGC3_Adapt_TOP_1_Step,     /* 1 level below AGC3 TOP  */
    TDA18273_AGC3_Adapt_TOP_2_Step,     /* 2 level below AGC3 TOP  */
    TDA18273_AGC3_Adapt_TOP_3_Step      /* 3 level below AGC3 TOP  */
} TDA18273AGC3_Adapt_TOP_t, *pTDA18273AGC3_Adapt_TOP_t;

typedef enum _TDA18273AGC_Overload_TOP_t {
    TDA18273_AGC_Overload_TOP_plus_9_plus_3_5_min_3_5 = 0,  /* +9/+3.5/-3.5 */
    TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_4_5,      /* +9/+4.5/-4.5 */
    TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,      /* +9/+4.5/-3.5 */
    TDA18273_AGC_Overload_TOP_plus_9_plus_6_min_4_5,        /* +9/+6/-4.5   */
    TDA18273_AGC_Overload_TOP_plus_9_plus_6_min_6,          /* +9/+6/-6     */
    TDA18273_AGC_Overload_TOP_plus_9_plus_6_min_9,          /* +9/+6/-9     */
    TDA18273_AGC_Overload_TOP_plus_9_plus_7_5_min_9,        /* +9/+7.5/-9   */
    TDA18273_AGC_Overload_TOP_plus_12_plus_7_5_min_9        /* +12/+7.5/-9   */
} TDA18273AGC_Overload_TOP_t, *pTDA18273AGC_Overload_TOP_t;

typedef enum _TDA18273TH_AGC_Adapt34_t {
    TDA18273_TH_AGC_Adapt34_2dB = 0,    /* Adapt TOP 34 Gain Threshold 2dB */
    TDA18273_TH_AGC_Adapt34_5dB         /* Adapt TOP 34 Gain Threshold 5dB */
} TDA18273TH_AGC_Adapt34_t, *pTDA18273TH_AGC_Adapt34_t;

typedef enum _TDA18273RF_Atten_3dB_t {
    TDA18273_RF_Atten_3dB_Disabled = 0, /* RF_Atten_3dB Disabled */
    TDA18273_RF_Atten_3dB_Enabled,      /* RF_Atten_3dB Enabled */
    TDA18273_RF_Atten_3dB_Max
} TDA18273RF_Atten_3dB_t, *pTDA18273RF_Atten_3dB_t;

typedef enum _TDA18273IF_Output_Level_t {
    TDA18273_IF_Output_Level_2Vpp_0_30dB = 0,           /* 2Vpp       0 - 30dB      */
    TDA18273_IF_Output_Level_1_25Vpp_min_4_26dB,        /* 1.25Vpp   -4 - 26dB      */
    TDA18273_IF_Output_Level_1Vpp_min_6_24dB,           /* 1Vpp      -6 - 24dB      */
    TDA18273_IF_Output_Level_0_8Vpp_min_8_22dB,         /* 0.8Vpp    -8 - 22dB      */
    TDA18273_IF_Output_Level_0_85Vpp_min_7_5_22_5dB,    /* 0.85Vpp   -7.5 - 22.5dB  */
    TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,         /* 0.7Vpp    -9 - 21dB      */
    TDA18273_IF_Output_Level_0_6Vpp_min_10_3_19_7dB,    /* 0.6Vpp    -10.3 - 19.7dB */
    TDA18273_IF_Output_Level_0_5Vpp_min_12_18dB,        /* 0.5Vpp    -12 - 18dB     */
    TDA18273_IF_Output_Level_Max
} TDA18273IF_Output_Level_t, *pTDA18273IF_Output_Level_t;

typedef enum _TDA18273S2D_Gain_t {
    TDA18273_S2D_Gain_3dB = 0,  /* 3dB */
    TDA18273_S2D_Gain_6dB,      /* 6dB */
    TDA18273_S2D_Gain_9dB,      /* 9dB */
    TDA18273_S2D_Gain_Max
} TDA18273S2D_Gain_t, *pTDA18273S2D_Gain_t;

typedef enum _TDA18273Negative_Modulation_t {
    TDA18273_Negative_Modulation_Disabled = 0,
    TDA18273_Negative_Modulation_Enabled,
    TDA18273_Negative_Modulation_Max
} TDA18273Negative_Modulation_t, *pTDA18273Negative_Modulation_t;

typedef enum _TDA18273AGCK_Steps_t {
    TDA18273_AGCK_Steps_0_2dB = 0,  /* 0.2dB */
    TDA18273_AGCK_Steps_0_4dB,      /* 0.4dB */
    TDA18273_AGCK_Steps_0_6dB,      /* 0.6dB */
    TDA18273_AGCK_Steps_0_8dB,      /* 0.8dB */
    TDA18273_AGCK_Steps_Max
} TDA18273AGCK_Steps_t, *pTDA18273AGCK_Steps_t;

typedef enum _TDA18273AGCK_Time_Constant_t {
    TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE = 0,  /* 1 Step Each VSYNC Rising Edge */
    TDA18273_AGCK_Time_Constant_0_512ms,                            /* 0.512ms                       */
    TDA18273_AGCK_Time_Constant_8_192ms,                            /* 8.192ms                       */
    TDA18273_AGCK_Time_Constant_32_768ms,                           /* 32.768ms                      */
    TDA18273_AGCK_Time_Constant_Max
} TDA18273AGCK_Time_Constant_t, *pTDA18273AGCK_Time_Constant_t;

typedef enum _TDA18273AGC5_HPF_t {
    TDA18273_AGC5_HPF_Disabled = 0, /* AGC5 HPF Disabled */
    TDA18273_AGC5_HPF_Enabled,      /* AGC5 HPF Enabled  */
    TDA18273_AGC5_HPF_Max
} TDA18273AGC5_HPF_t, *pTDA18273AGC5_HPF_t;

typedef enum _TDA18273Pulse_Shaper_Disable_t {
    TDA18273_Pulse_Shaper_Disable_Disabled = 0,
    TDA18273_Pulse_Shaper_Disable_Enabled,
    TDA18273_Pulse_Shaper_Disable_Max
} TDA18273Pulse_Shaper_Disable_t, *pTDA18273Pulse_Shaper_Disable_t;

typedef enum _TDA18273VHF_III_Mode_t {
    TDA18273_VHF_III_Mode_Disabled = 0, /* VHF_III_Mode Disabled */
    TDA18273_VHF_III_Mode_Enabled,      /* VHF_III_Mode Enabled  */
    TDA18273_VHF_III_Mode_Max
} TDA18273VHF_III_Mode_t, *pTDA18273VHF_III_Mode_t;

typedef enum _TDA18273LO_CP_Current_t {
    TDA18273_LO_CP_Current_Disabled = 0,    /* LO CP Current Disabled */
    TDA18273_LO_CP_Current_Enabled,         /* LO CP Current Enabled  */
    TDA18273_LO_CP_Current_Max
} TDA18273LO_CP_Current_t, *pTDA18273LO_CP_Current_t;

typedef enum _TDA18273PD_Underload_t {
    TDA18273_PD_Underload_Disabled = 0,    /* PD Underload Disabled */
    TDA18273_PD_Underload_Enabled,         /* PD Underload Enabled  */
    TDA18273_PD_Underload_Max
} TDA18273PD_Underload_t, *pTDA18273PD_Underload_t;

typedef struct _TDA18273StdCoefficients
{
    /****************************************************************/
    /* IF Settings                                                  */
    /****************************************************************/
    UInt32                                      IF;                                 /* IF Frequency */
    Int32                                       CF_Offset;

    /****************************************************************/
    /* IF SELECTIVITY Settings                                      */
    /****************************************************************/
    TDA18273LPF_t                               LPF;                                /* LPF Cut off */
    TDA18273LPFOffset_t                         LPF_Offset;                         /* LPF offset */
    TDA18273DC_Notch_IF_PPF_t                   DC_Notch_IF_PPF;                    /* DC notch IF PPF */
    TDA18273IF_HPF_t                            IF_HPF;                             /* Hi Pass */
    TDA18273IF_Notch_t                          IF_Notch;                           /* IF notch */
    TDA18273IFnotchToRSSI_t                     IFnotchToRSSI;                      /* IFnotchToRSSI */

    /****************************************************************/
    /* AGC TOP Settings                                             */
    /****************************************************************/
    TDA18273AGC1_TOP_I2C_DN_UP_t                AGC1_TOP_I2C_DN_UP;                 /* AGC1 TOP I2C DN/UP */
    TDA18273AGC1_Adapt_TOP_DN_UP_t              AGC1_Adapt_TOP_DN_UP;               /* AGC1 Adapt TOP DN/UP */
	TDA18273AGC1_DN_Time_Constant_t             AGC1_DN_Time_Constant;              /* AGC1 DN Time Constant */
    TDA18273AGC1_Mode_t                         AGC1_Mode;                          /* AGC1 mode */
    TDA18273Range_LNA_Adapt_t                   Range_LNA_Adapt;                    /* Range_LNA_Adapt */
    TDA18273LNA_Adapt_RFAGC_Gv_Threshold        LNA_Adapt_RFAGC_Gv_Threshold;       /* LNA_Adapt_RFAGC_Gv_Threshold */
    TDA18273AGC1_Top_Adapt_RFAGC_Gv_Threshold   AGC1_Top_Adapt_RFAGC_Gv_Threshold;  /* AGC1_Top_Adapt_RFAGC_Gv_Threshold */
    TDA18273AGC2_TOP_DN_UP_t                    AGC2_TOP_DN_UP;                     /* AGC2 TOP DN/UP */
	TDA18273AGC2_DN_Time_Constant_t             AGC2_DN_Time_Constant;              /* AGC2 DN Time Constant */
    TDA18273AGC3_TOP_I2C_t                      AGC3_TOP_I2C_Low_Band;              /* AGC3 TOP I2C Low Band */
    TDA18273AGC3_TOP_I2C_t                      AGC3_TOP_I2C_High_Band;             /* AGC3 TOP I2C High Band */
    TDA18273AGC4_TOP_DN_UP_t                    AGC4_TOP_DN_UP;                     /* AGC4 TOP DN/UP */
    TDA18273AGC5_TOP_DN_UP_t                    AGC5_TOP_DN_UP;                     /* AGC5 TOP DN/UP */
    TDA18273AGC3_Top_Adapt_Algorithm            AGC3_Top_Adapt_Algorithm;           /* AGC3_Top_Adapt_Algorithm */
    TDA18273AGC3_Adapt_TOP_t                    AGC3_Adapt_TOP_Low_Band;            /* AGC3 Adapt TOP Low Band */
    TDA18273AGC3_Adapt_TOP_t                    AGC3_Adapt_TOP_High_Band;           /* AGC3 Adapt TOP High Band */
    TDA18273AGC_Overload_TOP_t                  AGC_Overload_TOP;                   /* AGC Overload TOP */
    TDA18273TH_AGC_Adapt34_t                    TH_AGC_Adapt34;                     /* Adapt TOP 34 Gain Threshold */
    TDA18273RF_Atten_3dB_t                      RF_Atten_3dB;                       /* RF atten 3dB */
    TDA18273IF_Output_Level_t                   IF_Output_Level;                    /* IF Output Level */
    TDA18273S2D_Gain_t                          S2D_Gain;                           /* S2D gain */
    TDA18273Negative_Modulation_t               Negative_Modulation;                /* Negative modulation */

    /****************************************************************/
    /* GSK Settings                                                 */
    /****************************************************************/
    TDA18273AGCK_Steps_t                        AGCK_Steps;                         /* Step */
    TDA18273AGCK_Time_Constant_t                AGCK_Time_Constant;                 /* AGCK Time Constant */
    TDA18273AGC5_HPF_t                          AGC5_HPF;                           /* AGC5 HPF */
    TDA18273Pulse_Shaper_Disable_t              Pulse_Shaper_Disable;               /* Pulse Shaper Disable */

    /****************************************************************/
    /* H3H5 Settings                                                */
    /****************************************************************/
    TDA18273VHF_III_Mode_t                      VHF_III_Mode;                       /* VHF_III_Mode */

    /****************************************************************/
    /* PLL Settings                                                 */
    /****************************************************************/
    TDA18273LO_CP_Current_t                     LO_CP_Current;                      /* LO_CP_Current */

	/****************************************************************/
    /* MISC Settings                                                */
    /****************************************************************/
	TDA18273PD_Underload_t                      PD_Underload;                       /* PD Underload */
	UInt32										Freq_Start_LTE;                     /* Frequency start of high band for LTE */
} TDA18273StdCoefficients, *pTDA18273StdCoefficients;

typedef struct _TDA18273Object_t
{
    UInt32                          uRF;
    UInt32                          uProgRF;
    UInt32                          uIF;
    TDA18273StandardMode_t          StandardMode;
    pTDA18273StdCoefficients        pStandard;
    TDA18273StdCoefficients         Std_Array[TDA18273_StandardMode_Max-1];
} TDA18273Object_t, *pTDA18273Object_t, **ppTDA18273Object_t;

/* Standard Preset Definitions: */
#define TDA18273_INSTANCE_CUSTOM_STD_QAM_6MHZ \
    {                                                               /* QAM 6MHz */ \
	3600000,				                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_6MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_min_4pc,                                 /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_1MHz,                                       /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89wdBuV,                   /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,             /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_WIDE,                          /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,             /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_100dBuV,                      /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_NO_TOP_ADAPT,                            /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt Low Band */ \
	TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                     /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_QAM_8MHZ \
    {                                                               /* QAM 8MHz */ \
	5000000,				      				/* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_9MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_min_8pc,                                 /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_0_85MHz,                                    /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89wdBuV,                   /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_WIDE,                          /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_NO_TOP_ADAPT,                            /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ATSC_6MHZ \
    {                                                               /* ATSC */ \
	3250000,								/* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_6MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_0_4MHz,                                     /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d100_u94dBuV,                   /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_104dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_104dBuV,                              /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d112_u107dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d112_u107dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_3_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_3_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_6Vpp_min_10_3_19_7dB,           /* IF Output Level */ \
	TDA18273_S2D_Gain_3dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		662000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ISDBT_6MHZ \
    {                                                               /* ISDBT */ \
	3250000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_6MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_0_4MHz,                                     /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_6Vpp_min_10_3_19_7dB,            /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_DVBT_1_7MHZ \
    {                                                               /* DVB-T/T2 1.7MHz */ \
	850000,                                                     /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_1_5MHz,                                        /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_4_094ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_2_047ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_94dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Enabled,                              /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_DVBT_6MHZ \
    {                                                               /* DVB-T/T2 6MHz */ \
	3250000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_6MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_0_4MHz,                                     /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_4_094ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_2_047ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_94dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Enabled,                              /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_DVBT_7MHZ \
    {                                                               /* DVB-T/T2 7MHz */ \
	3500000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_7MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_min_8pc,                                 /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_4_094ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_2_047ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_94dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Enabled,                              /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_DVBT_8MHZ \
    {                                                               /* DVB-T/T2 8MHz */ \
	4000000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_4_094ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_2_047ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_94dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Enabled,                              /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_DVBT_10MHZ \
    {                                                               /* DVB-T/T2 10MHz */ \
	5000000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_9MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_DMBT_8MHZ \
    {                                                               /* DMB-T */ \
	4000000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT,                               /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT35,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_2_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Enabled,                              /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Enabled,                              /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_FM_RADIO \
    {                                                               /* FM */ \
	1250000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_1_5MHz,                                        /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_0_85MHz,                                    /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_8Vpp_min_8_22dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_MN \
    {                                                               /* NTSC M/N */ \
	5400000,                                                    /* IF */ \
	1750000,                                                    /* CF_Offset */ \
	TDA18273_LPF_6MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Enabled,                       /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_B \
    {                                                               /* PAL B */ \
	6400000,                                                    /* IF */ \
	2250000,                                                    /* CF_Offset */ \
	TDA18273_LPF_7MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Enabled,                       /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_GH \
    {                                                               /* PAL G/H */ \
	6750000,                                                    /* IF */ \
	2750000,                                                    /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Enabled,                       /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_I \
    {                                                               /* PAL I */ \
	7250000,                                                    /* IF */ \
	2750000,                                                    /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Enabled,                       /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_DK \
    {                                                               /* SECAM D/K */ \
	6850000,                                                    /* IF */ \
	2750000,                                                    /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Enabled,                       /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_L \
    {                                                               /* SECAM L */ \
	6750000,                                                    /* IF */ \
	2750000,                                                    /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Enabled,                                  /* IF notch */ \
	TDA18273_IFnotchToRSSI_Disabled,                            /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_6_min_9,              /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_LL \
    {                                                               /* SECAM L' */ \
	1250000,                                                    /* IF */ \
	-2750000,                                                   /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_6_min_9,              /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_ANALOG_BLIND_SCANNING \
    {                                                               /* Blind Scanning copy of PAL-I */ \
	7250000,                                                    /* IF */ \
	2750000,                                                    /* CF_Offset */ \
	TDA18273_LPF_8MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_0pc,                                     /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Disabled,                          /* DC notch IF PPF */ \
	TDA18273_IF_HPF_Disabled,                                   /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89dBuV,                    /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_1_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_TOP_ADAPT,                     /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_15_25dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_96dBuV,                               /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d105_u100dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d105_u100dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_TOP_ADAPT34,                             /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_1_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_0_7Vpp_min_9_21dB,                 /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_1_STEP_EACH_VSYNC_RISING_EDGE,  /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Enabled,                                  /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

#define TDA18273_INSTANCE_CUSTOM_STD_SCANXPRESS \
    {                                                               /* ScanXpress */ \
	5000000,                                                    /* IF */ \
	0,                                                          /* CF_Offset */ \
	TDA18273_LPF_9MHz,                                          /* LPF */ \
	TDA18273_LPFOffset_min_8pc,                                 /* LPF_Offset */ \
	TDA18273_DC_Notch_IF_PPF_Enabled,                           /* DC notch IF PPF */ \
	TDA18273_IF_HPF_0_85MHz,                                    /* Hi Pass */ \
	TDA18273_IF_Notch_Disabled,                                 /* IF notch */ \
	TDA18273_IFnotchToRSSI_Enabled,                             /* IF notch To RSSI */ \
	TDA18273_AGC1_TOP_I2C_DN_UP_d95_u89wdBuV,                   /* AGC1 TOP I2C DN/UP */ \
	TDA18273_AGC1_Adapt_TOP_DN_UP_0_Step,                       /* AGC1 Adapt TOP DN/UP */ \
		TDA18273_AGC1_DN_Time_Constant_8_188ms,                     /* AGC1 DN Time Constant */ \
	TDA18273_AGC1_Mode_LNA_ADAPT_WIDE,                          /* AGC1 mode */ \
	TDA18273_Range_LNA_Adapt_20dB_11dB,                         /* Range LNA Adapt */ \
	TDA18273_LNA_Adapt_RFAGC_Gv_Threshold_18_25dB,              /* LNA Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC1_Top_Adapt_RFAGC_Gv_Threshold_16_75dB,         /* AGC1 TOP Adapt RFAGC Gv Threshold */ \
	TDA18273_AGC2_TOP_DN_UP_d90_u84dBuV,                        /* AGC2 TOP DN/UP */ \
		TDA18273_AGC2_DN_Time_Constant_8_188ms,                     /* AGC2 DN Time Constant */ \
	TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C Low Band */ \
		TDA18273_AGC3_TOP_I2C_100dBuV,                              /* AGC3 TOP I2C High Band */ \
	TDA18273_AGC4_TOP_DN_UP_d110_u105dBuV,                      /* AGC4 TOP DN/UP */ \
	TDA18273_AGC5_TOP_DN_UP_d110_u105dBuV,                      /* AGC5 TOP DN/UP */ \
	TDA18273_Top_Adapt_NO_TOP_ADAPT,                            /* AGC3 TOP Adapt Algorithm */ \
	TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt Low Band */ \
		TDA18273_AGC3_Adapt_TOP_0_Step,                             /* AGC3 Adapt High Band */ \
	TDA18273_AGC_Overload_TOP_plus_9_plus_4_5_min_3_5,          /* AGC Overload TOP */ \
	TDA18273_TH_AGC_Adapt34_5dB,                                /* Adapt TOP 34 Gain Threshold */ \
	TDA18273_RF_Atten_3dB_Disabled,                             /* RF Atten 3dB */ \
	TDA18273_IF_Output_Level_1Vpp_min_6_24dB,                   /* IF Output Level */ \
	TDA18273_S2D_Gain_6dB,                                      /* S2D Gain */ \
	TDA18273_Negative_Modulation_Disabled,                      /* Negative Modulation */ \
	TDA18273_AGCK_Steps_0_2dB,                                  /* Step */ \
	TDA18273_AGCK_Time_Constant_32_768ms,                       /* AGCK Time Constant */ \
	TDA18273_AGC5_HPF_Disabled,                                 /* AGC5 HPF */ \
	TDA18273_Pulse_Shaper_Disable_Enabled,                      /* Pulse Shaper Disable */ \
	TDA18273_VHF_III_Mode_Disabled,                             /* VHF III Mode */ \
	TDA18273_LO_CP_Current_Enabled,                             /* LO CP Current */ \
		TDA18273_PD_Underload_Disabled,                             /* PD Underload */ \
		754000000													/* Frequency Start LTE */ \
    }

/* Standard Presets Aggregation: */
#define TDA18273_INSTANCE_CUSTOM_STD_DEF \
    { \
	TDA18273_INSTANCE_CUSTOM_STD_QAM_6MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_QAM_8MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_ATSC_6MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_ISDBT_6MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_DVBT_1_7MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_DVBT_6MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_DVBT_7MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_DVBT_8MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_DVBT_10MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_DMBT_8MHZ, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_FM_RADIO, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_MN, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_B, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_GH, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_I, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_DK, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_L, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_LL, \
	TDA18273_INSTANCE_CUSTOM_STD_ANALOG_BLIND_SCANNING, \
	TDA18273_INSTANCE_CUSTOM_STD_SCANXPRESS \
    }

#endif	/* __TDA18273_PRIV */
