#ifndef __SAA716x_FF_H
#define __SAA716x_FF_H

#include "dvb_filter.h"
#include "dvb_ringbuffer.h"
#include <linux/version.h>
#include <linux/workqueue.h>

#define TECHNOTREND			0x13c2
#define S2_6400_DUAL_S2_PREMIUM_DEVEL	0x3009
#define S2_6400_DUAL_S2_PREMIUM_PROD	0x300A

#define TT_PREMIUM_GPIO_POWER_ENABLE	27
#define TT_PREMIUM_GPIO_RESET_BACKEND	26
#define TT_PREMIUM_GPIO_FPGA_CS1	17
#define TT_PREMIUM_GPIO_FPGA_CS0	16
#define TT_PREMIUM_GPIO_FPGA_PROGRAMN	15
#define TT_PREMIUM_GPIO_FPGA_DONE	14
#define TT_PREMIUM_GPIO_FPGA_INITN	13

/* fpga interrupt register addresses */
#define FPGA_ADDR_PHI_ICTRL	0x8000 /* PHI General control of the PC => STB interrupt controller */
#define FPGA_ADDR_PHI_ISR	0x8010 /* PHI Interrupt Status Register */
#define FPGA_ADDR_PHI_ISET	0x8020 /* PHI Interrupt Set Register */
#define FPGA_ADDR_PHI_ICLR	0x8030 /* PHI Interrupt Clear Register */
#define FPGA_ADDR_EMI_ICTRL	0x8100 /* EMI General control of the STB => PC interrupt controller */
#define FPGA_ADDR_EMI_ISR	0x8110 /* EMI Interrupt Status Register */
#define FPGA_ADDR_EMI_ISET	0x8120 /* EMI Interrupt Set Register */
#define FPGA_ADDR_EMI_ICLR	0x8130 /* EMI Interrupt Clear Register */

/* fpga TS router register addresses */
#define FPGA_ADDR_TSR_CTRL	0x8200 /* TS router control register */
#define FPGA_ADDR_TSR_MUX1	0x8210 /* TS multiplexer 1 selection register */
#define FPGA_ADDR_TSR_MUX2	0x8220 /* TS multiplexer 2 selection register */
#define FPGA_ADDR_TSR_MUX3	0x8230 /* TS multiplexer 3 selection register */
#define FPGA_ADDR_TSR_MUXCI1	0x8240 /* TS multiplexer CI 1 selection register */
#define FPGA_ADDR_TSR_MUXCI2	0x8250 /* TS multiplexer CI 2 selection register */

#define FPGA_ADDR_TSR_BRFE1	0x8280 /* bit rate for TS coming from frontend 1 */
#define FPGA_ADDR_TSR_BRFE2	0x8284 /* bit rate for TS coming from frontend 2 */
#define FPGA_ADDR_TSR_BRFF1	0x828C /* bit rate for TS coming from FIFO 1 */
#define FPGA_ADDR_TSR_BRO1	0x8294 /* bit rate for TS going to output 1 */
#define FPGA_ADDR_TSR_BRO2	0x8298 /* bit rate for TS going to output 2 */
#define FPGA_ADDR_TSR_BRO3	0x829C /* bit rate for TS going to output 3 */

/* fpga TS FIFO register addresses */
#define FPGA_ADDR_FIFO_CTRL	0x8300 /* FIFO control register */
#define FPGA_ADDR_FIFO_STAT	0x8310 /* FIFO status register */

#define FPGA_ADDR_VERSION	0x80F0 /* FPGA bitstream version register */

#define FPGA_ADDR_PIO_CTRL	0x8500 /* FPGA GPIO control register */

#define ISR_CMD_MASK		0x0001 /* interrupt source for normal cmds (osd, fre, av, ...) */
#define ISR_READY_MASK		0x0002 /* interrupt source for command acknowledge */
#define ISR_BLOCK_MASK		0x0004 /* interrupt source for single block transfers and acknowledge */
#define ISR_DATA_MASK		0x0008 /* interrupt source for data transfer acknowledge */
#define ISR_BOOT_FINISH_MASK	0x0010 /* interrupt source for boot finish indication */
#define ISR_AUDIO_PTS_MASK	0x0020 /* interrupt source for audio PTS */
#define ISR_VIDEO_PTS_MASK	0x0040 /* interrupt source for video PTS */
#define ISR_CURRENT_STC_MASK	0x0080 /* interrupt source for current system clock */
#define ISR_REMOTE_EVENT_MASK	0x0100 /* interrupt source for remote events */
#define ISR_DVO_FORMAT_MASK	0x0200 /* interrupt source for DVO format change */
#define ISR_OSD_CMD_MASK	0x0400 /* interrupt source for OSD cmds */
#define ISR_OSD_READY_MASK	0x0800 /* interrupt source for OSD command acknowledge */
#define ISR_FE_CMD_MASK		0x1000 /* interrupt source for frontend cmds */
#define ISR_FE_READY_MASK	0x2000 /* interrupt source for frontend command acknowledge */
#define ISR_LOG_MESSAGE_MASK	0x4000 /* interrupt source for log messages */
#define ISR_FIFO1_EMPTY_MASK	0x8000 /* interrupt source for FIFO1 empty */

#define ADDR_CMD_DATA		0x0000 /* address for cmd data in fpga dpram */
#define ADDR_OSD_CMD_DATA	0x01A0 /* address for OSD cmd data */
#define ADDR_FE_CMD_DATA	0x05C0 /* address for frontend cmd data */
#define ADDR_BLOCK_DATA		0x0600 /* address for block data */
#define ADDR_AUDIO_PTS		0x3E00 /* address for audio PTS (64 Bits) */
#define ADDR_VIDEO_PTS		0x3E08 /* address for video PTS (64 Bits) */
#define ADDR_CURRENT_STC	0x3E10 /* address for system clock (64 Bits) */
#define ADDR_DVO_FORMAT		0x3E18 /* address for DVO format 32 Bits) */
#define ADDR_REMOTE_EVENT	0x3F00 /* address for remote events (32 Bits) */
#define ADDR_LOG_MESSAGE	0x3F80 /* address for log messages */

#define SIZE_CMD_DATA		0x01A0 /* maximum size for command data (416 Bytes) */
#define SIZE_OSD_CMD_DATA	0x0420 /* maximum size for OSD command data (1056 Bytes) */
#define SIZE_FE_CMD_DATA	0x0040 /* maximum size for frontend command data (64 Bytes) */
#define SIZE_BLOCK_DATA		0x3800 /* maximum size for block data (14 kB) */
#define SIZE_LOG_MESSAGE_DATA	0x0080 /* maximum size for log message data (128 Bytes) */

#define SIZE_BLOCK_HEADER	8      /* block header size */

#define MAX_RESULT_LEN		256
#define MAX_DATA_LEN		(1024 * 1024)

#define TSOUT_LEN		(1024 * TS_SIZE)

#define TSOUT_STAT_RESET	0
#define TSOUT_STAT_FILL 	1
#define TSOUT_STAT_RUN  	2

#define VIDEO_CAPTURE_OFF	0
#define VIDEO_CAPTURE_ONE_SHOT	1


/* place to store all the necessary device information */
struct sti7109_dev {
	struct saa716x_dev	*dev;
	struct dvb_device	*osd_dev;
	struct dvb_device	*video_dev;
	struct dvb_device	*audio_dev;

	void			*iobuf;	 /* memory for all buffers */
	struct dvb_ringbuffer	tsout;   /* buffer for TS output */
	u32			tsout_stat;

	struct workqueue_struct *fifo_workq;
	struct work_struct	fifo_work;

	wait_queue_head_t	boot_finish_wq;
	int			boot_finished;

	wait_queue_head_t	cmd_ready_wq;
	int			cmd_ready;
	u8			cmd_data[SIZE_CMD_DATA];
	u32			cmd_len;

	wait_queue_head_t	result_avail_wq;
	int			result_avail;
	u8			result_data[MAX_RESULT_LEN];
	u32			result_len;
	u32			result_max_len;

	wait_queue_head_t	osd_cmd_ready_wq;
	int			osd_cmd_ready;
	u8			osd_cmd_data[SIZE_OSD_CMD_DATA];
	u32			osd_cmd_len;

	wait_queue_head_t	osd_result_avail_wq;
	int			osd_result_avail;
	u8			osd_result_data[MAX_RESULT_LEN];
	u32			osd_result_len;
	u32			osd_result_max_len;

	u16			data_handle;
	u8			*data_buffer; /* raw data transfer buffer */
	wait_queue_head_t	data_ready_wq;
	int			data_ready;
	wait_queue_head_t	block_done_wq;
	int			block_done;

	struct mutex		cmd_lock;
	struct mutex		osd_cmd_lock;
	struct mutex		data_lock;
	struct mutex		video_lock;

	u64			audio_pts;
	u64			video_pts;
	u64			current_stc;

	u32			video_capture;
	u32			video_format;

	u32			int_count_enable;
	u32			total_int_count;
	u32			vi_int_count[2];
	u32			fgpi_int_count[4];
	u32			i2c_int_count[2];
	u32			ext_int_total_count;
	u32			ext_int_source_count[16];
	u32			last_int_ticks;

	u16			fpga_version;
};

#endif /* __SAA716x_FF_H */
