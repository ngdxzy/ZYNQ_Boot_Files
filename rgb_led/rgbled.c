/*
 * DMA.c
 *
 *  Created on: 2018年11月1日
 *      Author: Beats
 */


/*****************************************************
This is a template for char device drivers
Author: Alfred
******************************************************/
#include <linux/bitops.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/irqchip/chained_irq.h>
#include <linux/irqdomain.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/kfifo.h>
#include <linux/clk.h>
#include <linux/device.h>     //class_create
#include<linux/fs.h>
#include<linux/delay.h>
#include<asm/uaccess.h>
#include<asm/irq.h>
#include <linux/delay.h>
#include<linux/poll.h>

/**************************************************************************/

/*	Define Gloable Parameters	*/
#define DEVICE_NAME "rgbled"


static dev_t dev_id;
//static struct cdev drv_cdev;
static struct class *drv_class;



/*	Define functions	*/
static irqreturn_t drv_irq_handle(int irq,void *dev_id);
static int drv_open(struct inode *inode,struct file *filp);
static int drv_read(struct file *filp,char __user *buf, size_t count,loff_t* pos);
static int drv_write(struct file *filp,const char __user *buf, size_t count,loff_t* pos);
static int drv_close(struct inode *inode,struct file *filp);
static int drv_remove(struct platform_device* pdev);
static int drv_probe(struct platform_device* pdev);
static long drv_ioctl(struct file *filp,unsigned int cmd,unsigned long arg);
int rc_device(char* name,struct file_operations* f_ops,int *major,struct class ** class_ptr);
int del_device(int major,struct class * class,char* name);
static int drv_fasync(int fd, struct file *filp, int on);

static unsigned int drv_poll(struct file *file, poll_table *wait);
static int drv_init(void);
static void drv_exit(void);

/*	Define gloable structs	*/
static struct file_operations drv_fops = {
	.owner = THIS_MODULE,
	.open = drv_open,
	.read = drv_read,
	.write = drv_write,
	.release = drv_close,
	.unlocked_ioctl = drv_ioctl,
};

static const struct of_device_id of_match_drv[] = {
	{ .compatible = "rgbled",},
	{},
};

static struct platform_driver drv_drv = {
	.probe = drv_probe,
	.remove = drv_remove,
	.driver = {
		.name = DEVICE_NAME,
		.of_match_table = of_match_drv,
	},
};

/**************************************************************************/
/*	User Definitions	*/
typedef unsigned int u32;
typedef struct COLOR{
  unsigned int blue;
  unsigned int green;
  unsigned int red;
}color;

typedef struct LEDS{
  color led[2];
}rgbleds;
typedef struct MMIO
{
	rgbleds* led_src;
	struct cdev dev;
}Mdev;

/**************************************************************************/
// User Functions
void set_leds(Mdev* mdev,int id,int val)
{
	if(val < 100)//g__
	{
		mdev->led_src->led[id].green = val/3;
		mdev->led_src->led[id].blue = 0;
		mdev->led_src->led[id].red = 0;
	}
	else if(val < 200)//gb_
	{
		mdev->led_src->led[id].green = 99/3;
		mdev->led_src->led[id].blue = (val - 100)/3;
		mdev->led_src->led[id].red = 0;
	}
	else if(val < 300)//_b_
	{
		mdev->led_src->led[id].green = (399 - val)/3;
		mdev->led_src->led[id].blue = 99/3;
		mdev->led_src->led[id].red = 0;
	}
	else if(val < 400)//_br
	{
		mdev->led_src->led[id].green = 0;
		mdev->led_src->led[id].blue = 99/3;
		mdev->led_src->led[id].red = (val - 300)/3;
	}
	else if(val < 500)//__r
	{
		mdev->led_src->led[id].green = 0;
		mdev->led_src->led[id].blue = (499 - val)/3;
		mdev->led_src->led[id].red = 99/3;
	}
	else//rgb
	{
		mdev->led_src->led[id].green = val - 500;
		mdev->led_src->led[id].blue = val - 500;
		mdev->led_src->led[id].red = val - 500;
	}
}
/**************************************************************************/
static irqreturn_t drv_irq_handle(int irq,void *dev_id)
{
	return IRQ_HANDLED;
}

/*	File operations functions	*/
/* Open */

static int drv_open(struct inode *inode,struct file *filp)
{
	static char inited = 0;
	Mdev *mdev = container_of(inode->i_cdev,Mdev,dev);
	filp->private_data = mdev;
	return 0;
}
/* Open End*/

static long drv_ioctl(struct file *filp,unsigned int cmd,unsigned long arg)
{
	return 0;
}

/* Read */
static int drv_read(struct file *filp,char __user *buf, size_t count,loff_t* pos)
{
	int val;

	return 0;
}
/* Read End */

/* Write */
static int drv_write(struct file *filp,const char __user *buf, size_t count,loff_t* pos)
{
	int val[2];
	Mdev* mdev = filp->private_data;
	if(count != 8)
		return -1;
	copy_from_user((char*)val,buf,count);//从用户空间拷贝到内核
	set_leds(mdev,0,val[0]);
	set_leds(mdev,1,val[1]);
	return 0;
}
/* Write End */

/* Close(Release) */
static int drv_close(struct inode *inode,struct file *filp)
{
	int i;
	Mdev *mdev = container_of(inode->i_cdev,Mdev,dev);
	//printk(KERN_ALERT "(Kernal)SSM2603 Closed\n");
	return 0;
}
/* Close(Release) End */


static int drv_remove(struct platform_device* pdev)
{
	Mdev *mdev = platform_get_drvdata(pdev);
	iounmap(mdev->led_src);
	unregister_chrdev(MKDEV(dev_id,0),DEVICE_NAME);
	device_destroy(drv_class,MKDEV(dev_id,0));
	kzfree(mdev);
	return 0;
}



static int drv_probe(struct platform_device* pdev)
{
	static int called = -1;
	unsigned int temp;
	Mdev* mdev;
	//Allocate
	mdev = kzalloc(sizeof(*mdev),GFP_KERNEL);
	if(mdev == NULL)
	{
		//printk(KERN_ALERT "Unable to allocate memory!\n");
		return -1;
	}
	//Set all parameters
	//
	of_property_read_u32(pdev->dev.of_node,"reg",&temp);
	//Remap the struct to the Phyaddr
	mdev->led_src = ioremap(temp,0x1000);

	//printk(KERN_ALERT "(Kernal)ssm2603 base %p\n",temp);

	alloc_chrdev_region(&dev_id,0,1,DEVICE_NAME);
	drv_class = class_create(THIS_MODULE,DEVICE_NAME);

	cdev_init(&mdev->dev, &drv_fops);
	cdev_add(&mdev->dev,MKDEV(MAJOR(dev_id),0),1);

	//automatic create device nodes
	device_create(drv_class,NULL,MKDEV(MAJOR(dev_id),0),NULL,DEVICE_NAME);
	//InitXadc(mdev);
	//Save all the informations
	platform_set_drvdata(pdev, mdev);

    
	//printk(KERN_ALERT "(Kernal)ssm2603 probed!Test id = %p!\n",mdev->ssm2603_inst->test_id);
	return 0;
}


/***********************************************************************
 Functions and variables behind are fixed and should not be changed
if not necessary
***********************************************************************/

int del_device(int major,struct class * class,char* name)
{
	unregister_chrdev(major,name);
	device_destroy(class,MKDEV(major,0));
	class_destroy(class);
	return 0;
}



static int drv_init(void)
{
	platform_driver_register(&drv_drv);
	return 0;
}
static void drv_exit(void)
{
	platform_driver_unregister(&drv_drv);
	return;
}

module_init(drv_init);
module_exit(drv_exit);
MODULE_AUTHOR("Xilinx, Inc.");
MODULE_DESCRIPTION("Alfred DMA driver");
MODULE_LICENSE("GPL");



