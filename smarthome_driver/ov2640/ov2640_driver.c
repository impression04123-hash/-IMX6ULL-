
/**
 * @file i2c_ov2640_driver.c
 * @author your name (you@domain.com)
 * @brief ov2640的设备驱动
 * @version 0.1
 * @date 2024-05-04
 * 
 * 
 */
#include "asm-generic/errno-base.h"
#include <linux/device.h>
#include "linux/gfp.h"
#include "linux/gpio.h"
#include "linux/kernel.h"
#include "linux/printk.h"
#include "media/v4l2-async.h"
#include "media/v4l2-common.h"
#include "media/v4l2-subdev.h"
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/delay.h>
#include "ov2640_reg.h"

#define I2C_DRIVER_NAME "ov2640-camera"
#define PIDH  0x0A
#define PIDL  0x0B

struct ov2640_device{
    u32 code;
    int width;
    int height;
    struct v4l2_subdev subdev;
    int rst_gpio;
    //int pwn_gpio;
};

/**
 * @brief 检测ov2640
 * 
 * @param client 
 * @return int 
 */
int detect_ov2640(struct i2c_client * client)
{
    int pid[2];
    struct v4l2_subdev * subdev = i2c_get_clientdata(client);
    struct ov2640_device * ovdev = container_of(subdev, struct ov2640_device, subdev);

    /*复位*/
    gpio_direction_output(ovdev->rst_gpio, 0);
    mdelay(500);
    gpio_direction_output(ovdev->rst_gpio, 1);
    mdelay(500);

    /*将睡眠引脚给拉低,让摄像头开始工作*/
    //gpio_direction_output(ovdev->pwn_gpio, 0);
    //mdelay(500);

    pid[1] = i2c_smbus_read_byte_data(client, PIDH);
    if(pid[1] < 0){
        printk("fail to read PIDH\n");
        return pid[1];
    }
    pid[0] = i2c_smbus_read_byte_data(client, PIDL);
    if(pid[0] < 0){
        printk("fail to read PIDL\n");
        return pid[0];
    }

    printk("Product ID:0x%x%x\n",pid[1],pid[0]);

    return 0;
}

/**
 * @brief 开机的电源管理的方法,当video_device的open方法打开的时候
 *          就会逐步的调用到这个方法,让摄像头工作
 * 
 * @param sd 
 * @param on 
 * @return int 
 */
static int ov2640_s_power(struct v4l2_subdev *sd, int on)
{
    //struct ov2640_device * ovdev = container_of(sd, struct ov2640_device, subdev);
    //
    //if(on){
    //    gpio_direction_output(ovdev->pwn_gpio, 0);
    //}else{
    //    gpio_direction_output(ovdev->pwn_gpio, 1);
    //}
    
    return 0;
}

static const struct v4l2_subdev_core_ops ov2640_core_ops = {
    .s_power = ov2640_s_power,
};

/**
 * @brief 匹配大小格式
 * 
 * @param width 
 * @param height 
 * @return const struct ov2640_win_size* 
 */
const struct ov2640_win_size * match_win_ov2640_size(int width,int height)
{
    int i= 0;
    int vga = 3;

    for(i = 0;i <  ARRAY_SIZE(ov2640_supported_win_sizes);i++){
        if(ov2640_supported_win_sizes[i].width >= width &&ov2640_supported_win_sizes[i].height >= height){
            return &ov2640_supported_win_sizes[i];
        }  
    }

    return &ov2640_supported_win_sizes[vga];
}

/**
 * @brief 将配置写入寄存器,使用了i2c_smbus
 * 
 * @param client 
 * @param vals 
 */
static void ov2640_write_array(const struct i2c_client * client,const struct regval_list * vals)
{
    int err;
    while((vals->reg_num != 0xff) ||(vals->value != 0xff)){
        err = i2c_smbus_write_byte_data(client, vals->reg_num, vals->value);
        if(err){
            printk("i2c_smbus_write_byte_data,reg:%#x,val:%#x\n",vals->reg_num,vals->value);
            return;
        }

        vals++;
    }
    return;
}

/**
 * @brief 给摄像头设置格式
 * 
 * @param sd 
 * @param fmt 
 * @return int 
 */
static int ov2640_s_mbus_fmt(struct v4l2_subdev *sd, struct v4l2_mbus_framefmt *fmt)
{
    struct i2c_client * client = v4l2_get_subdevdata(sd);
    struct ov2640_device * ovdev = container_of(sd, struct ov2640_device, subdev);

    /*大小格式*/
    const struct ov2640_win_size *win = match_win_ov2640_size(fmt->width,fmt->height);

    /*像素格式*/
    const struct regval_list * fmt_regs;
    
    switch(fmt->code){
        case MEDIA_BUS_FMT_YUYV8_2X8:fmt_regs = ov2640_yuyv_regs;break;
        case MEDIA_BUS_FMT_UYVY8_2X8:fmt_regs = ov2640_uyvy_regs;break;
        case MEDIA_BUS_FMT_RGB565_2X8_BE:fmt_regs = ov2640_rgb565_be_regs;break;
        case MEDIA_BUS_FMT_RGB565_2X8_LE:fmt_regs = ov2640_rgb565_le_regs;break;
        default:fmt_regs = ov2640_yuyv_regs;fmt->code = MEDIA_BUS_FMT_YUYV8_2X8;break;
    }

    ov2640_write_array(client,ov2640_init_regs);
    ov2640_write_array(client,ov2640_size_change_preamble_regs);
    ov2640_write_array(client,win->regs);

    ov2640_write_array(client,ov2640_format_change_preamble_regs);
    ov2640_write_array(client,fmt_regs);
    ov2640_write_array(client,ov2640_light_mode_sunny_regs);

    fmt->width = win->width;
    fmt->height = win->height;

    ovdev->code   = fmt->code;
    ovdev->width  = win->width;
    ovdev->height = win->height; 

    return 0;
}

/**
 * @brief 支持的格式
 * 
 * @param sd 
 * @param index 
 * @param code 
 * @return int 
 */
static int ov2640_enum_mbus_fmt(struct v4l2_subdev *sd, unsigned index, u32 *code)
{
    uint32_t ov2640_code[]={
        MEDIA_BUS_FMT_YUYV8_2X8,
        MEDIA_BUS_FMT_UYVY8_2X8,
        MEDIA_BUS_FMT_RGB565_2X8_BE,
	    MEDIA_BUS_FMT_RGB565_2X8_LE,
    };

    if(index > ARRAY_SIZE(ov2640_code)){
        return -EINVAL;
    }

    *code = ov2640_code[index];

    return 0;
}

static int ov2640_g_mbus_fmt(struct v4l2_subdev *sd,struct v4l2_mbus_framefmt *mf)
{
    struct ov2640_device * ovdev = container_of(sd, struct ov2640_device, subdev);

    mf->code = ovdev->code;
    mf->width = ovdev->width;
    mf->height = ovdev->height;

    return 0;
}

/**
 * @brief 开启流
 * 
 * @param sd 
 * @param on 
 * @return int 
 */
static int ov2640_s_stream(struct v4l2_subdev *sd, int on)
{
    return 0;
}
static const struct v4l2_subdev_video_ops ov2640_video_ops = {
    .enum_mbus_fmt = ov2640_enum_mbus_fmt,
    .s_mbus_fmt = ov2640_s_mbus_fmt,
    .g_mbus_fmt    = ov2640_g_mbus_fmt,
    .s_stream      = ov2640_s_stream,
};

static const struct v4l2_subdev_ops ov2640_ops = {
    .core  = &ov2640_core_ops,
    .video = &ov2640_video_ops,
};

static int i2c_ov2640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct ov2640_device *ovdev;
    struct device_node * np = client->dev.of_node;
    int err;

    /*这里不要用kmalloc,因为可能会因为这个kmalloc导致内部的地址是随机的从而导致后面有些地方会报错*/
    ovdev = devm_kzalloc(&client->dev, sizeof(*ovdev), GFP_KERNEL);
    if(!ovdev){
        printk("fail to devm kmalloc ovdev\n");
        return -ENOMEM;
    }

    /*获取gpio管脚资源并且占用它*/
    ovdev->rst_gpio = of_get_named_gpio(np, "rst-gpios", 0);
    if(!gpio_is_valid(ovdev->rst_gpio)){
        printk("invalid gpio %d\n",ovdev->rst_gpio);
        return -ENODEV;
    }
    err = devm_gpio_request_one(&client->dev, ovdev->rst_gpio, GPIOF_OUT_INIT_HIGH, "ov2640_reset");
    if(err){
        printk("devm_gpio_request_one rst_gpio\n");
        return err;
    }

    //ovdev->pwn_gpio = of_get_named_gpio(np, "pwn-gpios", 0);
    //if(!gpio_is_valid(ovdev->rst_gpio)){
    //    printk("invalid gpio %d\n",ovdev->pwn_gpio);
    //    return -ENODEV;
    //}
    //err = devm_gpio_request_one(&client->dev, ovdev->pwn_gpio, GPIOF_OUT_INIT_HIGH, "ov2640_pwn");
    //if(err){
    //    printk("devm_gpio_request_one pwn_gpio\n");
    //    return err;
    //}

    /*初始化v4l2_subdev*/
    v4l2_i2c_subdev_init(&ovdev->subdev, client, &ov2640_ops);
    
    err = v4l2_async_register_subdev(&ovdev->subdev);
    if(err){
        printk("fail to register subdev\n");
        return err;
    }

    err = detect_ov2640(client);
    if(err){
        printk("Fail to detect ov2640\n");
        return err;
    }

    return 0;
}

static int i2c_ov2640_remove(struct i2c_client *client)
{
    struct v4l2_subdev * subdev = i2c_get_clientdata(client);
    v4l2_async_unregister_subdev(subdev);

    return 0;
}

static const struct i2c_device_id i2c_ov2640_id[] = {
    {I2C_DRIVER_NAME, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, i2c_ov2640_id);

static const struct of_device_id i2c_ov2640_of_match[] = {
    {.compatible = "ovti,ov2640"},
    {},
};
MODULE_DEVICE_TABLE(of, i2c_ov2640_of_match);

static struct i2c_driver i2c_ov2640_driver = {
    .id_table = i2c_ov2640_id,
    .probe = i2c_ov2640_probe,
    .remove = i2c_ov2640_remove,
    .driver = {
        .name = I2C_DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(i2c_ov2640_of_match),
    },
};

module_i2c_driver(i2c_ov2640_driver);
MODULE_LICENSE("GPL");