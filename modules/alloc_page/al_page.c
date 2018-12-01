/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: al_page.c
> Author: WANGYONGKUN
> Mail:932911564@QQ.COM 
> Created Time: 2018年07月31日 星期二 14时45分02秒
***************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/gfp.h>

//定义struct page类型变量
static struct page *all_page;

static struct page_num {
	int a;
	char b;
}temp = {
	1,2
};

static int __init al_page_init(void)
{
	//申请内存
	all_page = alloc_pages(GFP_KERNEL,sizeof(struct page_num));
	if(!all_page) {
		return -ENOMEM;
	}
	printk("the page alloced ok.....a = %d  b = %d\n",temp.a,temp.b);
	printk("page == %lx\n",(unsigned long)all_page);

	return 0;
}

module_init(al_page_init);

static void __exit al_page_exit(void)
{
	if(all_page) {
		//释放内存
		__free_pages(all_page,sizeof(struct page_num));
	}
	printk("free pages ok.....\n");
}

module_exit(al_page_exit);

MODULE_LICENSE("GPL");

