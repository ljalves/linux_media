#include <linux/kernel.h>

#include "saa716x_mod.h"

#include "saa716x_vip_reg.h"
#include "saa716x_dma_reg.h"
#include "saa716x_msi_reg.h"

#include "saa716x_priv.h"


static const u32 vi_ch[] = {
	VI0,
	VI1
};

static const u32 msi_int_tagack[] = {
	MSI_INT_TAGACK_VI0_0,
	MSI_INT_TAGACK_VI0_1,
	MSI_INT_TAGACK_VI0_2,
	MSI_INT_TAGACK_VI1_0,
	MSI_INT_TAGACK_VI1_1,
	MSI_INT_TAGACK_VI1_2
};

static const u32 msi_int_avint[] = {
	MSI_INT_AVINT_VI0,
	MSI_INT_AVINT_VI1
};

void saa716x_vipint_disable(struct saa716x_dev *saa716x)
{
	SAA716x_EPWR(VI0, INT_ENABLE, 0); /* disable VI 0 IRQ */
	SAA716x_EPWR(VI1, INT_ENABLE, 0); /* disable VI 1 IRQ */
	SAA716x_EPWR(VI0, INT_CLR_STATUS, 0x3ff); /* clear IRQ */
	SAA716x_EPWR(VI1, INT_CLR_STATUS, 0x3ff); /* clear IRQ */
}
EXPORT_SYMBOL_GPL(saa716x_vipint_disable);

void saa716x_vip_disable(struct saa716x_dev *saa716x)
{
       SAA716x_EPWR(VI0, VIP_POWER_DOWN, VI_PWR_DWN);
       SAA716x_EPWR(VI1, VIP_POWER_DOWN, VI_PWR_DWN);
}
EXPORT_SYMBOL_GPL(saa716x_vip_disable);

int saa716x_vip_get_write_index(struct saa716x_dev *saa716x, int port)
{
	u32 buf_mode, val;

	buf_mode = BAM_DMA_BUF_MODE(saa716x->vip[port].dma_channel[0]);

	val = SAA716x_EPRD(BAM, buf_mode);
	return (val >> 3) & 0x7;
}
EXPORT_SYMBOL_GPL(saa716x_vip_get_write_index);

static int saa716x_vip_init_ptables(struct saa716x_dmabuf *dmabuf, int channel,
				    struct vip_stream_params *stream_params)
{
	struct saa716x_dev *saa716x = dmabuf->saa716x;
	u32 config, i;

	for (i = 0; i < VIP_BUFFERS; i++)
		BUG_ON((dmabuf[i].mem_ptab_phys == 0));

	config = MMU_DMA_CONFIG(channel); /* DMACONFIGx */

	SAA716x_EPWR(MMU, config, (VIP_BUFFERS - 1));

	if ((stream_params->stream_flags & VIP_INTERLACED) &&
	    (stream_params->stream_flags & VIP_ODD_FIELD) &&
	    (stream_params->stream_flags & VIP_EVEN_FIELD)) {
		/* In interlaced mode the same buffer is written twice, once
		   the odd field and once the even field */
		SAA716x_EPWR(MMU, MMU_PTA0_LSB(channel), PTA_LSB(dmabuf[0].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA0_MSB(channel), PTA_MSB(dmabuf[0].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA1_LSB(channel), PTA_LSB(dmabuf[0].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA1_MSB(channel), PTA_MSB(dmabuf[0].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA2_LSB(channel), PTA_LSB(dmabuf[1].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA2_MSB(channel), PTA_MSB(dmabuf[1].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA3_LSB(channel), PTA_LSB(dmabuf[1].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA3_MSB(channel), PTA_MSB(dmabuf[1].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA4_LSB(channel), PTA_LSB(dmabuf[2].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA4_MSB(channel), PTA_MSB(dmabuf[2].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA5_LSB(channel), PTA_LSB(dmabuf[2].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA5_MSB(channel), PTA_MSB(dmabuf[2].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA6_LSB(channel), PTA_LSB(dmabuf[3].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA6_MSB(channel), PTA_MSB(dmabuf[3].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA7_LSB(channel), PTA_LSB(dmabuf[3].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA7_MSB(channel), PTA_MSB(dmabuf[3].mem_ptab_phys)); /* High */
	} else {
		SAA716x_EPWR(MMU, MMU_PTA0_LSB(channel), PTA_LSB(dmabuf[0].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA0_MSB(channel), PTA_MSB(dmabuf[0].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA1_LSB(channel), PTA_LSB(dmabuf[1].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA1_MSB(channel), PTA_MSB(dmabuf[1].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA2_LSB(channel), PTA_LSB(dmabuf[2].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA2_MSB(channel), PTA_MSB(dmabuf[2].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA3_LSB(channel), PTA_LSB(dmabuf[3].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA3_MSB(channel), PTA_MSB(dmabuf[3].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA4_LSB(channel), PTA_LSB(dmabuf[4].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA4_MSB(channel), PTA_MSB(dmabuf[4].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA5_LSB(channel), PTA_LSB(dmabuf[5].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA5_MSB(channel), PTA_MSB(dmabuf[5].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA6_LSB(channel), PTA_LSB(dmabuf[6].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA6_MSB(channel), PTA_MSB(dmabuf[6].mem_ptab_phys)); /* High */
		SAA716x_EPWR(MMU, MMU_PTA7_LSB(channel), PTA_LSB(dmabuf[7].mem_ptab_phys)); /* Low */
		SAA716x_EPWR(MMU, MMU_PTA7_MSB(channel), PTA_MSB(dmabuf[7].mem_ptab_phys)); /* High */
	}
	return 0;
}

static int saa716x_vip_setparams(struct saa716x_dev *saa716x, int port,
				 struct vip_stream_params *stream_params)
{
	u32 vi_port, buf_mode, mid, val, i;
	u8 dma_channel;
	u32 num_pages;
	u32 start_x, start_line, end_line, num_lines;
	u32 base_address, base_offset, pitch;
	u32 vin_format;

	vi_port = vi_ch[port];
	buf_mode = BAM_DMA_BUF_MODE(saa716x->vip[port].dma_channel[0]);
	dma_channel = saa716x->vip[port].dma_channel[0];

	/* number of pages needed for a buffer */
	num_pages = (stream_params->bits / 8 * stream_params->samples
		     * stream_params->lines) / SAA716x_PAGE_SIZE;
	/* check if these will fit into one page table */
	if (num_pages > (SAA716x_PAGE_SIZE / 8))
		saa716x->vip[port].dual_channel = 1;
	else
		saa716x->vip[port].dual_channel = 0;

	/* Reset DMA channel */
	SAA716x_EPWR(BAM, buf_mode, 0x00000040);
	saa716x_vip_init_ptables(saa716x->vip[port].dma_buf[0],
				 saa716x->vip[port].dma_channel[0],
				 stream_params);
	if (saa716x->vip[port].dual_channel)
		saa716x_vip_init_ptables(saa716x->vip[port].dma_buf[1],
					 saa716x->vip[port].dma_channel[1],
					 stream_params);

	/* get module ID */
	mid = SAA716x_EPRD(vi_port, VI_MODULE_ID);
	if (mid != 0x11A5100) {
		dprintk(SAA716x_ERROR, 1, "VIP Id<%04x> is not supported", mid);
		return -1;
	}

	start_x = stream_params->offset_x;
	start_line = stream_params->offset_y + 1;
	end_line = 0;
	num_lines = stream_params->lines;
	pitch = stream_params->pitch;
	base_address = saa716x->vip[port].dma_channel[0] << 21;
	base_offset = 0;
	vin_format = 0x00004000;

	if ((stream_params->stream_flags & VIP_INTERLACED) &&
	    (stream_params->stream_flags & VIP_ODD_FIELD) &&
	    (stream_params->stream_flags & VIP_EVEN_FIELD)) {
		num_lines /= 2;
		pitch *= 2;
		base_offset = stream_params->pitch;
	}
	if (stream_params->stream_flags & VIP_HD) {
		if (stream_params->stream_flags & VIP_INTERLACED) {
			vin_format |= 0x01000000;
		} else {
			/* suppress the windower break message */
			vin_format |= 0x01000200;
		}
	}
	if (stream_params->stream_flags & VIP_NO_SCALER) {
		vin_format |= 0x00000400;
	}

	end_line = stream_params->offset_y + num_lines;

	/* set device to normal operation */
	SAA716x_EPWR(vi_port, VIP_POWER_DOWN, 0);
	/* disable ANC bit detection */
	SAA716x_EPWR(vi_port, ANC_DID_FIELD0, 0);
	SAA716x_EPWR(vi_port, ANC_DID_FIELD1, 0);
	/* set line threshold to 0 (interrupt is disabled anyway)*/
	SAA716x_EPWR(vi_port, VI_LINE_THRESH, 0);

	vin_format |= 2;
	SAA716x_EPWR(vi_port, VIN_FORMAT, vin_format);

	/* disable dithering */
	SAA716x_EPWR(vi_port, PRE_DIT_CTRL, 0);
	SAA716x_EPWR(vi_port, POST_DIT_CTRL, 0);
	/* set alpha value */
	SAA716x_EPWR(vi_port, CSM_CKEY, 0);

	SAA716x_EPWR(vi_port, WIN_XYSTART, (start_x << 16) + start_line);
	SAA716x_EPWR(vi_port, WIN_XYEND,
		     ((start_x + stream_params->samples - 1) << 16) + end_line);

	/* enable cropping to assure that VIP does not exceed buffer boundaries */
	SAA716x_EPWR(vi_port, PSU_WINDOW,
		     (stream_params->samples << 16) + num_lines);
	/* set packet YUY2 output format */
	SAA716x_EPWR(vi_port, PSU_FORMAT, 0x800000A1);

	SAA716x_EPWR(vi_port, PSU_BASE1, base_address);
	SAA716x_EPWR(vi_port, PSU_PITCH1, pitch);
	SAA716x_EPWR(vi_port, PSU_PITCH2, 0);
	SAA716x_EPWR(vi_port, PSU_BASE2, 0);
	SAA716x_EPWR(vi_port, PSU_BASE3, 0);
	SAA716x_EPWR(vi_port, PSU_BASE4, base_address + base_offset);
	SAA716x_EPWR(vi_port, PSU_BASE5, 0);
	SAA716x_EPWR(vi_port, PSU_BASE6, 0);

	/* monitor BAM reset */
	i = 0;
	val = SAA716x_EPRD(BAM, buf_mode);
	while (val && (i < 100)) {
		msleep(30);
		val = SAA716x_EPRD(BAM, buf_mode);
		i++;
	}
	if (val) {
		dprintk(SAA716x_ERROR, 1, "Error: BAM VIP Reset failed!");
		return -EIO;
	}

	/* set buffer count */
	SAA716x_EPWR(BAM, buf_mode, VIP_BUFFERS - 1);
	/* initialize all available address offsets to 0 */
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_0(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_1(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_2(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_3(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_4(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_5(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_6(dma_channel), 0x0);
	SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_7(dma_channel), 0x0);

	if (saa716x->vip[port].dual_channel) {
		buf_mode = BAM_DMA_BUF_MODE(saa716x->vip[port].dma_channel[1]);
		dma_channel = saa716x->vip[port].dma_channel[1];

		/* set buffer count */
		SAA716x_EPWR(BAM, buf_mode, VIP_BUFFERS - 1);
		/* initialize all available address offsets to 0 */
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_0(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_1(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_2(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_3(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_4(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_5(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_6(dma_channel), 0x0);
		SAA716x_EPWR(BAM, BAM_ADDR_OFFSET_7(dma_channel), 0x0);
	}

	return 0;
}

int saa716x_vip_start(struct saa716x_dev *saa716x, int port, int one_shot,
		      struct vip_stream_params *stream_params)
{
	u32 vi_port;
	u32 config1;
	u32 config2;
	u32 val;
	u32 i;

	vi_port = vi_ch[port];
	config1 = MMU_DMA_CONFIG(saa716x->vip[port].dma_channel[0]);
	config2 = MMU_DMA_CONFIG(saa716x->vip[port].dma_channel[1]);

	if (saa716x_vip_setparams(saa716x, port, stream_params) != 0) {
		return -EIO;
	}

	val = SAA716x_EPRD(MMU, config1);
	SAA716x_EPWR(MMU, config1, val & ~0x40);
	SAA716x_EPWR(MMU, config1, val | 0x40);
	if (saa716x->vip[port].dual_channel) {
		val = SAA716x_EPRD(MMU, config2);
		SAA716x_EPWR(MMU, config2, val & ~0x40);
		SAA716x_EPWR(MMU, config2, val | 0x40);
	}

	SAA716x_EPWR(vi_port, INT_ENABLE, 0x33F);

	i = 0;
	while (i < 500) {
		val = SAA716x_EPRD(MMU, config1);
		if (saa716x->vip[port].dual_channel)
			val &= SAA716x_EPRD(MMU, config2);
		if (val & 0x80)
			break;
		msleep(10);
		i++;
	}

	if (!(val & 0x80)) {
		dprintk(SAA716x_ERROR, 1, "Error: PTE pre-fetch failed!");
		return -EIO;
	}

	/* enable video capture path */
	val = SAA716x_EPRD(vi_port, VI_MODE);
	val &= ~(VID_CFEN | VID_FSEQ | VID_OSM);

	if ((stream_params->stream_flags & VIP_INTERLACED) &&
	    (stream_params->stream_flags & VIP_ODD_FIELD) &&
	    (stream_params->stream_flags & VIP_EVEN_FIELD)) {
		val |= VID_CFEN_BOTH; /* capture both fields */
		val |= VID_FSEQ; /* start capture with odd field */
	} else {
		val |= VID_CFEN_BOTH; /* capture both fields */
	}

	if (one_shot)
		val |= VID_OSM; /* stop capture after receiving one frame */

	saa716x_set_clk_external(saa716x, saa716x->vip[port].dma_channel[0]);

	SAA716x_EPWR(vi_port, VI_MODE, val);

	SAA716x_EPWR(MSI, MSI_INT_ENA_SET_L, msi_int_tagack[port]);

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_vip_start);

int saa716x_vip_stop(struct saa716x_dev *saa716x, int port)
{
	u32 val;

	SAA716x_EPWR(MSI, MSI_INT_ENA_CLR_L, msi_int_tagack[port]);

	/* disable capture */
	val = SAA716x_EPRD(vi_ch[port], VI_MODE);
	val &= ~VID_CFEN;
	SAA716x_EPWR(vi_ch[port], VI_MODE, val);

	saa716x_set_clk_internal(saa716x, saa716x->vip[port].dma_channel[0]);

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_vip_stop);

int saa716x_vip_init(struct saa716x_dev *saa716x, int port,
		     void (*worker)(unsigned long))
{
	int n;
	int i;
	int ret;

	/* reset VI */
	SAA716x_EPWR(vi_ch[port], VI_MODE, SOFT_RESET);

	for (n = 0; n < 2; n++)
	{
		saa716x->vip[port].dma_channel[n] = port * 3 + n;
		for (i = 0; i < VIP_BUFFERS; i++)
		{
			ret = saa716x_dmabuf_alloc(
					saa716x,
					&saa716x->vip[port].dma_buf[n][i],
					512 * SAA716x_PAGE_SIZE);
			if (ret < 0) {
				return ret;
			}
		}
	}
	saa716x->vip[port].saa716x = saa716x;
	tasklet_init(&saa716x->vip[port].tasklet, worker,
		     (unsigned long)&saa716x->vip[port]);
	saa716x->vip[port].read_index = 0;

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_vip_init);

int saa716x_vip_exit(struct saa716x_dev *saa716x, int port)
{
	int n;
	int i;

	tasklet_kill(&saa716x->vip[port].tasklet);
	for (n = 0; n < 2; n++)
	{
		for (i = 0; i < VIP_BUFFERS; i++)
		{
			saa716x_dmabuf_free(
				saa716x, &saa716x->vip[port].dma_buf[n][i]);
		}
	}

	return 0;
}
EXPORT_SYMBOL_GPL(saa716x_vip_exit);
