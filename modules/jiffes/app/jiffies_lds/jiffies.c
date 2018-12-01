/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: jiffes.c
> Author: WANGYONGKUN
> Mail:932911564@QQ.COM 
> Created Time: 2018年07月27日 星期五 11时10分51秒
***************************************************/

#include <stdio.h>

unsigned long long jiffies_64 = 0x112345678;
unsigned long jiffies;

int main(int argc,const char* argv[])
{
	printf("jiffes_64 = 0x%llx\n",jiffies_64);
	printf("jiffes_64 = %p\n",&jiffies_64);
	printf("jiffes = 0x%lx\n",jiffies);
	printf("jiffes = %p\n",&jiffies);
    return 0;
}
