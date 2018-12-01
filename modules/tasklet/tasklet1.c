/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: tasklet.c
> Author: WANGYONGKUN
> Mail:932911564@QQ.COM 
> Created Time: 2018年07月26日 星期四 09时06分49秒
***************************************************/
/*
 *	tasklet机制实现中断
 * */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/pwm.h>
#include <linux/slab.h>
#include <linux/miscdevice.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/gpio.h>
#include <linux/timer.h>  /*timer*/
#include <asm/uaccess.h>  /*jiffies*/
#include <linux/delay.h>
#include <linux/interrupt.h>

//定义一个tasklet结构体
static struct tasklet_struct task;

//tasklet处理函数
static void task_func(unsigned long data)
{
	//判断此刻是位于进程上下文还是中断上下文
	if(in_interrupt()) {
		printk("%s in interrupt handle...\n",__FUNCTION__);
	}
}


static  irqreturn_t irq_func(int irq, void *dev_id)
{
	//将tasklet让内核调度
	tasklet_schedule(&task);

	if(in_interrupt()) {
		printk("%s is interrupt handle...\n",__FUNCTION__);
	}
	
	return IRQ_HANDLED;
}

static int __init task_init(void)
{
	int data = 100;
	int ret;
	int irq_num;

	//初始化tasklet  DECLARE_TASKLET(task,task_func,0);宏初始化tasklet，不用定义结构体
	tasklet_init(&task,task_func,(unsigned long)data);
	
	//申请中断号
	irq_num = gpio_to_irq(EXYNOS4_GPX3(2));//EXYNOS4_GPX3(2);
		
	//请求中断
	ret = request_irq(irq_num,irq_func,IRQF_SHARED,"tiny4412_key",(void *)"key1");
	
	if(ret != 0) {
		free_irq(irq_num,(void *)"key1");
		return -1;
	}
	
	return 0;
}

module_init(task_init);

static void __exit tasklet_exit(void)
{
	int irq_num;
	irq_num = gpio_to_irq(EXYNOS4_GPX3(2));
	free_irq(irq_num,(void *)"key1");
	printk("key is exit......\n");
}

module_exit(tasklet_exit);

MODULE_LICENSE("GPL");
