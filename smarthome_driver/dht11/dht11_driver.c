#include "asm/delay.h"
#include "asm/gpio.h"
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
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/fs.h>
#include <linux/platform_device.h>

#define DHT11_GPIO_HIGH 1
#define DHT11_GPIO_LOW 0

static struct class * dht11_class;

struct dht11_device
{
    dev_t deviceid;
    struct cdev dht11_cdev;
    struct device * dht11_dev;
    int dht11_pin;
    spinlock_t dht11_lock;
};


//读取管脚电平
uint8_t dht_gpioread(int gpio_pin)
{
    uint8_t pin_value;
    pin_value = gpio_get_value(gpio_pin);
	return pin_value;
}


void dht_start(int gpio_pin)
{
	//把总线拉低至少18ms
	gpio_direction_output(gpio_pin, DHT11_GPIO_LOW);
	mdelay(18);
	
	//再把总线拉高20us-40us
	gpio_set_value(gpio_pin, DHT11_GPIO_HIGH);
	udelay(40);
	
    return;
}

//成功响应则返回0 否则返回1
uint8_t dht_check(int gpio_pin)
{
    gpio_direction_input(gpio_pin);

    if (dht_gpioread(gpio_pin)) {
        printk(KERN_ERR "DHT11 sensor not responding\n");
        return 1;
    }
    udelay(80);

    if (!dht_gpioread(gpio_pin)) {
        printk(KERN_ERR "DHT11 sensor not responding\n");
        return 1;
    }
    udelay(80);

    return 0;
}

/*
读取温湿度数据放在数组中
return 0 成功 1失败
*/
uint8_t dht11_getdata(uint8_t *buf,int gpio_pin)
{
    uint8_t i;
	dht_start(gpio_pin);//打电话

	if(dht_check(gpio_pin) == 0){//接电话
        for (i = 0; i < 5; i++) {
            int j;
            for (j = 7; j >= 0; j--) {
                u32 bit;
                while (!dht_gpioread(gpio_pin));
                udelay(50);

                bit = dht_gpioread(gpio_pin);

                if (bit) {
                    buf[i] |= (1 << j);
                    udelay(40);
                }
            }
        }

		//校验
		if(buf[0]+buf[1]+buf[2]+buf[3] == buf[4]){
			return 0;
		}else{
			return 1;
		}
	}else{
		return 1;
	}
	
	return 1;

}


static ssize_t dht11_device_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
    int err;
    unsigned long flags;
    uint8_t recv_buf[5] = {0};

    struct dht11_device * pdht11 = filp->private_data;

    //使用自旋锁保护读取数据
    spin_lock_irqsave(&pdht11->dht11_lock, flags);
    err = dht11_getdata(recv_buf,pdht11->dht11_pin);
    if(err){
        spin_unlock_irqrestore(&pdht11->dht11_lock, flags);
        return -EFAULT;
    }
    spin_unlock_irqrestore(&pdht11->dht11_lock, flags);

    err = copy_to_user(buf, recv_buf, sizeof(recv_buf));
    if (err) {
        return -EFAULT;  // 如果复制失败，返回 -EFAULT
    }

    return sizeof(recv_buf);
}

#if 0
static ssize_t dht11_device_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    char status;

    struct dht11_device * pdht11 = filp->private_data;
    gpiod_direction_output(pdht11->dht11_gpio, 0);//设置输出方向

    err = copy_from_user(&status, buf, 1);
    gpiod_set_value(pdht11->dht11_gpio,status);//也可以不要上面的设置输出方向直接gpiod_direction_output(pdht11->dht11_gpio, status);

    return 1;
}
#endif

static int dht11_device_open (struct inode *node, struct file *filp)
{

    struct dht11_device * pdht11 = container_of(node->i_cdev, struct dht11_device, dht11_cdev);
    filp->private_data = pdht11;//将pdht11放入file中供ioctl使用
  
    return 0;
}
static int dht11_device_release (struct inode *node, struct file *filp)
{
    //printk("dht11_device_release funcation run ...\n");
    return 0;
}

static const struct file_operations dht11_device_ops = {
    .owner = THIS_MODULE,
    .open = dht11_device_open,
    .release = dht11_device_release,
    .read = dht11_device_read,
};

static int dht11_driver_probe(struct platform_device * pdev)
{
    int err = 0;
    struct dht11_device * pdht11;
    enum of_gpio_flags gpio_flags;
    struct device_node *np = pdev->dev.of_node;

    //分配
    pdht11 = kmalloc(sizeof(*pdht11), GFP_KERNEL);
    if(!pdht11){
        err = -ENOMEM;
        goto err_malloc;
    }

    //从设备树中获取gpio资源,在设备树中，必定有一属性名为"dht11-gpios"或"dht11-gpio"
    pdht11->dht11_pin = of_get_named_gpio_flags(np,"dht11-gpios",0,&gpio_flags);
    if(!gpio_is_valid(pdht11->dht11_pin)){
        printk("fail to of_get_named_gpio_flags\n");
        goto err_gpio_get;
    }
    printk("dht11_pin:%d\n",pdht11->dht11_pin);
    err = gpio_request( pdht11->dht11_pin, "dht11_pin");
	if(err < 0) {
		printk("fail to gpio_request\n");
		goto err_malloc;
	}

    err = alloc_chrdev_region(&pdht11->deviceid, 0, 1, "dht11_device");
    if(err){
        printk("fail to alloc_chrdev_region\n");
        goto err_alloc_chrdev_region;
    }

    cdev_init(&pdht11->dht11_cdev,&dht11_device_ops);
    err = cdev_add(&pdht11->dht11_cdev,pdht11->deviceid,1);
    if(err){
        printk("fail to cdev_add\n");
        goto err_cdev_add;
    }

    dht11_class = class_create(THIS_MODULE,"dht11_class");
    if(IS_ERR(dht11_class)){
        err = PTR_ERR(dht11_class);
        printk("fail to class create!\n");
        goto err_class_create;
    }

    pdht11->dht11_dev = device_create(dht11_class,NULL,pdht11->deviceid,NULL,"dht11_device");
    if(IS_ERR(pdht11->dht11_dev)){
        err = PTR_ERR(pdht11->dht11_dev);
        printk("fail to device_create");
        goto err_device_create;
    }

    //初始化自旋锁
    spin_lock_init(&pdht11->dht11_lock);

    //将字符设备添加到platform中这个是给其他函数用,比如remove方法用的
    platform_set_drvdata(pdev, pdht11);

    printk("smarthome:dht11_driver insmod success\n");

    return err;

err_device_create:
    class_destroy(dht11_class);
err_class_create:
    cdev_del(&pdht11->dht11_cdev);
err_cdev_add:
    unregister_chrdev_region(pdht11->deviceid,1);
err_alloc_chrdev_region:
    gpio_free(pdht11->dht11_pin);
err_gpio_get:
    kfree(pdht11);
err_malloc:
        return err;
}

static int dht11_driver_remove(struct platform_device *pdev)
{
    struct dht11_device * pdht11 = platform_get_drvdata(pdev);

    device_destroy(dht11_class, pdht11->deviceid);
    class_destroy(dht11_class);
    cdev_del(&pdht11->dht11_cdev);
    unregister_chrdev_region(pdht11->deviceid, 1);
    gpio_free(pdht11->dht11_pin);
    kfree(pdht11);

    printk("smarthome:dht11_driver rmmod success\n");
    return 0;
}

static const struct of_device_id dht11_of_match[]={
    {
        .compatible = "smarthome_dht11",//这个名字不能写错,驱动就是通过这个名字去设备树找到设备资源的
    },
    {}
};

MODULE_DEVICE_TABLE(of, dht11_of_match);
struct platform_driver dht11_driver = {
    .driver = {
        .name = "smarthome_dht11",
        .owner = THIS_MODULE,
        .of_match_table = dht11_of_match,
    },
    .probe = dht11_driver_probe,
    .remove = dht11_driver_remove,
};

static int dht11_drviver_init(void)
{
    int err;
    err = platform_driver_register(&dht11_driver);
    if(err){
        printk("fail to platform_driver_register");
        return -1;
    }

    return 0;
}

static void dht11_drvier_exit(void)
{
    platform_driver_unregister(&dht11_driver);
    return ;
}

module_init(dht11_drviver_init);
module_exit(dht11_drvier_exit);
MODULE_LICENSE("GPL v2");


#if 0
printf("fuck!");
#endif