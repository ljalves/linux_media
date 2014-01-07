#include <linux/types.h>

#include <linux/dvb/video.h>
#include <linux/dvb/osd.h>

#include "saa716x_mod.h"

#include "saa716x_phi_reg.h"

#include "saa716x_phi.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"
#include "saa716x_ff.h"
#include "saa716x_ff_cmd.h"


int sti7109_cmd_init(struct sti7109_dev *sti7109)
{
	mutex_init(&sti7109->cmd_lock);
	mutex_init(&sti7109->osd_cmd_lock);
	mutex_init(&sti7109->data_lock);

	init_waitqueue_head(&sti7109->boot_finish_wq);
	sti7109->boot_finished = 0;

	init_waitqueue_head(&sti7109->cmd_ready_wq);
	sti7109->cmd_ready = 0;

	init_waitqueue_head(&sti7109->result_avail_wq);
	sti7109->result_avail = 0;

	init_waitqueue_head(&sti7109->osd_cmd_ready_wq);
	sti7109->osd_cmd_ready = 0;
	init_waitqueue_head(&sti7109->osd_result_avail_wq);
	sti7109->osd_result_avail = 0;

	sti7109->data_handle = 0;
	sti7109->data_buffer = (u8 *) (sti7109->iobuf + TSOUT_LEN);
	init_waitqueue_head(&sti7109->data_ready_wq);
	sti7109->data_ready = 0;
	init_waitqueue_head(&sti7109->block_done_wq);
	sti7109->block_done = 0;
	return 0;
}

static int sti7109_do_raw_cmd(struct sti7109_dev * sti7109)
{
	struct saa716x_dev * saa716x = sti7109->dev;
	unsigned long timeout;

	timeout = 1 * HZ;
	timeout = wait_event_interruptible_timeout(sti7109->cmd_ready_wq,
						   sti7109->cmd_ready == 1,
						   timeout);

	if (timeout == -ERESTARTSYS || sti7109->cmd_ready == 0) {
		if (timeout == -ERESTARTSYS) {
			/* a signal arrived */
			dprintk(SAA716x_ERROR, 1, "cmd ERESTARTSYS");
			return -ERESTARTSYS;
		}
		dprintk(SAA716x_ERROR, 1,
			"timed out waiting for command ready");
		return -EIO;
	}

	sti7109->cmd_ready = 0;
	sti7109->result_avail = 0;
	saa716x_phi_write(saa716x, ADDR_CMD_DATA, sti7109->cmd_data,
			  sti7109->cmd_len);
	SAA716x_EPWR(PHI_1, FPGA_ADDR_PHI_ISET, ISR_CMD_MASK);

	if (sti7109->result_max_len > 0) {
		timeout = 1 * HZ;
		timeout = wait_event_interruptible_timeout(
				sti7109->result_avail_wq,
				sti7109->result_avail == 1,
				timeout);

		if (timeout == -ERESTARTSYS || sti7109->result_avail == 0) {
			sti7109->result_len = 0;
			if (timeout == -ERESTARTSYS) {
				/* a signal arrived */
				dprintk(SAA716x_ERROR, 1, "result ERESTARTSYS");
				return -ERESTARTSYS;
			}
			dprintk(SAA716x_ERROR, 1,
				"timed out waiting for command result");
			return -EIO;
		}

		if (sti7109->result_len > sti7109->result_max_len) {
			sti7109->result_len = sti7109->result_max_len;
			dprintk(SAA716x_NOTICE, 1,
				"not enough space in result buffer");
		}
	}

	return 0;
}

int sti7109_raw_cmd(struct sti7109_dev * sti7109, osd_raw_cmd_t * cmd)
{
	struct saa716x_dev * saa716x = sti7109->dev;
	int err;

	if (cmd->cmd_len > SIZE_CMD_DATA) {
		dprintk(SAA716x_ERROR, 1, "command too long");
		return -EFAULT;
	}

	mutex_lock(&sti7109->cmd_lock);

	err = -EFAULT;
	if (copy_from_user(sti7109->cmd_data, (void __user *)cmd->cmd_data,
			   cmd->cmd_len))
		goto out;

	sti7109->cmd_len = cmd->cmd_len;
	sti7109->result_max_len = cmd->result_len;

	err = sti7109_do_raw_cmd(sti7109);
	if (err)
		goto out;

	cmd->result_len = sti7109->result_len;
	if (sti7109->result_len > 0) {
		if (copy_to_user((void __user *)cmd->result_data,
				 sti7109->result_data,
				 sti7109->result_len))
			err = -EFAULT;
	}

out:
	mutex_unlock(&sti7109->cmd_lock);
	return err;
}

static int sti7109_do_raw_osd_cmd(struct sti7109_dev * sti7109)
{
	struct saa716x_dev * saa716x = sti7109->dev;
	unsigned long timeout;

	timeout = 1 * HZ;
	timeout = wait_event_interruptible_timeout(sti7109->osd_cmd_ready_wq,
						   sti7109->osd_cmd_ready == 1,
						   timeout);

	if (timeout == -ERESTARTSYS || sti7109->osd_cmd_ready == 0) {
		if (timeout == -ERESTARTSYS) {
			/* a signal arrived */
			dprintk(SAA716x_ERROR, 1, "osd cmd ERESTARTSYS");
			return -ERESTARTSYS;
		}
		dprintk(SAA716x_ERROR, 1,
			"timed out waiting for osd command ready");
		return -EIO;
	}

	sti7109->osd_cmd_ready = 0;
	sti7109->osd_result_avail = 0;
	saa716x_phi_write(saa716x, ADDR_OSD_CMD_DATA, sti7109->osd_cmd_data,
			  sti7109->osd_cmd_len);
	SAA716x_EPWR(PHI_1, FPGA_ADDR_PHI_ISET, ISR_OSD_CMD_MASK);

	if (sti7109->osd_result_max_len > 0) {
		timeout = 1 * HZ;
		timeout = wait_event_interruptible_timeout(
				sti7109->osd_result_avail_wq,
				sti7109->osd_result_avail == 1,
				timeout);

		if (timeout == -ERESTARTSYS || sti7109->osd_result_avail == 0) {
			sti7109->osd_result_len = 0;
			if (timeout == -ERESTARTSYS) {
				/* a signal arrived */
				dprintk(SAA716x_ERROR, 1,
					"osd result ERESTARTSYS");
				return -ERESTARTSYS;
			}
			dprintk(SAA716x_ERROR, 1,
				"timed out waiting for osd command result");
			return -EIO;
		}

		if (sti7109->osd_result_len > sti7109->osd_result_max_len) {
			sti7109->osd_result_len = sti7109->osd_result_max_len;
			dprintk(SAA716x_NOTICE, 1,
				"not enough space in result buffer");
		}
	}

	return 0;
}

int sti7109_raw_osd_cmd(struct sti7109_dev * sti7109, osd_raw_cmd_t * cmd)
{
	struct saa716x_dev * saa716x = sti7109->dev;
	int err;

	if (cmd->cmd_len > SIZE_OSD_CMD_DATA) {
		dprintk(SAA716x_ERROR, 1, "command too long");
		return -EFAULT;
	}

	mutex_lock(&sti7109->osd_cmd_lock);

	err = -EFAULT;
	if (copy_from_user(sti7109->osd_cmd_data, (void __user *)cmd->cmd_data,
			   cmd->cmd_len))
		goto out;

	sti7109->osd_cmd_len = cmd->cmd_len;
	sti7109->osd_result_max_len = cmd->result_len;

	err = sti7109_do_raw_osd_cmd(sti7109);
	if (err)
		goto out;

	cmd->result_len = sti7109->osd_result_len;
	if (sti7109->osd_result_len > 0) {
		if (copy_to_user((void __user *)cmd->result_data,
				 sti7109->osd_result_data,
				 sti7109->osd_result_len))
			err = -EFAULT;
	}

out:
	mutex_unlock(&sti7109->osd_cmd_lock);
	return err;
}

static int sti7109_do_raw_data(struct sti7109_dev * sti7109, osd_raw_data_t * data)
{
	struct saa716x_dev * saa716x = sti7109->dev;
	unsigned long timeout;
	u16 blockSize;
	u16 lastBlockSize;
	u16 numBlocks;
	u16 blockIndex;
	u8 blockHeader[SIZE_BLOCK_HEADER];
	u8 * blockPtr;
	int activeBlock;

	timeout = 1 * HZ;
	timeout = wait_event_interruptible_timeout(sti7109->data_ready_wq,
						   sti7109->data_ready == 1,
						   timeout);

	if (timeout == -ERESTARTSYS || sti7109->data_ready == 0) {
		if (timeout == -ERESTARTSYS) {
			/* a signal arrived */
			dprintk(SAA716x_ERROR, 1, "data ERESTARTSYS");
			return -ERESTARTSYS;
		}
		dprintk(SAA716x_ERROR, 1, "timed out waiting for data ready");
		return -EIO;
	}

	sti7109->data_ready = 0;

	/*
	 * 8 bytes is the size of the block header. Block header structure is:
	 * 16 bit - block index
	 * 16 bit - number of blocks
	 * 16 bit - current block data size
	 * 16 bit - block handle. This is used to reference the data in the
	 *          command that uses it.
	 */
	blockSize = (SIZE_BLOCK_DATA / 2) - SIZE_BLOCK_HEADER;
	numBlocks = data->data_length / blockSize;
	lastBlockSize = data->data_length % blockSize;
	if (lastBlockSize > 0)
		numBlocks++;

	blockHeader[2] = (u8) (numBlocks >> 8);
	blockHeader[3] = (u8) numBlocks;
	blockHeader[6] = (u8) (sti7109->data_handle >> 8);
	blockHeader[7] = (u8) sti7109->data_handle;
	blockPtr = sti7109->data_buffer;
	activeBlock = 0;
	for (blockIndex = 0; blockIndex < numBlocks; blockIndex++) {
		u32 addr;

		if (lastBlockSize && (blockIndex == (numBlocks - 1)))
			blockSize = lastBlockSize;

		blockHeader[0] = (uint8_t) (blockIndex >> 8);
		blockHeader[1] = (uint8_t) blockIndex;
		blockHeader[4] = (uint8_t) (blockSize >> 8);
		blockHeader[5] = (uint8_t) blockSize;

		addr = ADDR_BLOCK_DATA + activeBlock * (SIZE_BLOCK_DATA / 2);
		saa716x_phi_write(saa716x, addr, blockHeader,
				  SIZE_BLOCK_HEADER);
		saa716x_phi_write(saa716x, addr + SIZE_BLOCK_HEADER, blockPtr,
				  blockSize);
		activeBlock = (activeBlock + 1) & 1;
		if (blockIndex > 0) {
			timeout = 1 * HZ;
			timeout = wait_event_timeout(sti7109->block_done_wq,
						     sti7109->block_done == 1,
						     timeout);

			if (sti7109->block_done == 0) {
				dprintk(SAA716x_ERROR, 1,
					"timed out waiting for block done");
				/* send a data interrupt to cancel the transfer
				   and reset the data handling */
				SAA716x_EPWR(PHI_1, FPGA_ADDR_PHI_ISET,
						ISR_DATA_MASK);
				return -EIO;
			}
		}
		sti7109->block_done = 0;
		SAA716x_EPWR(PHI_1, FPGA_ADDR_PHI_ISET, ISR_BLOCK_MASK);
		blockPtr += blockSize;
	}
	timeout = 1 * HZ;
	timeout = wait_event_timeout(sti7109->block_done_wq,
				     sti7109->block_done == 1,
				     timeout);

	if (sti7109->block_done == 0) {
		dprintk(SAA716x_ERROR, 1, "timed out waiting for block done");
		/* send a data interrupt to cancel the transfer and reset the
		   data handling */
		SAA716x_EPWR(PHI_1, FPGA_ADDR_PHI_ISET, ISR_DATA_MASK);
		return -EIO;
	}
	sti7109->block_done = 0;

	data->data_handle = sti7109->data_handle;
	sti7109->data_handle++;
	return 0;
}

int sti7109_raw_data(struct sti7109_dev * sti7109, osd_raw_data_t * data)
{
	struct saa716x_dev * saa716x = sti7109->dev;
	int err;

	if (data->data_length > MAX_DATA_LEN) {
		dprintk(SAA716x_ERROR, 1, "data too big");
		return -EFAULT;
	}

	mutex_lock(&sti7109->data_lock);

	err = -EFAULT;
	if (copy_from_user(sti7109->data_buffer,
			   (void __user *)data->data_buffer,
			   data->data_length))
		goto out;

	err = sti7109_do_raw_data(sti7109, data);
	if (err)
		goto out;

out:
	mutex_unlock(&sti7109->data_lock);
	return err;
}

int sti7109_cmd_get_fw_version(struct sti7109_dev *sti7109, u32 *fw_version)
{
	int ret_val = -EINVAL;

	mutex_lock(&sti7109->cmd_lock);

	sti7109->cmd_data[0] = 0x00;
	sti7109->cmd_data[1] = 0x04;
	sti7109->cmd_data[2] = 0x00;
	sti7109->cmd_data[3] = 0x00;
	sti7109->cmd_data[4] = 0x00;
	sti7109->cmd_data[5] = 0x00;
	sti7109->cmd_len = 6;
	sti7109->result_max_len = MAX_RESULT_LEN;

	ret_val = sti7109_do_raw_cmd(sti7109);
	if (ret_val == 0) {
		*fw_version = (sti7109->result_data[6] << 16)
			    | (sti7109->result_data[7] << 8)
			    | sti7109->result_data[8];
	}

	mutex_unlock(&sti7109->cmd_lock);

	return ret_val;
}

int sti7109_cmd_get_video_format(struct sti7109_dev *sti7109, video_size_t *vs)
{
	int ret_val = -EINVAL;

	mutex_lock(&sti7109->cmd_lock);

	sti7109->cmd_data[0] = 0x00;
	sti7109->cmd_data[1] = 0x05; /* command length */
	sti7109->cmd_data[2] = 0x00;
	sti7109->cmd_data[3] = 0x01; /* A/V decoder command group */
	sti7109->cmd_data[4] = 0x00;
	sti7109->cmd_data[5] = 0x10; /* get video format info command */
	sti7109->cmd_data[6] = 0x00; /* decoder index 0 */
	sti7109->cmd_len = 7;
	sti7109->result_max_len = MAX_RESULT_LEN;

	ret_val = sti7109_do_raw_cmd(sti7109);
	if (ret_val == 0) {
		vs->w = (sti7109->result_data[7] << 8)
		      | sti7109->result_data[8];
		vs->h = (sti7109->result_data[9] << 8)
		      | sti7109->result_data[10];
		vs->aspect_ratio = sti7109->result_data[11] >> 4;
	}

	mutex_unlock(&sti7109->cmd_lock);

	return ret_val;
}

