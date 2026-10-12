#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>

const char *voltage1_raw = "/sys/bus/iio/devices/iio:device0/in_voltage1_raw";
const char *voltage1_scale = "/sys/bus/iio/devices/iio:device0/in_voltage_scale";

int main(int argc, char const *argv[])
{
    FILE *raw_fd,*scale_fd;
    char data[20];
    int len1,len2,len;
    int raw;
    double scale;

    while(1){
        raw_fd = fopen(voltage1_raw,"r");
        if(raw_fd ==NULL){
            printf("open raw_fd failed!\n");
			return -1;
        }

        scale_fd = fopen(voltage1_scale,"r");
        if(raw_fd == NULL){
            printf("open raw_fd failed!\n");
			return -1;
        }

        //读取数据
        rewind(raw_fd);
        fscanf(raw_fd,"%s",data);
        raw = atoi(data);

        memset(data,0,sizeof(data));
        fscanf(scale_fd,"%s",data);
        scale = atof(data);

        printf("ADC原始值：%d，电压值：%.3fV\r\n", raw, raw * scale / 1000.f);

        fclose(raw_fd);
        fclose(scale_fd);

        sleep(2);
    }
    return 0;
}
