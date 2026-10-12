#include "sg90.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

Sg90::Sg90(QObject *parent) : QObject(parent)
{
    sg90_fd = open(sg90_device.toStdString().c_str(),O_WRONLY);
    if(sg90_fd < 0){
        printf("fail to open");
    }
}

int Sg90::sg90_write(char *buf)
{
    int ret = write(sg90_fd,buf,1);
    return ret;
}
