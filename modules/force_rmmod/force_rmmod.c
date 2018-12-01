/***************************************************
  > Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
  > File Name: force_rmmod.c
  > Author: WANGYONGKUN
  > Mail:932911564@QQ.COM 
  > Created Time: 2018年07月27日 星期五 15时43分19秒
 ***************************************************/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cpumask.h>
#include <linux/list.h>
#include <asm-generic/local.h>
#include <linux/platform_device.h>
#include <linux/kallsyms.h>
#include <linux/sched.h>


static char *modname = NULL;
module_param(modname, charp, 0644);
MODULE_PARM_DESC(modname, "The name of module you want do clean or delete...\n");


#define CONFIG_REPLACE_EXIT_FUNCTION

#ifdef CONFIG_REPLACE_EXIT_FUNCTION

void force_replace_exit_module_function(void)
{
	
	printk("module %s exit SUCCESS...\n", modname);

}
#endif  //  CONFIG_REPLACE_EXIT_FUNCTION


static int force_cleanup_module(char *del_mod_name)
{
	struct module   *mod = NULL, *relate = NULL;
	int              cpu;
#ifdef CONFIG_REPLACE_EXIT_FUNCTION
	void            *origin_exit_addr = NULL;
#endif

#if 0

	struct module *list_mod = NULL;

	list_for_each_entry(list_mod, THIS_MODULE->list.prev, list)
	{
		if (strcmp(list_mod->name, del_mod_name) == 0)
		{
			mod = list_mod;
		}
	}

	if(mod == NULL)
	{
		printk("[%s] module %s not found\n", THIS_MODULE->name, modname);
		return -1;
	}
#endif


	if((mod = find_module(del_mod_name)) == NULL)
	{
		printk("[%s] module %s not found\n", THIS_MODULE->name, del_mod_name);
		return -1;
	}
	else
	{
		printk("[before] name:%s, state:%d, refcnt:%u\n",
				mod->name ,mod->state, module_refcount(mod));
	}


	if (!list_empty(&mod->source_list))
	{

		list_for_each_entry(relate, &mod->source_list, source_list)
		{
			printk("[relate]:%s\n", relate->name);
		}
	}
	else
	{
		printk("No modules depond on %s...\n", del_mod_name);
	}


	mod->state = MODULE_STATE_LIVE;


	for_each_possible_cpu(cpu)
	{
		local_set((local_t*)per_cpu_ptr(&(mod->refcnt), cpu), 0);

	}
	atomic_set(&mod->refcnt, 1);

#ifdef CONFIG_REPLACE_EXIT_FUNCTION

	origin_exit_addr = mod->exit;
	if (origin_exit_addr == NULL)
	{
		printk("module %s don't have exit function...\n", mod->name);
	}
	else
	{
		printk("module %s exit function address %p\n", mod->name, origin_exit_addr);
	}

	mod->exit = force_replace_exit_module_function;
	printk("replace module %s exit function address (%p -=> %p)\n", mod->name, origin_exit_addr, mod->exit);
#endif

	printk("[after] name:%s, state:%d, refcnt:%u\n",
			mod->name, mod->state, module_refcount(mod));

	return 0;
}


static int __init force_rmmod_init(void)
{
	return force_cleanup_module(modname);
}


static void __exit force_rmmod_exit(void)
{
	printk("=======name : %s, state : %d EXIT=======\n", THIS_MODULE->name, THIS_MODULE->state);
}

module_init(force_rmmod_init);
module_exit(force_rmmod_exit);

MODULE_LICENSE("GPL");


