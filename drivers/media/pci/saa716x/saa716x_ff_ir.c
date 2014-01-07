/*
 * Driver for the remote control of the TT6400 DVB-S2 card
 *
 * Copyright (C) 2010 Oliver Endriss <o.endriss@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 * Or, point your browser to http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <linux/types.h>
#include <linux/input.h>

#include "saa716x_spi.h"
#include "saa716x_priv.h"
#include "saa716x_ff.h"


/* infrared remote control */
struct infrared {
	u16			key_map[128];
	struct input_dev	*input_dev;
	char			input_phys[32];
	struct timer_list	keyup_timer;
	struct tasklet_struct	tasklet;
	u32			command;
	u32			device_mask;
	u8			protocol;
	u16			last_key;
	u16			last_toggle;
	bool			delay_timer_finished;
};

#define IR_RC5		0
#define UP_TIMEOUT	(HZ*7/25)


/* key-up timer */
static void ir_emit_keyup(unsigned long parm)
{
	struct infrared *ir = (struct infrared *) parm;

	if (!ir || !test_bit(ir->last_key, ir->input_dev->key))
		return;

	input_report_key(ir->input_dev, ir->last_key, 0);
	input_sync(ir->input_dev);
}


/* tasklet */
static void ir_emit_key(unsigned long parm)
{
	struct saa716x_dev *saa716x = (struct saa716x_dev *) parm;
	struct infrared *ir = saa716x->ir_priv;
	u32 ircom = ir->command;
	u8 data;
	u8 addr;
	u16 toggle;
	u16 keycode;

	/* extract device address and data */
	if (ircom & 0x80000000) { /* CEC remote command */
		addr = 0;
		data = ircom & 0x7F;
		toggle = 0;
	} else {
		switch (ir->protocol) {
		case IR_RC5: /* extended RC5: 5 bits device address, 7 bits data */
			addr = (ircom >> 6) & 0x1f;
			/* data bits 1..6 */
			data = ircom & 0x3f;
			/* data bit 7 (inverted) */
			if (!(ircom & 0x1000))
				data |= 0x40;
			toggle = ircom & 0x0800;
			break;

		default:
			printk(KERN_ERR "%s: invalid protocol %x\n",
				__func__, ir->protocol);
			return;
		}
	}

	input_event(ir->input_dev, EV_MSC, MSC_RAW, (addr << 16) | data);
	input_event(ir->input_dev, EV_MSC, MSC_SCAN, data);

	keycode = ir->key_map[data];

	dprintk(SAA716x_DEBUG, 0,
		"%s: code %08x -> addr %i data 0x%02x -> keycode %i\n",
		__func__, ircom, addr, data, keycode);

	/* check device address */
	if (!(ir->device_mask & (1 << addr)))
		return;

	if (!keycode) {
		printk(KERN_WARNING "%s: code %08x -> addr %i data 0x%02x -> unknown key!\n",
			__func__, ircom, addr, data);
		return;
	}

	if (timer_pending(&ir->keyup_timer)) {
		del_timer(&ir->keyup_timer);
		if (ir->last_key != keycode || toggle != ir->last_toggle) {
			ir->delay_timer_finished = false;
			input_event(ir->input_dev, EV_KEY, ir->last_key, 0);
			input_event(ir->input_dev, EV_KEY, keycode, 1);
			input_sync(ir->input_dev);
		} else if (ir->delay_timer_finished) {
			input_event(ir->input_dev, EV_KEY, keycode, 2);
			input_sync(ir->input_dev);
		}
	} else {
		ir->delay_timer_finished = false;
		input_event(ir->input_dev, EV_KEY, keycode, 1);
		input_sync(ir->input_dev);
	}

	ir->last_key = keycode;
	ir->last_toggle = toggle;

	ir->keyup_timer.expires = jiffies + UP_TIMEOUT;
	add_timer(&ir->keyup_timer);

}


/* register with input layer */
static void ir_register_keys(struct infrared *ir)
{
	int i;

	set_bit(EV_KEY, ir->input_dev->evbit);
	set_bit(EV_REP, ir->input_dev->evbit);
	set_bit(EV_MSC, ir->input_dev->evbit);

	set_bit(MSC_RAW, ir->input_dev->mscbit);
	set_bit(MSC_SCAN, ir->input_dev->mscbit);

	memset(ir->input_dev->keybit, 0, sizeof(ir->input_dev->keybit));

	for (i = 0; i < ARRAY_SIZE(ir->key_map); i++) {
		if (ir->key_map[i] > KEY_MAX)
			ir->key_map[i] = 0;
		else if (ir->key_map[i] > KEY_RESERVED)
			set_bit(ir->key_map[i], ir->input_dev->keybit);
	}

	ir->input_dev->keycode = ir->key_map;
	ir->input_dev->keycodesize = sizeof(ir->key_map[0]);
	ir->input_dev->keycodemax = ARRAY_SIZE(ir->key_map);
}


/* called by the input driver after rep[REP_DELAY] ms */
static void ir_repeat_key(unsigned long parm)
{
	struct infrared *ir = (struct infrared *) parm;

	ir->delay_timer_finished = true;
}


/* interrupt handler */
void saa716x_ir_handler(struct saa716x_dev *saa716x, u32 ir_cmd)
{
	struct infrared *ir = saa716x->ir_priv;

	if (!ir)
		return;

	ir->command = ir_cmd;
	tasklet_schedule(&ir->tasklet);
}


int saa716x_ir_init(struct saa716x_dev *saa716x)
{
	struct input_dev *input_dev;
	struct infrared *ir;
	int rc;
	int i;

	if (!saa716x)
		return -ENOMEM;

	ir = kzalloc(sizeof(struct infrared), GFP_KERNEL);
	if (!ir)
		return -ENOMEM;

	init_timer(&ir->keyup_timer);
	ir->keyup_timer.function = ir_emit_keyup;
	ir->keyup_timer.data = (unsigned long) ir;

	input_dev = input_allocate_device();
	if (!input_dev)
		goto err;

	ir->input_dev = input_dev;
	input_dev->name = "TT6400 DVB IR receiver";
	snprintf(ir->input_phys, sizeof(ir->input_phys),
		"pci-%s/ir0", pci_name(saa716x->pdev));
	input_dev->phys = ir->input_phys;
	input_dev->id.bustype = BUS_PCI;
	input_dev->id.version = 1;
	input_dev->id.vendor = saa716x->pdev->subsystem_vendor;
	input_dev->id.product = saa716x->pdev->subsystem_device;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 6, 22)
	input_dev->dev.parent = &saa716x->pdev->dev;
#else
	input_dev->cdev.dev = &saa716x->pdev->dev;
#endif
	rc = input_register_device(input_dev);
	if (rc)
		goto err;

	/* TODO: fix setup/keymap */
	ir->protocol = IR_RC5;
	ir->device_mask = 0xffffffff;
	for (i = 0; i < ARRAY_SIZE(ir->key_map); i++)
		ir->key_map[i] = i+1;
	ir_register_keys(ir);

	/* override repeat timer */
	input_dev->timer.function = ir_repeat_key;
	input_dev->timer.data = (unsigned long) ir;

	tasklet_init(&ir->tasklet, ir_emit_key, (unsigned long) saa716x);
	saa716x->ir_priv = ir;

	return 0;

err:
	if (ir->input_dev)
		input_free_device(ir->input_dev);
	kfree(ir);
	return -ENOMEM;
}


void saa716x_ir_exit(struct saa716x_dev *saa716x)
{
	struct infrared *ir = saa716x->ir_priv;

	saa716x->ir_priv = NULL;
	tasklet_kill(&ir->tasklet);
	del_timer_sync(&ir->keyup_timer);
	input_unregister_device(ir->input_dev);
	kfree(ir);
}
