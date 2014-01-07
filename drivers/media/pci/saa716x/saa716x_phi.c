#include <linux/kernel.h>

#include "saa716x_mod.h"

#include "saa716x_phi_reg.h"

#include "saa716x_spi.h"
#include "saa716x_phi.h"
#include "saa716x_priv.h"

u32 PHI_0_REGS[] = {
	PHI_0_MODE,
	PHI_0_0_CONFIG,
	PHI_0_1_CONFIG,
	PHI_0_2_CONFIG,
	PHI_0_3_CONFIG
};

u32 PHI_1_REGS[] = {
	PHI_1_MODE,
	PHI_1_0_CONFIG,
	PHI_1_1_CONFIG,
	PHI_1_2_CONFIG,
	PHI_1_3_CONFIG,
	PHI_1_4_CONFIG,
	PHI_1_5_CONFIG,
	PHI_1_6_CONFIG,
	PHI_1_7_CONFIG
};

#define PHI_BASE(__port)	((				\
	(__port == PHI_1) ?					\
		PHI_1_BASE :					\
		PHI_0_BASE					\
))

#define PHI_APERTURE(_port)	((				\
	(__port == PHI_1) ?					\
		PHI_1_APERTURE:					\
		PHI_0_APERTURE					\
))

#define PHI_REG(__port, __reg)	((				\
	(__port == PHI_1) ?					\
		PHI_1_REGS[__reg] :				\
		PHI_0_REGS[__reg]				\
))

#define PHI_SLAVE(__port, __slave)	((			\
	PHI_BASE(__port) + (__slave * (PHI_APERTURE(__port)))	\
))

/* // Read SAA716x registers
 * SAA716x_EPRD(PHI_0, PHI_REG(__port, __reg))
 * SAA716x_EPWR(PHI_1, PHI_REG(__port, __reg), __data)
 *
 * // Read slave registers
 * SAA716x_EPRD(PHI_0, PHI_SLAVE(__port, __slave, __offset))
 * SAA716x_EPWR(PHI_1, PHI_SLAVE(__port, __slave, _offset), __data)
 */

int saa716x_init_phi(struct saa716x_dev *saa716x, u32 port, u8 slave)
{
	int i;

	/* Reset */
	SAA716x_EPWR(PHI_0, PHI_SW_RST, 0x1);

	for (i = 0; i < 20; i++) {
		msleep(1);
		if (!(SAA716x_EPRD(PHI_0, PHI_SW_RST)))
			break;
	}

	return 0;
}

int saa716x_phi_init(struct saa716x_dev *saa716x)
{
	uint32_t value;

	/* init PHI 0 to FIFO mode */
	value = 0;
	value |= PHI_FIFO_MODE;
	SAA716x_EPWR(PHI_0, PHI_0_MODE, value);

	value = 0;
	value |= 0x02; /* chip select 1 */
	value |= 0x00 << 8; /* ready mask */
	value |= 0x03 << 12; /* strobe time */
	value |= 0x06 << 20; /* cycle time */
	SAA716x_EPWR(PHI_0, PHI_0_0_CONFIG, value);

	/* init PHI 1 to SRAM mode, auto increment on */
	value = 0;
	value |= PHI_AUTO_INCREMENT;
	SAA716x_EPWR(PHI_0, PHI_1_MODE, value);

	value = 0;
	value |= 0x01; /* chip select 0 */
	value |= 0x00 << 8; /* ready mask */
	value |= 0x03 << 12; /* strobe time */
	value |= 0x05 << 20; /* cycle time */
	SAA716x_EPWR(PHI_0, PHI_1_0_CONFIG, value);

	value = 0;
	value |= PHI_ALE_POL; /* ALE is active high */
	SAA716x_EPWR(PHI_0, PHI_POLARITY, value);

	SAA716x_EPWR(PHI_0, PHI_TIMEOUT, 0x2a);

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_phi_init);

int saa716x_phi_write(struct saa716x_dev *saa716x, u32 address, const u8 * data, int length)
{
	int i;

	for (i = 0; i < length; i += 4) {
		SAA716x_EPWR(PHI_1, address, *((u32 *) &data[i]));
		address += 4;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_phi_write);

int saa716x_phi_read(struct saa716x_dev *saa716x, u32 address, u8 * data, int length)
{
	int i;

	for (i = 0; i < length; i += 4) {
		*((u32 *) &data[i]) = SAA716x_EPRD(PHI_1, address);
		address += 4;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_phi_read);

int saa716x_phi_write_fifo(struct saa716x_dev *saa716x, const u8 * data, int length)
{
	int i;

	for (i = 0; i < length; i += 4) {
		SAA716x_EPWR(PHI_0, PHI_0_0_RW_0, *((u32 *) &data[i]));
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_phi_write_fifo);
