#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdint.h>
#include <sys/ioctl.h>

#define AP3216C_IOCTL_BASE    'L'
#define AP3216C_GETPS       _IOR(AP3216C_IOCTL_BASE, 1, int)
#define AP3216C_GETLIGHT    _IOR(AP3216C_IOCTL_BASE, 2, int)
#define AP3216C_GETIR    	_IOR(AP3216C_IOCTL_BASE, 3, int)

int main(int argc, const char *argv[])
{
    int fd;
    int ps_data;
    int als_data;
	int ir_data;

    fd = open("/dev/ap3216c_device",O_RDWR);
    if(fd < 0){
        perror("fail to open\n");
        return -1;
    }

    while(1){
        ioctl(fd,AP3216C_GETPS,&ps_data);
        printf("ps data:%d\n",ps_data);

        ioctl(fd,AP3216C_GETLIGHT,&als_data);
        printf("als data:%d\n",als_data);

		ioctl(fd,AP3216C_GETIR,&ir_data);
        printf("ir_data:%d\n",ir_data);

        sleep(1);
    }
}