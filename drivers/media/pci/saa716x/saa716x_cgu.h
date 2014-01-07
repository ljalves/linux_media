#ifndef __SAA716x_CGU_H
#define __SAA716x_CGU_H

#define PLL_FREQ	2500

#define SAA716x_CGU_CLKRUN(__reg)  do {						\
	SAA716x_EPWR(CGU, CGU_PCR_##__reg, CGU_PCR_RUN); /* Run */		\
	SAA716x_EPWR(CGU, CGU_SCR_##__reg, CGU_SCR_ENF1); /* Switch */		\
	SAA716x_EPWR(CGU, CGU_FS1_##__reg, 0x00000000); /* PLL Clk */		\
	SAA716x_EPWR(CGU, CGU_ESR_##__reg, CGU_ESR_FD_EN); /* Frac div */	\
} while (0)

enum saa716x_clk_domain {
	CLK_DOMAIN_PSS		= 0,
	CLK_DOMAIN_DCS		= 1,
	CLK_DOMAIN_SPI		= 2,
	CLK_DOMAIN_I2C		= 3,
	CLK_DOMAIN_PHI		= 4,
	CLK_DOMAIN_VI0		= 5,
	CLK_DOMAIN_VI1		= 6,
	CLK_DOMAIN_FGPI0	= 7,
	CLK_DOMAIN_FGPI1	= 8,
	CLK_DOMAIN_FGPI2	= 9,
	CLK_DOMAIN_FGPI3	= 10,
	CLK_DOMAIN_AI0		= 11,
	CLK_DOMAIN_AI1		= 12,
	CLK_DOMAIN_PHY		= 13,
	CLK_DOMAIN_VI0VBI	= 14,
	CLK_DOMAIN_VI1VBI	= 15
};

#define PORT_VI0_VIDEO		0
#define PORT_VI0_VBI		2
#define	PORT_VI1_VIDEO		3
#define PORT_VI1_VBI		5
#define PORT_FGPI0		6
#define	PORT_FGPI1		7
#define PORT_FGPI2		8
#define PORT_FGPI3		9
#define PORT_AI0		10
#define PORT_AI1		11
#define PORT_ALL		12

#define CGU_CLKS	14

struct saa716x_cgu {
	u8	clk_int_port[12];
	u32	clk_vi_0[3];
	u32	clk_vi_1[3];
	u32	clk_boot_div[CGU_CLKS];
	u32	clk_curr_div[CGU_CLKS];
	u32	clk_freq[CGU_CLKS];
	u32	clk_freq_min;
	u32	clk_freq_max;
};

extern int saa716x_cgu_init(struct saa716x_dev *saa716x);
extern int saa716x_set_clk_internal(struct saa716x_dev *saa716x, u32 port);
extern int saa716x_set_clk_external(struct saa716x_dev *saa716x, u32 port);

#endif /* __SAA716x_CGU_H */
