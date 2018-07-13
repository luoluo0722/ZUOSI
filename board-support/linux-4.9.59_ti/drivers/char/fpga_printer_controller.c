/*
 * Char device driver fpga.
 * Do a global replace of 'fpga' with your driver name.
 */

#define DEBUG
#include <linux/platform_device.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/omap-dma.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/mtd/nand_bch.h>
#include <linux/platform_data/elm.h>
#include <linux/omap-gpmc.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define	DRIVER_NAME	"omap2-fpga-printer-ctl"
#define	DEVICE_NAME	"fpga_printer_ctl"

#define USER_BUFF_SIZE SZ_1M

struct omap_fpga_printer_ctl_dev {
	dev_t dev_id;
	struct cdev cdev;
	struct platform_device *pdev;
	const char *name;
	struct semaphore sem;
	struct class *drv_class;
	int gpmc_cs;
	char *user_buff;
	unsigned long phys_base;
	struct dentry *debug_dir;
	struct dentry *debug_entry;
	struct task_struct *trans_thread;
	int rd_wr_flag;
	struct dma_chan			*dma;
	struct completion		comp;
	/* DMA address of the buffer in memory */
	//dma_addr_t		rx_addr;
	//dma_addr_t		tx_addr;
	//void			*rx_buf;
	//void			*tx_buf;

	void __iomem *IO_ADDR_R;
	void __iomem *IO_ADDR_W;
};

/*
 * omap_fpga_printer_ctl_dma_callback: callback on the completion of dma transfer
 * @data: pointer to completion data structure
 */
static void omap_fpga_printer_ctl_dma_callback(void *data)
{
	complete((struct completion *) data);
}

/*
 * omap_fpga_printer_ctl_dma_transfer: configure and start dma transfer
 * @mtd: MTD device structure
 * @addr: virtual address in RAM of source/destination
 * @len: number of data bytes to be transferred
 * @is_write: flag for read/write operation
 */
static inline int omap_fpga_printer_ctl_dma_transfer(struct omap_fpga_printer_ctl_dev *ctl_dev,
		void *addr, unsigned int len, int offset, int is_write)
		//dma_addr_t addr, size_t len, int offset, int is_write)
{
	struct dma_async_tx_descriptor *tx;
	struct dma_slave_config cfg;
	struct platform_device *pdev = ctl_dev->pdev;
	enum dma_data_direction dir = is_write ? DMA_TO_DEVICE :
							DMA_FROM_DEVICE;
	struct scatterlist sg;
	unsigned n;
	int err;

	if (!virt_addr_valid(addr)){
		dev_err(&pdev->dev, "Addr is not valid: 0x%p\n", addr);
		goto out_copy;
	}

	memset(&cfg, 0, sizeof(cfg));
	cfg.src_addr = ctl_dev->phys_base + offset;
	cfg.dst_addr = ctl_dev->phys_base + offset;
	cfg.src_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	cfg.dst_addr_width = DMA_SLAVE_BUSWIDTH_2_BYTES;
	cfg.src_maxburst = SZ_32K;
	cfg.dst_maxburst = SZ_32K;
	err = dmaengine_slave_config(ctl_dev->dma, &cfg);
	if (err) {
		dev_err(&pdev->dev, "DMA engine slave config failed: %d\n",
			err);
		goto out_copy;
	}

	sg_init_one(&sg, addr, len);
	n = dma_map_sg(ctl_dev->dma->device->dev, &sg, 1, dir);
	if (n == 0) {
		dev_err(&pdev->dev,
			"Couldn't DMA map a %d byte buffer\n", len);
		goto out_copy;
	}

	//tx = dmaengine_prep_slave_single(ctl_dev->dma, addr, len,
	tx = dmaengine_prep_slave_sg(ctl_dev->dma, &sg, n,
		is_write ? DMA_MEM_TO_DEV : DMA_DEV_TO_MEM,
		DMA_PREP_INTERRUPT | DMA_CTRL_ACK);
	if (!tx){
		dev_err(&pdev->dev,
			"dmaengine_prep_slave_sg err\n");
		goto out_copy_unmap;
	}

	tx->callback = omap_fpga_printer_ctl_dma_callback;
	tx->callback_param = &ctl_dev->comp;
	dmaengine_submit(tx);

	init_completion(&ctl_dev->comp);

	/* setup and start DMA using dma_addr */
	dma_async_issue_pending(ctl_dev->dma);

	dev_dbg(&pdev->dev,
				"before waiting\n");
	wait_for_completion(&ctl_dev->comp);
	dev_dbg(&pdev->dev,
				"after waiting\n");

	dma_unmap_sg(ctl_dev->dma->device->dev, &sg, 1, dir);
	return 0;

out_copy_unmap:
	dma_unmap_sg(ctl_dev->dma->device->dev, &sg, 1, dir);
out_copy:
#if 0
	if (info->nand.options & NAND_BUSWIDTH_16)
		is_write == 0 ? omap_read_buf16(mtd, (u_char *) addr, len)
			: omap_write_buf16(mtd, (u_char *) addr, len);
	else
		is_write == 0 ? omap_read_buf8(mtd, (u_char *) addr, len)
			: omap_write_buf8(mtd, (u_char *) addr, len);
#endif
	return 1;
}


static ssize_t omap_fpga_printer_ctl_write(struct file *filp, const char __user *buff,
size_t count, loff_t *f_pos)
{
	ssize_t write_num = 0;
	size_t once_write_num = USER_BUFF_SIZE;
	int i, tmp = 0;
	unsigned short offset = 0;
	struct omap_fpga_printer_ctl_dev *ctl_dev = filp->private_data;
	struct platform_device *pdev = ctl_dev->pdev;

	if (count == 0 || count > SZ_1M){
		dev_err(&pdev->dev, "omap_fpga_prienter_ctl_write: count fault %d\n", count);
		return 0;
	}

	if(copy_from_user(&offset, buff, sizeof(offset))) {
		dev_err(&pdev->dev, "omap_fpga_printer_ctl_write: get offset failed\n");
		write_num = -EFAULT;
		goto fpga_write_done;
	}
	buff += 2;
	count -= 2;
	dev_dbg(&pdev->dev, "omap_fpga_printer_ctl_write: offset = %x\n", offset);

	if (down_interruptible(&ctl_dev->sem))
		return -ERESTARTSYS;

	memset(ctl_dev->user_buff, 0, USER_BUFF_SIZE);

	while(write_num < count){
		
		if (once_write_num > (count - write_num))
			once_write_num = count - write_num;
		
		if (copy_from_user(ctl_dev->user_buff, buff, once_write_num)){
			dev_err(&pdev->dev, "omap_fpga_prienter_ctl_write: copy_from_user failed\n");
			write_num = -EFAULT;
			goto fpga_write_done;
		}
		buff += once_write_num;
		write_num += once_write_num;
		if(once_write_num >= SZ_64K 
			//&& !omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->tx_addr, once_write_num, offset, 0x1)){
			&& !omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->user_buff, once_write_num, offset, 0x1)){
			dev_dbg(&pdev->dev, "write DMA trans: %d\n", once_write_num);
		}else{
			for (i = 0; i < once_write_num; i += 2){
				tmp = ctl_dev->user_buff[i] | ctl_dev->user_buff[i+1] << 8;
				writew(tmp,ctl_dev->IO_ADDR_W + offset);
			}
		}
	}

fpga_write_done:
	up(&ctl_dev->sem);
	return write_num;
}

static ssize_t omap_fpga_printer_ctl_read(struct file *filp, char __user *buff,
size_t count, loff_t *offp)
{
	size_t read_num = 0;
	size_t once_read_num = USER_BUFF_SIZE;
	unsigned short offset;
	unsigned short tmp;
	int i;
	struct omap_fpga_printer_ctl_dev *ctl_dev = filp->private_data;
	struct platform_device *pdev = ctl_dev->pdev;

	/*
   	Generic user progs like cat will continue calling until we
   	return zero. So if *offp != 0, we know this is at least the
   	second call.
	*/
	if (*offp > 0)
		return 0;

	if(copy_from_user(&offset, buff, sizeof(offset))){
		dev_err(&pdev->dev, "omap_fpga_printer_ctl_read: get offset failed\n");
		read_num = -EFAULT;
		goto fpga_read_done;
	}
	buff += 2;
	count -= 2;
	dev_dbg(&pdev->dev, "omap_fpga_printer_ctl_read: offset = %x\n", offset);
	if (down_interruptible(&ctl_dev->sem))
		return -ERESTARTSYS;

	while(read_num < count){
		if (once_read_num > (count - read_num))
			once_read_num = count - read_num;

		if(once_read_num >= SZ_64K 
			//&& !omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->rx_addr, once_read_num, offset, 0x0)){
			&& !omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->user_buff, once_read_num, offset, 0x0)){
			dev_dbg(&pdev->dev, "read DMA trans: %d\n", once_read_num);
		} else{
			for(i = 0;i < once_read_num;i += 2){
				tmp = readw(ctl_dev->IO_ADDR_R + offset);
				ctl_dev->user_buff[i] = tmp;
				ctl_dev->user_buff[i+1] = tmp>>8;
			}
		}
		if(copy_to_user(buff, ctl_dev->user_buff, once_read_num)){
			dev_err(&pdev->dev, "omap_fpga_printer_ctl_read: copy_to_user failed\n");
			read_num = -EFAULT;
			goto fpga_read_done;
		}
		buff += once_read_num;
		read_num += once_read_num;
	}

fpga_read_done:
	up(&ctl_dev->sem);
	return read_num;
}

static int omap_fpga_printer_ctl_open(struct inode *inode, struct file *filp)
{
	int err = 0;
	struct omap_fpga_printer_ctl_dev *ctl_dev;
	struct platform_device *pdev;

	ctl_dev = container_of(inode->i_cdev, struct omap_fpga_printer_ctl_dev, cdev);

	if (ctl_dev != NULL) {
		if (down_interruptible(&ctl_dev->sem))
			return -ERESTARTSYS;

		filp->private_data = ctl_dev;
		pdev = ctl_dev->pdev;
		if (!ctl_dev->user_buff) {
			ctl_dev->user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);

			if (!ctl_dev->user_buff) {
				dev_err(&pdev->dev, "fpga_open: user_buff alloc failed\n");
				err = -ENOMEM;
			}
		}

		up(&ctl_dev->sem);
	} else {
		printk(KERN_ALERT "fpga_open: ctl_dev is null\n");
		err = -ENXIO;
	}

	return err;
}

static const struct file_operations omap_fpga_printer_ctl_fops = {
	.owner = THIS_MODULE,
	.open = omap_fpga_printer_ctl_open,
	.read = omap_fpga_printer_ctl_read,
	.write = omap_fpga_printer_ctl_write,
};

static int omap_fpga_printer_ctl_get_dt_info(struct device *dev, struct omap_fpga_printer_ctl_dev *ctl_dev)
{
	return 0;
}

static int omap_fpga_printer_debugfs_show(struct seq_file *s, void *what)
{
	//struct omap_fpga_printer_ctl_dev *ctl_dev = s->private;
	gpmc_cs_show_timings(2, "debugfs modify timging ");
	return 0;
}

static int omap_fpga_printer_debugfs_open(struct inode *inode, struct file *file)
{
	return single_open(file, omap_fpga_printer_debugfs_show, inode->i_private);
}

static int omap_fpga_printer_trans_thread(void *arg)
{
	struct omap_fpga_printer_ctl_dev *ctl_dev = (struct omap_fpga_printer_ctl_dev *)arg;
	struct platform_device *pdev = ctl_dev->pdev;
	unsigned short tmp;

	dev_dbg(&pdev->dev, "trans thread started\n");

	if(ctl_dev->rd_wr_flag == 0){
		while(!kthread_should_stop()){
			unsigned int offset = 0;
			while(offset < 1024 ){
				tmp = readw(ctl_dev->IO_ADDR_R + offset);
				offset += 2;
			}
		}
	}
	else if(ctl_dev->rd_wr_flag == 1){
		while(!kthread_should_stop()){
			unsigned int offset = 0;
			while(offset < 1024 ){
				writew(0x55AA,ctl_dev->IO_ADDR_W + offset);
				offset += 2;
			}
		}
	}
	else if(ctl_dev->rd_wr_flag == 3){
		while(!kthread_should_stop()){
			unsigned int offset = 0;
			while(offset < 1024 ){
				writew(0x55AA,ctl_dev->IO_ADDR_W + offset);
				tmp = readw(ctl_dev->IO_ADDR_R + offset);
				offset += 2;
			}
		}
	}
	else if(ctl_dev->rd_wr_flag == 5){
		//omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->rx_addr, SZ_1K, 0x4C, 0x0);
		omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->user_buff, SZ_1M, 0x4C, 0x0);
	}
	else if(ctl_dev->rd_wr_flag == 6){
		//omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->tx_addr, SZ_1K, 0x4C, 0x1);
		omap_fpga_printer_ctl_dma_transfer(ctl_dev, ctl_dev->user_buff, SZ_1M, 0x4C, 0x1);
	}
	else{
		dev_err(&pdev->dev, "trans thread unknown flag\n");
	}

	dev_dbg(&pdev->dev, "trans thread ended\n");

	return 0;
}

static ssize_t omap_fpga_printer_debugfs_write(struct file *file, const char __user *user_buf,
			      size_t size, loff_t *ppos)
{
	char buf[64];
	int buf_size;
	int ret = 0;
	struct omap_fpga_printer_ctl_dev *ctl_dev = ((struct seq_file *)file->private_data)->private;
	struct platform_device *pdev = ctl_dev->pdev;

	buf_size = min(size, (sizeof(buf) - 1));
	if (strncpy_from_user(buf, user_buf, buf_size) < 0)
		return -EFAULT;
	buf[buf_size] = 0;

	if(strncmp(buf, "start_rd_trans", 14) == 0) {
		if (ctl_dev->trans_thread == NULL){
			ctl_dev->rd_wr_flag = 0;
			ctl_dev->trans_thread = kthread_run(omap_fpga_printer_trans_thread, (void *)ctl_dev, "rd_trans_thread");
			if (IS_ERR(ctl_dev->trans_thread)) {
				dev_err(&pdev->dev, "Failed to create the rd_trans_thread\n");
				ctl_dev->trans_thread = NULL;
			}
		}

	}
	else if(strncmp(buf, "start_wr_trans", 14) == 0) {
		if (ctl_dev->trans_thread == NULL){
			ctl_dev->rd_wr_flag = 1;
			ctl_dev->trans_thread = kthread_run(omap_fpga_printer_trans_thread, (void *)ctl_dev, "wr_trans_thread");
			if (IS_ERR(ctl_dev->trans_thread)) {
				dev_err(&pdev->dev, "Failed to create the wr_trans_thread\n");
				ctl_dev->trans_thread = NULL;
			}
		}

	}
	else if(strncmp(buf, "start_rdwr_trans", 16) == 0) {
		if (ctl_dev->trans_thread == NULL){
			ctl_dev->rd_wr_flag = 3;
			ctl_dev->trans_thread = kthread_run(omap_fpga_printer_trans_thread, (void *)ctl_dev, "rdwr_trans_thread");
			if (IS_ERR(ctl_dev->trans_thread)) {
				dev_err(&pdev->dev, "Failed to create the rdwr_trans_thread\n");
				ctl_dev->trans_thread = NULL;
			}
		}

	}
	else if(strncmp(buf, "start_dmard_trans", 17) == 0) {
		if (ctl_dev->trans_thread == NULL){
			ctl_dev->rd_wr_flag = 5;
			ctl_dev->trans_thread = kthread_run(omap_fpga_printer_trans_thread, (void *)ctl_dev, "dmard_trans_thread");
			if (IS_ERR(ctl_dev->trans_thread)) {
				dev_err(&pdev->dev, "Failed to create the dmard_trans_thread\n");
				ctl_dev->trans_thread = NULL;
			}
		}

	}
	else if(strncmp(buf, "start_dmawr_trans", 17) == 0) {
		if (ctl_dev->trans_thread == NULL){
			ctl_dev->rd_wr_flag = 6;
			ctl_dev->trans_thread = kthread_run(omap_fpga_printer_trans_thread, (void *)ctl_dev, "dmawr_trans_thread");
			if (IS_ERR(ctl_dev->trans_thread)) {
				dev_err(&pdev->dev, "Failed to create the dmawr_trans_thread\n");
				ctl_dev->trans_thread = NULL;
			}
		}

	}
	else if (strncmp(buf, "stop_trans", 10) == 0){
		if (ctl_dev->trans_thread) {
			if(ctl_dev->rd_wr_flag != 5 && ctl_dev->rd_wr_flag != 6){
				kthread_stop(ctl_dev->trans_thread);
			}
			ctl_dev->trans_thread = NULL;
			ctl_dev->rd_wr_flag = -1;
		}
	}

	else if (strncmp(buf, "gpmc,cs-on-ns=", 14) == 0){
		unsigned long secs;

		ret = kstrtoul(buf + 14, 0, &secs);
		if (ret < 0)
			goto out;
	}

	else if (strncmp(buf, "gpmc,cs-rd-off-ns=", 18) == 0){}
	else if (strncmp(buf, "gpmc,cs-wr-off-ns=", 18) == 0){}
	else if (strncmp(buf, "gpmc,adv-on-ns=", 15) == 0){}
	else if (strncmp(buf, "gpmc,adv-rd-off-ns=", 19) == 0) {} 
	else if (strncmp(buf, "gpmc,adv-wr-off-ns=", 19) == 0){}
	else if (strncmp(buf, "gpmc,adv-aad-mux-on-ns=", 23) == 0){}
	else if (strncmp(buf, "gpmc,adv-aad-mux-rd-off-ns=", 27) == 0){}
	else if (strncmp(buf, "gpmc,adv-aad-mux-wr-off-ns=", 27) == 0){}
	else if (strncmp(buf, "gpmc,oe-on-ns=", 14) == 0){}
	else if (strncmp(buf, "gpmc,oe-off-ns=", 15) == 0){}
	else if (strncmp(buf, "gpmc,oe-aad-mux-on-ns=", 22) == 0){}
	else if (strncmp(buf, "gpmc,oe-aad-mux-off-ns=", 23) == 0){}
	else if (strncmp(buf, "gpmc,we-on-ns=", 14) == 0){}
	else if (strncmp(buf, "gpmc,we-off-ns=", 15) == 0){}
	else if (strncmp(buf, "gpmc,rd-cycle-ns=", 17) == 0){}
	else if (strncmp(buf, "gpmc,we-on-ns=", 14) == 0){}
	else if (strncmp(buf, "gpmc,wr-cycle-ns=", 17) == 0){}
	else if (strncmp(buf, "gpmc,access-ns=", 15) == 0){}
	else if (strncmp(buf, "gpmc,page-burst-access-ns=", 26) == 0){}
	else if (strncmp(buf, "gpmc,bus-turnaround-ns=", 23) == 0){}
	else if (strncmp(buf, "gpmc,cycle2cycle-delay-ns=", 26) == 0){}
	else if (strncmp(buf, "gpmc,wait-monitoring-ns=", 24) == 0){}
	else if (strncmp(buf, "gpmc,clk-activation-ns=", 23) == 0){}
	else if (strncmp(buf, "gpmc,wr-data-mux-bus-ns=", 24) == 0){}
	else if (strncmp(buf, "gpmc,wr-access-ns=", 18) == 0){}
	else
		ret = -EINVAL;

out:
	if (ret < 0)
		return ret;

	/* ignore the rest of the buffer, only one command at a time */
	*ppos += size;
	return size;
}


static const struct file_operations omap_fpga_printer_debugfs_fops = {
	.owner		= THIS_MODULE,
	.open		= omap_fpga_printer_debugfs_open,
	.read		= seq_read,
	.write		= omap_fpga_printer_debugfs_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

int omap_fpga_printer_ctl_debugfs_add(struct omap_fpga_printer_ctl_dev *ctl_dev)
{
	struct platform_device *pdev = ctl_dev->pdev;
	ctl_dev->debug_dir = debugfs_create_dir("omap_fpga_printer", NULL);
	if (!ctl_dev->debug_dir) {
		dev_err(&pdev->dev, "Failed to create bank debug dir.\n");
		return -EFAULT;
	}

	ctl_dev->debug_entry = debugfs_create_file("debug", S_IRUSR | S_IWUSR,
						   ctl_dev->debug_dir, ctl_dev,
						   &omap_fpga_printer_debugfs_fops);
	if (!ctl_dev->debug_entry) {
		dev_err(&pdev->dev, "Failed to create bank debug entry.\n");
		debugfs_remove(ctl_dev->debug_dir);
		return -EFAULT;
	}
	return 0;
}

void omap_fpga_printer_ctl_debugfs_rm(struct omap_fpga_printer_ctl_dev *ctl_dev)
{
	debugfs_remove(ctl_dev->debug_entry);
	debugfs_remove(ctl_dev->debug_dir);
}

static int omap_fpga_printer_ctl_probe(struct platform_device *pdev)
{
	int				err;
	struct resource			*res;
	struct device			*dev = &pdev->dev;
	struct omap_fpga_printer_ctl_dev *ctl_dev;
	struct device *drv_device;

	ctl_dev = devm_kzalloc(&pdev->dev, sizeof(struct omap_fpga_printer_ctl_dev),
				GFP_KERNEL);
	if (!ctl_dev)
		return -ENOMEM;

	ctl_dev->user_buff = kzalloc(USER_BUFF_SIZE, GFP_KERNEL);

	if (!ctl_dev->user_buff) {
		dev_err(&pdev->dev, "user_buff alloc failed\n");
		err = -ENOMEM;
		goto err_release_dev;
	}

	ctl_dev->pdev = pdev;

	if (dev->of_node) {
		if (omap_fpga_printer_ctl_get_dt_info(dev, ctl_dev)){
			err = -EFAULT;
			goto err_release_buf;
		}
	} else {
		err = -EFAULT;
		goto err_release_buf;
	}
	sema_init(&ctl_dev->sem, 1);

	if (!ctl_dev->name) {
		ctl_dev->name = devm_kasprintf(&pdev->dev, GFP_KERNEL,
					   "omap2-nand.%d", ctl_dev->gpmc_cs);
		if (!ctl_dev->name) {
			dev_err(&pdev->dev, "Failed to set fpga printer dev name\n");
			err =  -ENOMEM;
			goto err_release_buf;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctl_dev->IO_ADDR_R = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ctl_dev->IO_ADDR_R)){
		err = PTR_ERR(ctl_dev->IO_ADDR_R);
		dev_err(&pdev->dev, "devm_ioremap_resource() failed: %d\n", err);
		goto err_release_buf;
	}

	ctl_dev->phys_base = res->start;
	ctl_dev->IO_ADDR_W = ctl_dev->IO_ADDR_R;
	ctl_dev->dev_id = MKDEV(0, 0);

	err = alloc_chrdev_region(&ctl_dev->dev_id, 0, 1, DEVICE_NAME);
	if (err) {
		dev_err(&pdev->dev, "alloc_chrdev_region() failed: %d\n", err);
		goto err_unmap_res;
	}

	ctl_dev->drv_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(ctl_dev->drv_class)) {
		dev_err(&pdev->dev, "class_create() failed\n");
		goto err_chrdev_unreg;
	}

	cdev_init(&ctl_dev->cdev, &omap_fpga_printer_ctl_fops);
	err = cdev_add(&ctl_dev->cdev, ctl_dev->dev_id, 1);
	if (err) {
		dev_err(&pdev->dev, "cdev_add() failed: %d\n", err);
		goto err_class_destr;
	}

	drv_device = device_create(ctl_dev->drv_class, NULL,
				   MKDEV(MAJOR(ctl_dev->dev_id), 0),
				   NULL, DEVICE_NAME);
	if (IS_ERR(drv_device)) {
		dev_err(&pdev->dev, "failed to create device\n");
		goto err_cdev_del;
	}

	ctl_dev->dma = dma_request_chan(pdev->dev.parent, "fpga_printer");
	if (IS_ERR(ctl_dev->dma)) {
		err = PTR_ERR(ctl_dev->dma);
		dev_err(&pdev->dev, "DMA engine request failed: %x\n", err);
		goto err_device_destroy;
	}
#if 0
	ctl_dev->rx_buf = dma_alloc_coherent(ctl_dev->dma->device->dev, SZ_128K,
				&ctl_dev->rx_addr, GFP_KERNEL);
	if (!ctl_dev->rx_buf) {
		err = -ENOMEM;
		dev_err(&pdev->dev, "dma_alloc_coherent rx failed\n");
		goto err_dma_release;
	}
	ctl_dev->tx_buf = dma_alloc_coherent(ctl_dev->dma->device->dev, SZ_128K,
				&ctl_dev->tx_addr, GFP_KERNEL);
	if (!ctl_dev->tx_buf) {
		err = -ENOMEM;
		dma_free_coherent(ctl_dev->dma->device->dev, SZ_1M,
				  ctl_dev->rx_buf, ctl_dev->rx_addr);
		dev_err(&pdev->dev, "dma_alloc_coherent tx failed\n");
		goto err_dma_release;
	}
#endif
	platform_set_drvdata(pdev, ctl_dev);
	omap_fpga_printer_ctl_debugfs_add(ctl_dev);
	return 0;

//err_dma_release:
//	dma_release_channel(ctl_dev->dma);
err_device_destroy:
	device_destroy(ctl_dev->drv_class, MKDEV(MAJOR(ctl_dev->dev_id), 0));
err_cdev_del:
	cdev_del(&ctl_dev->cdev);
err_class_destr:
	class_destroy(ctl_dev->drv_class);
err_chrdev_unreg:
	unregister_chrdev_region(ctl_dev->dev_id, 1);
err_unmap_res:
	devm_iounmap(&pdev->dev, ctl_dev->IO_ADDR_R);
err_release_buf:
	kfree(ctl_dev->user_buff);
err_release_dev:
	devm_kfree(&pdev->dev, ctl_dev);
	return err;
}

static int omap_fpga_printer_ctl_remove(struct platform_device *pdev)
{
	struct omap_fpga_printer_ctl_dev *ctl_dev = platform_get_drvdata(pdev);

	omap_fpga_printer_ctl_debugfs_rm(ctl_dev);
	dma_release_channel(ctl_dev->dma);
	device_destroy(ctl_dev->drv_class, MKDEV(MAJOR(ctl_dev->dev_id), 0));
	cdev_del(&ctl_dev->cdev);
	class_destroy(ctl_dev->drv_class);
	unregister_chrdev_region(ctl_dev->dev_id, 1);
	devm_iounmap(&pdev->dev, ctl_dev->IO_ADDR_R);
	devm_kfree(&pdev->dev, ctl_dev);
	return 0;
}


static const struct of_device_id omap_fpga_printer_ctl_ids[] = {
	{ .compatible = "zuosi,omap2-fpga-printer-ctl", },
	{},
};

static struct platform_driver omap_fpga_printer_ctl_driver = {
	.probe		= omap_fpga_printer_ctl_probe,
	.remove		= omap_fpga_printer_ctl_remove,
	.driver		= {
		.name	= DRIVER_NAME,
		.of_match_table = of_match_ptr(omap_fpga_printer_ctl_ids),
	},
};

module_platform_driver(omap_fpga_printer_ctl_driver);


MODULE_AUTHOR("yuanbo");
MODULE_DESCRIPTION("fpga driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION("0.1");
