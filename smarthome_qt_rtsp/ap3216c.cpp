#include "ap3216c.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
#include <string.h>
#include <stdint.h>
#include <sys/ioctl.h>

Ap3216c::Ap3216c(QObject *parent) : QObject(parent)
{
    ap3216c_fd = open(ap3216c_device.toStdString().c_str(),O_RDWR);
    if(ap3216c_fd < 0){
        qDebug() << "fail to open ap3216c_device\n";
        return;
    }

    ap3216c_timer = new QTimer(this);
    ap3216c_timer->setInterval(1000);//周期1s
    connect(ap3216c_timer,&QTimer::timeout,this,&Ap3216c::timeto_read_ap3216cdata);
    ap3216c_timer->start();
}

void Ap3216c::ap3216c_read(char *buf)
{
    ioctl(ap3216c_fd,AP3216C_GETIR,&buf[0]);
    ioctl(ap3216c_fd,AP3216C_GETLIGHT,&buf[1]);
    ioctl(ap3216c_fd,AP3216C_GETPS,&buf[2]);
}

void Ap3216c::timeto_read_ap3216cdata()
{
    emit readyto_read_ap3216cdata();
}
