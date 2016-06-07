/*
    TBS ECP3 FPGA based cards PCIe driver

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "tbsecp3.h"

#define TS_PACKET_SIZE		188
#define TBSECP3_DMA_PAGE_SIZE	(2048 * TS_PACKET_SIZE)
#define TBSECP3_DMA_BUF_SIZE	(TBSECP3_DMA_PAGE_SIZE / TBSECP3_DMA_BUFFERS)
#define TBSECP3_BUF_PACKETS	(TBSECP3_DMA_BUF_SIZE / TS_PACKET_SIZE)

static void tbsecp3_dma_tasklet(unsigned long adap)
{
	struct tbsecp3_adapter *adapter = (struct tbsecp3_adapter *) adap;
	struct tbsecp3_dev *dev = adapter->dev;
	u8* data;
	u32 read_buffer;
	int i;

	spin_lock(&adapter->adap_lock);

	/* read data from two buffers below the active one */
	read_buffer = (tbs_read(adapter->dma.base, TBSECP3_DMA_STAT) - 2) & 7;
	data = adapter->dma.buf[read_buffer];

	if (data[adapter->dma.offset] != 0x47) {
		/* find sync byte offset */
		for (i = 0; i < TS_PACKET_SIZE; i++)
			if ((data[i] == 0x47) &&
			    (data[i + TS_PACKET_SIZE] == 0x47) &&
		 	    (data[i + 2 * TS_PACKET_SIZE] == 0x47)) {
				adapter->dma.offset = i;
				break;
			}
	}

	if (adapter->dma.offset != 0) {
		data += adapter->dma.offset;
		/* copy the remains of the last packet from buffer 0 to end of 7 */
		if (read_buffer == 7) {
			memcpy( adapter->dma.buf[8],
				adapter->dma.buf[0], adapter->dma.offset);
		}
	}
	dvb_dmx_swfilter_packets(&adapter->demux, data, TBSECP3_BUF_PACKETS);
	spin_unlock(&adapter->adap_lock);
}

void tbsecp3_dma_enable(struct tbsecp3_adapter *adap)
{
	struct tbsecp3_dev *dev = adap->dev;

	spin_lock_irq(&adap->adap_lock);
	adap->dma.offset = 0;
	adap->dma.cnt = 0;
	tbs_read(adap->dma.base, TBSECP3_DMA_STAT);
	tbs_write(TBSECP3_INT_BASE, TBSECP3_DMA_IE(adap->cfg->ts_in), 1); 
	tbs_write(adap->dma.base, TBSECP3_DMA_EN, 1);
	spin_unlock_irq(&adap->adap_lock);
}

void tbsecp3_dma_disable(struct tbsecp3_adapter *adap)
{
	struct tbsecp3_dev *dev = adap->dev;

	spin_lock_irq(&adap->adap_lock);
	tbs_read(adap->dma.base, TBSECP3_DMA_STAT);
	tbs_write(TBSECP3_INT_BASE, TBSECP3_DMA_IE(adap->cfg->ts_in), 0);
	tbs_write(adap->dma.base, TBSECP3_DMA_EN, 0);
	spin_unlock_irq(&adap->adap_lock);
}

void tbsecp3_dma_reg_init(struct tbsecp3_dev *dev)
{
	int i;
	struct tbsecp3_adapter *adapter = dev->adapter;

	for (i = 0; i < dev->info->adapters; i++) {
		tbs_write(adapter->dma.base, TBSECP3_DMA_EN, 0);
		tbs_write(adapter->dma.base, TBSECP3_DMA_ADDRH, 0);
		tbs_write(adapter->dma.base, TBSECP3_DMA_ADDRL, (u32) adapter->dma.dma_addr);
		tbs_write(adapter->dma.base, TBSECP3_DMA_TSIZE, TBSECP3_DMA_PAGE_SIZE);
		tbs_write(adapter->dma.base, TBSECP3_DMA_BSIZE, TBSECP3_DMA_BUF_SIZE);
		adapter++;
	}
}

void tbsecp3_dma_free(struct tbsecp3_dev *dev)
{
	struct tbsecp3_adapter *adapter = dev->adapter;
	int i;
	for (i = 0; i < dev->info->adapters; i++) {
		if (adapter->dma.buf[0] == NULL)
			continue;

		pci_free_consistent(dev->pci_dev,
			TBSECP3_DMA_PAGE_SIZE + 0x100,
			adapter->dma.buf[0], adapter->dma.dma_addr);
		adapter->dma.buf[0] = NULL;
		adapter++;
	}
}

int tbsecp3_dma_init(struct tbsecp3_dev *dev)
{
	struct tbsecp3_adapter *adapter = dev->adapter;
	int i, j;

	for (i = 0; i < dev->info->adapters; i++) {
		adapter->dma.buf[0] = pci_alloc_consistent(dev->pci_dev,
				TBSECP3_DMA_PAGE_SIZE + 0x100,
				&adapter->dma.dma_addr);
		if (!adapter->dma.buf[0])
			goto err;

		adapter->dma.base = TBSECP3_DMA_BASE(adapter->cfg->ts_in);
		adapter->dma.cnt = 0;
		for (j = 1; j < TBSECP3_DMA_BUFFERS + 1; j++)
			adapter->dma.buf[j] = adapter->dma.buf[j-1] + TBSECP3_DMA_BUF_SIZE;

		tasklet_init(&adapter->tasklet, tbsecp3_dma_tasklet, (unsigned long) adapter);
		spin_lock_init(&adapter->adap_lock);
		adapter++;
	}
	tbsecp3_dma_reg_init(dev);
	return 0;
err:
	dev_err(&dev->pci_dev->dev, "dma: memory alloc failed\n");
	tbsecp3_dma_free(dev);
	return -ENOMEM;
}

