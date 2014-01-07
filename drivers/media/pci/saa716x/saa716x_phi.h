#ifndef __SAA716x_PHI_H
#define __SAA716x_PHI_H

/* PHI SLAVE */
#define PHI_SLAVE_0		0
#define PHI_SLAVE_1		1
#define PHI_SLAVE_2		2
#define PHI_SLAVE_3		3
#define PHI_SLAVE_4		4
#define PHI_SLAVE_5		5
#define PHI_SLAVE_6		6
#define PHI_SLAVE_7		7

/* PHI_REG */
#define PHI_MODE		0
#define PHI_CONFIG_0		1
#define PHI_CONFIG_1		2
#define PHI_CONFIG_2		3
#define PHI_CONFIG_3		4
#define PHI_CONFIG_4		5
#define PHI_CONFIG_5		6
#define PHI_CONFIG_6		7
#define PHI_CONFIG_7		8

#define PHI_0_BASE		0x1000
#define PHI_0_APERTURE		0x0800

#define PHI_1_BASE		0x0000
#define PHI_1_APERTURE		0xfffc

struct saa716x_dev;

extern int saa716x_init_phi(struct saa716x_dev *saa716x, u32 port, u8 slave);
extern int saa716x_phi_init(struct saa716x_dev *saa716x);
extern int saa716x_phi_write(struct saa716x_dev *saa716x, u32 address, const u8 *data, int length);
extern int saa716x_phi_read(struct saa716x_dev *saa716x, u32 address, u8 *data, int length);
extern int saa716x_phi_write_fifo(struct saa716x_dev *saa716x, const u8 * data, int length);

#endif /* __SAA716x_PHI_H */
