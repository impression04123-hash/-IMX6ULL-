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
	if (argc != 2) {
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
	len =read(fd, &status, 1);
	printf("read data =%d\n",status);
	
}