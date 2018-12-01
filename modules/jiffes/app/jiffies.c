/***************************************************
> Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
> File Name: jiffies.c
> Author: WANGYONGKUN
> Mail:932911564@QQ.COM 
> Created Time: 2018年07月27日 星期五 13时26分13秒
***************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,const char* argv[])
{
	int fd;

	fd = open("/dev/kiss",O_APPEND);
	if(fd < 0) {
		perror("open faild...\n");
		return -1;
	}
	sleep(10);
	close(fd);
    return 0;
}
