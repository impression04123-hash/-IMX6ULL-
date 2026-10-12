#include "jdq.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

Jdq::Jdq(QObject *parent) : QObject(parent)
{
    jdq_fd = open(jdq_device.toStdString().c_str(),O_RDWR);
    if(jdq_fd < 0){
        printf("fail to open");
    }
}

int Jdq::jdq_write(char *buf)
{
    int ret = write(jdq_fd,buf,1);
    return ret;
}
