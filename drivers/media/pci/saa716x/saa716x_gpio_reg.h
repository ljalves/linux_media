#ifndef __SAA716x_GPIO_REG_H
#define __SAA716x_GPIO_REG_H

/* -------------- GPIO Registers -------------- */

#define GPIO_RD				0x000
#define GPIO_WR				0x004
#define GPIO_WR_MODE			0x008
#define GPIO_OEN			0x00c

#define GPIO_SW_RST			0xff0
#define GPIO_SW_RESET			(0x00000001 <<  0)

#define GPIO_31				(1 << 31)
#define GPIO_30				(1 << 30)
#define GPIO_29				(1 << 29)
#define GPIO_28				(1 << 28)
#define GPIO_27				(1 << 27)
#define GPIO_26				(1 << 26)
#define GPIO_25				(1 << 25)
#define GPIO_24				(1 << 24)
#define GPIO_23				(1 << 23)
#define GPIO_22				(1 << 22)
#define GPIO_21				(1 << 21)
#define GPIO_20				(1 << 20)
#define GPIO_19				(1 << 19)
#define GPIO_18				(1 << 18)
#define GPIO_17				(1 << 17)
#define GPIO_16				(1 << 16)
#define GPIO_15				(1 << 15)
#define GPIO_14				(1 << 14)
#define GPIO_13				(1 << 13)
#define GPIO_12				(1 << 12)
#define GPIO_11				(1 << 11)
#define GPIO_10				(1 << 10)
#define GPIO_09				(1 <<  9)
#define GPIO_08				(1 <<  8)
#define GPIO_07				(1 <<  7)
#define GPIO_06				(1 <<  6)
#define GPIO_05				(1 <<  5)
#define GPIO_04				(1 <<  4)
#define GPIO_03				(1 <<  3)
#define GPIO_02				(1 <<  2)
#define GPIO_01				(1 <<  1)
#define GPIO_00				(1 <<  0)

#endif /* __SAA716x_GPIO_REG_H */
