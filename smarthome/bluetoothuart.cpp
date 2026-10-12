#include "bluetoothuart.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <QDebug>
#include <thread>
#include <errno.h>
#include <termios.h>
#include <stdlib.h>
#include <QThread>

int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop)
{
    struct termios newtio,oldtio;//这个oldtio用来保存之前的设置防止我们设置错了还能找回来

        if ( tcgetattr( fd,&oldtio) != 0) {
                return -1;
        }

        bzero( &newtio, sizeof( newtio ) );
        newtio.c_cflag |= CLOCAL | CREAD;
        newtio.c_cflag &= ~CSIZE;

        newtio.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /*Input*/
        newtio.c_oflag  &= ~OPOST;   /*Output*/

        switch( nBits )
        {
        case 7:
            newtio.c_cflag |= CS7;
        break;
        case 8:
            newtio.c_cflag |= CS8;
        break;
        }

        switch( nEvent )
        {
        case 'O':
            newtio.c_cflag |= PARENB;
            newtio.c_cflag |= PARODD;
            newtio.c_iflag |= (INPCK | ISTRIP);
        break;
        case 'E':
            newtio.c_iflag |= (INPCK | ISTRIP);
            newtio.c_cflag |= PARENB;
            newtio.c_cflag &= ~PARODD;
        break;
        case 'N':
            newtio.c_cflag &= ~PARENB;
        break;
        }

        switch( nSpeed )
        {
        case 2400:
            cfsetispeed(&newtio, B2400);
            cfsetospeed(&newtio, B2400);
        break;
        case 4800:
            cfsetispeed(&newtio, B4800);
            cfsetospeed(&newtio, B4800);
        break;
        case 9600:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
        break;
        case 115200:
            cfsetispeed(&newtio, B115200);
            cfsetospeed(&newtio, B115200);
        break;
        default:
            cfsetispeed(&newtio, B9600);
            cfsetospeed(&newtio, B9600);
        break;
        }

        if( nStop == 1 )
            newtio.c_cflag &= ~CSTOPB;
        else if ( nStop == 2 )
            newtio.c_cflag |= CSTOPB;

        newtio.c_cc[VMIN]  = 1;  /* 读数据时的最小字节数: 没读到这些数据我就不返回! */
        newtio.c_cc[VTIME] = 0; /* 等待第1个数据的时间:
                                 * 比如VMIN设为10表示至少读到10个数据才返回,
                                 * 但是没有数据总不能一直等吧? 可以设置VTIME(单位是10秒)
                                 * 假设VTIME=1，表示:
                                 *    10秒内一个数据都没有的话就返回
                                 *    如果10秒内至少读到了1个字节，那就继续等待，完全读到VMIN个数据再返回
                                 */

        tcflush(fd,TCIFLUSH);

        if((tcsetattr(fd,TCSANOW,&newtio))!=0)
        {
            return -1;
        }

        return 0;
}

BluetoothUart::BluetoothUart(QObject *parent) : QThread(parent)
{
    uart_fd = open(uart_device.toStdString().c_str(),O_RDWR);
    if(uart_fd < 0){
        qDebug()<<"fail to open uart\n";
        return;
    }

    if(fcntl(uart_fd, F_SETFL, 0)<0){ /* 设置串口为阻塞状态*/
        qDebug()<<"fcntl failed!\n";
        return ;
     }

    set_opt(uart_fd, 115200, 8, 'N', 1);

}

/**
  为什么要使用线程，因为你不知道人家什么时候会来连蓝牙所以这里会一直while(1)
   但是一直while就会导致界面卡住按钮也不能按了，所以这里必须要开一个线程

   那卫什么要用这种继承的方式呢，因为使用movetoThread的试过了还是不行
 * @brief BluetoothUart::run
 */
void BluetoothUart::run()
{
    int cnt = 0;
    char buf[20] = {0};//这里给20是因为20足够了,并不一定就是20
    while(1){
       read(uart_fd,&buf[cnt++],1);
       //因为蓝牙那边每次发送的数据不是每个都是固定的在长度的,所以要等到一帧数据接收完
       //如果都包含了{和}那么就是代表发送完了
       if(strchr(buf,'{') && strchr(buf,'}')){

           QString module = QString(buf).mid(QString(buf).indexOf('{')+1,QString(buf).indexOf(',')-QString(buf).indexOf('{')-1);
           QString value = QString(buf).mid(QString(buf).indexOf(',') + 1,QString(buf).indexOf('}')-QString(buf).indexOf(',')-1);

           qDebug()<<"module:"<<module <<"value:"<<value;

           cnt = 0;
           memset(buf,0,sizeof(buf));

           emit uart_recv(module.toUtf8().data(),value.toUtf8().data());
       }

       if(cnt == 20){
           cnt = 0;
           memset(buf,0,sizeof(buf));
       }
    }

}


