#include "v4l2camera.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#include <string.h>
#include <sys/mman.h>
#include <signal.h>
#include <poll.h>
#include <linux/fb.h>
#include <stdlib.h>
#include <string.h>
#include <QDebug>
#include <QtGui/QScreen>
#include <QDateTime>
#include <QRandomGenerator>
#include <QDebug>
#include <QDir>


V4l2Camera::V4l2Camera(QObject *parent): QThread(parent)
{
    // 获取当前目录
    QString currentDir = QDir::currentPath();
    // 拼接photo文件夹路径
    QString photoDirPath = currentDir + "/photo";
    // 创建QDir对象
    QDir photoDir(photoDirPath);
    // 判断photo文件夹是否存在
    if (!photoDir.exists()){
        // 创建photo文件夹
        if (photoDir.mkdir(photoDirPath)){
            qDebug()<<"创建 photo 文件夹成功\n";
        }else{
             qDebug()<<"创建 photo 文件夹失败\n";
        }
    }else{
         qDebug()<<"photo 文件夹已存在\n";
    }
}

void V4l2Camera::run()
{
    //如果打开设备和设置参数成功
    if(0 == v4l2_open()){
        //如果是开始采集状态
        while(start_flag){
            for (uint32_t i= 0; i < VIDEO_COUNT; i++) {
                struct v4l2_buffer buf;
                buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
                buf.memory = V4L2_MEMORY_MMAP;
                buf.index = i;

                //从内核队列中取出一个buf
                if (0 > ioctl(video_fd, VIDIOC_DQBUF, &buf)) {
                    qDebug()<<video_fd<<":"<<buf.type<<":"<<buf.memory<<":"<<buf.index;
                    perror("ERROR: failed to VIDIOC_DQBUF\n");
                    return;
                }

                QImage qImage((unsigned char*)user_buffer[i].start, VIDEO_WIDTH, VIDEO_HEIGH, QImage::Format_RGB16);
                emit image_ready(qImage);

                if(photo_flag){
                    generator_photo(buf);

                    mutex.lock();
                    this->photo_flag = false;
                    mutex.unlock();
                }

                //把buf重新放入内核队列
                if(0 > ioctl(video_fd,VIDIOC_QBUF,&buf)){
                    perror("Fail to ioctl:VIDIOC_QBUF");
                    return;
                }
            }
        }

        msleep(800);//at lease 650
        v4l2_close();
    }

}

/**
 * @brief V4l2Camera::set_thread_start
 * @param start
 * 开始或停止采集
 */
void V4l2Camera::set_thread_start(bool start)
{
    if(start){
        mutex.lock();
        this->start_flag = true;
        mutex.unlock();

        this->start();
    }else{
        mutex.lock();
        this->start_flag = false;
        mutex.unlock();

        this->quit();

        //不可以在这里关闭流和释放空间,会报各种错误,原因就是你在线程内申请资源也应该在线程内关闭资源
        //v4l2_close();
    }
}

/**
 * @brief V4l2Camera::take_photo
 * 点击拍照
 */
void V4l2Camera::take_photo()
{
    mutex.lock();
    this->photo_flag = true;
    mutex.unlock();
}

/**
 * @brief V4l2Camera::v4l2_open
 * @return 0:成功 负数:失败
 * 打开,查询可以支持的格式,设置格式,映射内存
 */
int V4l2Camera::v4l2_open()
{
    int ret;
    struct v4l2_capability cap;
    struct v4l2_fmtdesc fmt;
    struct v4l2_streamparm streamparm;
    struct v4l2_frmsizeenum frmsizeenum;
    struct v4l2_format format;
    struct v4l2_requestbuffers requestbuffers;

    //1==>.打开设备
    video_fd = open(video_device.toStdString().c_str(),O_RDWR);
    if(video_fd < 0){
        qDebug() << "fail to open video_device\n";
        return -1;
    }

    //2==>.查看摄像头是否支持采集或者内存映射,可以不要这个直接到第4步
    ret = ioctl(video_fd,VIDIOC_QUERYCAP,&cap);
    if(0 == ret){
        if(0 == (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)||
                0 == (cap.capabilities & V4L2_CAP_STREAMING)){
            qDebug()<<"该摄像头设备不支持视频采集！";
            close(video_fd);
            return -2;
        }
        if(0 == (cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)){
            qDebug()<<"该摄像头设备不支持mmap内存映射！";
            close(video_fd);
            return -2;
        }

    }

    //3==>.枚举支持的格式,好判断选择哪个格式,可以不要这个直接到第4步
    memset(&fmt, 0, sizeof(fmt));
    fmt.index = 0;
    fmt.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    while(1){
        ret = ioctl(video_fd, VIDIOC_ENUM_FMT, &fmt);
        if(ret < 0){
            break;
        }

        printf("支持格式：%s, %c%c%c%c\n", fmt.description,
                                        fmt.pixelformat & 0xff,
                                        fmt.pixelformat >> 8 & 0xff,
                                        fmt.pixelformat >> 16 & 0xff,
                                        fmt.pixelformat >> 24 & 0xff);
        // 列出该格式下支持的分辨率
        //默认帧率 VIDIOC_G_PARM
        streamparm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        if(0 == ioctl(video_fd, VIDIOC_G_PARM, &streamparm)){
            printf("该格式默认帧率 %d fps\n", streamparm.parm.capture.timeperframe.denominator);
        }

        //循环列出支持的分辨率 VIDIOC_ENUM_FRAMESIZES
        frmsizeenum.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        frmsizeenum.pixel_format = fmt.pixelformat;   //设置成对应的格式

        printf("支持的分辨率有：\n");
        while(1){
            ret = ioctl(video_fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum);
            if(ret < 0){
                break;
            }

            printf("%d x %d\n", frmsizeenum.discrete.width, frmsizeenum.discrete.height);
            frmsizeenum.index++;

        }

       fmt.index++;
    }

    //4==>.设置格式,帧率
    format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;          /* 视频采集 */
    format.fmt.pix.width = VIDEO_WIDTH;                 /* 宽 */
    format.fmt.pix.height = VIDEO_HEIGH;                /* 高 */
    format.fmt.pix.colorspace = V4L2_COLORSPACE_SRGB;
    format.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;   /* 设置输出类型：RGB565,由你的驱动决定的 */

    ret = ioctl(video_fd, VIDIOC_S_FMT, &format);
    if(ret < 0){
        perror("设置摄像头参数失败！");
        close(video_fd);
        return -4;
    }


    /*5==>.向内核申请内存 (VIDIOC_REQBUFS：个数、映射方式为mmap) 将申请到的缓存加入内核队列 (VIDIOC_QBUF) 将内核内存映射到用户空间 (mmap) */
    requestbuffers.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    requestbuffers.count = VIDEO_COUNT;                       //申请缓存个数
    requestbuffers.memory = V4L2_MEMORY_MMAP;       //申请为物理连续的内存空间

    //请求buffer
    ret = ioctl(video_fd, VIDIOC_REQBUFS, &requestbuffers);
    if(ret < 0){
        perror("申请内存失败！");
        close(video_fd);
        return -5;
    }

    for(uint32_t i = 0;i < requestbuffers.count;i ++){
        struct v4l2_buffer buf;

        bzero(&buf,sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.index= i;
        buf.memory= V4L2_MEMORY_MMAP;

        //查询并映射buffer
        ret = ioctl(video_fd,VIDIOC_QUERYBUF,&buf);
        if(ret < 0){
            perror("Fail to ioctl:VIDIOC_QUERYBUF");
            close(video_fd);
            return -5;
        }

        user_buffer[i].length = buf.length;
        user_buffer[i].start  = mmap(NULL,
                                     buf.length,
                                     PROT_READ | PROT_WRITE,
                                     MAP_SHARED,
                                     video_fd,
                                     buf.m.offset);
        if(MAP_FAILED == user_buffer[i].start){
            perror("Fail to mmap");
            close(video_fd);
            return -5;
        }

        //把buffer放入内核队列
        ret = ioctl(video_fd,VIDIOC_QBUF,&buf);
        if(ret < 0){
            perror("Fail to ioctl:VIDIOC_QBUF");
            close(video_fd);
            return -5;
        }
     }


    //6.==>打开视频流
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    ret = ioctl(video_fd,VIDIOC_STREAMON,&type);
    if(ret < 0){
        perror("Fail to ioctl:VIDIOC_STREAMON");
        close(video_fd);
        return -6;
    }

    return 0;

}

/**
 * @brief V4l2Camera::generator_photo
 * @param buf
 * 生成图片用于在相册中显示
 */
void V4l2Camera::generator_photo(struct v4l2_buffer buf)
{
    //随机产生10个数字,作为照片的名字
    QString randomNumbers;
    for(int i=0; i<10; i++) {
        int randomNumber = QRandomGenerator::global()->bounded(10);
        randomNumbers.append(QString::number(randomNumber));
    }
    QString str = "./photo/photo_" + randomNumbers + ".jpg";

    // 将缓冲区中的RGB565格式数据转换为QImage
    QImage image((unsigned char*)user_buffer[buf.index].start, VIDEO_WIDTH, VIDEO_HEIGH, QImage::Format_RGB16);

    // 保存图像为JPEG格式
    if (image.save(str, "JPEG")) {
        printf("Saved image to %s\n", str.toStdString().c_str());
    } else {
        printf("Failed to save image to %s\n", str.toStdString().c_str());
    }

    //让屏幕闪一下然后继续采集视频流
    emit image_photo();
    msleep(10);
}

/**
 * @brief V4l2Camera::v4l2_close
 * 关闭视频流,释放资源
 */
void V4l2Camera::v4l2_close()
{
    //7.==>关闭视频流和释放映射
    int ret = -1;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    ret = ioctl(video_fd, VIDIOC_STREAMOFF, &type);
    if(ret < 0){
        return;
    }

    for(uint32_t i = 0; i < VIDEO_COUNT; i++){
        munmap(user_buffer[i].start, user_buffer[i].length);
    }

    close(video_fd);
    qDebug()<<"Camera closed successfully.";

}
