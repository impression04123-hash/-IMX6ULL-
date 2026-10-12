#include "linux/printk.h"
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/i2c.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/fs.h>

#define AP3216C_SYSTEMCONG  0x00 /* 配置寄存器 */
#define AP3216C_IRDATALOW   0x0A /* 红外数据低字节 */
#define AP3216C_IRDATAHIGH  0x0B /* 红外数据高字节 */
#define AP3216C_ALSDATALOW  0x0C /* 光照数据低字节 */
#define AP3216C_ALSDATAHIGH 0X0D /* 光照数据高字节 */
#define AP3216C_PSDATALOW   0X0E /* 距离数据低字节 */
#define AP3216C_PSDATAHIGH  0X0F /* 举例数据高字节 */

#define AP3216C_IOCTL_BASE    'L'
#define AP3216C_GETPS       _IOR(AP3216C_IOCTL_BASE, 1, int)
#define AP3216C_GETLIGHT    _IOR(AP3216C_IOCTL_BASE, 2, int)
#define AP3216C_GETIR       _IOR(AP3216C_IOCTL_BASE, 3, int)

struct ap3216c_device{
    struct i2c_client *client;
    struct class * cls;
    struct device *dev;
    int major;
};

struct ap3216c_device * ap_dev;

/**
 * @brief:  读取ap3216c寄存器数据
 * @param:  client - [参数说明] 
 * @param:  regaddr - [寄存器地址] 
 * @param:  regval - [参数说明] 
 * @return: int - [0成功 其他失败] 
 */
int read_ap3216c_reg(struct i2c_client *client,uint8_t regaddr,uint8_t *regval)
{
    int err;
    struct i2c_msg msg[2];
    uint8_t rx_buf[1];
    uint8_t tx_buf[]= {regaddr};

    /* msg[0]，第一条写消息，发送要读取的寄存器首地址 */
    msg[0].addr = client->addr;
    msg[0].flags = !I2C_M_RD;
    msg[0].len = sizeof(tx_buf)/sizeof(tx_buf[0]);
    msg[0].buf = tx_buf;//写什么

    /* msg[1]，第二条读消息，读取寄存器数据 */
    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = sizeof(rx_buf)/sizeof(rx_buf[0]);
    msg[1].buf = rx_buf;//读到数据存放的地方

    err = i2c_transfer(client->adapter, msg, sizeof(msg)/sizeof(msg[0]));
    if(err < 0){
        printk("fail to read_ap3216c_reg:i2c_transfer\n");
        return err;
    }

    //将读取到的数据通过指针返回
    *regval = rx_buf[0];

    return 0;
}

/**
 * @brief:  向ap3216c寄存器写入数据
 * @param:  client - [参数说明] 
 * @param:  regaddr - [参数说明] 
 * @param:  regval - [参数说明] 
 * @return: int - [返回值说明] 
 */
int write_ap3216c_reg(struct i2c_client *client,int regaddr,uint8_t regval)
{
    int err;
    struct i2c_msg msg;
    uint8_t tx_buf[]= {regaddr,regval};

    /* 发送要写入的寄存器首地址以及要写入的数据 */
    msg.addr = client->addr;
    msg.flags = !I2C_M_RD;
    msg.len = sizeof(tx_buf)/sizeof(tx_buf[0]);
    msg.buf = tx_buf;//写什么

    err = i2c_transfer(client->adapter, &msg, 1);
    if(err < 0){
        printk("fail to write_ap3216c_reg:i2c_transfer\n");
        return err;
    }

    return 0;
}

/**
 * @brief:  读取接近传感器数据
 * @param:  arg - [参数说明] 
 * @return: int - [返回值说明] 
 */
int read_ap3216c_psdata(void __user *arg)
{
    int err;
    int data;
    uint8_t ps_data_low;
    uint8_t ps_data_high;

    err = read_ap3216c_reg(ap_dev->client,AP3216C_PSDATALOW,&ps_data_low);
    if(err){
        printk("Fail to read_ap3216c_psdata:read_ap3216c_reg\n");
        return err;
    }

    err = read_ap3216c_reg(ap_dev->client,AP3216C_PSDATAHIGH,&ps_data_high);
    if(err){
        printk("Fail to read_ap3216c_psdata:read_ap3216c_reg\n");
        return err;
    }

    data = ((ps_data_high & 0X3F) << 4) |(ps_data_low & 0X0F);
    if(copy_to_user(arg,&data,sizeof(data))){
        return -EFAULT;
    }

    return data;
}

/**
 * @brief:  读取光环境传感器数据
 * @param:  arg - [参数说明] 
 * @return: int - [返回值说明] 
 */
int read_ap3216c_lightdata(void __user *arg)
{
    int err;
    int data;
    uint8_t als_data_low;
    uint8_t als_data_high;

    err = read_ap3216c_reg(ap_dev->client,AP3216C_ALSDATALOW,&als_data_low);
    if(err){
        printk("Fail to read_ap3216c_lightdata:read_ap3216c_reg\n");
        return err;
    }

    err = read_ap3216c_reg(ap_dev->client,AP3216C_ALSDATAHIGH,&als_data_high);
    if(err){
        printk("Fail to read_ap3216c_lightdata:read_ap3216c_reg\n");
        return err;
    }

    data = (als_data_high << 8) |als_data_low;
    if(copy_to_user(arg,&data,sizeof(data))){
        return -EFAULT;
    }

    return data;
}

/**
 * @brief 读取红外传感器数据
 * 
 * @param arg 
 * @return int 
 */
int read_ap3216c_irdata(void __user *arg)
{
    int err;
    int data;
    uint8_t ir_data_low;
    uint8_t ir_data_high;

    err = read_ap3216c_reg(ap_dev->client,AP3216C_IRDATALOW,&ir_data_low);
    if(err){
        printk("Fail to read_ap3216c_irdata:read_ap3216c_reg\n");
        return err;
    }

    err = read_ap3216c_reg(ap_dev->client,AP3216C_IRDATAHIGH,&ir_data_high);
    if(err){
        printk("Fail to read_ap3216c_irdata:read_ap3216c_reg\n");
        return err;
    }

    data = (ir_data_high << 8) |ir_data_low;
    if(copy_to_user(arg,&data,sizeof(data))){
        return -EFAULT;
    }

    return data;
}


/**
 * @brief:  打开ap3216c这个设备,这里是对ap3216c进行复位
 * @param:  inode - [参数说明] 
 * @param:  filp - [参数说明] 
 * @return: int - [返回值说明] 
 */
static int ap3216c_open(struct inode *inode, struct file *filp)
{
    /* 初始化 AP3216C */
    write_ap3216c_reg(ap_dev->client, AP3216C_SYSTEMCONG, 0x04); 
    mdelay(50); /* AP3216C 复位最少 10ms */
    write_ap3216c_reg(ap_dev->client, AP3216C_SYSTEMCONG, 0X03);

    return 0;
}

static long ap3216c_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    switch(cmd)
    {
        case AP3216C_GETPS:
            return read_ap3216c_psdata((void *)arg);

        case AP3216C_GETLIGHT:
            return read_ap3216c_lightdata((void *)arg);

        case AP3216C_GETIR:
            return read_ap3216c_irdata((void *)arg);
    }
    return 0;
}

static int ap3216c_relase(struct inode *inode, struct file *filp)
{
    return 0;
}

static const struct file_operations ap3216c_fops = {
    .owner =	THIS_MODULE,
	.open =		ap3216c_open,
	.release =	ap3216c_relase,
	.unlocked_ioctl = ap3216c_ioctl,
};

//用于和一般的i2c设备匹配，不管i2c设备来自设备树还是手工创建,没有这个会报错
static const struct i2c_device_id ap3216c_id[] = {
	{ "ap3216c", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ap3216c_id);
static const struct of_device_id ap3216c_of_match[] = {
    {.compatible = "smarthome,ap3216c", .data = NULL},
    {},
};
MODULE_DEVICE_TABLE(of, ap3216c_of_match);

/**
 * @brief:  probe函数--获取资源,创建字符设备提供操作函数给应用层
 * @param:  client - [参数说明] 
 * @param:  did - [参数说明] 
 * @return: int - [返回值说明] 
 */
static int ap3216c_probe(struct i2c_client *client,const struct i2c_device_id *did)
{
    int err;

    //使用devm的方式的话就不用自己释放资源了
    ap_dev = devm_kmalloc(&client->dev, sizeof(*ap_dev), GFP_KERNEL);
    if(!ap_dev){
        printk("fail to devm_kmalloc\n");
        return -ENOMEM;
    }

    ap_dev->client = client;
    i2c_set_clientdata(client, ap_dev);//主要是用来给remove函数用的

    //创建设备号
    ap_dev->major = register_chrdev(0,  "ap3216c", &ap3216c_fops);//这里是使用了老的注册字符设备的接口
    if(ap_dev->major < 0){
        printk("fail to __register_chrdev\n");
        err = ap_dev->major;
        goto err_register_chrdev;
    }

    //创建字符设备的类和设备
    ap_dev->cls = class_create(THIS_MODULE, "ap3216c_class");
    if(IS_ERR(ap_dev->cls)){
        printk("fail to class_create\n");
        err = PTR_ERR(ap_dev->cls);
        goto err_class_create;
    }
    ap_dev->dev = device_create(ap_dev->cls, &client->dev, MKDEV(ap_dev->major,0), NULL, "ap3216c_device");
    if(IS_ERR(ap_dev->dev)){
        printk("fail to class_create\n");
        err = PTR_ERR(ap_dev->dev);
        goto err_device_create;
    }

   printk("smarthome:ap3216c_driver insmod success\n");

    return 0;

err_device_create:
    class_destroy(ap_dev->cls);
err_class_create:
    unregister_chrdev(ap_dev->major,"ap3216c");
err_register_chrdev:
        return err;
}

static int ap3216c_remove(struct i2c_client *client)
{
    struct ap3216c_device * ap_dev = i2c_get_clientdata(client);

    device_destroy(ap_dev->cls, MKDEV(ap_dev->major, 0));
    class_destroy(ap_dev->cls);
    unregister_chrdev(ap_dev->major,"ap3216c");

    printk("smarthome:ap3216c_driver rmmod success\n");
    return 0;
}

static struct i2c_driver ap3216c_i2c_driver = {
    .driver = {
        .name = "ap3216c",
        .of_match_table = of_match_ptr(ap3216c_of_match),
    },
    .probe    = ap3216c_probe,
    .remove   = ap3216c_remove,
    .id_table = ap3216c_id,
};

//i2c_add_driver(driver)​
//i2c_del_driver(struct i2c_driver *)​
module_i2c_driver(ap3216c_i2c_driver);
MODULE_LICENSE("GPL v2");
