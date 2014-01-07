#ifndef __SAA716x_I2C_H
#define __SAA716x_I2C_H

#define SAA716x_I2C_ADAPTERS	2

#define SAA716x_I2C_ADAPTER(__dev) ((	\
	(__dev == 1) ?			\
		"SAA716x I2C Core 1" :	\
		"SAA716x I2C Core 0"))

#define SAA716x_I2C_BUS(__x) ((__x == 1) ? 0x0000c000 : 0x0000b000)

#define SAA716x_I2C_BUS_A		0x01
#define SAA716x_I2C_BUS_B		0x00

struct saa716x_dev;

enum saa716x_i2c_rate {
	SAA716x_I2C_RATE_400 = 1,
	SAA716x_I2C_RATE_100,
};

enum saa716x_i2c_mode {
	SAA716x_I2C_MODE_POLLING = 0,
	SAA716x_I2C_MODE_IRQ,
	SAA716x_I2C_MODE_IRQ_BUFFERED
};

struct saa716x_i2c {
	struct i2c_adapter		i2c_adapter;
	struct mutex			i2c_lock;
	struct saa716x_dev		*saa716x;
	u8				i2c_dev;

	enum saa716x_i2c_rate		i2c_rate; /* run time */
	enum saa716x_i2c_mode		i2c_mode;
	u32				block_size; /* block size for buffered
						       mode, 1 otherwise */
	u32				i2c_stat;

	u32				stat_tx_prior;
	u32				stat_tx_done;
	wait_queue_head_t		i2c_wq;
	int				i2c_op;
};

extern int saa716x_i2c_init(struct saa716x_dev *saa716x);
extern int saa716x_i2c_exit(struct saa716x_dev *saa716x);
extern void saa716x_i2cint_disable(struct saa716x_dev *saa716x);
extern int saa716x_i2c_irqevent(struct saa716x_dev *saa716x, u8 bus);

#endif /* __SAA716x_I2C_H */
