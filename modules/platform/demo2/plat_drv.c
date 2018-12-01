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

static int pltdrv_probe(struct platform_device *pltdev)
{
	printk("match is ok...\n");

	return 0;
}

static int pltdrv_remove(struct platform_device *pltdev)
{
	printk("driver remove ok.....\n");

	return 0;
}

static struct platform_device_id pltdrv_ids[] = {
	{"aaaa"},
	{"yong"},
	{"apple"},
	{"kun"},
	{},
};


static struct platform_driver pltdrv = {
	.probe	=	pltdrv_probe,
	.remove	=	pltdrv_remove,
	.driver	=	{
		.name = "wang"	
	},
	.id_table = pltdrv_ids,
};

static int __init demo_init(void)
{
	return platform_driver_register(&pltdrv);
}

module_init(demo_init);

static void __exit demo_exit(void)
{
	platform_driver_unregister(&pltdrv);
}

module_exit(demo_exit);

MODULE_LICENSE("GPL");



