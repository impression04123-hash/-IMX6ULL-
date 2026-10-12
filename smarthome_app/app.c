#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#define LED_BEEP_JDQ 1
#define SG90 0
#define DHT11 0

int main(int argc,char **argv)
{
	
    int fd;
	ssize_t len;
    char status;
	char result;

    /* 1. 判断参数 */
	if (argc != 3) {
		printf("Usage: %s <dev> <on | off>\n", argv[0]);
		return -1;
	}

    /* 2. 打开文件 */
	fd = open(argv[1], O_RDWR);
	if (fd == -1){
		printf("can not open file %s\n", argv[1]);
		return -1;
	}

	/* 3. 写文件 */
#if LED_BEEP_JDQ
	//led,beep,jdq
	if (0 == strcmp(argv[2], "on")){
		status = 0;
		write(fd, &status, 1);
	}else{
		status = 1;
		write(fd, &status, 1);
	}
#elif SG90
	//舵机
	char data;
	data = strtol(argv[2], NULL, 10);
	write(fd,&data,sizeof(data));

	
#elif DHT11
	uint8_t recv_buf[5]= {0};
	while(1){
		sleep(1);
		len = read(fd, recv_buf, 5);
		if(len > 0){
			printf("DHT11 Temperature: %d°C, Humidity: %d%%\n", recv_buf[2], recv_buf[0]);
		}
	}

#endif	
	close(fd);
    return 0;
}