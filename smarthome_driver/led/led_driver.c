#include "asm/uaccess.h"
#include "linux/err.h"
#include "linux/gpio/consumer.h"
#include "linux/slab.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>

static struct class * led_class;

struct led_device
{
    dev_t deviceid;
    struct cdev led_cdev;
    struct device * led_dev;
    struct gpio_desc * led_gpio;
};

static ssize_t led_device_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{

#if 0 
    //不知道为什么实现不了read,不管怎么弄read出来的值都是不会变的
    int err;
    char status = -1;//不要用int会有问题

    struct led_device * pled = filp->private_data;

    //gpiod_direction_input(pled->led_gpio);
    status = gpiod_get_value(pled->led_gpio);
    printk("read:status:%d\n",status);

    err = copy_to_user(buf, &status, 1);
    if (err) {
        return -EFAULT;  // 如果复制失败，返回 -EFAULT
    }
#endif
    return 1;
}

static ssize_t led_device_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    char status;

    struct led_device * pled = filp->private_data;
    gpiod_direction_output(pled->led_gpio, 0);//设置输出方向

    err = copy_from_user(&status, buf, 1);
    gpiod_set_value(pled->led_gpio,status);//也可以不要上面的设置输出方向直接gpiod_direction_output(pled->led_gpio, status);

    return 1;
}

static int led_device_open (struct inode *node, struct file *filp)
{

    struct led_device * pled = container_of(node->i_cdev, struct led_device, led_cdev);
    filp->private_data = pled;//将pled放入file中供ioctl使用
  
    return 0;
}
static int led_device_release (struct inode *node, struct file *filp)
{
    //printk("led_device_release funcation run ...\n");
    return 0;
}

static const struct file_operations led_device_ops = {
    .owner = THIS_MODULE,
    .open = led_device_open,
    .release = led_device_release,
    .read = led_device_read,
    .write = led_device_write,
};

static int led_driver_probe(struct platform_device * pdev)
{
    int err = 0;
    struct led_device * pled;
    //分配
    pled = kmalloc(sizeof(*pled), GFP_KERNEL);
    if(!pled){
        err = -ENOMEM;
        goto err_malloc;
    }

    //从设备树中获取gpio资源,在设备树中，必定有一属性名为"led-gpios"或"led-gpio"
    pled->led_gpio = gpiod_get(&pdev->dev, "led", GPIOD_OUT_LOW);
    if (IS_ERR(pled->led_gpio)) {
        printk("Fail to get GPIO for led\n");
        goto err_gpio_get;
    }

    //gpiod_direction_output(pled->led_gpio, 0);   

    err = alloc_chrdev_region(&pled->deviceid, 0, 1, "led_device");
    if(err){
        printk("fail to alloc_chrdev_region\n");
        goto err_alloc_chrdev_region;
    }

    cdev_init(&pled->led_cdev,&led_device_ops);
    err = cdev_add(&pled->led_cdev,pled->deviceid,1);
    if(err){
        printk("fail to cdev_add\n");
        goto err_cdev_add;
    }

    led_class = class_create(THIS_MODULE,"led_class");
    if(IS_ERR(led_class)){
        err = PTR_ERR(led_class);
        printk("fail to class create!\n");
        goto err_class_create;
    }

    pled->led_dev = device_create(led_class,NULL,pled->deviceid,NULL,"led_device");
    if(IS_ERR(pled->led_dev)){
        err = PTR_ERR(pled->led_dev);
        printk("fail to device_create");
        goto err_device_create;
    }

    //将字符设备添加到platform中这个是给其他函数用,比如remove方法用的
    platform_set_drvdata(pdev, pled);

    printk("smarthome:led_driver insmod success\n");

    return err;

err_device_create:
    class_destroy(led_class);
err_class_create:
    cdev_del(&pled->led_cdev);
err_cdev_add:
    unregister_chrdev_region(pled->deviceid,1);
err_alloc_chrdev_region:
    gpiod_put(pled->led_gpio);
err_gpio_get:
    kfree(pled);
err_malloc:
        return err;
}

static int led_driver_remove(struct platform_device *pdev)
{
    struct led_device * pled = platform_get_drvdata(pdev);
    device_destroy(led_class, pled->deviceid);
    class_destroy(led_class);
    cdev_del(&pled->led_cdev);
    unregister_chrdev_region(pled->deviceid, 1);
    gpiod_put(pled->led_gpio);
    kfree(pled);

    printk("smarthome:led_driver rmmod success\n");
    return 0;
}

static const struct of_device_id led_of_match[]={
    {
        .compatible = "smarthome_led",//这个名字不能写错,驱动就是通过这个名字去设备树找到设备资源的
    },
    {}
};

MODULE_DEVICE_TABLE(of, led_of_match);
struct platform_driver led_driver = {
    .driver = {
        .name = "smarthome_led",
        .owner = THIS_MODULE,
        .of_match_table = led_of_match,
    },
    .probe = led_driver_probe,
    .remove = led_driver_remove,
};

static int led_drviver_init(void)
{
    int err;
    err = platform_driver_register(&led_driver);
    if(err){
        printk("fail to platform_driver_register");
        return -1;
    }

    return 0;
}

static void led_drvier_exit(void)
{
    platform_driver_unregister(&led_driver);
    return ;
}

module_init(led_drviver_init);
module_exit(led_drvier_exit);
MODULE_LICENSE("GPL v2");