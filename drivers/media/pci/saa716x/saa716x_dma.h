#ifndef __SAA716x_DMA_H
#define __SAA716x_DMA_H

#define SAA716x_PAGE_SIZE	4096

#define PTA_LSB(__mem)		((u32 ) (__mem))
#define PTA_MSB(__mem)		((u32 ) ((u64)(__mem) >> 32))

#define BAM_DMA_BUF_MODE_BASE		0x00
#define BAM_DMA_BUF_MODE_OFFSET		0x24

#define BAM_DMA_BUF_MODE(__ch)		(BAM_DMA_BUF_MODE_BASE + (BAM_DMA_BUF_MODE_OFFSET * __ch))

#define BAM_ADDR_OFFSET_BASE		0x04
#define BAM_ADDR_OFFSET_OFFSET		0x24

#define BAM_ADDR_OFFSET(__ch)		(BAM_ADDR_OFFSET_BASE + (BAM_ADDR_OFFSET_OFFSET * __ch))

#define BAM_ADDR_OFFSET_0(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x00)
#define BAM_ADDR_OFFSET_1(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x04)
#define BAM_ADDR_OFFSET_2(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x08)
#define BAM_ADDR_OFFSET_3(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x0c)
#define BAM_ADDR_OFFSET_4(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x10)
#define BAM_ADDR_OFFSET_5(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x14)
#define BAM_ADDR_OFFSET_6(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x18)
#define BAM_ADDR_OFFSET_7(__ch)		(BAM_ADDR_OFFSET(__ch) + 0x1c)


enum saa716x_dma_type {
	SAA716x_DMABUF_EXT_LIN, /* Linear external */
	SAA716x_DMABUF_EXT_SG, /* SG external */
	SAA716x_DMABUF_INT /* Linear internal */
};

struct saa716x_dev;

struct saa716x_dmabuf {
	enum saa716x_dma_type	dma_type;

	void			*mem_virt_noalign;
	void			*mem_virt; /* page aligned */
	dma_addr_t		mem_ptab_phys;
	void			*mem_ptab_virt;
	void			*sg_list; /* SG list */

	struct saa716x_dev	*saa716x;

	int			list_len; /* buffer len */
	int			offset; /* page offset */
};

extern int saa716x_dmabuf_alloc(struct saa716x_dev *saa716x,
				struct saa716x_dmabuf *dmabuf,
				int size);
extern void saa716x_dmabuf_free(struct saa716x_dev *saa716x,
				struct saa716x_dmabuf *dmabuf);

extern void saa716x_dmabufsync_dev(struct saa716x_dmabuf *dmabuf);
extern void saa716x_dmabufsync_cpu(struct saa716x_dmabuf *dmabuf);

#endif /* __SAA716x_DMA_H */
