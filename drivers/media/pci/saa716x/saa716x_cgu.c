#include <linux/delay.h>

#include "saa716x_mod.h"

#include "saa716x_cgu_reg.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

u32 cgu_clk[14] = {
	CGU_FDC_0,
	CGU_FDC_1,
	CGU_FDC_2,
	CGU_FDC_3,
	CGU_FDC_4,
	CGU_FDC_5,
	CGU_FDC_6,
	CGU_FDC_7,
	CGU_FDC_8,
	CGU_FDC_9,
	CGU_FDC_10,
	CGU_FDC_11,
	CGU_FDC_12,
	CGU_FDC_13
};

char *clk_desc[14] = {
	"Clk PSS",
	"Clk DCS",
	"Clk SPI",
	"Clk I2C/Boot",
	"Clk PHI",
	"Clk VI0",
	"Clk VI1",
	"Clk FGPI0",
	"Clk FGPI1",
	"Clk FGPI2",
	"Clk FGPI3",
	"Clk AI0",
	"Clk AI1",
	"Clk Phy"
};

int saa716x_getbootscript_setup(struct saa716x_dev *saa716x)
{
	struct saa716x_cgu *cgu = &saa716x->cgu;

	u8 i;
	s8 N = 0;
	s16 M = 0;

	SAA716x_EPWR(CGU, CGU_PCR_0_6, CGU_PCR_RUN); /* GREG */
	SAA716x_EPWR(CGU, CGU_PCR_0_3, CGU_PCR_RUN); /* PSS_MMU */
	SAA716x_EPWR(CGU, CGU_PCR_0_4, CGU_PCR_RUN); /* PSS_DTL2MTL */
	SAA716x_EPWR(CGU, CGU_PCR_0_5, CGU_PCR_RUN); /* MSI */
	SAA716x_EPWR(CGU, CGU_PCR_3_2, CGU_PCR_RUN); /* I2C */
	SAA716x_EPWR(CGU, CGU_PCR_4_1, CGU_PCR_RUN); /* PHI */
	SAA716x_EPWR(CGU, CGU_PCR_0_7, CGU_PCR_RUN); /* GPIO */
	SAA716x_EPWR(CGU, CGU_PCR_2_1, CGU_PCR_RUN); /* SPI */
	SAA716x_EPWR(CGU, CGU_PCR_1_1, CGU_PCR_RUN); /* DCS */
	SAA716x_EPWR(CGU, CGU_PCR_3_1, CGU_PCR_RUN); /* BOOT */

	/* get all dividers */
	for (i = 0; i < CGU_CLKS; i++) {
		cgu->clk_boot_div[i] = SAA716x_EPRD(CGU, cgu_clk[i]);
		cgu->clk_curr_div[i] = cgu->clk_boot_div[i];

		N = (cgu->clk_boot_div[i] >> 11) & 0xff;
		N *= -1;
		M = ((cgu->clk_boot_div[i] >>  3) & 0xff) + N;

		if (M)
			cgu->clk_freq[i] = (u32 ) N * PLL_FREQ / (u32 ) M;
		else
			cgu->clk_freq[i] = 0;

		dprintk(SAA716x_DEBUG, 1, "Domain %d: %s <0x%02x> Divider: 0x%x --> N=%d, M=%d, freq=%d",
			i, clk_desc[i], cgu_clk[i], cgu->clk_boot_div[i], N, M, cgu->clk_freq[i]);
	}
	/* store clock settings */
	cgu->clk_vi_0[0] = cgu->clk_freq[CLK_DOMAIN_VI0];
	cgu->clk_vi_0[1] = cgu->clk_freq[CLK_DOMAIN_VI0];
	cgu->clk_vi_0[2] = cgu->clk_freq[CLK_DOMAIN_VI0];
	cgu->clk_vi_1[0] = cgu->clk_freq[CLK_DOMAIN_VI1];
	cgu->clk_vi_1[1] = cgu->clk_freq[CLK_DOMAIN_VI1];
	cgu->clk_vi_1[2] = cgu->clk_freq[CLK_DOMAIN_VI1];

	return 0;
}

int saa716x_set_clk_internal(struct saa716x_dev *saa716x, u32 port)
{
	struct saa716x_cgu *cgu = &saa716x->cgu;

	u8 delay = 1;

	switch (port) {
	case PORT_VI0_VIDEO:
		cgu->clk_int_port[PORT_VI0_VIDEO] = 1;

		if (!cgu->clk_int_port[PORT_VI0_VBI]) {
			delay = 0;
			break;
		}

		SAA716x_CGU_CLKRUN(5);
		break;

	case PORT_VI0_VBI:
		cgu->clk_int_port[PORT_VI0_VBI] = 1;

		if (!cgu->clk_int_port[PORT_VI0_VIDEO]) {
			delay = 0;
			break;
		}

		SAA716x_CGU_CLKRUN(5);
		break;

	case PORT_VI1_VIDEO:
		cgu->clk_int_port[PORT_VI1_VIDEO] = 1;

		if (!cgu->clk_int_port[PORT_VI1_VBI]) {
			delay = 0;
			break;
		}

		SAA716x_CGU_CLKRUN(6);
		break;

	case PORT_VI1_VBI:
		cgu->clk_int_port[PORT_VI1_VBI] = 1;

		if (!cgu->clk_int_port[PORT_VI1_VIDEO]) {
			delay = 0;
			break;
		}

		SAA716x_CGU_CLKRUN(6);
		break;

	case PORT_FGPI0:
		cgu->clk_int_port[PORT_FGPI0] = 1;
		SAA716x_CGU_CLKRUN(7);
		break;

	case PORT_FGPI1:
		cgu->clk_int_port[PORT_FGPI1] = 1;
		SAA716x_CGU_CLKRUN(8);
		break;

	case PORT_FGPI2:
		cgu->clk_int_port[PORT_FGPI2] = 1;
		SAA716x_CGU_CLKRUN(9);
		break;

	case PORT_FGPI3:
		cgu->clk_int_port[PORT_FGPI3] = 1;
		SAA716x_CGU_CLKRUN(10);
		break;

	case PORT_AI0:
		cgu->clk_int_port[PORT_AI0] = 1;
		SAA716x_CGU_CLKRUN(11);
		break;

	case PORT_AI1:
		cgu->clk_int_port[PORT_AI1] = 1;
		SAA716x_CGU_CLKRUN(12);
		break;

	case PORT_ALL:
		SAA716x_CGU_CLKRUN(5);
		SAA716x_CGU_CLKRUN(6);
		SAA716x_CGU_CLKRUN(7);
		SAA716x_CGU_CLKRUN(8);
		SAA716x_CGU_CLKRUN(9);
		SAA716x_CGU_CLKRUN(10);
		SAA716x_CGU_CLKRUN(11);
		SAA716x_CGU_CLKRUN(12);

		cgu->clk_int_port[PORT_VI0_VIDEO] = 1;
		cgu->clk_int_port[PORT_VI0_VBI] = 1;
		cgu->clk_int_port[PORT_VI1_VIDEO] = 1;
		cgu->clk_int_port[PORT_VI1_VBI] = 1;
		cgu->clk_int_port[PORT_FGPI0] = 1;
		cgu->clk_int_port[PORT_FGPI1] = 1;
		cgu->clk_int_port[PORT_FGPI2] = 1;
		cgu->clk_int_port[PORT_FGPI3] = 1;
		cgu->clk_int_port[PORT_AI0] = 1;
		cgu->clk_int_port[PORT_AI1] = 1;
		break;

	default:
		dprintk(SAA716x_ERROR, 1, "Unknown port <%02x>", port);
		delay = 0;
		break;
	}

	/* wait for PLL */
	if (delay)
		msleep(1);

	return 0;
}

int saa716x_set_clk_external(struct saa716x_dev *saa716x, u32 port)
{
	struct saa716x_cgu *cgu = &saa716x->cgu;

	u8 delay = 1;

	switch (port) {
	case PORT_VI0_VIDEO:
		cgu->clk_int_port[PORT_VI0_VIDEO] = 0;

		if (!cgu->clk_int_port[PORT_VI0_VBI]) {
			delay = 0;
			break;
		}

		SAA716x_EPWR(CGU, CGU_FS1_5, 0x2); /* VI 0 clk */
		SAA716x_EPWR(CGU, CGU_ESR_5, 0x0); /* disable divider */
		break;

	case PORT_VI0_VBI:
		cgu->clk_int_port[PORT_VI0_VBI] = 0;

		if (!cgu->clk_int_port[PORT_VI0_VIDEO]) {
			delay = 0;
			break;
		}

		SAA716x_EPWR(CGU, CGU_FS1_5, 0x2); /* VI 0 clk */
		SAA716x_EPWR(CGU, CGU_ESR_5, 0x0); /* disable divider */
		break;

	case PORT_VI1_VIDEO:
		cgu->clk_int_port[PORT_VI1_VIDEO] = 0;

		if (!cgu->clk_int_port[PORT_VI1_VBI]) {
			delay = 0;
			break;
		}

		SAA716x_EPWR(CGU, CGU_FS1_6, 0x3); /* VI 1 clk */
		SAA716x_EPWR(CGU, CGU_ESR_6, 0x0); /* disable divider */
		break;

	case PORT_VI1_VBI:
		cgu->clk_int_port[PORT_VI1_VBI] = 0;

		if (!cgu->clk_int_port[PORT_VI1_VIDEO]) {
			delay = 0;
			break;
		}

		SAA716x_EPWR(CGU, CGU_FS1_6, 0x3); /* VI 1 clk */
		SAA716x_EPWR(CGU, CGU_ESR_6, 0x0); /* disable divider */
		break;

	case PORT_FGPI0:
		cgu->clk_int_port[PORT_FGPI0] = 0;

		SAA716x_EPWR(CGU, CGU_FS1_7, 0x4); /* FGPI 0 clk */
		SAA716x_EPWR(CGU, CGU_ESR_7, 0x0); /* disable divider */
		break;

	case PORT_FGPI1:
		cgu->clk_int_port[PORT_FGPI1] = 0;

		SAA716x_EPWR(CGU, CGU_FS1_8, 0x5); /* FGPI 1 clk */
		SAA716x_EPWR(CGU, CGU_ESR_8, 0x0); /* disable divider */
		break;

	case PORT_FGPI2:
		cgu->clk_int_port[PORT_FGPI2] = 0;

		SAA716x_EPWR(CGU, CGU_FS1_9, 0x6); /* FGPI 2 clk */
		SAA716x_EPWR(CGU, CGU_ESR_9, 0x0); /* disable divider */
		break;

	case PORT_FGPI3:
		cgu->clk_int_port[PORT_FGPI3] = 0;

		SAA716x_EPWR(CGU, CGU_FS1_10, 0x7); /* FGPI 3 clk */
		SAA716x_EPWR(CGU, CGU_ESR_10, 0x0); /* disable divider */
		break;

	case PORT_AI0:
		cgu->clk_int_port[PORT_AI0] = 1;

		SAA716x_EPWR(CGU, CGU_FS1_11, 0x8); /* AI 0 clk */
		SAA716x_EPWR(CGU, CGU_ESR_11, 0x0); /* disable divider */
		break;

	case PORT_AI1:
		cgu->clk_int_port[PORT_AI1] = 1;

		SAA716x_EPWR(CGU, CGU_FS1_12, 0x9); /* AI 1 clk */
		SAA716x_EPWR(CGU, CGU_ESR_12, 0x0); /* disable divider */
		break;

	default:
		dprintk(SAA716x_ERROR, 1, "Unknown port <%02x>", port);
		delay = 0;
		break;

	}

	if (delay)
		msleep(1);

	return 0;
}

int saa716x_get_clk(struct saa716x_dev *saa716x,
		    enum saa716x_clk_domain domain,
		    u32 *frequency)
{
	struct saa716x_cgu *cgu = &saa716x->cgu;

	switch (domain) {
	case CLK_DOMAIN_PSS:
	case CLK_DOMAIN_DCS:
	case CLK_DOMAIN_SPI:
	case CLK_DOMAIN_I2C:
	case CLK_DOMAIN_PHI:
	case CLK_DOMAIN_VI0:
	case CLK_DOMAIN_VI1:
	case CLK_DOMAIN_FGPI0:
	case CLK_DOMAIN_FGPI1:
	case CLK_DOMAIN_FGPI2:
	case CLK_DOMAIN_FGPI3:
	case CLK_DOMAIN_AI0:
	case CLK_DOMAIN_AI1:
	case CLK_DOMAIN_PHY:
		*frequency = cgu->clk_freq[domain];
		break;

	case CLK_DOMAIN_VI0VBI:
		*frequency = cgu->clk_freq[CLK_DOMAIN_VI0];
		break;

	case CLK_DOMAIN_VI1VBI:
		*frequency =cgu->clk_freq[CLK_DOMAIN_VI1];
		break;
	default:
		dprintk(SAA716x_ERROR, 1, "Error Clock domain <%02x>", domain);
		break;
	}

	return 0;
}

int saa716x_set_clk(struct saa716x_dev *saa716x,
		    enum saa716x_clk_domain domain,
		    u32 frequency)
{
	struct saa716x_cgu *cgu = &saa716x->cgu;

	u32 M = 1, N = 1, reset, i;
	s8 N_tmp, M_tmp, sub, add, lsb;


	if (cgu->clk_freq_min > frequency)
		frequency = cgu->clk_freq_min;

	if (cgu->clk_freq_max < frequency)
		frequency = cgu->clk_freq_max;

	switch (domain) {
	case CLK_DOMAIN_PSS:
	case CLK_DOMAIN_DCS:
	case CLK_DOMAIN_SPI:
	case CLK_DOMAIN_I2C:
	case CLK_DOMAIN_PHI:
	case CLK_DOMAIN_FGPI0:
	case CLK_DOMAIN_FGPI1:
	case CLK_DOMAIN_FGPI2:
	case CLK_DOMAIN_FGPI3:
	case CLK_DOMAIN_AI0:
	case CLK_DOMAIN_AI1:
	case CLK_DOMAIN_PHY:

		if (frequency == cgu->clk_freq[domain])
			return 0; /* same frequency */
		break;

	case CLK_DOMAIN_VI0:

		if (frequency == cgu->clk_vi_0[1]) {
			return 0;

		} else if (frequency == cgu->clk_vi_0[0]) {
			cgu->clk_vi_0[1] = frequency; /* store */

			if (frequency == cgu->clk_vi_0[2])
				return 0;

		} else {
			cgu->clk_vi_0[1] = frequency;

			if (frequency != cgu->clk_vi_0[2])
				return 0;

		}
		break;

	case CLK_DOMAIN_VI1:
		if (frequency == cgu->clk_vi_1[1]) {
			return 0;

		} else if (frequency == cgu->clk_vi_1[0]) {
			cgu->clk_vi_1[1] = frequency; /* store */

			if (frequency == cgu->clk_vi_1[2])
				return 0;

		} else {
			cgu->clk_vi_1[1] = frequency;

			if (frequency != cgu->clk_vi_1[2])
				return 0;

		}
		break;

	case CLK_DOMAIN_VI0VBI:
		if (frequency == cgu->clk_vi_0[2]) {
			return 0;

		} else if (frequency == cgu->clk_vi_0[0]) {
			cgu->clk_vi_0[2] = frequency; /* store */

			if (frequency == cgu->clk_vi_0[1])
				return 0;

		} else {
			cgu->clk_vi_0[2] = frequency; /* store */

			if (frequency != cgu->clk_vi_0[1])
				return 0;

		}
		domain = CLK_DOMAIN_VI0; /* change domain */
		break;

	case CLK_DOMAIN_VI1VBI:
		if (frequency == cgu->clk_vi_1[2]) {
			return 0;

		} else if (frequency == cgu->clk_vi_1[0]) {
			cgu->clk_vi_1[2] = frequency; /* store */

			if (frequency == cgu->clk_vi_1[1])
				return 0;

		} else {
			cgu->clk_vi_1[2] = frequency; /* store */

			if (frequency != cgu->clk_vi_1[1])
				return 0;

		}
		domain = CLK_DOMAIN_VI1; /* change domain */
		break;
	}

	/* calculate divider */
	do {
		M = (N * PLL_FREQ) / frequency;
		if (M == 0)
			N++;

	} while (M == 0);

	/* calculate frequency */
	cgu->clk_freq[domain] = (N * PLL_FREQ) / M;

	N_tmp = N & 0xff;
	M_tmp = M & 0xff;
	sub = -N_tmp;
	add = M_tmp - N_tmp;
	lsb = 4; /* run */

	if (((10 * N) / M) <= 5)
		lsb |= 1; /* stretch */

	/* store new divider */
	cgu->clk_curr_div[domain] = sub & 0xff;
	cgu->clk_curr_div[domain] <<= 8;
	cgu->clk_curr_div[domain] |= add & 0xff;
	cgu->clk_curr_div[domain] <<= 3;
	cgu->clk_curr_div[domain] |= lsb;

	dprintk(SAA716x_DEBUG, 1, "Domain <0x%02x> Frequency <%d> Set Freq <%d> N=%d M=%d Divider <0x%02x>",
		domain,
		frequency,
		cgu->clk_freq[domain],
		N,
		M,
		cgu->clk_curr_div[domain]);

	reset = 0;

	/* Reset */
	SAA716x_EPWR(CGU, cgu_clk[domain], cgu->clk_curr_div[domain] | 0x2);

	/* Reset disable */
	for (i = 0; i < 1000; i++) {
		udelay(10);
		reset = SAA716x_EPRD(CGU, cgu_clk[domain]);

		if (cgu->clk_curr_div[domain] == reset)
			break;
	}

	if (cgu->clk_curr_div[domain] != reset)
		SAA716x_EPWR(CGU, cgu_clk[domain], cgu->clk_curr_div[domain]);

	return 0;
}

int saa716x_cgu_init(struct saa716x_dev *saa716x)
{
	struct saa716x_cgu *cgu = &saa716x->cgu;

	cgu->clk_freq_min = PLL_FREQ / 255;
	if (PLL_FREQ > (cgu->clk_freq_min * 255))
		cgu->clk_freq_min++;

	cgu->clk_freq_max = PLL_FREQ;

	saa716x_getbootscript_setup(saa716x);
	saa716x_set_clk_internal(saa716x, PORT_ALL);

	return 0;
}
EXPORT_SYMBOL(saa716x_cgu_init);
