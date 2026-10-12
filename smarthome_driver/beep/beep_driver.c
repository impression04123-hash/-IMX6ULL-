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

static struct class * beep_class;

struct beep_device
{
    dev_t deviceid;
    struct cdev beep_cdev;
    struct device * beep_dev;
    struct gpio_desc * beep_gpio;
};

static ssize_t beep_device_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
#if 0
    //不知道为什么实现不了read,不管怎么弄read出来的值都是不会变的  
    int err;
    char status = -1;//不要用int会有问题

    struct beep_device * pbeep = filp->private_data;
    status = gpiod_get_value(pbeep->beep_gpio);

    err = copy_to_user(buf, &status, 1);
    if (err) {
        return -EFAULT;  // 如果复制失败，返回 -EFAULT
    }
#endif
    return 1;
}

static ssize_t beep_device_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    char status;

    struct beep_device * pbeep = filp->private_data;
    gpiod_direction_output(pbeep->beep_gpio, 0);//设置输出方向

    err = copy_from_user(&status, buf, 1);
    gpiod_set_value(pbeep->beep_gpio,status);//也可以不要上面的设置输出方向直接gpiod_direction_output(pbeep->beep_gpio, status);

    return 1;
}

static int beep_device_open (struct inode *node, struct file *filp)
{

    struct beep_device * pbeep = container_of(node->i_cdev, struct beep_device, beep_cdev);
    filp->private_data = pbeep;//将pbeep放入file中供ioctl使用
  
    return 0;
}
static int beep_device_release (struct inode *node, struct file *filp)
{
    //printk("beep_device_release funcation run ...\n");
    return 0;
}

static const struct file_operations beep_device_ops = {
    .owner = THIS_MODULE,
    .open = beep_device_open,
    .release = beep_device_release,
    .read = beep_device_read,
    .write = beep_device_write,
};

static int beep_driver_probe(struct platform_device * pdev)
{
    int err = 0;
    struct beep_device * pbeep;
    //分配
    pbeep = kmalloc(sizeof(*pbeep), GFP_KERNEL);
    if(!pbeep){
        err = -ENOMEM;
        goto err_malloc;
    }

    //从设备树中获取gpio资源,在设备树中，必定有一属性名为"beep-gpios"或"beep-gpio"
    pbeep->beep_gpio = gpiod_get(&pdev->dev, "beep", GPIOD_OUT_LOW);
    if (IS_ERR(pbeep->beep_gpio)) {
        printk("Fail to get GPIO for beep\n");
        goto err_gpio_get;
    }

    err = alloc_chrdev_region(&pbeep->deviceid, 0, 1, "beep_device");
    if(err){
        printk("fail to alloc_chrdev_region\n");
        goto err_alloc_chrdev_region;
    }

    cdev_init(&pbeep->beep_cdev,&beep_device_ops);
    err = cdev_add(&pbeep->beep_cdev,pbeep->deviceid,1);
    if(err){
        printk("fail to cdev_add\n");
        goto err_cdev_add;
    }

    beep_class = class_create(THIS_MODULE,"beep_class");
    if(IS_ERR(beep_class)){
        err = PTR_ERR(beep_class);
        printk("fail to class create!\n");
        goto err_class_create;
    }

    pbeep->beep_dev = device_create(beep_class,NULL,pbeep->deviceid,NULL,"beep_device");
    if(IS_ERR(pbeep->beep_dev)){
        err = PTR_ERR(pbeep->beep_dev);
        printk("fail to device_create");
        goto err_device_create;
    }

    //将字符设备添加到platform中这个是给其他函数用,比如remove方法用的
    platform_set_drvdata(pdev, pbeep);

    printk("smarthome:beep_driver insmod success\n");

    return err;

err_device_create:
    class_destroy(beep_class);
err_class_create:
    cdev_del(&pbeep->beep_cdev);
err_cdev_add:
    unregister_chrdev_region(pbeep->deviceid,1);
err_alloc_chrdev_region:
    gpiod_put(pbeep->beep_gpio);
err_gpio_get:
    kfree(pbeep);
err_malloc:
        return err;
}

static int beep_driver_remove(struct platform_device *pdev)
{
    struct beep_device * pbeep = platform_get_drvdata(pdev);
    device_destroy(beep_class, pbeep->deviceid);
    class_destroy(beep_class);
    cdev_del(&pbeep->beep_cdev);
    unregister_chrdev_region(pbeep->deviceid, 1);
    gpiod_put(pbeep->beep_gpio);
    kfree(pbeep);

     printk("smarthome:beep_driver rmmod success\n");
    return 0;
}

static const struct of_device_id beep_of_match[]={
    {
        .compatible = "smarthome_beep",//这个名字不能写错,驱动就是通过这个名字去设备树找到设备资源的
    },
    {}
};

MODULE_DEVICE_TABLE(of, beep_of_match);
struct platform_driver beep_driver = {
    .driver = {
        .name = "smarthome_beep",
        .owner = THIS_MODULE,
        .of_match_table = beep_of_match,
    },
    .probe = beep_driver_probe,
    .remove = beep_driver_remove,
};

static int beep_drviver_init(void)
{
    int err;
    err = platform_driver_register(&beep_driver);
    if(err){
        printk("fail to platform_driver_register");
        return -1;
    }

    return 0;
}

static void beep_drvier_exit(void)
{
    platform_driver_unregister(&beep_driver);
    return ;
}

module_init(beep_drviver_init);
module_exit(beep_drvier_exit);
MODULE_LICENSE("GPL v2");