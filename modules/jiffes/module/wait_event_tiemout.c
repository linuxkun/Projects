/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: jiffies.c
> Author: WANGYONGKUN
> Mail:932911564@QQ.COM 
> Created Time: 2018年07月27日 星期五 11时25分40秒
***************************************************/

/*
 *	jiffies定时器,定时10秒
 *
 * */


#include <linux/init.h>
#include <linux/module.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>

#define DEVICE_NAME "kiss"

dev_t major = 0;

static wait_queue_head_t work;

int jiff_open (struct inode *inode, struct file *filp)
{
	//jiffies/HZ代表内核启动到现在系统运行的秒数
	unsigned long delay_num = jiffies/HZ + 10;
	
	printk("jiff open ok ....\n");
	
	while(wait_event_timeout(work,0,delay_num)) {
		printk("dida ....\n");
	}

	return 0;
}

int jiff_release (struct inode *inode, struct file *filp)
{
	printk("jiff release....\n");
	return 0;
}


static struct file_operations fops = {
	.owner	= THIS_MODULE,
	.open	= jiff_open,
	.release = jiff_release,
};

static struct class *class;
static struct device *device;

static int __init jiff_init(void)
{
	int ret;
	
	major = register_chrdev(0,DEVICE_NAME,&fops);
	printk("major ==== %d.\n",major);
	if(major < 0) {
		return major;
	}
	

	class = class_create(THIS_MODULE,DEVICE_NAME);
	
	if(IS_ERR(class)) {
		ret = PTR_ERR(class);
		goto error0;
	}
		

	device = device_create(class,NULL,MKDEV(major,0),NULL,DEVICE_NAME);
	if(IS_ERR(device)) {
		ret = PTR_ERR(device);
		goto error1;
	}
	
	printk("init jiffies == %ld  HZ === %d\n....\n",jiffies,HZ);

	return 0;

error1:
	class_destroy(class);
	kfree(device);

error0:
	unregister_chrdev(major,DEVICE_NAME);
	kfree(class);

	return ret;
}

module_init(jiff_init);

static void __exit jiff_exit(void)
{
	unregister_chrdev(major,DEVICE_NAME);
	device_unregister(device);
	device_destroy(class,major);
	class_destroy(class);

	printk("jiff exit....\n");
}

module_exit(jiff_exit);

MODULE_LICENSE("GPL");


