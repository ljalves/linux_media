#ifndef __SAA716x_GPIO_H
#define __SAA716x_GPIO_H

#define BOOT_MODE	GPIO_31 | GPIO_30
#define AV_UNIT_B	GPIO_25
#define AV_UNIT_A	GPIO_24
#define AV_INTR_B	GPIO_01
#define AV_INTR_A	GPIO_00

struct saa716x_dev;

extern void saa716x_gpio_init(struct saa716x_dev *saa716x);

extern u32 saa716x_gpio_rd(struct saa716x_dev *saa716x);
extern void saa716x_gpio_wr(struct saa716x_dev *saa716x, u32 data);
extern void saa716x_gpio_ctl(struct saa716x_dev *saa716x, u32 mask, u32 bits);

extern void saa716x_gpio_bits(struct saa716x_dev *saa716x, u32 bits);

extern void saa716x_gpio_set_output(struct saa716x_dev *saa716x, int gpio);
extern void saa716x_gpio_set_input(struct saa716x_dev *saa716x, int gpio);
extern void saa716x_gpio_set_mode(struct saa716x_dev *saa716x, int gpio, int mode);
extern void saa716x_gpio_write(struct saa716x_dev *saa716x, int gpio, int set);
extern int saa716x_gpio_read(struct saa716x_dev *saa716x, int gpio);

#endif /* __SAA716x_GPIO_H */
