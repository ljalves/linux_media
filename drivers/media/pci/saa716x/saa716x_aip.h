#ifndef __SAA716x_AIP_H
#define __SAA716x_AIP_H

struct saa716x_dev;

extern int saa716x_aip_status(struct saa716x_dev *saa716x, u32 dev);
extern void saa716x_aip_disable(struct saa716x_dev *saa716x);

#endif /* __SAA716x_AIP_H */
