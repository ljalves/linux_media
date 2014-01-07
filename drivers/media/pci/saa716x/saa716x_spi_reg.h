#ifndef __SAA716x_SPI_REG_H
#define __SAA716x_SPI_REG_H

/* -------------- SPI Registers -------------- */

#define SPI_CONTROL_REG			0x000
#define SPI_SERIAL_INTER_ENABLE		(0x00000001 <<  7)
#define SPI_LSB_FIRST_ENABLE		(0x00000001 <<  6)
#define SPI_MODE_SELECT			(0x00000001 <<  5)
#define SPI_CLOCK_POLARITY		(0x00000001 <<  4)
#define SPI_CLOCK_PHASE			(0x00000001 <<  3)

#define SPI_STATUS			0x004
#define SPI_TRANSFER_FLAG		(0x00000001 <<  7)
#define SPI_WRITE_COLLISSION		(0x00000001 <<  6)
#define SPI_READ_OVERRUN		(0x00000001 <<  5)
#define SPI_MODE_FAULT			(0x00000001 <<  4)
#define SPI_SLAVE_ABORT			(0x00000001 <<  3)

#define SPI_DATA			0x008
#define SPI_BIDI_DATA			(0x000000ff <<  0)

#define SPI_CLOCK_COUNTER		0x00c
#define SPI_CLOCK			(0x00000001 <<  0)


#endif /* __SAA716x_SPI_REG_H */
