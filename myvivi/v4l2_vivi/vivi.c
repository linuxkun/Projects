#include <linux/module.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/font.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#include <media/videobuf2-vmalloc.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-fh.h>
#include <media/v4l2-event.h>
#include <media/v4l2-common.h>

#define VFL_TYPE_GRABBER    0

#define MAX_WIDTH 1920
#define MAX_HEIGHT 1200
static unsigned int vid_limit = 16;

static struct video_device *video_dev;      // video_device 结构，用来描述一个 video 设备
static struct vb2_queue vivi_queue;         // vivi_queue 用来存放缓冲区信息，缓冲区链表等
struct task_struct         *kthread;        // 内核线程，用来向缓冲区中填充数据
DECLARE_WAIT_QUEUE_HEAD(wait_queue_head);   // 等待队列头
struct list_head        my_list;            // 链表头

// 用来存放应用程序设置的视频格式
static struct mformat {
    __u32           width;
    __u32           height;
    __u32           pixelsize;
    __u32           field;
    __u32           fourcc;
    __u32           depth;
}mformat;

static void mvideo_device_release(struct video_device *vdev)
{

}

static long mvideo_ioctl(struct file *file, unsigned int cmd, void *arg)
{
    int ret = 0;
    struct v4l2_fh *fh = NULL;
    switch (cmd) {

        case VIDIOC_QUERYCAP:
        {
            struct v4l2_capability *cap = (struct v4l2_capability *)arg;
            cap->version = LINUX_VERSION_CODE;
            ret = video_dev->ioctl_ops->vidioc_querycap(file, NULL, cap);
            break;
        }
        case VIDIOC_ENUM_FMT:
        {
            struct v4l2_fmtdesc *f = arg;
            if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                ret = video_dev->ioctl_ops->vidioc_enum_fmt_vid_cap(file, fh, f);
            }else{
                printk("V4L2_BUF_TYPE_VIDEO_CAPTURE error\n");
            }
            break;
        }
        case VIDIOC_G_FMT:
        {
            struct v4l2_format *f = (struct v4l2_format *)arg;
            if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                ret = video_dev->ioctl_ops->vidioc_g_fmt_vid_cap(file, fh, f);
            }else{
                printk("V4L2_BUF_TYPE_VIDEO_CAPTURE error\n");
            }
            break;
        }
        case VIDIOC_TRY_FMT:
        {
            struct v4l2_format *f = (struct v4l2_format *)arg;
            if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                ret = video_dev->ioctl_ops->vidioc_try_fmt_vid_cap(file, fh, f);
            }else{
                printk("V4L2_BUF_TYPE_VIDEO_CAPTURE error\n");
            }
            break;
        }
        case VIDIOC_S_FMT:
        {
            struct v4l2_format *f = (struct v4l2_format *)arg;
            if (f->type == V4L2_BUF_TYPE_VIDEO_CAPTURE) {
                video_dev->ioctl_ops->vidioc_s_fmt_vid_cap(file, fh, f);
            }else{
                printk("V4L2_BUF_TYPE_VIDEO_CAPTURE error\n");
            }
            break;
        }
        case VIDIOC_REQBUFS:
        {
            struct v4l2_requestbuffers *p = arg;
            ret = video_dev->ioctl_ops->vidioc_reqbufs(file, fh, p);
            break;
        }
        case VIDIOC_QUERYBUF:
        {   
            struct v4l2_buffer *p = arg;
            ret = video_dev->ioctl_ops->vidioc_querybuf(file, fh, p);
            break;
        }
        case VIDIOC_QBUF:
        {
            struct v4l2_buffer *p = arg;
            ret = video_dev->ioctl_ops->vidioc_qbuf(file, fh, p);
            break;
        }
        case VIDIOC_DQBUF:
        {
            struct v4l2_buffer *p = arg;
            ret = video_dev->ioctl_ops->vidioc_dqbuf(file, fh, p);
            break;
        }
        case VIDIOC_STREAMON:
        {
            enum v4l2_buf_type i = *(int *)arg;
            ret = video_dev->ioctl_ops->vidioc_streamon(file, fh, i);
            break;
        }
        case VIDIOC_STREAMOFF:
        {
            enum v4l2_buf_type i = *(int *)arg;
            ret = video_dev->ioctl_ops->vidioc_streamoff(file, fh, i);
            break;
        }
    }
    return ret;
}

static int vivi_mmap(struct file *file, struct vm_area_struct *vma)
{   
    int ret;
    printk("enter mmap\n");
    ret = vb2_mmap(&vivi_queue, vma);
    if(ret == 0){
        printk("mmap ok\n");    
    }else{
        printk("mmap error\n"); 
    }
    return ret;
}


// 查询设备能力
static int mvidioc_querycap(struct file *file, void  *priv, struct v4l2_capability *cap)
{
    strcpy(cap->driver, "vivi");
    strcpy(cap->card, "vivi");
    strcpy(cap->bus_info, "mvivi");
    cap->device_caps = V4L2_CAP_VIDEO_CAPTURE;
    cap->capabilities = cap->device_caps | V4L2_CAP_DEVICE_CAPS | V4L2_CAP_STREAMING;
    printk("mvidioc_querycap  \n");
    return 0;
}

// 枚举视频支持的格式
static int vidioc_enum_fmt_vid_cap(struct file *file, void  *priv, struct v4l2_fmtdesc *f)
{
    printk("vidioc_enum_fmt_vid_cap  \n");
    if (f->index >= 1)
        return -EINVAL;

    strcpy(f->description, "mvivi");
    f->pixelformat = mformat.fourcc;
    printk("vidioc_enum_fmt_vid_cap  \n");
    return 0;
}

// 修正应用层传入的视频格式
static int vidioc_try_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
    printk("vidioc_try_fmt_vid_cap\n");
    if (f->fmt.pix.pixelformat != V4L2_PIX_FMT_YUYV) {
        return -EINVAL;
    }

    f->fmt.pix.field = V4L2_FIELD_INTERLACED;
    v4l_bound_align_image(&f->fmt.pix.width, 48, MAX_WIDTH, 2,
                  &f->fmt.pix.height, 32, MAX_HEIGHT, 0, 0);
    f->fmt.pix.bytesperline = (f->fmt.pix.width * mformat.depth) / 8;
    f->fmt.pix.sizeimage    = f->fmt.pix.height * f->fmt.pix.bytesperline;
    if (mformat.fourcc == V4L2_PIX_FMT_YUYV)
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
    else
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    return 0;
}

// 获取支持的格式
static int vidioc_g_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
    // 将参数写回用户空间
    f->fmt.pix.width        = mformat.width;
    f->fmt.pix.height       = mformat.height;
    f->fmt.pix.field        = mformat.field;
    f->fmt.pix.pixelformat  = mformat.fourcc;
    f->fmt.pix.bytesperline = (f->fmt.pix.width * mformat.depth) / 8;
    f->fmt.pix.sizeimage    = f->fmt.pix.height * f->fmt.pix.bytesperline;
    if (mformat.fourcc == V4L2_PIX_FMT_YUYV)
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SMPTE170M;
    else
        f->fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    printk("vidioc_g_fmt_vid_cap  \n");
    return 0;
}

// 设置视频格式
static int vidioc_s_fmt_vid_cap(struct file *file, void *priv, struct v4l2_format *f)
{
    int ret = vidioc_try_fmt_vid_cap(file, priv, f);
    if (ret < 0)
        return ret;
        // 存储用户空间传入的参数设置
    mformat.fourcc      =  V4L2_PIX_FMT_YUYV;
    mformat.pixelsize   =  mformat.depth / 8;
    mformat.width       =  f->fmt.pix.width;
    mformat.height      =  f->fmt.pix.height;
    mformat.field       =  f->fmt.pix.field;
    printk("vidioc_s_fmt_vid_capp  \n");
    return 0;
}




// vb2 核心层 vb2_reqbufs 中调用它，确定申请缓冲区的大小
static int queue_setup(struct vb2_queue *vq, const struct v4l2_format *fmt,
                unsigned int *nbuffers, unsigned int *nplanes,
                unsigned int sizes[], void *alloc_ctxs[])
{
    unsigned long size;
    printk("mformat.width %d \n",mformat.width);
    printk("mformat.height %d \n",mformat.height);
    printk("mformat.pixelsize %d \n",mformat.pixelsize);

    size = mformat.width * mformat.height * mformat.pixelsize;

    if (0 == *nbuffers)
        *nbuffers = 32;

    while (size * *nbuffers > vid_limit * 1024 * 1024)
        (*nbuffers)--;

    *nplanes = 1;

    sizes[0] = size;
    return 0;
}

static int buffer_init(struct vb2_buffer *vb)
{
    return 0;
}

static int buffer_finish(struct vb2_buffer *vb)
{
    return 0;
}

static int buffer_prepare(struct vb2_buffer *vb)
{
    unsigned long size;
    size = mformat.width * mformat.height * mformat.pixelsize;
    vb2_plane_size(vb, 0);
    //vb2_set_plane_payload(&buf->vb, 0, size);
    return 0;
}

static void buffer_queue(struct vb2_buffer *vb)
{

}

// 内核线程中填充数据，效果是一个逐渐放大的圆形，视频大小为 640 * 480
static void vivi_fillbuff(struct vb2_buffer *vb)
{
    void *vbuf = NULL;   
    unsigned char (*p)[mformat.width][mformat.pixelsize];
    unsigned int i,j;
    static unsigned int t = 0;
    vbuf = vb2_plane_vaddr(vb, 0);
    p = vbuf;

    for(j = 0; j < mformat.height; j++)
        for(i = 0; i < mformat.width; i++){
            if((j - 240)*(j - 240) + (i - 320)*(i - 320) < (t * t)){
                *(*(*(p+j)+i)+0) = (unsigned char)0xff;
                *(*(*(p+j)+i)+1) = (unsigned char)0xff;
            }else{
                *(*(*(p+j)+i)+0) = (unsigned char)0;
                *(*(*(p+j)+i)+1) = (unsigned char)0;
            }           
        }
    t++;
    printk("%d\n",t);
    if( t >= mformat.height/2) t = 0;
}

// 内核线程每一次唤醒调用它
static void vivi_thread_tick(void)
{
    struct vb2_buffer *buf = NULL;
    if (list_empty(&vivi_queue.queued_list)) {
        //printk("No active queue to serve\n");
        return;
    }
    // 注意我们这里取出之后就删除了，剩的重复工作，但是在 dqbuf 时，vb2_dqbuf 还会删除一次，我做的处理是在dqbuf之前将buf随便挂入一个链表
    buf = list_entry(vivi_queue.queued_list.next, struct vb2_buffer, queued_entry);
    list_del(&buf->queued_entry);

    /* 填充数据 */
    vivi_fillbuff(buf);
    printk("filled buffer %p\n", buf->planes[0].mem_priv);

    // 它干两个工作，把buffer 挂入done_list 另一个唤醒应用层序，让它dqbuf
    vb2_buffer_done(buf, VB2_BUF_STATE_DONE);
}

#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(500)  /* 0.5 seconds */
#define frames_to_ms(frames)                    \
    ((frames * WAKE_NUMERATOR * 1000) / WAKE_DENOMINATOR)

static void vivi_sleep(void)
{
    int timeout;
    DECLARE_WAITQUEUE(wait, current);

    add_wait_queue(&wait_queue_head, &wait);
    if (kthread_should_stop())
        goto stop_task;

    /* Calculate time to wake up */
    timeout = msecs_to_jiffies(frames_to_ms(1));

    vivi_thread_tick();

    schedule_timeout_interruptible(timeout);

stop_task:
    remove_wait_queue(&wait_queue_head, &wait);
    try_to_freeze();
}

static int vivi_thread(void *data)
{
    set_freezable();

    for (;;) {
        vivi_sleep();

        if (kthread_should_stop())
            break;
    }
    printk("thread: exit\n");
    return 0;
}

static int vivi_start_generating(void)
{   
    kthread = kthread_run(vivi_thread, video_dev, video_dev->name);

    if (IS_ERR(kthread)) {
        printk("kthread_run error\n");
        return PTR_ERR(kthread);
    }

    /* Wakes thread */
    wake_up_interruptible(&wait_queue_head);

    return 0;
}

static int start_streaming(struct vb2_queue *vq, unsigned int count)
{
    vivi_start_generating();
    return 0;
}
static int stop_streaming(struct vb2_queue *vq)
{
    if (kthread) {
        kthread_stop(kthread);
        kthread = NULL;
    }
/*

    while (!list_empty(&vivi_queue.queued_list)) {
        struct vb2_buffer *buf;
        buf = list_entry(vivi_queue.queued_list.next, struct vb2_buffer, queued_entry);
        list_del(&buf->queued_entry);
        vb2_buffer_done(buf, VB2_BUF_STATE_ERROR);
    }
*/
    return 0;
}
static struct vb2_ops vivi_video_qops = {
    .queue_setup        = queue_setup,
    .buf_init       = buffer_init,
    .buf_finish     = buffer_finish,
    .buf_prepare        = buffer_prepare,
    .buf_queue      = buffer_queue,
    .start_streaming    = start_streaming,
    .stop_streaming     = stop_streaming,
};

static int mvivi_open(struct file *filp)
{   
    vivi_queue.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    vivi_queue.io_modes = VB2_MMAP;
    vivi_queue.drv_priv = video_dev;
    //vivi_queue.buf_struct_size = sizeof(struct vivi_buffer);
    vivi_queue.ops      = &vivi_video_qops;
    vivi_queue.mem_ops  = &vb2_vmalloc_memops;
  //  vivi_queue.name = "vb2";
    vivi_queue.buf_struct_size = sizeof(struct vb2_buffer);
    INIT_LIST_HEAD(&vivi_queue.queued_list);
    INIT_LIST_HEAD(&vivi_queue.done_list);
    spin_lock_init(&vivi_queue.done_lock);
    init_waitqueue_head(&vivi_queue.done_wq);
    mformat.fourcc = V4L2_PIX_FMT_YUYV;
    mformat.depth  = 16;
    INIT_LIST_HEAD(&my_list);
    return 0;
}

static int vidioc_reqbufs(struct file *file, void *priv, struct v4l2_requestbuffers *p)
{
    printk("vidioc_reqbufs  \n");
    printk("count %d\n",p->count);
    printk("memory %d\n",p->memory);
    return vb2_reqbufs(&vivi_queue, p);
}

static int vidioc_querybuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    printk("vidioc_querybuf  \n");
    return vb2_querybuf(&vivi_queue, p);
}

static int vidioc_qbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    printk("vidioc_qbuf buffer  \n");
    return vb2_qbuf(&vivi_queue, p);
}

static int vidioc_dqbuf(struct file *file, void *priv, struct v4l2_buffer *p)
{
    struct vb2_buffer *vb;
    printk("vidioc_dqbuf buffer  \n");
    vb = list_first_entry(&vivi_queue.done_list, struct vb2_buffer, done_entry);
    list_add_tail(&vb->queued_entry, &my_list);
    return vb2_dqbuf(&vivi_queue, p, file->f_flags & O_NONBLOCK);
}

static int vidioc_streamon(struct file *file, void *priv, enum v4l2_buf_type i)
{
    printk("vidioc_streamon  \n");
    return vb2_streamon(&vivi_queue, i);
}

static int vidioc_streamoff(struct file *file, void *priv, enum v4l2_buf_type i)
{
    printk("vidioc_streamoff  \n");
    return vb2_streamoff(&vivi_queue, i);
}

static struct v4l2_ioctl_ops mvivi_ioctl_ops = {
    .vidioc_querycap            = mvidioc_querycap,
    .vidioc_enum_fmt_vid_cap    = vidioc_enum_fmt_vid_cap,
    .vidioc_g_fmt_vid_cap       = vidioc_g_fmt_vid_cap,
    .vidioc_try_fmt_vid_cap     = vidioc_try_fmt_vid_cap,
    .vidioc_s_fmt_vid_cap       = vidioc_s_fmt_vid_cap,
    .vidioc_reqbufs             = vidioc_reqbufs,
    .vidioc_querybuf            = vidioc_querybuf,
    .vidioc_qbuf                = vidioc_qbuf,
    .vidioc_dqbuf               = vidioc_dqbuf,
    .vidioc_streamon            = vidioc_streamon,
    .vidioc_streamoff           = vidioc_streamoff,
};

static unsigned int mvivi_poll(struct file *file, struct poll_table_struct *wait)
{
    struct vb2_buffer *vb = NULL;
    int res = 0;
    printk("enter the poll \n");

    poll_wait(file, &vivi_queue.done_wq, wait);

    if (!list_empty(&vivi_queue.done_list))
        vb = list_first_entry(&vivi_queue.done_list, struct vb2_buffer, done_entry);

    if (vb && (vb->state == VB2_BUF_STATE_DONE || vb->state == VB2_BUF_STATE_ERROR)) {
        return (V4L2_TYPE_IS_OUTPUT(vivi_queue.type)) ?
                res | POLLOUT | POLLWRNORM :
                res | POLLIN | POLLRDNORM;  
    }
    return 0;
}

long video_ioctl2(struct file *file, unsigned int cmd, unsigned long arg)
{
    return video_usercopy(file, cmd, arg, mvideo_ioctl);
}

static struct v4l2_file_operations mvivi_fops = {
    .owner          = THIS_MODULE,
    .open           = mvivi_open,
    .unlocked_ioctl = video_ioctl2,

    //.release        = mvivi_close,
    .poll           = mvivi_poll,   
    .mmap           = vivi_mmap,
};

static struct video_device vivi_template = {
    .name       = "mvivi",
    .fops       = &mvivi_fops,
    .ioctl_ops  = &mvivi_ioctl_ops,
    .release    = mvideo_device_release,
};

static int  mvivi_init(void)
{
    int ret;
    video_dev   = video_device_alloc();
    *video_dev  = vivi_template;
    ret = video_register_device(video_dev, VFL_TYPE_GRABBER, -1);
    if(ret != 0){
        printk(" video_register_device error\n");
    }else{
        printk(" video_register_device ok\n");
    }
    return ret;
}

static void  mvivi_exit(void)
{
    video_unregister_device(video_dev); 
}

module_init(mvivi_init);
module_exit(mvivi_exit);
MODULE_LICENSE("GPL");
