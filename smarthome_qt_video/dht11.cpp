#include "dht11.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>


Dht11::Dht11(QObject *parent) : QObject(parent)
{
    dht11_fd = open(dht11_device.toStdString().c_str(),O_RDONLY);
    if(dht11_fd < 0){
        qDebug() << "fail to open dht11_device\n";
        return;
    }

    dht11_timer = new QTimer(this);
    dht11_timer->setInterval(3000);//周期3s,太快的dht11会反应不过来,因为它是单总线的
    connect(dht11_timer,&QTimer::timeout,this,&Dht11::timeto_read_dht11data);
    dht11_timer->start();
}

void Dht11::dht11_read(char *buf)
{
    int len;
    len = read(dht11_fd, buf, 5);
}

void Dht11::timeto_read_dht11data()
{
    //这里转发到主界面中去,在主界面对ui控件赋值
    emit readyto_read_dht11data();
}
