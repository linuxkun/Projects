/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: demo.c
> Author: WANGYONGKUN
> Mail:932911564@QQ.COM 
> Created Time: 2018年07月25日 星期三 09时05分02秒
***************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/device.h>

static void pltdev_release(struct device *pdev)
{
	printk("device is release....\n");
}

static struct platform_device pltdev = {
	.name = "kun",
	.id   = 0,
	.dev  = {
		.release = pltdev_release,
	},
};


static int __init demo_init(void)
{
	return platform_device_register(&pltdev);
}

module_init(demo_init);

static void __exit demo_exit(void)
{
	platform_device_unregister(&pltdev);
}

module_exit(demo_exit);


//module_driver(pltdev,platform_device_register,platform_device_unregister);

MODULE_LICENSE("GPL");
