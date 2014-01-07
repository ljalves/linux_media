#include <linux/kernel.h>
#include <linux/spinlock.h>

#include "saa716x_mod.h"

#include "saa716x_gpio_reg.h"

#include "saa716x_gpio.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

void saa716x_gpio_init(struct saa716x_dev *saa716x)
{
	spin_lock_init(&saa716x->gpio_lock);
}
EXPORT_SYMBOL_GPL(saa716x_gpio_init);

int saa716x_get_gpio_mode(struct saa716x_dev *saa716x, u32 *config)
{
	*config = SAA716x_EPRD(GPIO, GPIO_WR_MODE);

	return 0;
}

int saa716x_set_gpio_mode(struct saa716x_dev *saa716x, u32 mask, u32 config)
{
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&saa716x->gpio_lock, flags);
	reg = SAA716x_EPRD(GPIO, GPIO_WR_MODE);
	reg &= ~mask;
	reg |= (config & mask);
	SAA716x_EPWR(GPIO, GPIO_WR_MODE, reg);
	spin_unlock_irqrestore(&saa716x->gpio_lock, flags);

	return 0;
}

u32 saa716x_gpio_rd(struct saa716x_dev *saa716x)
{
	return SAA716x_EPRD(GPIO, GPIO_RD);
}

void saa716x_gpio_wr(struct saa716x_dev *saa716x, u32 data)
{
	SAA716x_EPWR(GPIO, GPIO_WR, data);
}

void saa716x_gpio_ctl(struct saa716x_dev *saa716x, u32 mask, u32 bits)
{
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&saa716x->gpio_lock, flags);

	reg  = SAA716x_EPRD(GPIO, GPIO_OEN);
	reg &= mask;
	reg |= bits;
	SAA716x_EPWR(GPIO, GPIO_OEN, reg);

	spin_unlock_irqrestore(&saa716x->gpio_lock, flags);
}

void saa716x_gpio_bits(struct saa716x_dev *saa716x, u32 bits)
{
	unsigned long flags;
	u32 reg;

	spin_lock_irqsave(&saa716x->gpio_lock, flags);

	reg  = SAA716x_EPRD(GPIO, GPIO_WR);
	reg &= ~bits;
	/* TODO ! add maskable config bits in here */
	/* reg |= (config->mask & bits) */
	reg |= bits;
	SAA716x_EPWR(GPIO, GPIO_WR, reg);

	spin_unlock_irqrestore(&saa716x->gpio_lock, flags);
}

void saa716x_gpio_set_output(struct saa716x_dev *saa716x, int gpio)
{
	uint32_t value;

	value = SAA716x_EPRD(GPIO, GPIO_OEN);
	value &= ~(1 << gpio);
	SAA716x_EPWR(GPIO, GPIO_OEN, value);
}
EXPORT_SYMBOL_GPL(saa716x_gpio_set_output);

void saa716x_gpio_set_input(struct saa716x_dev *saa716x, int gpio)
{
	uint32_t value;

	value = SAA716x_EPRD(GPIO, GPIO_OEN);
	value |= 1 << gpio;
	SAA716x_EPWR(GPIO, GPIO_OEN, value);
}
EXPORT_SYMBOL_GPL(saa716x_gpio_set_input);

void saa716x_gpio_set_mode(struct saa716x_dev *saa716x, int gpio, int mode)
{
	uint32_t value;

	value = SAA716x_EPRD(GPIO, GPIO_WR_MODE);
	if (mode)
		value |= 1 << gpio;
	else
		value &= ~(1 << gpio);
	SAA716x_EPWR(GPIO, GPIO_WR_MODE, value);
}
EXPORT_SYMBOL_GPL(saa716x_gpio_set_mode);

void saa716x_gpio_write(struct saa716x_dev *saa716x, int gpio, int set)
{
	uint32_t value;
	unsigned long flags;

	spin_lock_irqsave(&saa716x->gpio_lock, flags);
	value = SAA716x_EPRD(GPIO, GPIO_WR);
	if (set)
		value |= 1 << gpio;
	else
		value &= ~(1 << gpio);
	SAA716x_EPWR(GPIO, GPIO_WR, value);
	spin_unlock_irqrestore(&saa716x->gpio_lock, flags);
}
EXPORT_SYMBOL_GPL(saa716x_gpio_write);

int saa716x_gpio_read(struct saa716x_dev *saa716x, int gpio)
{
	uint32_t value;

	value = SAA716x_EPRD(GPIO, GPIO_RD);
	if (value & (1 << gpio))
		return 1;
	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_gpio_read);
