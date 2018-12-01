#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include "jyuv.h"
#include "CameralOpt.h"
#include "FrameBufferOpt.h"
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/videodev2.h>
#include <linux/ioctl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>



#define    WIDTH   640
#define    HIGHT   480

#define BMP "./demo.bmp"
extern struct v4l2_buffer dequeue;

int main(void)
{
	char yuyv[WIDTH*HIGHT*2];
	char bmp[WIDTH*HIGHT*3];
//	set_bmp_header((struct bmp_header_t *)bmp, WIDTH, HIGHT);
	//初始化摄像头
	Init_Cameral(WIDTH , HIGHT );
	//初始化framebuffer
//	Init_FrameBuffer(WIDTH , HIGHT ); 
	//开启摄像头
	Start_Cameral();
	//采集一张图片
	int count = 0 ; 
	while(1)
	{
		Get_Picture(yuyv);
		yuyv2rgb24(yuyv, bmp, WIDTH, HIGHT);
		break;
//		Write_FrameBuffer(bmp);
//		printf("count:%d \n" , count++);
	}
	//关闭摄像头
	Stop_Cameral();

	FILE *jpgfile;
	if((jpgfile = fopen(BMP,"wb")) < 0){   
		perror("open");
		exit(1);
	}
	fwrite(bmp,1, WIDTH*HIGHT*2,jpgfile);
	fclose(jpgfile);

	//关闭Framebuffer
//	Exit_Framebuffer();
	//退出
	Exit_Cameral();
	
	return 0;
}
