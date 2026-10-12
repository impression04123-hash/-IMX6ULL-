#include <stdio.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <poll.h>

int fd;

void handler(int signum)
{
    close(fd);
    exit(0);
    return ;
}
void sr501_handler(int signum)
{
    char val;

    read(fd,&val,1);    
    printf("val is %d, %s\n", val, val==1?"have people":"no people");
    
    return;
}

int main(int argc, char const *argv[])
{
    char buf[10];
	
	int val;
	char status = 1;
	int flags;
	int tim;

    signal(SIGINT,handler);
    signal(SIGIO,sr501_handler);

    fd = open("/dev/sr501_device",O_RDWR);
    if(fd < 0){
        printf("/dev/sr501 open failed\n");
		return 1;
    }

    //app将自己的id告诉驱动
    fcntl(fd,F_SETOWN,getpid());
    //取得fd的文件状态标志
    flags = fcntl(fd,F_GETFL);
    //把标志 Flag 里面的 FASYNC 位为 1，使能“异步通知”
    fcntl(fd,F_SETFL,flags | O_ASYNC);

    while(1){
        sleep(1);
    }

    return 0;
}
