#ifndef __SAA716x_FF_CMD_H
#define __SAA716x_FF_CMD_H

#if !defined OSD_RAW_CMD
typedef struct osd_raw_cmd_s {
	const void *cmd_data;
	int cmd_len;
	void *result_data;
	int result_len;
} osd_raw_cmd_t;

typedef struct osd_raw_data_s {
	const void *data_buffer;
	int data_length;
	int data_handle;
} osd_raw_data_t;

#define OSD_RAW_CMD            _IOWR('o', 162, osd_raw_cmd_t)
#define OSD_RAW_DATA           _IOWR('o', 163, osd_raw_data_t)
#endif

extern int sti7109_cmd_init(struct sti7109_dev *sti7109);
extern int sti7109_raw_cmd(struct sti7109_dev * sti7109,
			   osd_raw_cmd_t * cmd);
extern int sti7109_raw_osd_cmd(struct sti7109_dev * sti7109,
			       osd_raw_cmd_t * cmd);
extern int sti7109_raw_data(struct sti7109_dev * sti7109,
			    osd_raw_data_t * data);
extern int sti7109_cmd_get_fw_version(struct sti7109_dev *sti7109,
				      u32 *fw_version);
extern int sti7109_cmd_get_video_format(struct sti7109_dev *sti7109,
					video_size_t *vs);

#endif /* __SAA716x_FF_CMD_H */
