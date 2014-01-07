#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/spinlock.h>
#include <linux/io.h>

#include <linux/spi/spi.h>

#include "saa716x_mod.h"

#include "saa716x_spi_reg.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

#if 0 // not needed atm
int saa716x_spi_irqevent(struct saa716x_dev *saa716x)
{
	u32 stat, mask;

	BUG_ON(saa716x == NULL);

	stat = SAA716x_EPRD(SPI, SPI_STATUS);
	mask = SAA716x_EPRD(SPI, SPI_CONTROL_REG) & SPI_SERIAL_INTER_ENABLE;
	if ((!stat && !mask))
		return -1;

	dprintk(SAA716x_DEBUG, 0, "SPI event: Stat=<%02x>", stat);

	if (stat & SPI_TRANSFER_FLAG)
		dprintk(SAA716x_DEBUG, 0, "<TXFER> ");
	if (stat & SPI_WRITE_COLLISSION)
		dprintk(SAA716x_DEBUG, 0, "<WCOLL> ");
	if (stat & SPI_READ_OVERRUN)
		dprintk(SAA716x_DEBUG, 0, "<ROFLW> ");
	if (stat & SPI_MODE_FAULT)
		dprintk(SAA716x_DEBUG, 0, "<FAULT> ");
	if (stat & SPI_SLAVE_ABORT)
		dprintk(SAA716x_DEBUG, 0, "<ABORT> ");

	return 0;
}
#endif

void saa716x_spi_write(struct saa716x_dev *saa716x, const u8 *data, int length)
{
	int i;
	u32 value;
	int rounds;

	for (i = 0; i < length; i++) {
		SAA716x_EPWR(SPI, SPI_DATA, data[i]);
		rounds = 0;
		value = SAA716x_EPRD(SPI, SPI_STATUS);

		while ((value & SPI_TRANSFER_FLAG) == 0 && rounds < 5000) {
			value = SAA716x_EPRD(SPI, SPI_STATUS);
			rounds++;
		}
	}
}
EXPORT_SYMBOL_GPL(saa716x_spi_write);

#if 0 // not needed atm
static int saa716x_spi_status(struct saa716x_dev *saa716x, u32 *status)
{
	u32 stat;

	stat = SAA716x_EPRD(SPI, SPI_STATUS);

	if (stat & SPI_TRANSFER_FLAG)
		dprintk(SAA716x_DEBUG, 1, "Transfer complete <%02x>", stat);

	if (stat & SPI_WRITE_COLLISSION)
		dprintk(SAA716x_DEBUG, 1, "Write collission <%02x>", stat);

	if (stat & SPI_READ_OVERRUN)
		dprintk(SAA716x_DEBUG, 1, "Read Overrun <%02x>", stat);

	if (stat & SPI_MODE_FAULT)
		dprintk(SAA716x_DEBUG, 1, "MODE fault <%02x>", stat);

	if (stat & SPI_SLAVE_ABORT)
		dprintk(SAA716x_DEBUG, 1, "SLAVE abort <%02x>", stat);

	*status = stat;

	return 0;
}

#define SPI_CYCLE_TIMEOUT	100

static int saa716x_spi_xfer(struct saa716x_dev *saa716x, u32 *data)
{
	u32 i, status = 0;

	/* write data and wait for completion */
	SAA716x_EPWR(SPI, SPI_DATA, data[i]);
	for (i = 0; i < SPI_CYCLE_TIMEOUT; i++) {
		msleep(10);
		saa716x_spi_status(saa716x, &status);
#if 0
		if (status & SPI_TRANSFER_FLAG) {
			data = SAA716x_EPRD(SPI, SPI_DATA);
			return 0;
		}
#endif
		if (status & (SPI_WRITE_COLLISSION	|
			      SPI_READ_OVERRUN		|
			      SPI_MODE_FAULT		|
			      SPI_SLAVE_ABORT))

			return -EIO;
	}

	return -EIO;
}

#if 0
static int saa716x_spi_wr(struct saa716x_dev *saa716x, const u8 *data, int length)
{
	struct saa716x_spi_config *config = saa716x->spi_config;
	u32 gpio_mask;
	int ret = 0;

	// protect against multiple access
	spin_lock(&saa716x->gpio_lock);

	// configure the module
	saa716x_spi_config(saa716x);

    // check input

	// change polarity of GPIO if active high
	if (config->active_hi) {
		select  = 1;
		release = 0;
	}

	// configure GPIO, first set output register to low selected level
	saa716x_gpio_write(saa716x, gpio, select);

	// set mode register to register controlled (0)
	gpio_mask = (1 << gpio);
	saa716x_set_gpio_mode(saa716x, gpio_mask, 0);

	// configure bit as output (0)
	saa716x_gpio_ctl(saa716x, gpio_mask, 0);

	// wait at least 500ns before sending a byte
	msleep(1);

	// send command
	for (i = 0; i < dwCommandSize; i++) {
		ucData   = 0;
//		dwStatus = TransferData(pucCommand[i], &ucData);
		ret = saa716x_spi_xfer(saa716x);
		//tmDBGPRINTEx(4,("Info: Command 0x%x ", pucCommand[i]  ));

		/* If command length > 1, disable CS at the end of each command.
		 * But after the last command byte CS must be left active!
		 */
		if ((dwCommandSize > 1) && (i < dwCommandSize - 1)) {

			saa716x_gpio_write(saa716x, gpio, release);
			msleep(1); /* 500 nS minimum */
			saa716x_gpio_write(saa716x, gpio, select);
		}

		if (ret != 0) {
			dprintk(SAA716x_ERROR, 1, "ERROR: Command transfer failed");
			msleep(1); /* 500 nS minimum */
			saa716x_gpio_write(saa716x, gpio, release); /* release GPIO */
			spin_unlock(&saa716x->spi_lock);
			return ret;
		}

		if (config->LSB_first)
			dwTransferByte++;
		else
			dwTransferByte--;
	}

// assume that the byte order is the same as the bit order

// send read address

// send data

// wait at least 500ns before releasing slave

// release GPIO pin

	// release spinlock
	spin_unlock(&saa716x->gpio_lock);
}
#endif

#define MODEBITS (SPI_CPOL | SPI_CPHA)

static int saa716x_spi_setup(struct spi_device *spi)
{
	struct spi_master *master		= spi->master;
	struct saa716x_spi_state *saa716x_spi	= spi_master_get_devdata(master);
	struct saa716x_dev *saa716x		= saa716x_spi->saa716x;
	struct saa716x_spi_config *config	= &saa716x->spi_config;

	u8 control = 0;

	if (spi->mode & ~MODEBITS) {
		dprintk(SAA716x_ERROR, 1, "ERROR: Unsupported MODE bits <%x>",
			spi->mode & ~MODEBITS);

		return -EINVAL;
	}

	SAA716x_EPWR(SPI, SPI_CLOCK_COUNTER, config->clk_count);

	control |= SPI_MODE_SELECT; /* SPI Master */

	if (config->LSB_first)
		control |= SPI_LSB_FIRST_ENABLE;

	if (config->clk_pol)
		control |= SPI_CLOCK_POLARITY;

	if (config->clk_pha)
		control |= SPI_CLOCK_PHASE;

	SAA716x_EPWR(SPI, SPI_CONTROL_REG, control);

	return 0;
}

static void saa716x_spi_cleanup(struct spi_device *spi)
{

}

static int saa716x_spi_transfer(struct spi_device *spi, struct spi_message *msg)
{
	struct spi_master *master		= spi->master;
	struct saa716x_spi_state *saa716x_spi	= spi_master_get_devdata(master);
	struct saa716x_dev *saa716x		= saa716x_spi->saa716x;
	unsigned long flags;

	spin_lock_irqsave(&saa716x->gpio_lock, flags);
#if 0
	if (saa716x_spi->run == QUEUE_STOPPED) {
		spin_unlock_irqrestore(&saa716x_spi->lock, flags);
		return -ESHUTDOWN;
	}

	msg->actual_length = 0;
	msg->status = -EINPROGRESS;
	msg->state = START_STATE;

	list_add_tail(&msg->queue, &saa716x_spi->queue);

	if (saa716x_spi->run == QUEUE_RUNNING && !saa716x_spi->busy)
		queue_work(saa716x_spi->workqueue, &saa716x_spi->pump_messages);
#endif
	spin_unlock_irqrestore(&saa716x->gpio_lock, flags);

	return 0;
}

int saa716x_spi_init(struct saa716x_dev *saa716x)
{
	struct pci_dev *pdev = saa716x->pdev;
	struct spi_master *master;
	struct saa716x_spi_state *saa716x_spi;
	int ret;

	dprintk(SAA716x_DEBUG, 1, "Initializing SAA%02x I2C Core",
		saa716x->pdev->device);

	master = spi_alloc_master(&pdev->dev, sizeof (struct saa716x_spi_state));
	if (master == NULL) {
		dprintk(SAA716x_ERROR, 1, "ERROR: Cannot allocate SPI Master!");
		return -ENOMEM;
	}

	saa716x_spi		= spi_master_get_devdata(master);
	saa716x_spi->master	= master;
	saa716x_spi->saa716x	= saa716x;
	saa716x->saa716x_spi	= saa716x_spi;

	master->bus_num		= pdev->bus->number;
	master->num_chipselect	= 1; /* TODO! use config */
	master->cleanup		= saa716x_spi_cleanup;
	master->setup		= saa716x_spi_setup;
	master->transfer	= saa716x_spi_transfer;

	ret = spi_register_master(master);
	if (ret != 0) {
		dprintk(SAA716x_ERROR, 1, "ERROR: registering SPI Master!");
		goto err;
	}
err:
	spi_master_put(master);
	return ret;
}
EXPORT_SYMBOL(saa716x_spi_init);

void saa716x_spi_exit(struct saa716x_dev *saa716x)
{
	struct saa716x_spi_state *saa716x_spi = saa716x->saa716x_spi;

	spi_unregister_master(saa716x_spi->master);
	dprintk(SAA716x_DEBUG, 1, "SAA%02x SPI succesfully removed", saa716x->pdev->device);
}
EXPORT_SYMBOL(saa716x_spi_exit);
#endif

