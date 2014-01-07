#ifndef __SAA716x_SPI_H
#define __SAA716x_SPI_H

struct saa716x_dev;

struct saa716x_spi_config {
	u8 clk_count;
	u8 clk_pol:1;
	u8 clk_pha:1;
	u8 LSB_first:1;
};

struct saa716x_spi_state {
	struct spi_master *master;
	struct saa716x_dev *saa716x;
};

extern void saa716x_spi_write(struct saa716x_dev *saa716x, const u8 *data, int length);

extern int saa716x_spi_init(struct saa716x_dev *saa716x);
extern void saa716x_spi_exit(struct saa716x_dev *saa716x);

#endif /* __SAA716x_SPI_H */
