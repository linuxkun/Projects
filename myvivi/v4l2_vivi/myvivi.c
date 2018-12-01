/***************************************************
  > Copyright (C) 2018 ==WANGYONGKUN== All rights reserved.
  > File Name: myvivi.c
  > Author: kun
  > Mail:linux4420@163.COM 
  > Created Time: 2018年08月03日 星期五 19时44分30秒
 ***************************************************/

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/usb.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <media/videobuf-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-core.h>

#define DRIVER_NAME "myvivi_video"

struct myuvc_buffer {
	struct v4l2_buffer buf;
	int state;
	int vma_use_count; 
	wait_queue_head_t wait; 
	struct list_head stream;
	struct list_head irq;
};

struct myuvc_queue {
	void *mem;
	int count;
	int buf_size;
	struct myuvc_buffer buffer[32];
	struct list_head mainqueue;   
	struct list_head irqqueue;    
};

static struct video_device *myvivi_device;
static struct v4l2_device	myvivi_v4l2_dev;
static struct v4l2_format myuvc_format;
static struct myuvc_queue myuvc_queue;

extern int vb2_reqbufs(struct vb2_queue *q, struct v4l2_requestbuffers *req);
extern  int vb2_streamon(struct vb2_queue *q, enum v4l2_buf_type type);
extern	int vb2_streamoff(struct vb2_queue *q, enum v4l2_buf_type type);
extern int vb2_querybuf(struct vb2_queue *q, struct v4l2_buffer *b);
extern int vb2_qbuf(struct vb2_queue *q, struct v4l2_buffer *b);
extern int vb2_dqbuf(struct vb2_queue *q, struct v4l2_buffer *b, bool nonblocking);


static void myvivi_release(struct video_device *vdev)
{

}


static int myvivi_vidioc_querycap(struct file *file, void  *priv,struct v4l2_capability *cap)
{

	sprintf((char *)cap->bus_info, "PCI:%s", DRIVER_NAME);
	strcpy(cap->driver, "myvivi");
	strcpy(cap->card, "myvivi");
	cap->capabilities = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_STREAMING | V4L2_CAP_DEVICE_CAPS;   /*表明是个视频捕捉设备和通过ioctl来访问*/
	cap->device_caps =
		V4L2_CAP_VIDEO_CAPTURE |
		V4L2_CAP_STREAMING;

	return 0;
}


static struct v4l2_format myvivi_format;
/* 列举支持哪种格式 */
static int myvivi_vidioc_enum_fmt_vid_cap(struct file *file, void  *priv,
		struct v4l2_fmtdesc *f){

	if (f->index >= 1) /*我们这里设置为只支持一种格式，所以设置f->index如果大于等于1,就返回错误*/
		return -EINVAL;

	strcpy(f->description, "4:2:2, packed, YUYV");
	f->pixelformat = V4L2_PIX_FMT_YUYV;
	return 0;
}

/* 返回当前所使用的格式 */
static int myvivi_vidioc_g_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	memcpy(f, &myvivi_format, sizeof(myvivi_format));
	return (0);
}

/* 测试驱动程序是否支持某种格式 */
static int myvivi_vidioc_try_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	unsigned int maxw, maxh;

	if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV)
		return -EINVAL;

	/*该设备支持的最大宽度和高度*/
	maxw  = 1024;
	maxh  = 768;

	/* 调整format的最大最小width, height,和对齐方式
	 *	 * 计算bytesperline, sizeimage
	 *		 */
	v4l_bound_align_image(&f->fmt.pix.width, 48, maxw, 2,
			&f->fmt.pix.height, 32, maxh, 0, 0);

	f->fmt.pix.bytesperline =
		(f->fmt.pix.width * 16) >> 3;  /*设置颜色深度为16*/
	f->fmt.pix.sizeimage =
		f->fmt.pix.height * f->fmt.pix.bytesperline; /*设置图像size*/

	return 0;
}

static int myvivi_vidioc_s_fmt_vid_cap(struct file *file, void *priv,
		struct v4l2_format *f)
{
	int ret = myvivi_vidioc_try_fmt_vid_cap(file, NULL, f);
	if (ret < 0)
		return ret;

	memcpy(&myvivi_format, f, sizeof(myvivi_format));

	return ret;
}

static int myvivi_vidioc_reqbufs(struct file *file, void *priv,
		struct v4l2_requestbuffers *p)
{
	unsigned int buffers = p->count;
	int bufsize = PAGE_ALIGN(myuvc_format.fmt.pix.sizeimage);
	void *mem = NULL;	
	int i,ret;

	for(;buffers > 0 ;buffers--) {
		mem = kzalloc(buffers * bufsize,GFP_KERNEL);
		if (!mem) {
			printk("memroy alloc for buff struct faild..\n");
		}
	}

	memset(&myuvc_queue, 0, sizeof(myuvc_queue));

	INIT_LIST_HEAD(&myuvc_queue.mainqueue);
	INIT_LIST_HEAD(&myuvc_queue.irqqueue);

	for (i = 0; i < buffers; ++i) {
		myuvc_queue.buffer[i].buf.index = i;
		myuvc_queue.buffer[i].buf.m.offset = i * bufsize;
		myuvc_queue.buffer[i].buf.length = myuvc_format.fmt.pix.sizeimage;
		myuvc_queue.buffer[i].buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		myuvc_queue.buffer[i].buf.sequence = 0;
		myuvc_queue.buffer[i].buf.field = V4L2_FIELD_NONE;
		myuvc_queue.buffer[i].buf.memory = V4L2_MEMORY_MMAP;
		myuvc_queue.buffer[i].buf.flags = 0;
		myuvc_queue.buffer[i].state     = VIDEOBUF_IDLE;
		init_waitqueue_head(&myuvc_queue.buffer[i].wait);
	}

	myuvc_queue.mem = mem;
	myuvc_queue.count = buffers;
	myuvc_queue.buf_size = bufsize;
	ret = buffers;

	return ret;
}

static int myvivi_vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *v4l2_buf)
{   

	
	int ret = 0;
	
	if (v4l2_buf->index >= myuvc_queue.count) {
		ret = -EINVAL;
		printk("querybuf error...\n");
		goto done;
	}
	

	memcpy(v4l2_buf, &myuvc_queue.buffer[v4l2_buf->index].buf, sizeof(*v4l2_buf));

	if (myuvc_queue.buffer[v4l2_buf->index].vma_use_count)
		v4l2_buf->flags |= V4L2_BUF_FLAG_MAPPED;


	switch (myuvc_queue.buffer[v4l2_buf->index].state) {
		case VIDEOBUF_ERROR:
		case VIDEOBUF_DONE: 
			v4l2_buf->flags |= V4L2_BUF_FLAG_DONE;
			break;
		case VIDEOBUF_QUEUED:
		case VIDEOBUF_ACTIVE:
			v4l2_buf->flags |= V4L2_BUF_FLAG_QUEUED;
			break;
		case VIDEOBUF_IDLE:
		default:
			break;
	}
	return ret;
	
	return 0;
}

static int myvivi_vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *v4l2_buf)
{
	struct myuvc_buffer *buf = &myuvc_queue.buffer[v4l2_buf->index];
	buf->state = VIDEOBUF_QUEUED;
	v4l2_buf->bytesused = 0;

	list_add_tail(&buf->stream, &myuvc_queue.mainqueue);

//	list_add_tail(&buf->queue, &myuvc_queue.irqqueue);

	return 0;
}


static int myvivi_vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *v4l2_buf)
{

	struct myuvc_buffer *buf = &myuvc_queue.buffer[v4l2_buf->index];

	list_del(&buf->stream);

	return 0;
}   

static int myvivi_vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i
		)
{
	return 0;
}

static int myvivi_vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type 
		i)
{
	return 0;
}

int vivi_mmap (struct file *filp, struct vm_area_struct *vma)
{
	unsigned long page;
	unsigned char i;
	unsigned long start = (unsigned long)vma->vm_start;
	unsigned long end =  (unsigned long)vma->vm_end;
	unsigned long size = (unsigned long)(vma->vm_end - vma->vm_start);

	//得到物理地址
	page = virt_to_phys(buffer);    
	//将用户空间的一个vma虚拟内存区映射到以page开始的一段连续物理页面上
	if(remap_pfn_range(vma,start,page>>PAGE_SHIFT,size,PAGE_SHARED))//第三个参数是页帧号，由物理地址右移PAGE_SHIFT得到
		return -1;
				   
	return 0;
}



static const struct v4l2_ioctl_ops myvivi_ioctl_ops = {
	//表示它是一个摄像头设备
	.vidioc_querycap      = myvivi_vidioc_querycap,

	/* 用于列举、获得、测试、设置摄像头的数据的格式 */
	.vidioc_enum_fmt_vid_cap  = myvivi_vidioc_enum_fmt_vid_cap,
	.vidioc_g_fmt_vid_cap     = myvivi_vidioc_g_fmt_vid_cap,
	.vidioc_try_fmt_vid_cap   = myvivi_vidioc_try_fmt_vid_cap,
	.vidioc_s_fmt_vid_cap     = myvivi_vidioc_s_fmt_vid_cap,
	.vidioc_reqbufs			  = myvivi_vidioc_reqbufs,
	.vidioc_streamon		  = myvivi_vidioc_streamon,
	.vidioc_streamoff		  = myvivi_vidioc_streamoff,
	.vidioc_querybuf		  = myvivi_vidioc_querybuf,
	.vidioc_qbuf			  = myvivi_vidioc_qbuf,
	.vidioc_dqbuf			  = myvivi_vidioc_dqbuf,

};


static const struct v4l2_file_operations myvivi_fops = {                                                                             
	.owner      = THIS_MODULE,
	.unlocked_ioctl      = video_ioctl2, /* V4L2 ioctl handler */
	.mmap       = vivi_mmap,
};

static int myvivi_init(void)
{
	int error;

	/* 1. 分配一个video_device结构体 */
	myvivi_device = video_device_alloc();

	/* 2. 设置 */
	strlcpy(myvivi_v4l2_dev.name, DRIVER_NAME, sizeof(myvivi_v4l2_dev.name));
	error = v4l2_device_register(NULL, &myvivi_v4l2_dev);
	myvivi_device->v4l2_dev = &myvivi_v4l2_dev;
	myvivi_device->release = myvivi_release;
	myvivi_device->fops    = &myvivi_fops;
	myvivi_device->ioctl_ops   = &myvivi_ioctl_ops;
	/* 2.1 */

	error = video_register_device(myvivi_device, VFL_TYPE_GRABBER, -1);

	return error;
}



static void myvivi_exit(void)
{
	video_unregister_device(myvivi_device);
	video_device_release(myvivi_device);
}

module_init(myvivi_init);
module_exit(myvivi_exit);

MODULE_LICENSE("GPL");
