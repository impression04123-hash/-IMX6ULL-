#ifndef V4L2CAMERA_H
#define V4L2CAMERA_H

#include <QThread>
#include <QImage>
#include <QMutex>

#define		VIDEO_WIDTH		640
#define 	VIDEO_HEIGH 	480
#define     VIDEO_COUNT     5

typedef struct{
    void *start;
    int length;
}user_buffer_t;

class V4l2Camera : public QThread
{
    Q_OBJECT
public:
    explicit V4l2Camera(QObject *parent = nullptr);
    void run();

public slots:
    void set_thread_start(bool start);
    void take_photo(void);

signals:
     void image_ready(QImage image);
     void image_photo(void);

private:
    QString video_device = "/dev/video0";
    int video_fd;
    user_buffer_t user_buffer[VIDEO_COUNT];         /* 存放摄像头设备内核缓冲区映射后的用户内存地址 */
    bool start_flag = true;
    bool photo_flag =false;
    QMutex mutex;

    int v4l2_open();   //初始化相机参数
    void v4l2_close();
    void generator_photo(struct v4l2_buffer buf);

};

#endif // V4L2CAMERA_H
