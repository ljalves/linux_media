/*
 * Driver for the ST STV0910 DVB-S/S2 demodulator.
 *
 * Copyright (C) 2014 Digital Devices GmbH
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 only, as published by the Free Software Foundation.
 *
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/version.h>
#include <asm/div64.h>

#include "dvb_frontend.h"
#include "stv0910.h"
#include "stv0910_regs.h"


#define TUNING_DELAY    200
#define BER_SRC_S    0x20
#define BER_SRC_S2   0x20



u16 init_tab[] = {
0xf100, 0x0051, 0xf101, 0x0020, 0xf113, 0x0000, 0xf114, 0x0000,
0xf11a, 0x0005, 0xf11b, 0x0000, 0xf11c, 0x004c, 0xf120, 0x0000,
0xf121, 0x0000, 0xf122, 0x0000, 0xf123, 0x0000, 0xf124, 0x0000,
0xf125, 0x0000, 0xf126, 0x0000, 0xf127, 0x0000, 0xf129, 0x0088,
0xf12a, 0x0030, 0xf12b, 0x0030, 0xf140, 0x0082, 0xf141, 0x0082,
0xf142, 0x0082, 0xf143, 0x0082, 0xf144, 0x0082, 0xf145, 0x0004,
0xf146, 0x0006, 0xf147, 0x0082, 0xf148, 0x0082, 0xf149, 0x0082,
0xf14a, 0x0082, 0xf14b, 0x0082, 0xf14c, 0x0082, 0xf14d, 0x0082,
0xf14e, 0x0082, 0xf14f, 0x0082, 0xf150, 0x0082, 0xf151, 0x0082,
0xf152, 0x0082, 0xf153, 0x0082, 0xf154, 0x0082, 0xf155, 0x0082,
0xf156, 0x0082, 0xf16a, 0x0093, 0xf16b, 0x0071, 0xf16c, 0x0082,
0xf170, 0x008c, 0xf171, 0x0045, 0xf172, 0x00c9, 0xf173, 0x0001,
0xf174, 0x0037, 0xf175, 0x0008, 0xf176, 0x0010, 0xf177, 0x0045,
0xf178, 0x00c9, 0xf179, 0x0038, 0xf17a, 0x0071, 0xf17b, 0x0028,
0xf17c, 0x00ff, 0xf17d, 0x0013, 0xf17e, 0x0090, 0xf17f, 0x00be,
0xf180, 0x0080, 0xf181, 0x0000, 0xf182, 0x0058, 0xf183, 0x006f,
0xf184, 0x0000, 0xf185, 0x0001, 0xf186, 0x0000, 0xf187, 0x00e9,
0xf188, 0x004d, 0xf1b3, 0x0039, 0xf1b4, 0x0012, 0xf1b5, 0x0004,
0xf1b6, 0x0002, 0xf1b7, 0x0001, 0xf1b8, 0x0007, 0xf1c2, 0x0000,
0xf1c3, 0x0000, 0xf1c8, 0x0000, 0xf1df, 0x0004, 0xf1e0, 0x0026,
0xf1e1, 0x006b, 0xf1e2, 0x0026, 0xf200, 0x0000, 0xf201, 0x0014,
0xf202, 0x000e, 0xf203, 0x00fc, 0xf204, 0x0054, 0xf206, 0x0099,
0xf207, 0x0058, 0xf208, 0x000a, 0xf209, 0x0009, 0xf20a, 0x0009,
0xf20b, 0x000a, 0xf20c, 0x00fd, 0xf20d, 0x00fd, 0xf20e, 0x0000,
0xf20f, 0x0000, 0xf210, 0x0000, 0xf211, 0x0010, 0xf212, 0x0010,
0xf213, 0x0080, 0xf214, 0x00c9, 0xf215, 0x003b, 0xf216, 0x005c,
0xf217, 0x0040, 0xf21b, 0x001c, 0xf21c, 0x0000, 0xf21d, 0x0080,
0xf21e, 0x0008, 0xf21f, 0x0004, 0xf220, 0x0078, 0xf221, 0x0080,
0xf222, 0x00aa, 0xf224, 0x0010, 0xf225, 0x0001, 0xf226, 0x0000,
0xf227, 0x0000, 0xf228, 0x0000, 0xf229, 0x0000, 0xf22a, 0x0000,
0xf22b, 0x0000, 0xf22c, 0x005b, 0xf22d, 0x0038, 0xf22e, 0x0058,
0xf22f, 0x0038, 0xf230, 0x0038, 0xf231, 0x0038, 0xf232, 0x0038,
0xf233, 0x0038, 0xf234, 0x0047, 0xf235, 0x0038, 0xf236, 0x001c,
0xf237, 0x0074, 0xf238, 0x0046, 0xf239, 0x002b, 0xf23a, 0x001a,
0xf23b, 0x0000, 0xf23c, 0x0000, 0xf23d, 0x0079, 0xf23e, 0x001c,
0xf23f, 0x00d0, 0xf240, 0x00b8, 0xf241, 0x00f9, 0xf242, 0x000e,
0xf243, 0x0069, 0xf244, 0x0001, 0xf245, 0x00f5, 0xf246, 0x00f1,
0xf247, 0x0097, 0xf248, 0x0001, 0xf249, 0x00f5, 0xf24a, 0x0003,
0xf24b, 0x008e, 0xf24c, 0x0001, 0xf24d, 0x00f5, 0xf24e, 0x0000,
0xf24f, 0x00b6, 0xf250, 0x00d3, 0xf251, 0x0068, 0xf252, 0x0055,
0xf253, 0x0008, 0xf254, 0x0008, 0xf255, 0x0020, 0xf256, 0x00d0,
0xf257, 0x00a0, 0xf258, 0x0080, 0xf259, 0x0088, 0xf25a, 0x0080,
0xf25b, 0x0080, 0xf25d, 0x0006, 0xf25e, 0x0038, 0xf25f, 0x00e3,
0xf260, 0x003f, 0xf261, 0x00ff, 0xf262, 0x002e, 0xf263, 0x0039,
0xf264, 0x0038, 0xf265, 0x00e3, 0xf266, 0x0000, 0xf267, 0x0000,
0xf268, 0x0000, 0xf269, 0x0000, 0xf26a, 0x0000, 0xf26b, 0x00e4,
0xf26c, 0x0000, 0xf26d, 0x0010, 0xf26f, 0x0041, 0xf270, 0x0000,
0xf271, 0x0000, 0xf272, 0x0000, 0xf273, 0x0000, 0xf274, 0x0000,
0xf275, 0x0000, 0xf276, 0x0000, 0xf277, 0x0000, 0xf278, 0x0000,
0xf279, 0x0000, 0xf27a, 0x0000, 0xf27b, 0x0000, 0xf27c, 0x0000,
0xf27d, 0x0000, 0xf27e, 0x0000, 0xf27f, 0x0000, 0xf280, 0x00ff,
0xf281, 0x00ff, 0xf282, 0x00ff, 0xf283, 0x00ff, 0xf284, 0x00ff,
0xf285, 0x00ff, 0xf286, 0x00ff, 0xf287, 0x00ff, 0xf288, 0x00ff,
0xf289, 0x00ff, 0xf28a, 0x00ff, 0xf28b, 0x00ff, 0xf28c, 0x0000,
0xf28d, 0x0000, 0xf28e, 0x0000, 0xf28f, 0x0000, 0xf290, 0x0006,
0xf291, 0x00e5, 0xf292, 0x0002, 0xf293, 0x0000, 0xf294, 0x0000,
0xf295, 0x0000, 0xf297, 0x000b, 0xf298, 0x000a, 0xf299, 0x0049,
0xf29a, 0x0048, 0xf29c, 0x0084, 0xf29d, 0x0084, 0xf29e, 0x0084,
0xf29f, 0x0084, 0xf2ac, 0x0000, 0xf2ad, 0x0000, 0xf2ae, 0x0001,
0xf2b0, 0x00ff, 0xf2b1, 0x00fc, 0xf2b2, 0x0000, 0xf2b3, 0x0000,
0xf2b4, 0x0000, 0xf2b5, 0x0000, 0xf2b6, 0x0000, 0xf2b7, 0x00c0,
0xf2b8, 0x0000, 0xf2b9, 0x0000, 0xf2ba, 0x00c0, 0xf2bb, 0x0000,
0xf2bc, 0x0000, 0xf2bd, 0x0000, 0xf2be, 0x0000, 0xf2bf, 0x000f,
0xf2c0, 0x0098, 0xf2c1, 0x0030, 0xf2c2, 0x00ac, 0xf2c3, 0x0050,
0xf2c4, 0x0000, 0xf2c5, 0x0064, 0xf2c6, 0x0029, 0xf2c7, 0x0000,
0xf2c8, 0x0000, 0xf2c9, 0x0000, 0xf2ca, 0x0000, 0xf2cb, 0x0000,
0xf2cc, 0x0000, 0xf2cd, 0x0000, 0xf2ce, 0x0000, 0xf2cf, 0x0000,
0xf2d0, 0x0000, 0xf2d1, 0x0000, 0xf2d2, 0x0000, 0xf2d3, 0x0000,
0xf2d4, 0x0000, 0xf2d5, 0x0000, 0xf2d6, 0x0000, 0xf2d7, 0x0000,
0xf2d8, 0x0071, 0xf2e1, 0x0002, 0xf300, 0x0000, 0xf301, 0x0000,
0xf302, 0x0000, 0xf303, 0x0000, 0xf304, 0x0000, 0xf305, 0x0000,
0xf306, 0x0000, 0xf307, 0x0071, 0xf309, 0x0060, 0xf30a, 0x0069,
0xf30b, 0x0080, 0xf30c, 0x0035, 0xf30d, 0x0028, 0xf30e, 0x0026,
0xf30f, 0x0086, 0xf320, 0x0000, 0xf322, 0x00c0, 0xf323, 0x00d2,
0xf324, 0x003b, 0xf325, 0x0046, 0xf332, 0x0080, 0xf333, 0x0000,
0xf334, 0x00d7, 0xf335, 0x0085, 0xf336, 0x0058, 0xf337, 0x003a,
0xf338, 0x0034, 0xf339, 0x0028, 0xf33a, 0x000d, 0xf33b, 0x00ff,
0xf33c, 0x002f, 0xf33d, 0x0000, 0xf33e, 0x00a6, 0xf33f, 0x00d7,
0xf340, 0x0027, 0xf341, 0x0032, 0xf342, 0x0032, 0xf343, 0x0032,
0xf344, 0x0032, 0xf345, 0x0050, 0xf346, 0x0000, 0xf347, 0x0000,
0xf34f, 0x0001, 0xf350, 0x0000, 0xf351, 0x0020, 0xf354, 0x0041,
0xf35e, 0x0000, 0xf35f, 0x0000, 0xf360, 0x00f0, 0xf361, 0x0000,
0xf362, 0x0005, 0xf363, 0x00e0, 0xf364, 0x007d, 0xf365, 0x0080,
0xf366, 0x0047, 0xf367, 0x0000, 0xf368, 0x0000, 0xf369, 0x0094,
0xf36a, 0x0092, 0xf36b, 0x0000, 0xf36c, 0x0000, 0xf36d, 0x0000,
0xf36e, 0x0000, 0xf36f, 0x0000, 0xf370, 0x00f0, 0xf371, 0x0012,
0xf372, 0x0040, 0xf373, 0x0004, 0xf374, 0x0020, 0xf375, 0x0000,
0xf376, 0x0000, 0xf377, 0x0000, 0xf378, 0x0000, 0xf379, 0x0003,
0xf37a, 0x0000, 0xf380, 0x00ff, 0xf381, 0x0052, 0xf382, 0x00a8,
0xf383, 0x0000, 0xf384, 0x0003, 0xf385, 0x0000, 0xf389, 0x0000,
0xf38a, 0x0000, 0xf38b, 0x0000, 0xf38c, 0x0000, 0xf38d, 0x0000,
0xf38e, 0x0000, 0xf38f, 0x0000, 0xf391, 0x0004, 0xf392, 0x0001,
0xf393, 0x0018, 0xf394, 0x0000, 0xf398, 0x0067, 0xf399, 0x0000,
0xf39a, 0x0000, 0xf39b, 0x0000, 0xf39c, 0x00c1, 0xf39d, 0x0000,
0xf39e, 0x0000, 0xf39f, 0x0000, 0xf3a0, 0x00a8, 0xf3a1, 0x002c,
0xf3a2, 0x003a, 0xf3a3, 0x0007, 0xf3a4, 0x0000, 0xf3a8, 0x0000,
0xf3a9, 0x0000, 0xf3aa, 0x0000, 0xf3ab, 0x0000, 0xf3ac, 0x0000,
0xf3ad, 0x0000, 0xf3ae, 0x0000, 0xf3af, 0x0000, 0xf3b2, 0x0011,
0xf3c1, 0x00ff, 0xf3c3, 0x0046, 0xf3c4, 0x001f, 0xf3c5, 0x0022,
0xf3c6, 0x0024, 0xf3c7, 0x0024, 0xf3c8, 0x0029, 0xf3c9, 0x002c,
0xf3cc, 0x004e, 0xf3d0, 0x0000, 0xf3d8, 0x0094, 0xf3d9, 0x0080,
0xf3da, 0x0000, 0xf3db, 0x0000, 0xf400, 0x0000, 0xf401, 0x0014,
0xf402, 0x00fe, 0xf403, 0x0007, 0xf404, 0x0054, 0xf406, 0x0099,
0xf407, 0x0058, 0xf408, 0x0009, 0xf409, 0x00f6, 0xf40a, 0x0009,
0xf40b, 0x0009, 0xf40c, 0x00fd, 0xf40d, 0x0005, 0xf40e, 0x0000,
0xf40f, 0x0000, 0xf410, 0x0000, 0xf411, 0x0010, 0xf412, 0x0010,
0xf413, 0x0080, 0xf414, 0x00c9, 0xf415, 0x003b, 0xf416, 0x005c,
0xf417, 0x0040, 0xf41b, 0x001c, 0xf41c, 0x0000, 0xf41d, 0x0080,
0xf41e, 0x0008, 0xf41f, 0x0004, 0xf420, 0x0078, 0xf421, 0x0080,
0xf422, 0x00aa, 0xf424, 0x0010, 0xf425, 0x0001, 0xf426, 0x0000,
0xf427, 0x0000, 0xf428, 0x0000, 0xf429, 0x0000, 0xf42a, 0x0000,
0xf42b, 0x0000, 0xf42c, 0x005b, 0xf42d, 0x0038, 0xf42e, 0x0058,
0xf42f, 0x0038, 0xf430, 0x0038, 0xf431, 0x0038, 0xf432, 0x0038,
0xf433, 0x0038, 0xf434, 0x0047, 0xf435, 0x0038, 0xf436, 0x000b,
0xf437, 0x0061, 0xf438, 0x0046, 0xf439, 0x002b, 0xf43a, 0x001a,
0xf43b, 0x0000, 0xf43c, 0x0000, 0xf43d, 0x0079, 0xf43e, 0x001c,
0xf43f, 0x00d0, 0xf440, 0x00b8, 0xf441, 0x00f8, 0xf442, 0x000e,
0xf443, 0x0069, 0xf444, 0x00ff, 0xf445, 0x007a, 0xf446, 0x00f1,
0xf447, 0x0097, 0xf448, 0x00ff, 0xf449, 0x007a, 0xf44a, 0x0003,
0xf44b, 0x008e, 0xf44c, 0x00ff, 0xf44d, 0x007a, 0xf44e, 0x0000,
0xf44f, 0x00b6, 0xf450, 0x00d3, 0xf451, 0x0068, 0xf452, 0x0055,
0xf453, 0x0008, 0xf454, 0x0008, 0xf455, 0x0020, 0xf456, 0x00d0,
0xf457, 0x00a0, 0xf458, 0x0080, 0xf459, 0x0088, 0xf45a, 0x0080,
0xf45b, 0x0080, 0xf45d, 0x0006, 0xf45e, 0x0038, 0xf45f, 0x00e3,
0xf460, 0x0019, 0xf461, 0x0099, 0xf462, 0x0012, 0xf463, 0x007d,
0xf464, 0x0038, 0xf465, 0x00e3, 0xf466, 0x0000, 0xf467, 0x0000,
0xf468, 0x0000, 0xf469, 0x0000, 0xf46a, 0x0000, 0xf46b, 0x00e4,
0xf46c, 0x0000, 0xf46d, 0x0010, 0xf46f, 0x0041, 0xf470, 0x0000,
0xf471, 0x0000, 0xf472, 0x0000, 0xf473, 0x0000, 0xf474, 0x0000,
0xf475, 0x0000, 0xf476, 0x0000, 0xf477, 0x0000, 0xf478, 0x0000,
0xf479, 0x0000, 0xf47a, 0x0000, 0xf47b, 0x0000, 0xf47c, 0x0000,
0xf47d, 0x0000, 0xf47e, 0x0000, 0xf47f, 0x0000, 0xf480, 0x00ff,
0xf481, 0x00ff, 0xf482, 0x00ff, 0xf483, 0x00ff, 0xf484, 0x00ff,
0xf485, 0x00ff, 0xf486, 0x00ff, 0xf487, 0x00ff, 0xf488, 0x00ff,
0xf489, 0x00ff, 0xf48a, 0x00ff, 0xf48b, 0x00ff, 0xf48c, 0x0000,
0xf48d, 0x0000, 0xf48e, 0x0000, 0xf48f, 0x0000, 0xf490, 0x0006,
0xf491, 0x00e5, 0xf492, 0x0002, 0xf493, 0x0000, 0xf494, 0x0000,
0xf495, 0x0000, 0xf497, 0x000b, 0xf498, 0x000a, 0xf499, 0x0049,
0xf49a, 0x0048, 0xf49c, 0x0084, 0xf49d, 0x0084, 0xf49e, 0x0084,
0xf49f, 0x0084, 0xf4ac, 0x0000, 0xf4ad, 0x0000, 0xf4ae, 0x0001,
0xf4b0, 0x00ff, 0xf4b1, 0x00cf, 0xf4b2, 0x00ff, 0xf4b3, 0x00ff,
0xf4b4, 0x0000, 0xf4b5, 0x0000, 0xf4b6, 0x0000, 0xf4b7, 0x00c0,
0xf4b8, 0x0000, 0xf4b9, 0x0000, 0xf4ba, 0x00c0, 0xf4bb, 0x0000,
0xf4bc, 0x0000, 0xf4bd, 0x0000, 0xf4be, 0x0000, 0xf4bf, 0x000f,
0xf4c0, 0x0098, 0xf4c1, 0x0030, 0xf4c2, 0x00ac, 0xf4c3, 0x0050,
0xf4c4, 0x0000, 0xf4c5, 0x0064, 0xf4c6, 0x0029, 0xf4c7, 0x0000,
0xf4c8, 0x0000, 0xf4c9, 0x0000, 0xf4ca, 0x0000, 0xf4cb, 0x0000,
0xf4cc, 0x0000, 0xf4cd, 0x0000, 0xf4ce, 0x0000, 0xf4cf, 0x0000,
0xf4d0, 0x0000, 0xf4d1, 0x0000, 0xf4d2, 0x0000, 0xf4d3, 0x0000,
0xf4d4, 0x0000, 0xf4d5, 0x0000, 0xf4d6, 0x0000, 0xf4d7, 0x0000,
0xf4d8, 0x0071, 0xf4e1, 0x0002, 0xf500, 0x0000, 0xf501, 0x0000,
0xf502, 0x0000, 0xf503, 0x0000, 0xf504, 0x0000, 0xf505, 0x0000,
0xf506, 0x0000, 0xf507, 0x0071, 0xf509, 0x0060, 0xf50a, 0x0069,
0xf50b, 0x0080, 0xf50c, 0x0035, 0xf50d, 0x0028, 0xf50e, 0x0026,
0xf50f, 0x0086, 0xf520, 0x0000, 0xf522, 0x00c0, 0xf523, 0x00d1,
0xf524, 0x0005, 0xf525, 0x00c8, 0xf532, 0x0080, 0xf533, 0x0000,
0xf534, 0x00d7, 0xf535, 0x0085, 0xf536, 0x0058, 0xf537, 0x003a,
0xf538, 0x0034, 0xf539, 0x0028, 0xf53a, 0x000d, 0xf53b, 0x00ff,
0xf53c, 0x002f, 0xf53d, 0x0000, 0xf53e, 0x00a4, 0xf53f, 0x00d7,
0xf540, 0x0027, 0xf541, 0x0032, 0xf542, 0x0032, 0xf543, 0x0032,
0xf544, 0x0032, 0xf545, 0x0050, 0xf546, 0x0000, 0xf547, 0x0000,
0xf54f, 0x0001, 0xf550, 0x0000, 0xf551, 0x0000, 0xf554, 0x0041,
0xf55e, 0x0000, 0xf55f, 0x0000, 0xf560, 0x00f0, 0xf561, 0x0000,
0xf562, 0x0005, 0xf563, 0x00e0, 0xf564, 0x007d, 0xf565, 0x0080,
0xf566, 0x0047, 0xf567, 0x0000, 0xf568, 0x0000, 0xf569, 0x0094,
0xf56a, 0x0012, 0xf56b, 0x0000, 0xf56c, 0x0000, 0xf56d, 0x0000,
0xf56e, 0x0000, 0xf56f, 0x0000, 0xf570, 0x00f0, 0xf571, 0x0012,
0xf572, 0x0040, 0xf573, 0x0004, 0xf574, 0x0020, 0xf575, 0x0000,
0xf576, 0x0000, 0xf577, 0x0000, 0xf578, 0x0000, 0xf579, 0x0003,
0xf57a, 0x0000, 0xf580, 0x00ff, 0xf581, 0x0052, 0xf582, 0x0028,
0xf583, 0x0000, 0xf584, 0x0003, 0xf585, 0x0000, 0xf589, 0x0000,
0xf58a, 0x0000, 0xf58b, 0x0000, 0xf58c, 0x0000, 0xf58d, 0x0000,
0xf58e, 0x0000, 0xf58f, 0x0000, 0xf591, 0x0004, 0xf592, 0x0001,
0xf593, 0x0018, 0xf594, 0x0000, 0xf598, 0x0067, 0xf599, 0x0000,
0xf59a, 0x0000, 0xf59b, 0x0000, 0xf59c, 0x00c1, 0xf59d, 0x0000,
0xf59e, 0x0000, 0xf59f, 0x0000, 0xf5a0, 0x00a8, 0xf5a1, 0x002c,
0xf5a2, 0x003a, 0xf5a3, 0x0007, 0xf5a4, 0x0000, 0xf5a8, 0x0000,
0xf5a9, 0x0000, 0xf5aa, 0x0000, 0xf5ab, 0x0000, 0xf5ac, 0x0000,
0xf5ad, 0x0000, 0xf5ae, 0x0000, 0xf5af, 0x0000, 0xf5b2, 0x0011,
0xf5c1, 0x00ff, 0xf5c3, 0x0044, 0xf5c4, 0x001f, 0xf5c5, 0x0022,
0xf5c6, 0x0024, 0xf5c7, 0x0024, 0xf5c8, 0x0029, 0xf5c9, 0x002c,
0xf5cc, 0x0046, 0xf5d0, 0x0000, 0xf5d8, 0x0094, 0xf5d9, 0x0080,
0xf5da, 0x0000, 0xf5db, 0x0000, 0xf600, 0x0060, 0xf601, 0x0000,
0xf602, 0x0000, 0xf603, 0x0000, 0xf604, 0x0000, 0xf605, 0x0000,
0xf606, 0x0000, 0xf607, 0x00ff, 0xf630, 0x0000, 0xf700, 0x0000,
0xf701, 0x0000, 0xf702, 0x0002, 0xf703, 0x0020, 0xf704, 0x0000,
0xf705, 0x0000, 0xf706, 0x00c0, 0xf708, 0x0002, 0xf709, 0x008c,
0xf70a, 0x0034, 0xf70b, 0x0004, 0xf70c, 0x0000, 0xf70d, 0x0000,
0xf70e, 0x0000, 0xf70f, 0x0000, 0xf710, 0x0000, 0xf711, 0x0003,
0xf712, 0x00f6, 0xf714, 0x0001, 0xf715, 0x002b, 0xf716, 0x00a9,
0xf71c, 0x000f, 0xf71e, 0x0001, 0xf71f, 0x0014, 0xf740, 0x0000,
0xf741, 0x0000, 0xf742, 0x0002, 0xf743, 0x0020, 0xf744, 0x0000,
0xf745, 0x0000, 0xf746, 0x00c0, 0xf748, 0x0002, 0xf749, 0x008c,
0xf74a, 0x0034, 0xf74b, 0x0004, 0xf74c, 0x0000, 0xf74d, 0x0000,
0xf74e, 0x0000, 0xf74f, 0x0000, 0xf750, 0x0000, 0xf751, 0x0003,
0xf752, 0x00f1, 0xf754, 0x0001, 0xf755, 0x002b, 0xf756, 0x00a9,
0xf75c, 0x000f, 0xf75e, 0x0001, 0xf75f, 0x0014, 0xfa00, 0x001c,
0xfa01, 0x0019, 0xfa02, 0x0017, 0xfa03, 0x0018, 0xfa04, 0x0013,
0xfa05, 0x0019, 0xfa06, 0x0018, 0xfa07, 0x0017, 0xfa08, 0x0016,
0xfa09, 0x001c, 0xfa0a, 0x001c, 0xfa0b, 0x0013, 0xfa0c, 0x0019,
0xfa0d, 0x0018, 0xfa0e, 0x0016, 0xfa0f, 0x001c, 0xfa10, 0x001c,
0xfa11, 0x0019, 0xfa12, 0x0018, 0xfa13, 0x0017, 0xfa14, 0x0016,
0xfa15, 0x001c, 0xfa16, 0x001c, 0xfa17, 0x0018, 0xfa18, 0x0017,
0xfa19, 0x0016, 0xfa1a, 0x001c, 0xfa1b, 0x001c, 0xfa1c, 0x001b,
0xfa1d, 0x0019, 0xfa1e, 0x0017, 0xfa1f, 0x001a, 0xfa20, 0x0013,
0xfa21, 0x0019, 0xfa22, 0x001b, 0xfa23, 0x001d, 0xfa24, 0x001b,
0xfa25, 0x001c, 0xfa26, 0x0013, 0xfa27, 0x0019, 0xfa28, 0x001b,
0xfa29, 0x001b, 0xfa2a, 0x001c, 0xfa2b, 0x0019, 0xfa2c, 0x001b,
0xfa2d, 0x001d, 0xfa2e, 0x001b, 0xfa2f, 0x001c, 0xfa30, 0x001b,
0xfa31, 0x001d, 0xfa32, 0x001b, 0xfa33, 0x001c, 0xfa34, 0x0005,
0xfa35, 0x005b, 0xfa36, 0x0096, 0xfa37, 0x0000, 0xfa38, 0x0000,
0xfa39, 0x0000, 0xfa3a, 0x0000, 0xfa40, 0x0020, 0xfa41, 0x0020,
0xfa42, 0x0020, 0xfa43, 0x0020, 0xfa44, 0x0020, 0xfa45, 0x0020,
0xfa46, 0x0020, 0xfa47, 0x0020, 0xfa48, 0x0020, 0xfa49, 0x0020,
0xfa4a, 0x0020, 0xfa4b, 0x0020, 0xfa4c, 0x0020, 0xfa4d, 0x0020,
0xfa4e, 0x0020, 0xfa4f, 0x0020, 0xfa50, 0x0020, 0xfa51, 0x0022,
0xfa52, 0x0022, 0xfa53, 0x0024, 0xfa54, 0x0024, 0xfa55, 0x0025,
0xfa56, 0x0026, 0xfa57, 0x0020, 0xfa58, 0x0020, 0xfa59, 0x0020,
0xfa5a, 0x0020, 0xfa5b, 0x0020, 0xfa5c, 0x0020, 0xfa5d, 0x0020,
0xfa5e, 0x0020, 0xfa5f, 0x0020, 0xfa60, 0x0020, 0xfa61, 0x0020,
0xfa62, 0x0020, 0xfa63, 0x0020, 0xfa64, 0x0020, 0xfa65, 0x0020,
0xfa66, 0x0020, 0xfa67, 0x0020, 0xfa68, 0x0020, 0xfa69, 0x0020,
0xfa6a, 0x0020, 0xfa6b, 0x0022, 0xfa6c, 0x0022, 0xfa6d, 0x0024,
0xfa6e, 0x0024, 0xfa6f, 0x0025, 0xfa70, 0x0020, 0xfa71, 0x0020,
0xfa72, 0x0020, 0xfa73, 0x0020, 0xfa80, 0x0002, 0xfa86, 0x0015,
0xfa96, 0x0000, 0xfa97, 0x0000, 0xfa98, 0x0000, 0xfab1, 0x0007,
0xfab6, 0x0007, 0xfabc, 0x0000, 0xfabd, 0x0000, 0xfabe, 0x0000,
0xfabf, 0x0000, 0xfac0, 0x001c, 0xfac1, 0x0019, 0xfac2, 0x0017,
0xfac3, 0x0018, 0xfac4, 0x0013, 0xfac5, 0x0019, 0xfac6, 0x0018,
0xfac7, 0x0017, 0xfac8, 0x0016, 0xfac9, 0x001c, 0xfaca, 0x001c,
0xfacb, 0x0013, 0xfacc, 0x0019, 0xfacd, 0x0018, 0xface, 0x0016,
0xfacf, 0x001c, 0xfad0, 0x001c, 0xfad1, 0x0019, 0xfad2, 0x0018,
0xfad3, 0x0017, 0xfad4, 0x0016, 0xfad5, 0x001c, 0xfad6, 0x001c,
0xfad7, 0x0018, 0xfad8, 0x0017, 0xfad9, 0x0016, 0xfada, 0x001c,
0xfadb, 0x001c, 0xfadc, 0x001b, 0xfadd, 0x0019, 0xfade, 0x0017,
0xfadf, 0x001a, 0xfae0, 0x0013, 0xfae1, 0x0019, 0xfae2, 0x001b,
0xfae3, 0x001d, 0xfae4, 0x001b, 0xfae5, 0x001c, 0xfae6, 0x0013,
0xfae7, 0x0019, 0xfae8, 0x001b, 0xfae9, 0x001b, 0xfaea, 0x001c,
0xfaeb, 0x0019, 0xfaec, 0x001b, 0xfaed, 0x001d, 0xfaee, 0x001b,
0xfaef, 0x001c, 0xfaf0, 0x001b, 0xfaf1, 0x001d, 0xfaf2, 0x001b,
0xfaf3, 0x001c, 0xff11, 0x0000, 0xff12, 0x0000, 0xff13, 0x0000,
0xff20, 0x0000, 0xff24, 0x0000, 0xff28, 0x0000, 0xff37, 0x0000,
0xff40, 0x0000, 0xff44, 0x0000, 0xff48, 0x0000, 0xff57, 0x0000,
0xff6d, 0x0000, 0x0000, 0x0000, 
};









LIST_HEAD(stvlist);

enum ReceiveMode { Mode_None, Mode_DVBS, Mode_DVBS2, Mode_Auto };


enum DVBS2_FECType { DVBS2_64K, DVBS2_16K };

enum DVBS2_ModCod {
	DVBS2_DUMMY_PLF, DVBS2_QPSK_1_4, DVBS2_QPSK_1_3, DVBS2_QPSK_2_5,
	DVBS2_QPSK_1_2, DVBS2_QPSK_3_5, DVBS2_QPSK_2_3,	DVBS2_QPSK_3_4,
	DVBS2_QPSK_4_5,	DVBS2_QPSK_5_6,	DVBS2_QPSK_8_9,	DVBS2_QPSK_9_10,
	DVBS2_8PSK_3_5,	DVBS2_8PSK_2_3,	DVBS2_8PSK_3_4,	DVBS2_8PSK_5_6,
	DVBS2_8PSK_8_9,	DVBS2_8PSK_9_10, DVBS2_16APSK_2_3, DVBS2_16APSK_3_4,
	DVBS2_16APSK_4_5, DVBS2_16APSK_5_6, DVBS2_16APSK_8_9, DVBS2_16APSK_9_10,
	DVBS2_32APSK_3_4, DVBS2_32APSK_4_5, DVBS2_32APSK_5_6, DVBS2_32APSK_8_9,
	DVBS2_32APSK_9_10
};

enum FE_STV0910_ModCod {
	FE_DUMMY_PLF, FE_QPSK_14, FE_QPSK_13, FE_QPSK_25,
	FE_QPSK_12, FE_QPSK_35, FE_QPSK_23, FE_QPSK_34,
	FE_QPSK_45, FE_QPSK_56, FE_QPSK_89, FE_QPSK_910,
	FE_8PSK_35, FE_8PSK_23, FE_8PSK_34, FE_8PSK_56,
	FE_8PSK_89, FE_8PSK_910, FE_16APSK_23, FE_16APSK_34,
	FE_16APSK_45, FE_16APSK_56, FE_16APSK_89, FE_16APSK_910,
	FE_32APSK_34, FE_32APSK_45, FE_32APSK_56, FE_32APSK_89,
	FE_32APSK_910
};

enum FE_STV0910_RollOff { FE_SAT_35, FE_SAT_25, FE_SAT_20, FE_SAT_15 };

static inline u32 MulDiv32(u32 a, u32 b, u32 c)
{
	u64 tmp64;

	tmp64 = (u64)a * (u64)b;
	do_div(tmp64, c);

	return (u32) tmp64;
}

struct stv_base {
	struct list_head     stvlist;

	u8                   adr;
	struct i2c_adapter  *i2c;
	struct mutex         i2c_lock;
	struct mutex         reg_lock;
	int                  count;

	u32                  extclk;
	u32                  mclk;

	struct stv0910_cfg  *cfg;
};

struct stv {
	struct stv_base     *base;
	struct dvb_frontend  fe;
	int                  nr;
	u16                  regoff;
	u8                   i2crpt;
	u8                   tscfgh;
	u8                   tsspeed;
	unsigned long        tune_time;

	s32                  SearchRange;
	u32                  Started;
	u32                  DemodLockTime;
	enum ReceiveMode     ReceiveMode;
	u32                  DemodTimeout;
	u32                  FecTimeout;
	u32                  FirstTimeLock;
	u8                   DEMOD;
	u32                  SymbolRate;

	u8                      LastViterbiRate;
	enum fe_code_rate       PunctureRate;
	enum FE_STV0910_ModCod  ModCod;
	enum DVBS2_FECType      FECType;
	u32                     Pilots;
	enum FE_STV0910_RollOff FERollOff;

	u32   LastBERNumerator;
	u32   LastBERDenominator;
	u8    BERScale;
};

struct SInitTable {
	u16  Address;
	u8   Data;
};

struct SLookupSNTable {
	s16  SignalToNoise;
	u16  RefValue;
};

static inline int i2c_write(struct i2c_adapter *adap, u8 adr,
			    u8 *data, int len)
{
	struct i2c_msg msg = {.addr = adr, .flags = 0,
			      .buf = data, .len = len};

	return (i2c_transfer(adap, &msg, 1) == 1) ? 0 : -1;
}

static int i2c_write_reg16(struct i2c_adapter *adap, u8 adr, u16 reg, u8 val)
{
	u8 msg[3] = {reg >> 8, reg & 0xff, val};

	return i2c_write(adap, adr, msg, 3);
}

static int write_reg(struct stv *state, u16 reg, u8 val)
{
	return i2c_write_reg16(state->base->i2c, state->base->adr, reg, val);
}

static inline int i2c_read_reg16(struct i2c_adapter *adapter, u8 adr,
				 u16 reg, u8 *val)
{
	u8 msg[2] = {reg >> 8, reg & 0xff};
	struct i2c_msg msgs[2] = {{.addr = adr, .flags = 0,
				   .buf  = msg, .len   = 2},
				  {.addr = adr, .flags = I2C_M_RD,
				   .buf  = val, .len   = 1 } };
	return (i2c_transfer(adapter, msgs, 2) == 2) ? 0 : -1;
}

static int read_reg(struct stv *state, u16 reg, u8 *val)
{
	return i2c_read_reg16(state->base->i2c, state->base->adr, reg, val);
}


static inline int i2c_read_regs16(struct i2c_adapter *adapter, u8 adr,
				  u16 reg, u8 *val, int len)
{
	u8 msg[2] = {reg >> 8, reg & 0xff};
	struct i2c_msg msgs[2] = {{.addr = adr, .flags = 0,
				   .buf  = msg, .len   = 2},
				  {.addr = adr, .flags = I2C_M_RD,
				   .buf  = val, .len   = len } };
	return (i2c_transfer(adapter, msgs, 2) == 2) ? 0 : -1;
}

static int read_regs(struct stv *state, u16 reg, u8 *val, int len)
{
	return i2c_read_regs16(state->base->i2c, state->base->adr,
			       reg, val, len);
}

struct SLookupSNTable S1_SN_Lookup[] = {
	{   0,    9242  },  /*C/N=  0dB*/
	{  05,    9105  },  /*C/N=0.5dB*/
	{  10,    8950  },  /*C/N=1.0dB*/
	{  15,    8780  },  /*C/N=1.5dB*/
	{  20,    8566  },  /*C/N=2.0dB*/
	{  25,    8366  },  /*C/N=2.5dB*/
	{  30,    8146  },  /*C/N=3.0dB*/
	{  35,    7908  },  /*C/N=3.5dB*/
	{  40,    7666  },  /*C/N=4.0dB*/
	{  45,    7405  },  /*C/N=4.5dB*/
	{  50,    7136  },  /*C/N=5.0dB*/
	{  55,    6861  },  /*C/N=5.5dB*/
	{  60,    6576  },  /*C/N=6.0dB*/
	{  65,    6330  },  /*C/N=6.5dB*/
	{  70,    6048  },  /*C/N=7.0dB*/
	{  75,    5768  },  /*C/N=7.5dB*/
	{  80,    5492  },  /*C/N=8.0dB*/
	{  85,    5224  },  /*C/N=8.5dB*/
	{  90,    4959  },  /*C/N=9.0dB*/
	{  95,    4709  },  /*C/N=9.5dB*/
	{  100,   4467  },  /*C/N=10.0dB*/
	{  105,   4236  },  /*C/N=10.5dB*/
	{  110,   4013  },  /*C/N=11.0dB*/
	{  115,   3800  },  /*C/N=11.5dB*/
	{  120,   3598  },  /*C/N=12.0dB*/
	{  125,   3406  },  /*C/N=12.5dB*/
	{  130,   3225  },  /*C/N=13.0dB*/
	{  135,   3052  },  /*C/N=13.5dB*/
	{  140,   2889  },  /*C/N=14.0dB*/
	{  145,   2733  },  /*C/N=14.5dB*/
	{  150,   2587  },  /*C/N=15.0dB*/
	{  160,   2318  },  /*C/N=16.0dB*/
	{  170,   2077  },  /*C/N=17.0dB*/
	{  180,   1862  },  /*C/N=18.0dB*/
	{  190,   1670  },  /*C/N=19.0dB*/
	{  200,   1499  },  /*C/N=20.0dB*/
	{  210,   1347  },  /*C/N=21.0dB*/
	{  220,   1213  },  /*C/N=22.0dB*/
	{  230,   1095  },  /*C/N=23.0dB*/
	{  240,    992  },  /*C/N=24.0dB*/
	{  250,    900  },  /*C/N=25.0dB*/
	{  260,    826  },  /*C/N=26.0dB*/
	{  270,    758  },  /*C/N=27.0dB*/
	{  280,    702  },  /*C/N=28.0dB*/
	{  290,    653  },  /*C/N=29.0dB*/
	{  300,    613  },  /*C/N=30.0dB*/
	{  310,    579  },  /*C/N=31.0dB*/
	{  320,    550  },  /*C/N=32.0dB*/
	{  330,    526  },  /*C/N=33.0dB*/
	{  350,    490  },  /*C/N=33.0dB*/
	{  400,    445  },  /*C/N=40.0dB*/
	{  450,    430  },  /*C/N=45.0dB*/
	{  500,    426  },  /*C/N=50.0dB*/
	{  510,    425  }   /*C/N=51.0dB*/
};

struct SLookupSNTable S2_SN_Lookup[] = {
	{  -30,  13950  },  /*C/N=-2.5dB*/
	{  -25,  13580  },  /*C/N=-2.5dB*/
	{  -20,  13150  },  /*C/N=-2.0dB*/
	{  -15,  12760  },  /*C/N=-1.5dB*/
	{  -10,  12345  },  /*C/N=-1.0dB*/
	{  -05,  11900  },  /*C/N=-0.5dB*/
	{    0,  11520  },  /*C/N=   0dB*/
	{   05,  11080  },  /*C/N= 0.5dB*/
	{   10,  10630  },  /*C/N= 1.0dB*/
	{   15,  10210  },  /*C/N= 1.5dB*/
	{   20,   9790  },  /*C/N= 2.0dB*/
	{   25,   9390  },  /*C/N= 2.5dB*/
	{   30,   8970  },  /*C/N= 3.0dB*/
	{   35,   8575  },  /*C/N= 3.5dB*/
	{   40,   8180  },  /*C/N= 4.0dB*/
	{   45,   7800  },  /*C/N= 4.5dB*/
	{   50,   7430  },  /*C/N= 5.0dB*/
	{   55,   7080  },  /*C/N= 5.5dB*/
	{   60,   6720  },  /*C/N= 6.0dB*/
	{   65,   6320  },  /*C/N= 6.5dB*/
	{   70,   6060  },  /*C/N= 7.0dB*/
	{   75,   5760  },  /*C/N= 7.5dB*/
	{   80,   5480  },  /*C/N= 8.0dB*/
	{   85,   5200  },  /*C/N= 8.5dB*/
	{   90,   4930  },  /*C/N= 9.0dB*/
	{   95,   4680  },  /*C/N= 9.5dB*/
	{  100,   4425  },  /*C/N=10.0dB*/
	{  105,   4210  },  /*C/N=10.5dB*/
	{  110,   3980  },  /*C/N=11.0dB*/
	{  115,   3765  },  /*C/N=11.5dB*/
	{  120,   3570  },  /*C/N=12.0dB*/
	{  125,   3315  },  /*C/N=12.5dB*/
	{  130,   3140  },  /*C/N=13.0dB*/
	{  135,   2980  },  /*C/N=13.5dB*/
	{  140,   2820  },  /*C/N=14.0dB*/
	{  145,   2670  },  /*C/N=14.5dB*/
	{  150,   2535  },  /*C/N=15.0dB*/
	{  160,   2270  },  /*C/N=16.0dB*/
	{  170,   2035  },  /*C/N=17.0dB*/
	{  180,   1825  },  /*C/N=18.0dB*/
	{  190,   1650  },  /*C/N=19.0dB*/
	{  200,   1485  },  /*C/N=20.0dB*/
	{  210,   1340  },  /*C/N=21.0dB*/
	{  220,   1212  },  /*C/N=22.0dB*/
	{  230,   1100  },  /*C/N=23.0dB*/
	{  240,   1000  },  /*C/N=24.0dB*/
	{  250,    910  },  /*C/N=25.0dB*/
	{  260,    836  },  /*C/N=26.0dB*/
	{  270,    772  },  /*C/N=27.0dB*/
	{  280,    718  },  /*C/N=28.0dB*/
	{  290,    671  },  /*C/N=29.0dB*/
	{  300,    635  },  /*C/N=30.0dB*/
	{  310,    602  },  /*C/N=31.0dB*/
	{  320,    575  },  /*C/N=32.0dB*/
	{  330,    550  },  /*C/N=33.0dB*/
	{  350,    517  },  /*C/N=35.0dB*/
	{  400,    480  },  /*C/N=40.0dB*/
	{  450,    466  },  /*C/N=45.0dB*/
	{  500,    464  },  /*C/N=50.0dB*/
	{  510,    463  },  /*C/N=51.0dB*/
};

/*********************************************************************
Tracking carrier loop carrier QPSK 1/4 to 8PSK 9/10 long Frame
*********************************************************************/
static u8 S2CarLoop[] =	{
	/* Modcod  2MPon 2MPoff 5MPon 5MPoff 10MPon 10MPoff
	   20MPon 20MPoff 30MPon 30MPoff*/
	/* FE_QPSK_14  */
	0x0C,  0x3C,  0x0B,  0x3C,  0x2A,  0x2C,  0x2A,  0x1C,  0x3A,  0x3B,
	/* FE_QPSK_13  */
	0x0C,  0x3C,  0x0B,  0x3C,  0x2A,  0x2C,  0x3A,  0x0C,  0x3A,  0x2B,
	/* FE_QPSK_25  */
	0x1C,  0x3C,  0x1B,  0x3C,  0x3A,  0x1C,  0x3A,  0x3B,  0x3A,  0x2B,
	/* FE_QPSK_12  */
	0x0C,  0x1C,  0x2B,  0x1C,  0x0B,  0x2C,  0x0B,  0x0C,  0x2A,  0x2B,
	/* FE_QPSK_35  */
	0x1C,  0x1C,  0x2B,  0x1C,  0x0B,  0x2C,  0x0B,  0x0C,  0x2A,  0x2B,
	/* FE_QPSK_23  */
	0x2C,  0x2C,  0x2B,  0x1C,  0x0B,  0x2C,  0x0B,  0x0C,  0x2A,  0x2B,
	/* FE_QPSK_34  */
	0x3C,  0x2C,  0x3B,  0x2C,  0x1B,  0x1C,  0x1B,  0x3B,  0x3A,  0x1B,
	/* FE_QPSK_45  */
	0x0D,  0x3C,  0x3B,  0x2C,  0x1B,  0x1C,  0x1B,  0x3B,  0x3A,  0x1B,
	/* FE_QPSK_56  */
	0x1D,  0x3C,  0x0C,  0x2C,  0x2B,  0x1C,  0x1B,  0x3B,  0x0B,  0x1B,
	/* FE_QPSK_89  */
	0x3D,  0x0D,  0x0C,  0x2C,  0x2B,  0x0C,  0x2B,  0x2B,  0x0B,  0x0B,
	/* FE_QPSK_910 */
	0x1E,  0x0D,  0x1C,  0x2C,  0x3B,  0x0C,  0x2B,  0x2B,  0x1B,  0x0B,
	/* FE_8PSK_35  */
	0x28,  0x09,  0x28,  0x09,  0x28,  0x09,  0x28,  0x08,  0x28,  0x27,
	/* FE_8PSK_23  */
	0x19,  0x29,  0x19,  0x29,  0x19,  0x29,  0x38,  0x19,  0x28,  0x09,
	/* FE_8PSK_34  */
	0x1A,  0x0B,  0x1A,  0x3A,  0x0A,  0x2A,  0x39,  0x2A,  0x39,  0x1A,
	/* FE_8PSK_56  */
	0x2B,  0x2B,  0x1B,  0x1B,  0x0B,  0x1B,  0x1A,  0x0B,  0x1A,  0x1A,
	/* FE_8PSK_89  */
	0x0C,  0x0C,  0x3B,  0x3B,  0x1B,  0x1B,  0x2A,  0x0B,  0x2A,  0x2A,
	/* FE_8PSK_910 */
	0x0C,  0x1C,  0x0C,  0x3B,  0x2B,  0x1B,  0x3A,  0x0B,  0x2A,  0x2A,

	/**********************************************************************
	Tracking carrier loop carrier 16APSK 2/3 to 32APSK 9/10 long Frame
	**********************************************************************/
	/*Modcod 2MPon  2MPoff 5MPon 5MPoff 10MPon 10MPoff 20MPon
	  20MPoff 30MPon 30MPoff*/
	/* FE_16APSK_23  */
	0x0A,  0x0A,  0x0A,  0x0A,  0x1A,  0x0A,  0x39,  0x0A,  0x29,  0x0A,
	/* FE_16APSK_34  */
	0x0A,  0x0A,  0x0A,  0x0A,  0x0B,  0x0A,  0x2A,  0x0A,  0x1A,  0x0A,
	/* FE_16APSK_45  */
	0x0A,  0x0A,  0x0A,  0x0A,  0x1B,  0x0A,  0x3A,  0x0A,  0x2A,  0x0A,
	/* FE_16APSK_56  */
	0x0A,  0x0A,  0x0A,  0x0A,  0x1B,  0x0A,  0x3A,  0x0A,  0x2A,  0x0A,
	/* FE_16APSK_89  */
	0x0A,  0x0A,  0x0A,  0x0A,  0x2B,  0x0A,  0x0B,  0x0A,  0x3A,  0x0A,
	/* FE_16APSK_910 */
	0x0A,  0x0A,  0x0A,  0x0A,  0x2B,  0x0A,  0x0B,  0x0A,  0x3A,  0x0A,
	/* FE_32APSK_34  */
	0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,
	/* FE_32APSK_45  */
	0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,
	/* FE_32APSK_56  */
	0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,
	/* FE_32APSK_89  */
	0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,
	/* FE_32APSK_910 */
	0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,  0x09,
};

static u8 get_optim_cloop(struct stv *state,
			  enum FE_STV0910_ModCod ModCod, u32 Pilots)
{
	int i = 0;
	if (ModCod >= FE_32APSK_910)
		i = ((int)FE_32APSK_910 - (int)FE_QPSK_14) * 10;
	else if (ModCod >= FE_QPSK_14)
		i = ((int)ModCod - (int)FE_QPSK_14) * 10;

	if (state->SymbolRate <= 3000000)
		i += 0;
	else if (state->SymbolRate <=  7000000)
		i += 2;
	else if (state->SymbolRate <= 15000000)
		i += 4;
	else if (state->SymbolRate <= 25000000)
		i += 6;
	else
		i += 8;

	if (!Pilots)
		i += 1;

	return S2CarLoop[i];
}

static int GetCurSymbolRate(struct stv *state, u32 *pSymbolRate)
{
	int status = 0;
	u8 SymbFreq0;
	u8 SymbFreq1;
	u8 SymbFreq2;
	u8 SymbFreq3;
	u8 TimOffs0;
	u8 TimOffs1;
	u8 TimOffs2;
	u32 SymbolRate;
	s32 TimingOffset;

	*pSymbolRate = 0;
	if (!state->Started)
		return status;

	read_reg(state, RSTV0910_P2_SFR3 + state->regoff, &SymbFreq3);
	read_reg(state, RSTV0910_P2_SFR2 + state->regoff, &SymbFreq2);
	read_reg(state, RSTV0910_P2_SFR1 + state->regoff, &SymbFreq1);
	read_reg(state, RSTV0910_P2_SFR0 + state->regoff, &SymbFreq0);
	read_reg(state, RSTV0910_P2_TMGREG2 + state->regoff, &TimOffs2);
	read_reg(state, RSTV0910_P2_TMGREG1 + state->regoff, &TimOffs1);
	read_reg(state, RSTV0910_P2_TMGREG0 + state->regoff, &TimOffs0);

	SymbolRate = ((u32) SymbFreq3 << 24) | ((u32) SymbFreq2 << 16) |
		((u32) SymbFreq1 << 8) | (u32) SymbFreq0;
	TimingOffset = ((u32) TimOffs2 << 16) | ((u32) TimOffs1 << 8) |
		(u32) TimOffs0;

	if ((TimingOffset & (1<<23)) != 0)
		TimingOffset |= 0xFF000000; /* Sign extent */

	SymbolRate = (u32) (((u64) SymbolRate * state->base->mclk) >> 32);
	TimingOffset = (s32) (((s64) SymbolRate * (s64) TimingOffset) >> 29);

	*pSymbolRate = SymbolRate + TimingOffset;

	return 0;
}

static int GetSignalParameters(struct stv *state)
{
	if (!state->Started)
		return -1;

	if (state->ReceiveMode == Mode_DVBS2) {
		u8 tmp;
		u8 rolloff;

		read_reg(state, RSTV0910_P2_DMDMODCOD + state->regoff, &tmp);
		state->ModCod = (enum FE_STV0910_ModCod) ((tmp & 0x7c) >> 2);
		state->Pilots = (tmp & 0x01) != 0;
		state->FECType = (enum DVBS2_FECType) ((tmp & 0x02) >> 1);

		read_reg(state, RSTV0910_P2_TMGOBS + state->regoff, &rolloff);
		rolloff = rolloff >> 6;
		state->FERollOff = (enum FE_STV0910_RollOff) rolloff;

	} else if (state->ReceiveMode == Mode_DVBS) {
		/* todo */
	}
	return 0;
}

static int TrackingOptimization(struct stv *state)
{
	u32 SymbolRate = 0;
	u8 tmp;

	GetCurSymbolRate(state, &SymbolRate);
	read_reg(state, RSTV0910_P2_DMDCFGMD + state->regoff, &tmp);
	tmp &= ~0xC0;

	switch (state->ReceiveMode) {
	case Mode_DVBS:
		tmp |= 0x40; break;
	case Mode_DVBS2:
		tmp |= 0x80; break;
	default:
		tmp |= 0xC0; break;
	}
	write_reg(state, RSTV0910_P2_DMDCFGMD + state->regoff, tmp);

	if (state->ReceiveMode == Mode_DVBS2) {
		/* force to PRE BCH Rate */
		write_reg(state, RSTV0910_P2_ERRCTRL1 + state->regoff,
			  BER_SRC_S2 | state->BERScale);

		if (state->FECType == DVBS2_64K) {
			u8 aclc = get_optim_cloop(state, state->ModCod,
						  state->Pilots);

			if (state->ModCod <= FE_QPSK_910) {
				write_reg(state, RSTV0910_P2_ACLC2S2Q +
					  state->regoff, aclc);
			} else if (state->ModCod <= FE_8PSK_910) {
				write_reg(state, RSTV0910_P2_ACLC2S2Q +
					  state->regoff, 0x2a);
				write_reg(state, RSTV0910_P2_ACLC2S28 +
					  state->regoff, aclc);
			} else if (state->ModCod <= FE_16APSK_910) {
				write_reg(state, RSTV0910_P2_ACLC2S2Q +
					  state->regoff, 0x2a);
				write_reg(state, RSTV0910_P2_ACLC2S216A +
					  state->regoff, aclc);
			} else if (state->ModCod <= FE_32APSK_910) {
				write_reg(state, RSTV0910_P2_ACLC2S2Q +
					  state->regoff, 0x2a);
				write_reg(state, RSTV0910_P2_ACLC2S232A +
					  state->regoff, aclc);
			}
		}
	}
	if (state->ReceiveMode == Mode_DVBS) {
		u8 tmp;

		read_reg(state, RSTV0910_P2_VITCURPUN + state->regoff, &tmp);
		state->PunctureRate = FEC_NONE;
		switch (tmp & 0x1F) {
		case 0x0d:
			state->PunctureRate = FEC_1_2;
			break;
		case 0x12:
			state->PunctureRate = FEC_2_3;
			break;
		case 0x15:
			state->PunctureRate = FEC_3_4;
			break;
		case 0x18:
			state->PunctureRate = FEC_5_6;
			break;
		case 0x1A:
			state->PunctureRate = FEC_7_8;
			break;
		}
	}
	return 0;
}

static int GetSignalToNoise(struct stv *state, s32 *SignalToNoise)
{
	int i;
	u8 Data0;
	u8 Data1;
	u16 Data;
	int nLookup;
	struct SLookupSNTable *Lookup;

	*SignalToNoise = 0;

	if (!state->Started)
		return 0;

	if (state->ReceiveMode == Mode_DVBS2) {
		read_reg(state, RSTV0910_P2_NNOSPLHT1 + state->regoff, &Data1);
		read_reg(state, RSTV0910_P2_NNOSPLHT0 + state->regoff, &Data0);
		nLookup = ARRAY_SIZE(S2_SN_Lookup);
		Lookup = S2_SN_Lookup;
	} else {
		read_reg(state, RSTV0910_P2_NNOSDATAT1 + state->regoff, &Data1);
		read_reg(state, RSTV0910_P2_NNOSDATAT0 + state->regoff, &Data0);
		nLookup = ARRAY_SIZE(S1_SN_Lookup);
		Lookup = S1_SN_Lookup;
	}
	Data = (((u16)Data1) << 8) | (u16) Data0;
	if (Data > Lookup[0].RefValue) {
		*SignalToNoise = Lookup[0].SignalToNoise;
	} else if (Data <= Lookup[nLookup-1].RefValue) {
		*SignalToNoise = Lookup[nLookup-1].SignalToNoise;
	} else {
		for (i = 0; i < nLookup - 1; i += 1) {
			if (Data <= Lookup[i].RefValue &&
			    Data > Lookup[i+1].RefValue) {
				*SignalToNoise =
					(s32)(Lookup[i].SignalToNoise) +
					((s32)(Data - Lookup[i].RefValue) *
					 (s32)(Lookup[i+1].SignalToNoise -
					       Lookup[i].SignalToNoise)) /
					((s32)(Lookup[i+1].RefValue) -
					  (s32)(Lookup[i].RefValue));
				break;
			}
		}
	}
	return 0;
}

static int GetBitErrorRateS(struct stv *state, u32 *BERNumerator,
			    u32 *BERDenominator)
{
	u8 Regs[3];

	int status = read_regs(state, RSTV0910_P2_ERRCNT12 + state->regoff,
			       Regs, 3);

	if (status)
		return -1;

	if ((Regs[0] & 0x80) == 0) {
		state->LastBERDenominator = 1 << ((state->BERScale * 2) +
						  10 + 3);
		state->LastBERNumerator = ((u32) (Regs[0] & 0x7F) << 16) |
			((u32) Regs[1] << 8) | Regs[2];
		if (state->LastBERNumerator < 256 && state->BERScale < 6) {
			state->BERScale += 1;
			status = write_reg(state, RSTV0910_P2_ERRCTRL1 +
					   state->regoff,
					   0x20 | state->BERScale);
		} else if (state->LastBERNumerator > 1024 &&
			   state->BERScale > 2) {
			state->BERScale -= 1;
			status = write_reg(state, RSTV0910_P2_ERRCTRL1 +
					   state->regoff, 0x20 |
					   state->BERScale);
		}
	}
	*BERNumerator = state->LastBERNumerator;
	*BERDenominator = state->LastBERDenominator;
	return 0;
}

static u32 DVBS2_nBCH(enum DVBS2_ModCod ModCod, enum DVBS2_FECType FECType)
{
	static u32 nBCH[][2] = {
		{16200,  3240}, /* QPSK_1_4, */
		{21600,  5400}, /* QPSK_1_3, */
		{25920,  6480}, /* QPSK_2_5, */
		{32400,  7200}, /* QPSK_1_2, */
		{38880,  9720}, /* QPSK_3_5, */
		{43200, 10800}, /* QPSK_2_3, */
		{48600, 11880}, /* QPSK_3_4, */
		{51840, 12600}, /* QPSK_4_5, */
		{54000, 13320}, /* QPSK_5_6, */
		{57600, 14400}, /* QPSK_8_9, */
		{58320, 16000}, /* QPSK_9_10, */
		{43200,  9720}, /* 8PSK_3_5, */
		{48600, 10800}, /* 8PSK_2_3, */
		{51840, 11880}, /* 8PSK_3_4, */
		{54000, 13320}, /* 8PSK_5_6, */
		{57600, 14400}, /* 8PSK_8_9, */
		{58320, 16000}, /* 8PSK_9_10, */
		{43200, 10800}, /* 16APSK_2_3, */
		{48600, 11880}, /* 16APSK_3_4, */
		{51840, 12600}, /* 16APSK_4_5, */
		{54000, 13320}, /* 16APSK_5_6, */
		{57600, 14400}, /* 16APSK_8_9, */
		{58320, 16000}, /* 16APSK_9_10 */
		{48600, 11880}, /* 32APSK_3_4, */
		{51840, 12600}, /* 32APSK_4_5, */
		{54000, 13320}, /* 32APSK_5_6, */
		{57600, 14400}, /* 32APSK_8_9, */
		{58320, 16000}, /* 32APSK_9_10 */
	};

	if (ModCod >= DVBS2_QPSK_1_4 &&
	    ModCod <= DVBS2_32APSK_9_10 && FECType <= DVBS2_16K)
		return nBCH[FECType][ModCod];
	return 64800;
}

static int GetBitErrorRateS2(struct stv *state, u32 *BERNumerator,
			     u32 *BERDenominator)
{
	u8 Regs[3];

	int status = read_regs(state, RSTV0910_P2_ERRCNT12 + state->regoff,
			       Regs, 3);

	if (status)
		return -1;

	if ((Regs[0] & 0x80) == 0) {
		state->LastBERDenominator =
			DVBS2_nBCH((enum DVBS2_ModCod) state->ModCod,
				   state->FECType) <<
			(state->BERScale * 2);
		state->LastBERNumerator = (((u32) Regs[0] & 0x7F) << 16) |
			((u32) Regs[1] << 8) | Regs[2];
		if (state->LastBERNumerator < 256 && state->BERScale < 6) {
			state->BERScale += 1;
			write_reg(state, RSTV0910_P2_ERRCTRL1 + state->regoff,
				  0x20 | state->BERScale);
		} else if (state->LastBERNumerator > 1024 &&
			   state->BERScale > 2) {
			state->BERScale -= 1;
			write_reg(state, RSTV0910_P2_ERRCTRL1 + state->regoff,
				  0x20 | state->BERScale);
		}
	}
	*BERNumerator = state->LastBERNumerator;
	*BERDenominator = state->LastBERDenominator;
	return status;
}

static int GetBitErrorRate(struct stv *state, u32 *BERNumerator,
			   u32 *BERDenominator)
{
	*BERNumerator = 0;
	*BERDenominator = 1;

	switch (state->ReceiveMode) {
	case Mode_DVBS:
		return GetBitErrorRateS(state, BERNumerator, BERDenominator);
		break;
	case Mode_DVBS2:
		return GetBitErrorRateS2(state, BERNumerator, BERDenominator);
	default:
		break;
	}
	return 0;
}

static int init(struct dvb_frontend *fe)
{
	return 0;
}

static int set_mclock(struct stv *state, u32 MasterClock)
{
	u32 idf = 1;
	u32 odf = 4;
	u32 quartz = state->base->extclk / 1000000;
	u32 Fphi = MasterClock / 1000000;
	u32 ndiv = (Fphi * odf * idf) / quartz;
	u32 cp = 7;
	u32 fvco;

	if (ndiv >= 7 && ndiv <= 71)
		cp = 7;
	else if (ndiv >=  72 && ndiv <=  79)
		cp = 8;
	else if (ndiv >=  80 && ndiv <=  87)
		cp = 9;
	else if (ndiv >=  88 && ndiv <=  95)
		cp = 10;
	else if (ndiv >=  96 && ndiv <= 103)
		cp = 11;
	else if (ndiv >= 104 && ndiv <= 111)
		cp = 12;
	else if (ndiv >= 112 && ndiv <= 119)
		cp = 13;
	else if (ndiv >= 120 && ndiv <= 127)
		cp = 14;
	else if (ndiv >= 128 && ndiv <= 135)
		cp = 15;
	else if (ndiv >= 136 && ndiv <= 143)
		cp = 16;
	else if (ndiv >= 144 && ndiv <= 151)
		cp = 17;
	else if (ndiv >= 152 && ndiv <= 159)
		cp = 18;
	else if (ndiv >= 160 && ndiv <= 167)
		cp = 19;
	else if (ndiv >= 168 && ndiv <= 175)
		cp = 20;
	else if (ndiv >= 176 && ndiv <= 183)
		cp = 21;
	else if (ndiv >= 184 && ndiv <= 191)
		cp = 22;
	else if (ndiv >= 192 && ndiv <= 199)
		cp = 23;
	else if (ndiv >= 200 && ndiv <= 207)
		cp = 24;
	else if (ndiv >= 208 && ndiv <= 215)
		cp = 25;
	else if (ndiv >= 216 && ndiv <= 223)
		cp = 26;
	else if (ndiv >= 224 && ndiv <= 225)
		cp = 27;

	write_reg(state, RSTV0910_NCOARSE, (cp << 3) | idf); // 0x39
	write_reg(state, RSTV0910_NCOARSE2, odf);            // 0x04
	write_reg(state, RSTV0910_NCOARSE1, ndiv);           // 0x12

	fvco = (quartz * 2 * ndiv) / idf;
	state->base->mclk = fvco / (2 * odf) * 1000000;

	//pr_info("ndiv = %d, MasterClock = %d\n", ndiv, state->base->mclk);
	return 0;
}

static int Stop(struct stv *state)
{
	if (state->Started) {
		u8 tmp;

		write_reg(state, RSTV0910_P2_TSCFGH + state->regoff,
			  state->tscfgh | 0x01);
		read_reg(state, RSTV0910_P2_PDELCTRL1 + state->regoff, &tmp);
		tmp &= ~0x01; /*release reset DVBS2 packet delin*/
		write_reg(state, RSTV0910_P2_PDELCTRL1 + state->regoff, tmp);
		/* Blind optim*/
		write_reg(state, RSTV0910_P2_AGC2O + state->regoff, 0x5B);
		/* Stop the demod */
		write_reg(state, RSTV0910_P2_DMDISTATE + state->regoff, 0x5c);
		state->Started = 0;
	}
	state->ReceiveMode = Mode_None;
	return 0;
}


static int Start(struct stv *state, struct dtv_frontend_properties *p)
{
	s32 Freq;
	u8  regDMDCFGMD;
	u16 symb;

	if (p->symbol_rate < 100000 || p->symbol_rate > 70000000)
		return -EINVAL;

	state->ReceiveMode = Mode_None;
	state->DemodLockTime = 0;

	/* Demod Stop*/
	if (state->Started)
		write_reg(state, RSTV0910_P2_DMDISTATE + state->regoff, 0x5C);

	if (p->symbol_rate <= 1000000) {  /*SR <=1Msps*/
		state->DemodTimeout = 3000;
		state->FecTimeout = 2000;
	} else if (p->symbol_rate <= 2000000) {  /*1Msps < SR <=2Msps*/
		state->DemodTimeout = 2500;
		state->FecTimeout = 1300;
	} else if (p->symbol_rate <= 5000000) {  /*2Msps< SR <=5Msps*/
		state->DemodTimeout = 1000;
	    state->FecTimeout = 650;
	} else if (p->symbol_rate <= 10000000) {  /*5Msps< SR <=10Msps*/
		state->DemodTimeout = 700;
		state->FecTimeout = 350;
	} else if (p->symbol_rate < 20000000) {  /*10Msps< SR <=20Msps*/
		state->DemodTimeout = 400;
		state->FecTimeout = 200;
	} else {  /*SR >=20Msps*/
		state->DemodTimeout = 300;
		state->FecTimeout = 200;
	}

	/* Set the Init Symbol rate*/
	symb = MulDiv32(p->symbol_rate, 65536, state->base->mclk);
	write_reg(state, RSTV0910_P2_SFRINIT1 + state->regoff,
		  ((symb >> 8) & 0x7F));
	write_reg(state, RSTV0910_P2_SFRINIT0 + state->regoff, (symb & 0xFF));

	//pr_info("symb = %u\n", symb);

	state->DEMOD |= 0x80;
	//lja write_reg(state, RSTV0910_P2_DEMOD + state->regoff, state->DEMOD);
	write_reg(state, RSTV0910_P2_DEMOD + state->regoff, 0);

	/* FE_STV0910_SetSearchStandard */
	read_reg(state, RSTV0910_P2_DMDCFGMD + state->regoff, &regDMDCFGMD);
	write_reg(state, RSTV0910_P2_DMDCFGMD + state->regoff,
		  regDMDCFGMD |= 0xC0);

	/* Disable DSS */
	write_reg(state, RSTV0910_P2_FECM  + state->regoff, 0x00);
	write_reg(state, RSTV0910_P2_PRVIT + state->regoff, 0x2F);

	/* 8PSK 3/5, 8PSK 2/3 Poff tracking optimization WA*/
	write_reg(state, RSTV0910_P2_ACLC2S2Q + state->regoff, 0x0B);
	write_reg(state, RSTV0910_P2_ACLC2S28 + state->regoff, 0x0A);
	write_reg(state, RSTV0910_P2_BCLC2S2Q + state->regoff, 0x84);
	write_reg(state, RSTV0910_P2_BCLC2S28 + state->regoff, 0x84);
	write_reg(state, RSTV0910_P2_CARHDR + state->regoff, 0x1C);
	/* Reset demod */
	write_reg(state, RSTV0910_P2_DMDISTATE + state->regoff, 0x1F);

	write_reg(state, RSTV0910_P2_CARCFG + state->regoff, 0x46);

	Freq = (state->SearchRange / 2000) + 600;
	if (p->symbol_rate <= 5000000)
		Freq -= (600 + 80);
	Freq = (Freq << 16) / (state->base->mclk / 1000);


	//Freq = 0x01e5;

	write_reg(state, RSTV0910_P2_CFRUP1 + state->regoff,
		  (Freq >> 8) & 0xff);
	write_reg(state, RSTV0910_P2_CFRUP0 + state->regoff, (Freq & 0xff));

	//printk("freq CFRUP/LOW = %x\n", Freq);
	// lja 0x01 0xe5


	/*CFR Low Setting*/
	Freq = -Freq;
	write_reg(state, RSTV0910_P2_CFRLOW1 + state->regoff,
		  (Freq >> 8) & 0xff);
	write_reg(state, RSTV0910_P2_CFRLOW0 + state->regoff, (Freq & 0xff));


	/* init the demod frequency offset to 0 */
	write_reg(state, RSTV0910_P2_CFRINIT1 + state->regoff, 0);
	write_reg(state, RSTV0910_P2_CFRINIT0 + state->regoff, 0);

	write_reg(state, RSTV0910_P2_DMDISTATE + state->regoff, 0x1F);
	/* Trigger acq */
	write_reg(state, RSTV0910_P2_DMDISTATE + state->regoff, 0x18); //lja 0x15);

	state->DemodLockTime += TUNING_DELAY;
	state->Started = 1;

	return 0;
}

static int init_diseqc(struct stv *state)
{
	u16 offs = state->nr ? 0x40 : 0;  /* Address offset */
	u8 Freq = ((state->base->mclk + 11000 * 32) / (22000 * 32));

	/* Disable receiver */
	write_reg(state, RSTV0910_P1_DISRXCFG + offs, 0x00);
	write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0xBA); /* Reset = 1 */
	write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x3A); /* Reset = 0 */
	write_reg(state, RSTV0910_P1_DISTXF22 + offs, Freq);
	return 0;
}

static int probe(struct stv *state)
{
	u8 id;

	state->ReceiveMode = Mode_None;
	state->Started = 0;

	if (read_reg(state, RSTV0910_MID, &id) < 0)
		return -1;

	if (id != 0x51)
		return -EINVAL;
	pr_info("Found STV0910 id=0x%02x\n", id);

#if 1

	 /* Configure the I2C repeater to off */
	write_reg(state, RSTV0910_P1_I2CRPT, 0x24);
	/* Configure the I2C repeater to off */
	write_reg(state, RSTV0910_P2_I2CRPT, 0x24);
	/* Set the I2C to oversampling ratio */
	write_reg(state, RSTV0910_I2CCFG, 0x88);

	write_reg(state, RSTV0910_OUTCFG,    0x40);//0x00);  /* OUTCFG */
	write_reg(state, RSTV0910_PADCFG,    0x05);  /* RF AGC Pads Dev = 05 */
	write_reg(state, RSTV0910_SYNTCTRL,  0x02);  /* SYNTCTRL */
	write_reg(state, RSTV0910_TSGENERAL, 0x00);  /* TSGENERAL */
	write_reg(state, RSTV0910_CFGEXT,    0x02);  /* CFGEXT */
	write_reg(state, RSTV0910_GENCFG,    0x15);  /* GENCFG */


	write_reg(state, RSTV0910_TSTRES0, 0x80); /* LDPC Reset */
	write_reg(state, RSTV0910_TSTRES0, 0x00);

	set_mclock(state, 135000000);

	/* TS output */
	write_reg(state, RSTV0910_P1_TSCFGH , state->tscfgh | 0x01);
	write_reg(state, RSTV0910_P1_TSCFGH , state->tscfgh);
	write_reg(state, RSTV0910_P1_TSCFGM , 0x04);//0xC0);  /* Manual speed */
	write_reg(state, RSTV0910_P1_TSCFGL , 0x20);

	/* Speed = 67.5 MHz */
	//lja write_reg(state, RSTV0910_P1_TSSPEED , state->tsspeed);

	write_reg(state, RSTV0910_P2_TSCFGH , state->tscfgh | 0x01);
	write_reg(state, RSTV0910_P2_TSCFGH , state->tscfgh);
	write_reg(state, RSTV0910_P2_TSCFGM , 0x04);//0xC0);  /* Manual speed */
	write_reg(state, RSTV0910_P2_TSCFGL , 0x20);

	/* Speed = 67.5 MHz */
	//lkja write_reg(state, RSTV0910_P2_TSSPEED , state->tsspeed);

	/* Reset stream merger */
	write_reg(state, RSTV0910_P1_TSCFGH , state->tscfgh | 0x01);
	write_reg(state, RSTV0910_P2_TSCFGH , state->tscfgh | 0x01);
	write_reg(state, RSTV0910_P1_TSCFGH , state->tscfgh);
	write_reg(state, RSTV0910_P2_TSCFGH , state->tscfgh);

	write_reg(state, RSTV0910_P1_I2CRPT, state->i2crpt);
	write_reg(state, RSTV0910_P2_I2CRPT, state->i2crpt);

#endif

#if 0	
	i = 0;
	do {
		read_reg(state, init_tab[i], &v);
		if (v != (u8) init_tab[i+1]) {
			write_reg(state, init_tab[i], (u8) init_tab[i+1]);
			printk("init_tab: [0x%x] was %x is %x\n", init_tab[i], v, init_tab[i+1]);
		}
		i = i + 2;
	} while (init_tab[i] != 0);
#endif
	
	// single mode demod
	//write_reg(state, 0xfa86, 0x14);
#if 0

	write_reg(state, 0xff11, 0x80);
	write_reg(state, 0xff11, 0x00);

	write_reg(state, 0xf4e1, 0x02);
	write_reg(state, 0xf2e1, 0x02);

	write_reg(state, 0xf1b6, 0x22);

	write_reg(state, 0xf11c, 0x40);

	write_reg(state, 0xf572, 0x01);
	write_reg(state, 0xf572, 0x00);
	write_reg(state, 0xf372, 0x01);
	write_reg(state, 0xf372, 0x00);

	write_reg(state, 0xf702, 0x82);
	write_reg(state, 0xf742, 0x82);
	write_reg(state, 0xf702, 0x02);
	write_reg(state, 0xf742, 0x02);
#endif


	//write_reg(state, 0xf4e1, 0x06);

	init_diseqc(state);
	return 0;
}


static int gate_ctrl(struct dvb_frontend *fe, int enable)
{
	struct stv *state = fe->demodulator_priv;
	u8 i2crpt = state->i2crpt & ~0x86;

	if (enable)
		mutex_lock(&state->base->i2c_lock);

	if (enable)
		i2crpt |= 0x80;
	else
		i2crpt |= 0x02;

	if (write_reg(state, RSTV0910_P1_I2CRPT, i2crpt) < 0)//state->nr ? RSTV0910_P2_I2CRPT :
		      //RSTV0910_P1_I2CRPT, i2crpt) < 0)
		return -EIO;

	state->i2crpt = i2crpt;

	if (!enable)
		mutex_unlock(&state->base->i2c_lock);
	return 0;
}

static void release(struct dvb_frontend *fe)
{
	struct stv *state = fe->demodulator_priv;

	state->base->count--;
	if (state->base->count == 0) {
		//pr_info("remove STV base\n");
		list_del(&state->base->stvlist);
		kfree(state->base);
	}
	kfree(state);
}

static int set_parameters(struct dvb_frontend *fe)
{
	int stat = 0;
	struct stv *state = fe->demodulator_priv;
	u32 IF;
	struct dtv_frontend_properties *p = &fe->dtv_property_cache;

	Stop(state);
	if (fe->ops.tuner_ops.set_params)
		fe->ops.tuner_ops.set_params(fe);
	if (fe->ops.tuner_ops.get_if_frequency)
		fe->ops.tuner_ops.get_if_frequency(fe, &IF);
	state->SymbolRate = p->symbol_rate;
	stat = Start(state, p);
	return stat;
}


static int read_status(struct dvb_frontend *fe, enum fe_status *status)
{
	struct stv *state = fe->demodulator_priv;
	u8 DmdState = 0;
	u8 DStatus  = 0;
	enum ReceiveMode CurReceiveMode = Mode_None;
	u32 FECLock = 0;

	read_reg(state, RSTV0910_P2_DMDSTATE + state->regoff, &DmdState);

	if (DmdState & 0x40) {
		read_reg(state, RSTV0910_P2_DSTATUS + state->regoff, &DStatus);
		if (DStatus & 0x08)
			CurReceiveMode = (DmdState & 0x20) ?
				Mode_DVBS : Mode_DVBS2;
	}
	if (CurReceiveMode == Mode_None) {
		*status = 0;
		return 0;
	}

	*status |= 0x0f;
	if (state->ReceiveMode == Mode_None) {
		state->ReceiveMode = CurReceiveMode;
		state->DemodLockTime = jiffies;
		state->FirstTimeLock = 0;

		write_reg(state, RSTV0910_P2_TSCFGH + state->regoff,
			  state->tscfgh);
		usleep_range(3000, 4000);
		write_reg(state, RSTV0910_P2_TSCFGH + state->regoff,
			  state->tscfgh | 0x01);
		write_reg(state, RSTV0910_P2_TSCFGH + state->regoff,
			  state->tscfgh);
	}
	if (DmdState & 0x40) {
		if (state->ReceiveMode == Mode_DVBS2) {
			u8 PDELStatus;
			read_reg(state,
				 RSTV0910_P2_PDELSTATUS1 + state->regoff,
				 &PDELStatus);
			FECLock = (PDELStatus & 0x02) != 0;
		} else {
			u8 VStatus;
			read_reg(state,
				 RSTV0910_P2_VSTATUSVIT + state->regoff,
				 &VStatus);
			FECLock = (VStatus & 0x08) != 0;
		}
	}

	if (!FECLock)
		return 0;

	*status |= 0x10;

	if (state->FirstTimeLock) {
		u8 tmp;

		state->FirstTimeLock = 0;
		GetSignalParameters(state);

		if (state->ReceiveMode == Mode_DVBS2) {
			/* FSTV0910_P2_MANUALSX_ROLLOFF,
			   FSTV0910_P2_MANUALS2_ROLLOFF = 0 */
			state->DEMOD &= ~0x84;
			write_reg(state, RSTV0910_P2_DEMOD + state->regoff,
				  state->DEMOD);
			read_reg(state, RSTV0910_P2_PDELCTRL2 + state->regoff,
				 &tmp);
			/*reset DVBS2 packet delinator error counter */
			tmp |= 0x40;
			write_reg(state, RSTV0910_P2_PDELCTRL2 + state->regoff,
				  tmp);
			/*reset DVBS2 packet delinator error counter */
			tmp &= ~0x40;
			write_reg(state, RSTV0910_P2_PDELCTRL2 + state->regoff,
				  tmp);

			state->BERScale = 2;
			state->LastBERNumerator = 0;
			state->LastBERDenominator = 1;
			/* force to PRE BCH Rate */
			write_reg(state, RSTV0910_P2_ERRCTRL1 + state->regoff,
				  BER_SRC_S2 | state->BERScale);
		} else {
			state->BERScale = 2;
			state->LastBERNumerator = 0;
			state->LastBERDenominator = 1;
			/* force to PRE RS Rate */
			write_reg(state, RSTV0910_P2_ERRCTRL1 + state->regoff,
				  BER_SRC_S | state->BERScale);
		}
		/*Reset the Total packet counter */
		write_reg(state, RSTV0910_P2_FBERCPT4 + state->regoff, 0x00);
		/*Reset the packet Error counter2 (and Set it to
		  infinit error count mode )*/
		write_reg(state, RSTV0910_P2_ERRCTRL2 + state->regoff, 0xc1);

		TrackingOptimization(state);
	}
	return 0;
}

static int tune(struct dvb_frontend *fe, bool re_tune,
		unsigned int mode_flags,
		unsigned int *delay, enum fe_status *status)
{
	struct stv *state = fe->demodulator_priv;
	int r;

	if (re_tune) {
		r = set_parameters(fe);
		if (r)
			return r;
		state->tune_time = jiffies;
	}
	if (*status & FE_HAS_LOCK)
		return 0;
	*delay = HZ;

	r = read_status(fe, status);
	if (r)
		return r;
	return 0;
}


static int get_algo(struct dvb_frontend *fe)
{
	return DVBFE_ALGO_HW;
}

static int set_tone(struct dvb_frontend *fe, enum fe_sec_tone_mode tone)
{
	struct stv *state = fe->demodulator_priv;
	u16 offs = state->nr ? 0x40 : 0;

	switch (tone) {
	case SEC_TONE_ON:
		return write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x30); //0x38);
	case SEC_TONE_OFF:
		return write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x32); //0x3a);
	default:
		break;
	}
	return -EINVAL;
}

static int wait_dis(struct stv *state, u8 flag, u8 val)
{
	int i;
	u8 stat;
	u16 offs = state->nr ? 0x40 : 0;

	for (i = 0; i < 10; i++) {
		read_reg(state, RSTV0910_P1_DISTXSTATUS + offs, &stat);
		if ((stat & flag) == val)
			return 0;
		msleep(10);
	}
	return -1;
}

static int send_master_cmd(struct dvb_frontend *fe,
			   struct dvb_diseqc_master_cmd *cmd)
{
	struct stv *state = fe->demodulator_priv;
	u16 offs = state->nr ? 0x40 : 0;
	int i;

	//pr_info("master_cmd %02x %02x %02x %02x\n", cmd->msg[0],  cmd->msg[1],  cmd->msg[2],  cmd->msg[3]);
	write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x3E);
	for (i = 0; i < cmd->msg_len; i++) {
		wait_dis(state, 0x40, 0x00);
		write_reg(state, RSTV0910_P1_DISTXFIFO + offs, cmd->msg[i]);
	}
	write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x3A);
	wait_dis(state, 0x20, 0x20);
	return 0;
}

static int recv_slave_reply(struct dvb_frontend *fe,
			    struct dvb_diseqc_slave_reply *reply)
{
	return 0;
}

static int send_burst(struct dvb_frontend *fe, enum fe_sec_mini_cmd burst)
{
#if 0
	struct stv *state = fe->demodulator_priv;
	u16 offs = state->nr ? 0x40 : 0;
	u8 value;

	if (burst == SEC_MINI_A) {
		pr_info("burst A\n");
		write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x3F);
		value = 0x00;
	} else {
		pr_info("burst B\n");
		write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x3E);
		value = 0xFF;
	}
	wait_dis(state, 0x40, 0x00);
	write_reg(state, RSTV0910_P1_DISTXFIFO + offs, value);
	write_reg(state, RSTV0910_P1_DISTXCFG + offs, 0x3A);
	wait_dis(state, 0x20, 0x20);
#endif
	return 0;
}

static int sleep(struct dvb_frontend *fe)
{
	struct stv *state = fe->demodulator_priv;

	//pr_info("sleep %d\n", state->nr);
	Stop(state);
	return 0;
}

static int read_snr(struct dvb_frontend *fe, u16 *snr)
{
	struct stv *state = fe->demodulator_priv;
	s32 SNR;

	*snr = 0;
	if (GetSignalToNoise(state, &SNR))
		return -EIO;
	*snr = (SNR * 100);
	return 0;
}

static int read_ber(struct dvb_frontend *fe, u32 *ber)
{
	struct stv *state = fe->demodulator_priv;
	u32 n, d;

	GetBitErrorRate(state, &n, &d);
	if (d) 
		*ber = n / d;
	else
		*ber = 0;
	return 0;
}

static int read_signal_strength(struct dvb_frontend *fe, u16 *strength)
{
	struct stv *state = fe->demodulator_priv;
	u8 Agc1, Agc0;

	read_reg(state, RSTV0910_P2_AGCIQIN1 + state->regoff, &Agc1);
	read_reg(state, RSTV0910_P2_AGCIQIN0 + state->regoff, &Agc0);

	*strength = ((255 - Agc1) * 3300) / 256;
	return 0;
}

static int read_ucblocks(struct dvb_frontend *fe, u32 *ucblocks)
{
	/* struct stv *state = fe->demodulator_priv; */


	return 0;
}

static int stv0910_get_property(struct dvb_frontend *fe,
		struct dtv_property *p)
{
	int ret = 0;
	u16 tmp;
	u32 tmp2;

	switch (p->cmd) {
	case DTV_STAT_CNR:
		ret |= read_snr(fe, &tmp);
		p->u.st.stat[0].scale = FE_SCALE_DECIBEL;
		p->u.st.stat[0].svalue = tmp;

		if (tmp > 25000)
			tmp2 = 0xffff;
		else
			tmp2 = (tmp * 0xffff) / 25000;
		p->u.st.stat[1].scale = FE_SCALE_RELATIVE;
		p->u.st.stat[1].svalue = (u16) tmp2;
		p->u.st.len = 2;
		break;
	default:
		break;
	}

	return ret;
}



static struct dvb_frontend_ops stv0910_ops = {
	.delsys = { SYS_DVBS, SYS_DVBS2, SYS_DSS },
	.info = {
		.name			= "STV0910",
		.frequency_min		= 950000,
		.frequency_max		= 2150000,
		.frequency_stepsize	= 0,
		.frequency_tolerance	= 0,
		.symbol_rate_min	= 1000000,
		.symbol_rate_max	= 70000000,
		.caps			= FE_CAN_INVERSION_AUTO |
					  FE_CAN_FEC_AUTO       |
					  FE_CAN_QPSK           |
					  FE_CAN_2G_MODULATION
	},
	.init				= init,
	.sleep				= sleep,
	.release                        = release,
	.i2c_gate_ctrl                  = gate_ctrl,
	.get_frontend_algo              = get_algo,
	.tune                           = tune,
	.read_status			= read_status,
	.set_tone			= set_tone,

	.diseqc_send_master_cmd		= send_master_cmd,
	.diseqc_send_burst		= send_burst,
	.diseqc_recv_slave_reply	= recv_slave_reply,

	.read_snr			= read_snr,
	.read_ber			= read_ber,
	.read_signal_strength		= read_signal_strength,
	.read_ucblocks			= read_ucblocks,

	.get_property			= stv0910_get_property,
};

static struct stv_base *match_base(struct i2c_adapter  *i2c, u8 adr)
{
	struct stv_base *p;

	list_for_each_entry(p, &stvlist, stvlist)
		if (p->i2c == i2c && p->adr == adr)
			return p;
	return NULL;
}

struct dvb_frontend *stv0910_attach(struct i2c_adapter *i2c,
				    struct stv0910_cfg *cfg,
				    int nr)
{
	struct stv *state;
	struct stv_base *base;

	state = kzalloc(sizeof(struct stv), GFP_KERNEL);
	if (!state)
		return NULL;

	state->tscfgh = 0x00;//0x20 | (cfg->parallel ? 0 : 0x40);
	state->i2crpt = 0x0A | ((cfg->rptlvl & 0x07) << 4);
	state->tsspeed = 0x40;
	state->nr = nr;
	state->regoff = state->nr ? 0 : 0x200;
	state->SearchRange = 16000000;
	state->DEMOD = 0x10;     /* Inversion : Auto with reset to 0 */
	state->ReceiveMode   = Mode_None;

	base = match_base(i2c, cfg->adr);
	if (base) {
		base->count++;
		state->base = base;
	} else {
		base = kzalloc(sizeof(struct stv_base), GFP_KERNEL);
		if (!base)
			goto fail;
		base->i2c = i2c;
		base->adr = cfg->adr;
		base->cfg = cfg;
		base->count = 1;
		base->extclk = cfg->clk ? cfg->clk : 30000000;

		mutex_init(&base->i2c_lock);
		mutex_init(&base->reg_lock);
		state->base = base;
		if (probe(state) < 0) {
			kfree(base);
			goto fail;
		}
		list_add(&base->stvlist, &stvlist);
	}
	state->fe.ops               = stv0910_ops;
	state->fe.demodulator_priv  = state;

	return &state->fe;

fail:
	kfree(state);
	return NULL;
}
EXPORT_SYMBOL_GPL(stv0910_attach);

MODULE_DESCRIPTION("STV0910 driver");
MODULE_AUTHOR("Ralph Metzler, Manfred Voelkel");
MODULE_LICENSE("GPL");

