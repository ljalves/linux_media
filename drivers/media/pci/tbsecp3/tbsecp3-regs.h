/*
    TBS ECP3 FPGA based cards PCIe driver

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _TBSECP3_REGS_H_
#define _TBSECP3_REGS_H_

/* GPIO */
#define TBSECP3_GPIO_BASE	0x0000
#define TBSECP3_GPIO_PIN(_bank, _pin)	(((_bank) << 5) + _pin)

/* I2C */
#define TBSECP3_I2C_BASE(_n)	(0x4000 + 0x1000 * _n)
#define TBSECP3_I2C_STAT	0x0000
#define TBSECP3_I2C_CTRL	0x0000
#define TBSECP3_I2C_DATA	0x0004
#define TBSECP3_I2C_BAUD	0x0008

/* CA */
#define TBSECP3_CA_BASE(_n)	(0x6000 + 0x1000 * _n)

/* DMA */
#define TBSECP3_DMA_BASE(_n)	(_n < 4) ? (0x8000 + 0x1000 * _n) : (0x8800 + 0x1000 * (_n - 4))
#define TBSECP3_DMA_STAT	0x0000
#define TBSECP3_DMA_EN		0x0000
#define TBSECP3_DMA_TSIZE	0x0004
#define TBSECP3_DMA_ADDRH	0x0008
#define TBSECP3_DMA_ADDRL	0x000c
#define TBSECP3_DMA_BSIZE	0x0010

/* INTR */
#define TBSECP3_INT_BASE	0xc000
#define TBSECP3_INT_STAT	0x0000
#define TBSECP3_INT_EN		0x0004
#define TBSECP3_I2C_IE(_n)	(0x0008 + 4 * _n)
#define TBSECP3_DMA_IE(_n)	(0x0018 + 4 * _n)
#define TBSECP3_I2C_IF(_n)	(0x0001 << _n)
#define TBSECP3_DMA_IF(_n)	(0x0010 << _n)

#endif
