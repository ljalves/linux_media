#include <linux/kernel.h>
#include <linux/string.h>

#include "saa716x_rom.h"
#include "saa716x_adap.h"
#include "saa716x_spi.h"
#include "saa716x_priv.h"

int i;

static int eeprom_read_bytes(struct saa716x_dev *saa716x, u16 reg, u16 len, u8 *val)
{
	struct saa716x_i2c *i2c		= saa716x->i2c;
	struct i2c_adapter *adapter	= &i2c[SAA716x_I2C_BUS_B].i2c_adapter;

	u8 b0[] = { MSB(reg), LSB(reg) };
	int ret;

	struct i2c_msg msg[] = {
		{ .addr = 0x50, .flags = 0,	   .buf = b0,  .len = sizeof (b0) },
		{ .addr = 0x50,	.flags = I2C_M_RD, .buf = val, .len = len }
	};

	ret = i2c_transfer(adapter, msg, 2);
	if (ret != 2) {
		dprintk(SAA716x_ERROR, 1, "read error <reg=0x%02x, ret=%i>", reg, ret);
		return -EREMOTEIO;
	}

	return ret;
}

static int saa716x_read_rombytes(struct saa716x_dev *saa716x, u16 reg, u16 len, u8 *val)
{
	struct saa716x_i2c *i2c		= saa716x->i2c;
	struct i2c_adapter *adapter	= &i2c[SAA716x_I2C_BUS_B].i2c_adapter;
	struct i2c_msg msg[2];

	u8 b0[2];
	int ret, count;

	count = len / DUMP_BYTES;
	if (len % DUMP_BYTES)
		count++;

	count *= 2;

	for (i = 0; i < count; i += 2) {
		dprintk(SAA716x_DEBUG, 1, "Length=%d, Count=%d, Reg=0x%02x",
			len,
			count,
			reg);

		b0[0] = MSB(reg);
		b0[1] = LSB(reg);

		/* Write */
		msg[0].addr  = 0x50;
		msg[0].flags = 0;
		msg[0].buf   = b0;
		msg[0].len   = 2;

		/* Read */
		msg[1].addr  = 0x50;
		msg[1].flags = I2C_M_RD;
		msg[1].buf   = val;

		if (i == (count - 2)) {
			/* last message */
			if (len % DUMP_BYTES) {
				msg[1].len = len % DUMP_BYTES;
				dprintk(SAA716x_DEBUG, 1, "Last Message length=%d", len % DUMP_BYTES);
			} else {
				msg[1].len = DUMP_BYTES;
			}
		} else {
			msg[1].len = DUMP_BYTES;
		}

		ret = i2c_transfer(adapter, msg, 2);
		if (ret != 2) {
			dprintk(SAA716x_ERROR, 1, "read error <reg=0x%02x, ret=%i>", reg, ret);
			return -EREMOTEIO;
		}

		reg += DUMP_BYTES;
		val += DUMP_BYTES;
	}

	return 0;
}

static int saa716x_get_offset(struct saa716x_dev *saa716x, u8 *buf, u32 *offset)
{
	int i;

	*offset = 0;
	for (i = 0; i < 256; i++) {
		if (!(strncmp("START", buf + i, 5)))
			break;
	}
	dprintk(SAA716x_INFO, 1, "Offset @ %d", i);
	*offset = i;

	return 0;
}

static int saa716x_eeprom_header(struct saa716x_dev *saa716x,
				 struct saa716x_romhdr *rom_header,
				 u8 *buf,
				 u32 *offset)
{
	memcpy(rom_header, &buf[*offset], sizeof (struct saa716x_romhdr));
	if (rom_header->header_size != sizeof (struct saa716x_romhdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			sizeof (struct saa716x_romhdr),
			rom_header->header_size);

		return -1;
	}
	*offset += sizeof (struct saa716x_romhdr);

	dprintk(SAA716x_NOTICE, 0, "SAA%02x ROM: Data=%d bytes\n",
		saa716x->pdev->device,
		rom_header->data_size);

	dprintk(SAA716x_NOTICE, 0, "SAA%02x ROM: Version=%d\n",
		saa716x->pdev->device,
		rom_header->version);

	dprintk(SAA716x_NOTICE, 0, "SAA%02x ROM: Devices=%d\n",
		saa716x->pdev->device,
		rom_header->devices);

	dprintk(SAA716x_NOTICE, 0, "SAA%02x ROM: Compressed=%d\n\n",
		saa716x->pdev->device,
		rom_header->compression);

	return 0;
}

int saa716x_dump_eeprom(struct saa716x_dev *saa716x)
{
	struct saa716x_romhdr rom_header;
	u8 buf[DUMP_BYTES];
	int i, err = 0;
	u32 offset = 0;

	err = eeprom_read_bytes(saa716x, DUMP_OFFST, DUMP_BYTES, buf);
	if (err < 0) {
		dprintk(SAA716x_ERROR, 1, "EEPROM Read error");
		return err;
	}

	dprintk(SAA716x_NOTICE, 0, "    Card: %s\n",
		saa716x->config->model_name);

	dprintk(SAA716x_NOTICE, 0,
		"    ---------------- SAA%02x ROM @ Offset 0x%02x ----------------",
		saa716x->pdev->device,
		DUMP_OFFST);

	for (i = 0; i < DUMP_BYTES; i++) {
		if ((i % 16) == 0) {
			dprintk(SAA716x_NOTICE, 0, "\n    ");
			dprintk(SAA716x_NOTICE, 0, "%04x: ", i);
		}

		if ((i %  8) == 0)
			dprintk(SAA716x_NOTICE, 0, " ");
		if ((i %  4) == 0)
			dprintk(SAA716x_NOTICE, 0, " ");
		dprintk(SAA716x_NOTICE, 0, "%02x ", buf[i]);
	}
	dprintk(SAA716x_NOTICE, 0, "\n");
	dprintk(SAA716x_NOTICE, 0,
		"    ---------------- SAA%02x ROM Dump end ---------------------\n\n",
		saa716x->pdev->device);

	err = saa716x_get_offset(saa716x, buf, &offset);
	if (err != 0) {
		dprintk(SAA716x_ERROR, 1, "ERROR: Descriptor not found <%d>", err);
		return err;
	}
	offset += 5;
	saa716x->id_offst = offset;
	/* Get header */
	err = saa716x_eeprom_header(saa716x, &rom_header, buf, &offset);
	if (err != 0) {
		dprintk(SAA716x_ERROR, 1, "ERROR: Header Read failed <%d>", err);
		return -1;
	}
	saa716x->id_len = rom_header.data_size;

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_dump_eeprom);

static void saa716x_descriptor_dbg(struct saa716x_dev *saa716x,
				   u8 *buf,
				   u32 *offset,
				   u8 size,
				   u8 ext_size)
{
	int i;

	dprintk(SAA716x_INFO, 0, "       ");
	for (i = 0; i < 49; i++)
		dprintk(SAA716x_INFO, 0, "-");

	for (i = 0; i < size + ext_size; i++) {
		if ((i % 16) == 0)
			dprintk(SAA716x_INFO, 0, "\n      ");
		if ((i %  8) == 0)
			dprintk(SAA716x_INFO, 0, " ");
		if ((i %  4) == 0)
			dprintk(SAA716x_INFO, 0, " ");

		dprintk(SAA716x_INFO, 0, "%02x ", buf[*offset + i]);
	}

	dprintk(SAA716x_INFO, 0, "\n       ");
	for (i = 0; i < 49; i++)
		dprintk(SAA716x_INFO, 0, "-");
	dprintk(SAA716x_INFO, 0, "\n");

}

static int saa716x_decoder_info(struct saa716x_dev *saa716x,
				u8 *buf,
				u32 *offset)
{
	struct saa716x_decoder_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_decoder_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_decoder_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_decoder_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext Data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_gpio_info(struct saa716x_dev *saa716x,
			     u8 *buf,
			     u32 *offset)
{
	struct saa716x_gpio_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_gpio_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_gpio_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_gpio_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Pins=%d\n",
		saa716x->pdev->device,
		header.pins);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;

	return 0;
}

static int saa716x_video_decoder_info(struct saa716x_dev *saa716x,
				      u8 *buf,
				      u32 *offset)
{
	struct saa716x_video_decoder_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_video_decoder_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_video_decoder_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_video_decoder_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: PORT 0=0x%02x\n",
		saa716x->pdev->device,
		header.video_port0);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: PORT 1=0x%02x\n",
		saa716x->pdev->device,
		header.video_port1);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: PORT 2=0x%02x\n",
		saa716x->pdev->device,
		header.video_port2);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: VBI PORT ID=0x%02x\n",
		saa716x->pdev->device,
		header.vbi_port_id);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Video PORT Type=0x%02x\n",
		saa716x->pdev->device,
		header.video_port_type);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: VBI PORT Type=0x%02x\n",
		saa716x->pdev->device,
		header.vbi_port_type);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Encoder PORT Type=0x%02x\n",
		saa716x->pdev->device,
		header.encoder_port_type);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Video Output=0x%02x\n",
		saa716x->pdev->device,
		header.video_output);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: VBI Output=0x%02x\n",
		saa716x->pdev->device,
		header.vbi_output);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Encoder Output=0x%02x\n",
		saa716x->pdev->device,
		header.encoder_output);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_audio_decoder_info(struct saa716x_dev *saa716x,
				      u8 *buf,
				      u32 *offset)
{
	struct saa716x_audio_decoder_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_audio_decoder_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_audio_decoder_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_audio_decoder_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_event_source_info(struct saa716x_dev *saa716x,
				     u8 *buf,
				     u32 *offset)
{
	struct saa716x_evsrc_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_evsrc_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_evsrc_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_evsrc_hdr));

		return -1;
	}
	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_crossbar_info(struct saa716x_dev *saa716x,
				 u8 *buf,
				 u32 *offset)
{
	struct saa716x_xbar_hdr header;
	struct saa716x_xbar_pair_info pair_info;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_xbar_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_xbar_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_xbar_hdr));

		return -1;
	}

	memcpy(&pair_info, &buf[*offset], sizeof (struct saa716x_xbar_pair_info));

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Pairs=%d\n",
		saa716x->pdev->device,
		header.pair_inputs);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data + (sizeof (struct saa716x_xbar_pair_info) * header.pair_inputs);
	return 0;
}

static int saa716x_tuner_info(struct saa716x_dev *saa716x,
			      u8 *buf,
			      u32 *offset)
{
	struct saa716x_tuner_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_tuner_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_tuner_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_tuner_hdr));

		return -1;
	}
	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_pll_info(struct saa716x_dev *saa716x,
			    u8 *buf,
			    u32 *offset)
{
	struct saa716x_pll_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_pll_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_pll_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_pll_hdr));

		return -1;
	}
	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_channel_decoder_info(struct saa716x_dev *saa716x,
					u8 *buf,
					u32 *offset)
{
	struct saa716x_channel_decoder_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_channel_decoder_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_channel_decoder_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_channel_decoder_hdr));

		return -1;
	}
	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_encoder_info(struct saa716x_dev *saa716x,
				u8 *buf,
				u32 *offset)
{
	struct saa716x_encoder_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_encoder_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_encoder_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_encoder_hdr));

		return -1;
	}
	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_ir_info(struct saa716x_dev *saa716x,
			   u8 *buf,
			   u32 *offset)
{
	struct saa716x_ir_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_ir_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_ir_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_ir_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_eeprom_info(struct saa716x_dev *saa716x,
			       u8 *buf,
			       u32 *offset)
{
	struct saa716x_eeprom_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_eeprom_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_eeprom_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_eeprom_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_filter_info(struct saa716x_dev *saa716x,
			       u8 *buf,
			       u32 *offset)
{
	struct saa716x_filter_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_filter_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_filter_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_filter_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_streamdev_info(struct saa716x_dev *saa716x,
				  u8 *buf,
				  u32 *offset)
{
	struct saa716x_streamdev_hdr header;

	memcpy(&header, &buf[*offset], sizeof (struct saa716x_streamdev_hdr));
	saa716x_descriptor_dbg(saa716x, buf, offset, header.size, header.ext_data);
	if (header.size != sizeof (struct saa716x_streamdev_hdr)) {
		dprintk(SAA716x_ERROR, 1,
			"ERROR: Header size mismatch! Read size=%d bytes, Expected=%d",
			header.size,
			sizeof (struct saa716x_streamdev_hdr));

		return -1;
	}

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		header.size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n",
		saa716x->pdev->device,
		header.ext_data);

	*offset += header.size + header.ext_data;
	return 0;
}

static int saa716x_unknown_device_info(struct saa716x_dev *saa716x,
				       u8 *buf,
				       u32 *offset)
{
	u8 size;
	u8 ext_size = 0;

	size = buf[*offset];
	if (size > 1)
		ext_size = buf[*offset + size -1];

	saa716x_descriptor_dbg(saa716x, buf, offset, size, ext_size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		size);

	dprintk(SAA716x_NOTICE, 0,
		"        SAA%02x ROM: Ext data=%d bytes\n\n",
		saa716x->pdev->device,
		ext_size);

	*offset += size + ext_size;
	return 0;
}


static void saa716x_device_dbg(struct saa716x_dev *saa716x,
			       u8 *buf,
			       u32 *offset,
			       u8 size,
			       u8 ext_size,
			       u8 addr_size)
{
	int i;

	dprintk(SAA716x_INFO, 0, "   ");
	for (i = 0; i < 53; i++)
		dprintk(SAA716x_INFO, 0, "-");

	for (i = 0; i < size + ext_size + addr_size; i++) {
		if ((i % 16) == 0)
			dprintk(SAA716x_INFO, 0, "\n  ");
		if ((i %  8) == 0)
			dprintk(SAA716x_INFO, 0, " ");
		if ((i %  4) == 0)
			dprintk(SAA716x_INFO, 0, " ");

		dprintk(SAA716x_INFO, 0, "%02x ", buf[*offset + i]);
	}

	dprintk(SAA716x_INFO, 0, "\n   ");
	for (i = 0; i < 53; i++)
		dprintk(SAA716x_INFO, 0, "-");
	dprintk(SAA716x_INFO, 0, "\n");

}


static int saa716x_device_info(struct saa716x_dev *saa716x,
			       struct saa716x_devinfo *device,
			       u8 *buf,
			       u32 *offset)
{
	u8 address = 0;

	memcpy(device, &buf[*offset], sizeof (struct saa716x_devinfo));
	if (device->struct_size != sizeof (struct saa716x_devinfo)) {
		dprintk(SAA716x_ERROR, 1, "ERROR: Device size mismatch! Read=%d bytes, expected=%d bytes",
		device->struct_size,
		sizeof (struct saa716x_devinfo));

		return -1;
	}

	saa716x_device_dbg(saa716x,
			   buf,
			   offset,
			   device->struct_size,
			   device->extd_data_size,
			   device->addr_size);

	*offset += device->struct_size;

	if (device->addr_size) {
		address = buf[*offset];
		address >>= 1;
		*offset += device->addr_size;
	}

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Device @ 0x%02x\n",
		saa716x->pdev->device,
		address);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Size=%d bytes\n",
		saa716x->pdev->device,
		device->struct_size);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Device ID=0x%02x\n",
		saa716x->pdev->device,
		device->device_id);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Master ID=0x%02x\n",
		saa716x->pdev->device,
		device->master_devid);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Bus ID=0x%02x\n",
		saa716x->pdev->device,
		device->master_busid);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Device type=0x%02x\n",
		saa716x->pdev->device,
		device->device_type);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Implementation ID=0x%02x\n",
		saa716x->pdev->device,
		device->implem_id);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Path ID=0x%02x\n",
		saa716x->pdev->device,
		device->path_id);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: GPIO ID=0x%02x\n",
		saa716x->pdev->device,
		device->gpio_id);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Address=%d bytes\n",
		saa716x->pdev->device,
		device->addr_size);

	dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: Extended data=%d bytes\n\n",
		saa716x->pdev->device,
		device->extd_data_size);

	if (device->extd_data_size) {
		u32 mask;

		mask = 0x00000001;
		while (mask) {
			if (device->device_type & mask) {
				switch (mask) {
				case DECODER_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found decoder device\n",
						saa716x->pdev->device);

					saa716x_decoder_info(saa716x, buf, offset);
					break;

				case GPIO_SOURCE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found GPIO device\n",
						saa716x->pdev->device);

					saa716x_gpio_info(saa716x, buf, offset);
					break;

				case VIDEO_DECODER:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Video Decoder device\n",
						saa716x->pdev->device);

					saa716x_video_decoder_info(saa716x, buf, offset);
					break;

				case AUDIO_DECODER:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Audio Decoder device\n",
						saa716x->pdev->device);

					saa716x_audio_decoder_info(saa716x, buf, offset);
					break;

				case EVENT_SOURCE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Event source\n",
						saa716x->pdev->device);

					saa716x_event_source_info(saa716x, buf, offset);
					break;

				case CROSSBAR:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Crossbar device\n",
						saa716x->pdev->device);

					saa716x_crossbar_info(saa716x, buf, offset);
					break;

				case TUNER_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Tuner device\n",
						saa716x->pdev->device);

					saa716x_tuner_info(saa716x, buf, offset);
					break;

				case PLL_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found PLL device\n",
						saa716x->pdev->device);

					saa716x_pll_info(saa716x, buf, offset);
					break;

				case CHANNEL_DECODER:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Channel Demodulator device\n",
						saa716x->pdev->device);

					saa716x_channel_decoder_info(saa716x, buf, offset);
					break;

				case RDS_DECODER:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found RDS Decoder device\n",
						saa716x->pdev->device);

					saa716x_unknown_device_info(saa716x, buf, offset);
					break;

				case ENCODER_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Encoder device\n",
						saa716x->pdev->device);

					saa716x_encoder_info(saa716x, buf, offset);
					break;

				case IR_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found IR device\n",
						saa716x->pdev->device);

					saa716x_ir_info(saa716x, buf, offset);
					break;

				case EEPROM_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found EEPROM device\n",
						saa716x->pdev->device);

					saa716x_eeprom_info(saa716x, buf, offset);
					break;

				case NOISE_FILTER:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Noise filter device\n",
						saa716x->pdev->device);

					saa716x_filter_info(saa716x, buf, offset);
					break;

				case LNx_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found LNx device\n",
						saa716x->pdev->device);

					saa716x_unknown_device_info(saa716x, buf, offset);
					break;

				case STREAM_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found streaming device\n",
						saa716x->pdev->device);

					saa716x_streamdev_info(saa716x, buf, offset);
					break;

				case CONFIGSPACE_DEVICE:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found Configspace device\n",
						saa716x->pdev->device);

					saa716x_unknown_device_info(saa716x, buf, offset);
					break;

				default:
					dprintk(SAA716x_NOTICE, 0,
						"        SAA%02x ROM: Found unknown device\n",
						saa716x->pdev->device);

					saa716x_unknown_device_info(saa716x, buf, offset);
					break;
				}
			}
			mask <<= 1;
		}
	}

	dprintk(SAA716x_NOTICE, 0, "\n");

	return 0;
}

int saa716x_eeprom_data(struct saa716x_dev *saa716x)
{
	struct saa716x_romhdr rom_header;
	struct saa716x_devinfo *device;

	u8 buf[1024];
	int i, ret = 0;
	u32 offset = 0;

	/* dump */
	ret = saa716x_read_rombytes(saa716x, saa716x->id_offst, saa716x->id_len + 8, buf);
	if (ret < 0) {
		dprintk(SAA716x_ERROR, 1, "EEPROM Read error <%d>", ret);
		goto err0;
	}

	/* Get header */
	ret = saa716x_eeprom_header(saa716x, &rom_header, buf, &offset);
	if (ret != 0) {
		dprintk(SAA716x_ERROR, 1, "ERROR: Header Read failed <%d>", ret);
		goto err0;
	}

	/* allocate for device info */
	device = kzalloc(sizeof (struct saa716x_devinfo) * rom_header.devices, GFP_KERNEL);
	if (device == NULL) {
		dprintk(SAA716x_ERROR, 1, "ERROR: out of memory");
		goto err0;
	}

	for (i = 0; i < rom_header.devices; i++) {
		dprintk(SAA716x_NOTICE, 0, "    SAA%02x ROM: ===== Device %d =====\n",
			saa716x->pdev->device,
			i);

		ret = saa716x_device_info(saa716x, &device[i], buf, &offset);
		if (ret != 0) {
			dprintk(SAA716x_ERROR, 1, "ERROR: Device info read failed <%d>", ret);
			goto err1;
		}
	}

	kfree(device);

	return 0;

err1:
	kfree(device);

err0:
	return ret;
}
EXPORT_SYMBOL_GPL(saa716x_eeprom_data);
