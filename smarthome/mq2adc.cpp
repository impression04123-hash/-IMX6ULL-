#include "mq2adc.h"
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <QDebug>

Mq2ADC::Mq2ADC(QObject *parent) : QObject(parent)
{

    mq2_timer = new QTimer(this);
    mq2_timer->setInterval(1500);//周期1.5s
    connect(mq2_timer,&QTimer::timeout,this,&Mq2ADC::timeto_read_mq2data);
    mq2_timer->start();
}

void Mq2ADC::mq2_read(char *buf)
{
    char data[20];

    //每次都需要重新打开,否则呢,adc不会更新,否则文件指针可能会停留在文件的末尾，导致后续读取操作无法获取新的数据
    mq2_fd = fopen(mq2_device.toStdString().c_str(),"r");
    if(mq2_fd ==NULL){
        qDebug() << "fail to fopen mq2adc_device\n";
        return;
    }

    rewind(mq2_fd);// 将光标移回文件开头
    fscanf(mq2_fd, "%s", data);
    *buf = atoi(data);

    //一定要关闭文件，否则占用文件后内核无法更新ADC值
    fclose(mq2_fd);
}

void Mq2ADC::timeto_read_mq2data()
{
    emit readyto_read_mq2data();
}
