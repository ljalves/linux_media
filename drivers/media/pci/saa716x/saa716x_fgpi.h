#ifndef __SAA716x_FGPI_H
#define __SAA716x_FGPI_H

#include <linux/interrupt.h>

#define FGPI_BUFFERS		8


/*
 * Port supported streams
 *
 * FGPI_AUDIO_STREAM
 * FGPI_VIDEO_STREAM
 * FGPI_VBI_STREAM
 * FGPI_TRANSPORT_STREAM
 * FGPI_PROGRAM_STREAM
 */
enum fgpi_stream_type {
	FGPI_RAW_STREAM		= 0x00,
	FGPI_AUDIO_STREAM	= 0x01,
	FGPI_VIDEO_STREAM	= 0x02,
	FGPI_VBI_STREAM		= 0x04,
	FGPI_TRANSPORT_STREAM	= 0x08,
	FGPI_PROGRAM_STREAM	= 0x10
};

/*
 * Stream port flags
 *
 * FGPI_ODD_FIELD
 * FGPI_EVEN_FIELD
 * FGPI_HD_0
 * FGPI_HD_1
 * FGPI_PAL
 * FGPI_NTSC
 */
enum fgpi_stream_flags {
	FGPI_ODD_FIELD		= 0x0001,
	FGPI_EVEN_FIELD		= 0x0002,
	FGPI_INTERLACED		= 0x0004,
	FGPI_HD0		= 0x0010,
	FGPI_HD1		= 0x0020,
	FGPI_PAL		= 0x0040,
	FGPI_NTSC		= 0x0080,
	FGPI_NO_SCALER		= 0x0100,
};

/*
 * Stream port parameters
 * bits: Bits per sample
 * samples: samples perline
 * lines: number of lines
 * pitch: stream pitch in bytes
 * offset: offset to first valid line
 */
struct fgpi_stream_params {
	u32			bits;
	u32			samples;
	u32			lines;

	s32			pitch;

	u32			offset;
	u32			page_tables;

	enum fgpi_stream_flags	stream_flags;
	enum fgpi_stream_type	stream_type;
};

struct saa716x_dmabuf;

struct saa716x_fgpi_stream_port {
	u8			dma_channel;
	struct saa716x_dmabuf	dma_buf[FGPI_BUFFERS];
	struct saa716x_dev	*saa716x;
	struct tasklet_struct	tasklet;
	u8			read_index;
};

extern void saa716x_fgpiint_disable(struct saa716x_dmabuf *dmabuf, int channel);
extern int saa716x_fgpi_get_write_index(struct saa716x_dev *saa716x,
					u32 fgpi_index);
extern int saa716x_fgpi_start(struct saa716x_dev *saa716x, int port,
			      struct fgpi_stream_params *stream_params);
extern int saa716x_fgpi_stop(struct saa716x_dev *saa716x, int port);

extern int saa716x_fgpi_init(struct saa716x_dev *saa716x, int port,
			      int dma_buf_size,
			      void (*worker)(unsigned long));
extern int saa716x_fgpi_exit(struct saa716x_dev *saa716x, int port);

#endif /* __SAA716x_FGPI_H */
