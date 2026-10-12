#include <stdio.h> 
#include <sys/types.h> 
#include <sys/stat.h> 
#include <fcntl.h> 
#include <unistd.h> 
#include <stdint.h> 
#include <sys/ioctl.h> 
 
#define VERSION_REGISTER 0x37 
#define COMMAND_REGISTER 0x01 
 
int main(int argc, const char *argv[]) 
{ 
    int ret; 
    int fd; 
    uint8_t vbuf[2] = {VERSION_REGISTER}; 
    uint8_t rbuf[2] = {COMMAND_REGISTER}; 
    uint8_t wbuf[2] = {COMMAND_REGISTER,0xc}; 
 
    fd = open("/dev/rc522",O_RDWR); 
    if(fd < 0){ 
        perror("Fail to open"); 
        return -1; 
    } 
 
    while(1){
        ret = read(fd,vbuf,sizeof(vbuf)); 
        if(ret < 0){ 
            perror("Fail to read"); 
            return -1; 
        } 
        
        printf("Version reg:%#x value:%#x\n",vbuf[0],vbuf[1]); 
        //printf("Command reg:%#x value:%#x\n",rbuf[0],rbuf[1]); 
        sleep(1);
    }
    
#if 0 
    while(1){ 
        ret = write(fd,wbuf,sizeof(wbuf)); 
        if(ret < 0){ 
            perror("Fail to write"); 
            break; 
        } 
 
        ret = read(fd,rbuf,sizeof(rbuf)); 
        if(ret < 0){ 
            perror("Fail to read"); 
            break; 
        } 
        printf("Command reg:%#x value:%#x\n",rbuf[0],rbuf[1]); 
        sleep(1); 
    } 
#endif
    return 0; 
}