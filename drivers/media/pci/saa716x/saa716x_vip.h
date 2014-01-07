#ifndef __SAA716x_VIP_H
#define __SAA716x_VIP_H

#include "saa716x_dma.h"

#define VIP_BUFFERS	8

/*
 * Stream port flags
 */
enum vip_stream_flags {
	VIP_ODD_FIELD		= 0x0001,
	VIP_EVEN_FIELD		= 0x0002,
	VIP_INTERLACED		= 0x0004,
	VIP_HD			= 0x0010,
	VIP_NO_SCALER		= 0x0100
};

/*
 * Stream port parameters
 * bits: Bits per sample
 * samples: samples perline
 * lines: number of lines
 * pitch: stream pitch in bytes
 * offset_x: offset to first valid pixel
 * offset_y: offset to first valid line
 */
struct vip_stream_params {
	u32			bits;
	u32			samples;
	u32			lines;
	s32			pitch;
	u32			offset_x;
	u32			offset_y;
	enum vip_stream_flags	stream_flags;
};

struct saa716x_vip_stream_port {
	u8			dual_channel;
	u8			read_index;
	u8			dma_channel[2];
	struct saa716x_dmabuf	dma_buf[2][VIP_BUFFERS];
	struct saa716x_dev	*saa716x;
	struct tasklet_struct	tasklet;
};

extern void saa716x_vipint_disable(struct saa716x_dev *saa716x);
extern void saa716x_vip_disable(struct saa716x_dev *saa716x);
extern int saa716x_vip_get_write_index(struct saa716x_dev *saa716x, int port);
extern int saa716x_vip_start(struct saa716x_dev *saa716x, int port,
			     int one_shot,
			     struct vip_stream_params *stream_params);
extern int saa716x_vip_stop(struct saa716x_dev *saa716x, int port);
extern int saa716x_vip_init(struct saa716x_dev *saa716x, int port,
			    void (*worker)(unsigned long));
extern int saa716x_vip_exit(struct saa716x_dev *saa716x, int port);

#endif /* __SAA716x_VIP_H */
