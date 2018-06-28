/*
 * Char device driver fpga.
 * Do a global replace of 'fpga' with your driver name.
 */

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

#include <asm/io.h>
#include <asm/irq.h>
#include <asm/uaccess.h>

#define	DRIVER_NAME	"omap2-fpga-printer-ctl"
#define	DEVICE_NAME	"fpga_printer_ctl"

#define USER_BUFF_SIZE 128

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
	void __iomem *IO_ADDR_R;
	void __iomem *IO_ADDR_W;
};

static ssize_t omap_fpga_prienter_ctl_write(struct file *filp, const char __user *buff,
size_t count, loff_t *f_pos)
{
	ssize_t write_num = 0;
	size_t once_write_num = USER_BUFF_SIZE;
	int i, tmp = 0;
	//int test_mode = 0;  //by mtj 20140908
	int offset = 0;         //by mtj 20140908
	struct omap_fpga_printer_ctl_dev *ctl_dev = filp->private_data;
	struct platform_device *pdev = ctl_dev->pdev;

	if (count == 0)
		return 0;

	if (down_interruptible(&ctl_dev->sem))
		return -ERESTARTSYS;


	memset(ctl_dev->user_buff, 0, USER_BUFF_SIZE);//unnecessary


	offset = buff[0] | buff[1] << 8;
	buff    +=  2;
	count   -=  2;

	while(write_num < count)
	{
		
		if (once_write_num > (count - write_num))
			once_write_num = count - write_num;
		
		if (copy_from_user(ctl_dev->user_buff, buff, once_write_num))
		{
			dev_err(&pdev->dev, "omap_fpga_prienter_ctl_write: copy_from_user failed\n");
			write_num = -EFAULT;
			goto fpga_write_done;
		}
		buff += once_write_num;
		write_num += once_write_num;  //pang1567 add return the write data size
		for (i = 0; i < once_write_num; i += 2)
		{
			tmp = ctl_dev->user_buff[i] | ctl_dev->user_buff[i+1] << 8;  //mtj
			writew(tmp,ctl_dev->IO_ADDR_W + offset);   //by mtj 20140908
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
	size_t offset;
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

	if (down_interruptible(&ctl_dev->sem))
		return -ERESTARTSYS;

	offset = buff[0] | buff[1] << 8;

	while(read_num < count)
	{
		if (once_read_num > (count - read_num))
			once_read_num = count - read_num;

		for(i = 0;i < once_read_num;i += 2)
		{
			//gDmaCh
			tmp = readw(ctl_dev->IO_ADDR_R + offset);
			ctl_dev->user_buff[i] = tmp;
			ctl_dev->user_buff[i+1] = tmp>>8;
		}
		if (copy_to_user(buff, ctl_dev->user_buff, once_read_num)) {
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
	int status = 0;
	struct omap_fpga_printer_ctl_dev *ctl_dev;
	struct platform_device *pdev;

	ctl_dev = container_of(inode->i_cdev, struct omap_fpga_printer_ctl_dev, cdev);

	if (down_interruptible(&ctl_dev->sem))
		return -ERESTARTSYS;

	if (ctl_dev != NULL) {
		filp->private_data = ctl_dev;
		pdev = ctl_dev->pdev;
		if (!ctl_dev->user_buff) {
			ctl_dev->user_buff = kmalloc(USER_BUFF_SIZE, GFP_KERNEL);

			if (!ctl_dev->user_buff) {
				dev_err(&pdev->dev, "fpga_open: user_buff alloc failed\n");
				status = -ENOMEM;
			}
		}
	} else {
		printk(KERN_ALERT "fpga_open: ctl_dev is null\n");
	}

	up(&ctl_dev->sem);
	return status;
}

static const struct file_operations omap_fpga_printer_ctl_fops = {
	.owner = THIS_MODULE,
	.open = omap_fpga_printer_ctl_open,
	.read = omap_fpga_printer_ctl_read,
	.write = omap_fpga_prienter_ctl_write,
};

static int omap_fpga_printer_ctl_get_dt_info(struct device *dev, struct omap_fpga_printer_ctl_dev *ctl_dev)
{
	return 0;
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

	ctl_dev->pdev = pdev;

	if (dev->of_node) {
		if (omap_fpga_printer_ctl_get_dt_info(dev, ctl_dev))
			return -EINVAL;
	} else {
		return -EINVAL;
	}
	sema_init(&ctl_dev->sem, 1);

	if (!ctl_dev->name) {
		ctl_dev->name = devm_kasprintf(&pdev->dev, GFP_KERNEL,
					   "omap2-nand.%d", ctl_dev->gpmc_cs);
		if (!ctl_dev->name) {
			dev_err(&pdev->dev, "Failed to set fpga printer dev name\n");
			return -ENOMEM;
		}
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	ctl_dev->IO_ADDR_R = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(ctl_dev->IO_ADDR_R))
		return PTR_ERR(ctl_dev->IO_ADDR_R);

	ctl_dev->phys_base = res->start;

	//nand_chip->controller = &omap_gpmc_controller;

	ctl_dev->IO_ADDR_W = ctl_dev->IO_ADDR_R;
	//nand_chip->cmd_ctrl  = omap_hwcontrol;

	ctl_dev->dev_id = MKDEV(0, 0);

	err = alloc_chrdev_region(&ctl_dev->dev_id, 0, 1, DEVICE_NAME);
	if (err) {
		dev_err(&pdev->dev, "alloc_chrdev_region() failed: %d\n", err);
		return -EFAULT;
	}

	ctl_dev->drv_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(ctl_dev->drv_class)) {
		dev_err(&pdev->dev, "class_create(fpga) failed\n");
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
	platform_set_drvdata(pdev, ctl_dev);
	return 0;

err_cdev_del:
	cdev_del(&ctl_dev->cdev);
err_class_destr:
	class_destroy(ctl_dev->drv_class);
err_chrdev_unreg:
	unregister_chrdev_region(ctl_dev->dev_id, 1);
	return -EFAULT;
}

static int omap_fpga_printer_ctl_remove(struct platform_device *pdev)
{
	struct omap_fpga_printer_ctl_dev *ctl_dev = platform_get_drvdata(pdev);

	device_destroy(ctl_dev->drv_class, MKDEV(MAJOR(ctl_dev->dev_id), 0));
	cdev_del(&ctl_dev->cdev);
	class_destroy(ctl_dev->drv_class);
	unregister_chrdev_region(ctl_dev->dev_id, 1);

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
