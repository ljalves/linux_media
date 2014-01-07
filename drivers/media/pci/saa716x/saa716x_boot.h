#ifndef __SAA716x_BOOT_H
#define __SAA716x_BOOT_H

#define DISABLE_TIMEOUT		0x17
#define ENABLE_TIMEOUT		0x16

enum saa716x_boot_mode {
	SAA716x_EXT_BOOT = 1,
	SAA716x_INT_BOOT, /* GPIO[31:30] = 0x01 */
};

struct saa716x_dev;

extern int saa716x_core_boot(struct saa716x_dev *saa716x);
extern int saa716x_jetpack_init(struct saa716x_dev *saa716x);
extern void saa716x_core_reset(struct saa716x_dev *saa716x);

#endif /* __SAA716x_BOOT_H */
