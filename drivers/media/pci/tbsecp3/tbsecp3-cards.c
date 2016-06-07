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

#include "tbsecp3.h"

struct tbsecp3_board tbsecp3_boards[] = {
	[TBSECP3_BOARD_TBS6205] = {
		.name		= "TurboSight TBS 6205 (Quad DVB-T/C)",
		.i2c_speed	= 39,
		.eeprom_i2c	= 1,
		.adapters	= 4,
		.adap_config	= {
			{
				.ts_in = 0,
				.i2c_bus_nr = 0,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(0, 0),
			}, 
			{
				.ts_in = 1,
				.i2c_bus_nr = 1,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(2, 0),
			},
			{
				.ts_in = 2,
				.i2c_bus_nr = 2,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(1, 0),
			},
			{
				.ts_in = 3,
				.i2c_bus_nr = 3,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(3, 0),
			}
		}
	},
#if 0
	[TBSECP3_BOARD_TBS6903] = {
		.name			= "TurboSight TBS 6903 (Dual DVB-S/S2)",
		.adapters		= 2,
		.adap_config		= {
			{
				.ts_in = 0,
				.i2c_bus_nr = 0,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(0, 0),
			}, 
			{
				.ts_in = 1,
				.i2c_bus_nr = 0,
			},
		}
	},
#endif
	[TBSECP3_BOARD_TBS6904] = {
		.name		= "TurboSight TBS 6904 (Quad DVB-S/S2)",
		.adapters	= 4,
		.eeprom_i2c	= 1,
		.adap_config	= {
			{
				.ts_in = 0,
				.i2c_bus_nr = 0,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(0, 0),
			}, 
			{
				.ts_in = 1,
				.i2c_bus_nr = 1,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(1, 0),
			},
			{
				.ts_in = 2,
				.i2c_bus_nr = 2,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(2, 0),
			},
			{
				.ts_in = 3,
				.i2c_bus_nr = 3,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(3, 0),
			}
		}
	},
#if 0
	[TBSECP3_BOARD_TBS6909] = {
		.name			= "TurboSight TBS 6909 (Octa DVB-S/S2)",
		.adapters		= 8,
		.adap_config		= {
			{
				.ts_in = 0,
				.i2c_bus_nr = 0,
				.gpio.demod_reset_lvl = TBSECP3_GPIODEF_LOW,
				.gpio.demod_reset_pin = TBSECP3_GPIO_PIN(0, 0),
			}, 
			{
				.ts_in = 1,
				.i2c_bus_nr = 0,
			},
			{
				.ts_in = 2,
				.i2c_bus_nr = 0,
			},
			{
				.ts_in = 3,
				.i2c_bus_nr = 0,
			},
			{
				.ts_in = 4,
				.i2c_bus_nr = 0,
			},
			{
				.ts_in = 5,
				.i2c_bus_nr = 0,
			},
			{
				.ts_in = 6,
				.i2c_bus_nr = 0,
			},
			{
				.ts_in = 7,
				.i2c_bus_nr = 0,
			}
		}
	},
#endif
};

