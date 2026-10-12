#include "sr501.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/signal.h>

int sr501_fdstatic = 0;

Sr501::Sr501(QObject *parent) : QObject(parent)
{
    sr501_fd = open(sr501_device.toStdString().c_str(),O_RDWR);
    if(sr501_fd < 0){
        printf("fail to open sr501\n");
        return;
    }
    sr501_fdstatic = sr501_fd;

    fcntl(sr501_fd,F_SETOWN,getpid());//把自己的id告诉驱动
    sr501_flags = fcntl(sr501_fd,F_GETFL);//获得flags
    fcntl(sr501_fd,F_SETFL,sr501_flags | O_ASYNC);//启动驱动的fsync方法

}

int Sr501::sr501_read(char *buf)
{
    int ret;
    ret = read(sr501_fdstatic,buf,1);
    return ret;
}

