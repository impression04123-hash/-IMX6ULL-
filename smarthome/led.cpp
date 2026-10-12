#include "led.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

Led::Led(QObject *parent) : QObject(parent)
{
    led_fd = open(led_device.toStdString().c_str(),O_RDWR);
    if(led_fd < 0){
        printf("fail to open");
    }
}


int Led::led_write(char *buf)
{
    int ret = write(led_fd,buf,1);
    return ret;
}
