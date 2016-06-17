#ifndef _STV6120_H_
#define _STV6120_H_


struct stv6120_cfg {
	u8 adr;

	u32 xtal;
	u8 Rdiv;
};




struct dvb_frontend *stv6120_attach(struct dvb_frontend *fe,
		    struct i2c_adapter *i2c, struct stv6120_cfg *cfg);
#endif
