#ifndef __SAA716x_FF_CMD_H
#define __SAA716x_FF_CMD_H

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
