/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: demo_kobj.c
> Author: kun
> Mail:linux4420@163.COM 
> Created Time: 2018年08月09日 星期四 09时09分48秒
***************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kobject.h>

static struct kobject kobj;

//释放kobject内存
static void kobj_release(struct kobject *kobj);
//当读文件时调用该函数
static ssize_t kobj_show(struct kobject *kobj, struct attribute *kobj_attr,char *buf);
//当写文件时调用该函数
static ssize_t kobj_store(struct kobject *kobj,struct attribute *kobj_attr,const char *buf,
	   	size_t cnt);


static struct attribute demo_attr = {
	.name = "null_kun",//创建的属性文件名
	.mode = S_IRWXO,
};

static struct attribute *demo_attrs[] = {
	&demo_attr,
	NULL
};

static const struct sysfs_ops sysfs_ops = {
	.show = kobj_show,
	.store = kobj_store,
};

static struct kobj_type kj_tp = {
	.release	   = kobj_release,
	.default_attrs = demo_attrs,
	.sysfs_ops	   = &sysfs_ops,
} ;

static void kobj_release(struct kobject *kobj)
{
	printk("kobj_release ......\n");
}

static ssize_t kobj_show(struct kobject *kobj, struct attribute *kobj_attr,char *buf)
{
	printk("attrname = %s\n",kobj_attr->name);

	return strlen(kobj_attr->name)+2;
}

static ssize_t kobj_store(struct kobject *kobj,struct attribute *kobj_attr,const char *buf,size_t cnt)
{
	printk("store .... \n");

	return cnt;
}

static int __init kobj_init(void)
{
    kobject_init_and_add(&kobj,&kj_tp,NULL,"kun");
    printk("kun kobject init...\n");

    return 0;
}

module_init(kobj_init);

static void __exit kobj_exit(void)
{
    kobject_del(&kobj);
    printk("kun kobject exit....\n");
}

module_exit(kobj_exit);

MODULE_LICENSE("GPL");


