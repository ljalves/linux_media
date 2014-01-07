#ifndef __SAA716x_ROM_H
#define __SAA716x_ROM_H


#define MSB(__x)	((__x >> 8) & 0xff)
#define LSB(__x)	(__x & 0xff)

#define DUMP_BYTES	0xf0
#define DUMP_OFFST	0x000

struct saa716x_dev;

struct saa716x_romhdr {
	u16	header_size;
	u8	compression;
	u8	version;
	u16	data_size;
	u8	devices;
	u8	checksum;
} __attribute__((packed));

struct saa716x_devinfo {
	u8	struct_size;
	u8	device_id;
	u8	master_devid;
	u8	master_busid;
	u32	device_type;
	u16	implem_id;
	u8	path_id;
	u8	gpio_id;
	u16	addr_size;
	u16	extd_data_size;
} __attribute__((packed));

enum saa716x_device_types {
	DECODER_DEVICE		= 0x00000001,
	GPIO_SOURCE		= 0x00000002,
	VIDEO_DECODER		= 0x00000004,
	AUDIO_DECODER		= 0x00000008,
	EVENT_SOURCE		= 0x00000010,
	CROSSBAR		= 0x00000020,
	TUNER_DEVICE		= 0x00000040,
	PLL_DEVICE		= 0x00000080,
	CHANNEL_DECODER		= 0x00000100,
	RDS_DECODER		= 0x00000200,
	ENCODER_DEVICE		= 0x00000400,
	IR_DEVICE		= 0x00000800,
	EEPROM_DEVICE		= 0x00001000,
	NOISE_FILTER		= 0x00002000,
	LNx_DEVICE		= 0x00004000,
	STREAM_DEVICE		= 0x00010000,
	CONFIGSPACE_DEVICE	= 0x80000000
};

struct saa716x_decoder_hdr {
	u8 size;
	u8 ext_data;
};

struct saa716x_decoder_info {
	struct saa716x_decoder_hdr decoder_hdr;
	u8 *ext_data;
};

struct saa716x_gpio_hdr {
	u8 size;
	u8 pins;
	u8 rsvd;
	u8 ext_data;
};

struct saa716x_gpio_info {
	struct saa716x_gpio_hdr gpio_hdr;
	u8 *ext_data;
};

struct saa716x_video_decoder_hdr {
	u8 size;
	u8 video_port0;
	u8 video_port1;
	u8 video_port2;
	u8 vbi_port_id;
	u8 video_port_type;
	u8 vbi_port_type;
	u8 encoder_port_type;
	u8 video_output;
	u8 vbi_output;
	u8 encoder_output;
	u8 ext_data;
};

struct saa716x_video_decoder_info {
	struct saa716x_video_decoder_hdr decoder_hdr;
	u8 *ext_data;
};

struct saa716x_audio_decoder_hdr {
	u8 size;
	u8 port;
	u8 output;
	u8 ext_data;
};

struct saa716x_audio_decoder_info {
	struct saa716x_audio_decoder_hdr decoder_hdr;
	u8 *ext_data;
};

struct saa716x_evsrc_hdr {
	u8 size;
	u8 master_devid;
	u16 condition_id;
	u8 rsvd;
	u8 ext_data;
};

struct saa716x_evsrc_info {
	struct saa716x_evsrc_hdr evsrc_hdr;
	u8 *ext_data;
};

enum saa716x_input_pair_type {
	TUNER_SIF	= 0x00,
	TUNER_LINE	= 0x01,
	TUNER_SPDIF	= 0x02,
	TUNER_NONE	= 0x03,
	CVBS_LINE	= 0x04,
	CVBS_SPDIF	= 0x05,
	CVBS_NONE	= 0x06,
	YC_LINE		= 0x07,
	YC_SPDIF	= 0x08,
	YC_NONE		= 0x09,
	YPbPr_LINE	= 0x0a,
	YPbPr_SPDIF	= 0x0b,
	YPbPr_NONE	= 0x0c,
	NO_LINE		= 0x0d,
	NO_SPDIF	= 0x0e,
	RGB_LINE	= 0x0f,
	RGB_SPDIF	= 0x10,
	RGB_NONE	= 0x11
};

struct saa716x_xbar_pair_info {
	u8 pair_input_type;
	u8 video_input_id;
	u8 audio_input_id;
};

struct saa716x_xbar_hdr {
	u8 size;
	u8 pair_inputs;
	u8 pair_route_default;
	u8 ext_data;
};

struct saa716x_xbar_info {
	struct saa716x_xbar_hdr xbar_hdr;
	struct saa716x_xbar_pair_info *pair_info;
	u8 *ext_data;
};

struct saa716x_tuner_hdr {
	u8 size;
	u8 ext_data;
};

struct saa716x_tuner_info {
	struct saa716x_tuner_hdr tuner_hdr;
	u8 *ext_data;
};

struct saa716x_pll_hdr {
	u8 size;
	u8 ext_data;
};

struct saa716x_pll_info {
	struct saa716x_pll_hdr pll_hdr;
	u8 *ext_data;
};

struct saa716x_channel_decoder_hdr {
	u8 size;
	u8 port;
	u8 ext_data;
};

struct saa716x_channel_decoder_info {
	struct saa716x_channel_decoder_hdr channel_dec_hdr;
	u8 *ext_data;
};

struct saa716x_encoder_hdr {
	u8 size;
	u8 stream_port0;
	u8 stream_port1;
	u8 ext_data;
};

struct saa716x_encoder_info {
	struct saa716x_encoder_hdr encoder_hdr;
	u8 *ext_data;
};

struct saa716x_ir_hdr {
	u8 size;
	u8 ir_caps;
	u8 ext_data;
};

struct saa716x_ir_info {
	struct saa716x_ir_hdr ir_hdr;
	u8 *ext_data;
};

struct saa716x_eeprom_hdr {
	u8 size;
	u8 rel_device;
	u8 ext_data;
};

struct saa716x_eeprom_info {
	struct saa716x_eeprom_hdr eeprom_hdr;
	u8 *ext_data;
};

struct saa716x_filter_hdr {
	u8 size;
	u8 video_decoder;
	u8 audio_decoder;
	u8 event_source;
	u8 ext_data;
};

struct saa716x_filter_info {
	struct saa716x_filter_hdr filter_hdr;
	u8 *ext_data;
};

struct saa716x_streamdev_hdr {
	u8 size;
	u8 ext_data;
};

struct saa716x_streamdev_info {
	struct saa716x_streamdev_hdr streamdev_hdr;
	u8 *ext_data;
};

extern int saa716x_dump_eeprom(struct saa716x_dev *saa716x);
extern int saa716x_eeprom_data(struct saa716x_dev *saa716x);

#endif /* __SAA716x_ROM_H */
