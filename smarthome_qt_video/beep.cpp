#include "beep.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

Beep::Beep(QObject *parent) : QObject(parent)
{
    beep_fd = open(beep_device.toStdString().c_str(),O_RDWR);
    if(beep_fd < 0){
        printf("fail to open");
    }
}

int Beep::beep_write(char *buf)
{
    int ret = write(beep_fd,buf,1);
    return ret;
}
