/*************************************************************************
  > File Name: app.c
  > Author: 
  > Mail: 
  > Created Time: 2018年09月13日 星期四 22时38分24秒
 ************************************************************************/

#include<stdio.h>
#include <unistd.h>         
#include <sys/mman.h>       
#include <sys/types.h>      
#include <sys/stat.h>       
#include <fcntl.h>          
#include <linux/videodev2.h>
#include <linux/ioctl.h>    
#include <sys/ioctl.h>      
#include <string.h>         
#include <stdlib.h>         

#define LOG 1

int main()
{
    int fd;
    if((fd = open("/dev/video0", O_RDWR)) < 0){
        perror("open");
        exit(1);
    }

    struct v4l2_capability cap;
    if(ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0){
            perror("VIDIOC_QUERYCAP");
                exit(1);
    }

    if(!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
            fprintf(stderr, "The device does not handle single-planar video capture.\n");
                exit(1);
    }

    struct v4l2_format format;
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_SGRBG10;
    format.fmt.pix.width = 800;
    format.fmt.pix.height = 600;

    if(ioctl(fd, VIDIOC_S_FMT, &format) < 0){
            perror("VIDIOC_S_FMT");
                exit(1);
    }

    struct v4l2_requestbuffers bufrequest;
    bufrequest.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufrequest.memory = V4L2_MEMORY_MMAP;
    bufrequest.count = 1;

    if(ioctl(fd, VIDIOC_REQBUFS, &bufrequest) < 0){
            perror("VIDIOC_REQBUFS");
                exit(1);
    }

    struct v4l2_buffer bufferinfo;
    memset(&bufferinfo, 0, sizeof(bufferinfo));

    bufferinfo.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    bufferinfo.memory = V4L2_MEMORY_MMAP;
    bufferinfo.index = 0;

    if(ioctl(fd, VIDIOC_QUERYBUF, &bufferinfo) < 0){
            perror("VIDIOC_QUERYBUF");
                exit(1);
    }

    void* buffer_start = mmap(
            NULL,
            bufferinfo.length,
            PROT_READ | PROT_WRITE,
            MAP_SHARED,
            fd,
            bufferinfo.m.offset
            );

    if(buffer_start == MAP_FAILED){
            perror("mmap");
                exit(1);
    }
	printf("Frame buffer: address=0x%x, length=%d\n",(unsigned int)buffer_start, bufferinfo.length);

    memset(buffer_start, 0, bufferinfo.length);

    struct v4l2_buffer buffer;
    memset(&bufferinfo, 0, sizeof(buffer));
     
    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buffer.memory = V4L2_MEMORY_MMAP;
    buffer.index = 0; /* Queueing buffer index 0. */
    
    if(ioctl(fd, VIDIOC_QBUF, &buffer) < 0){
            perror("VIDIOC_QBUF");
                exit(1);
    }

    buffer.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int types = buffer.type;
    if(ioctl(fd, VIDIOC_STREAMON, &types) < 0){
            perror("VIDIOC_STREAMON");
                exit(1);
    }
     


    // The buffer's waiting in the outgoing queue.
    if(ioctl(fd, VIDIOC_DQBUF, &buffer) < 0){
        perror("VIDIOC_DQBUF");
        exit(1);
    }
    

    if(ioctl(fd, VIDIOC_STREAMOFF, &types) < 0){
            perror("VIDIOC_STREAMOFF");
                exit(1);
    }
    
   FILE *jpgfile;
   if((jpgfile = fopen("./myimage.jpeg","wb")) < 0){
           perror("open");
               exit(1);
   }

   fwrite( buffer_start,1, 800*600*2,jpgfile);
   fclose(jpgfile);

    
    close(fd);
    return EXIT_SUCCESS;
}
