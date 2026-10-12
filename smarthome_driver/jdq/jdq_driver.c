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

static struct class * jdq_class;

struct jdq_device
{
    dev_t deviceid;
    struct cdev jdq_cdev;
    struct device * jdq_dev;
    struct gpio_desc * jdq_gpio;
};

static ssize_t jdq_device_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
#if 0
    //不知道为什么实现不了read,不管怎么弄read出来的值都是不会变的
    int err;
    char status = -1;//不要用int会有问题

    struct jdq_device * pjdq = filp->private_data;
    status = gpiod_get_value(pjdq->jdq_gpio);
    printk("read:status:%d\n",status);

    err = copy_to_user(buf, &status, 1);
    if (err) {
        return -EFAULT;  // 如果复制失败，返回 -EFAULT
    }
#endif
    return 1;
}

static ssize_t jdq_device_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    char status;

    struct jdq_device * pjdq = filp->private_data;
    gpiod_direction_output(pjdq->jdq_gpio, 0);//设置输出方向

    err = copy_from_user(&status, buf, 1);
    gpiod_set_value(pjdq->jdq_gpio,status);//也可以不要上面的设置输出方向直接gpiod_direction_output(pjdq->jdq_gpio, status);

    return 1;
}

static int jdq_device_open (struct inode *node, struct file *filp)
{

    struct jdq_device * pjdq = container_of(node->i_cdev, struct jdq_device, jdq_cdev);
    filp->private_data = pjdq;//将pjdq放入file中供ioctl使用
  

    return 0;   
}
static int jdq_device_release (struct inode *node, struct file *filp)
{
    //printk("jdq_device_release funcation run ...\n");
    return 0;
}

static const struct file_operations jdq_device_ops = {
    .owner = THIS_MODULE,
    .open = jdq_device_open,
    .release = jdq_device_release,
    .read = jdq_device_read,
    .write = jdq_device_write,
};

static int jdq_driver_probe(struct platform_device * pdev)
{
    int err = 0;
    struct jdq_device * pjdq;
    //分配
    pjdq = kmalloc(sizeof(*pjdq), GFP_KERNEL);
    if(!pjdq){
        err = -ENOMEM;
        goto err_malloc;
    }

    //从设备树中获取gpio资源,在设备树中，必定有一属性名为"jdq-gpios"或"jdq-gpio"
    pjdq->jdq_gpio = gpiod_get(&pdev->dev, "jdq", GPIOD_OUT_LOW);
    if (IS_ERR(pjdq->jdq_gpio)) {
        printk("Fail to get GPIO for jdq\n");
        goto err_gpio_get;
    }

    err = alloc_chrdev_region(&pjdq->deviceid, 0, 1, "jdq_device");
    if(err){
        printk("fail to alloc_chrdev_region\n");
        goto err_alloc_chrdev_region;
    }

    cdev_init(&pjdq->jdq_cdev,&jdq_device_ops);
    err = cdev_add(&pjdq->jdq_cdev,pjdq->deviceid,1);
    if(err){
        printk("fail to cdev_add\n");
        goto err_cdev_add;
    }

    jdq_class = class_create(THIS_MODULE,"jdq_class");
    if(IS_ERR(jdq_class)){
        err = PTR_ERR(jdq_class);
        printk("fail to class create!\n");
        goto err_class_create;
    }

    pjdq->jdq_dev = device_create(jdq_class,NULL,pjdq->deviceid,NULL,"jdq_device");
    if(IS_ERR(pjdq->jdq_dev)){
        err = PTR_ERR(pjdq->jdq_dev);
        printk("fail to device_create");
        goto err_device_create;
    }

    //将字符设备添加到platform中这个是给其他函数用,比如remove方法用的
    platform_set_drvdata(pdev, pjdq);

    printk("smarthome:jdq_driver insmod success\n");

    return err;

err_device_create:
    class_destroy(jdq_class);
err_class_create:
    cdev_del(&pjdq->jdq_cdev);
err_cdev_add:
    unregister_chrdev_region(pjdq->deviceid,1);
err_alloc_chrdev_region:
    gpiod_put(pjdq->jdq_gpio);
err_gpio_get:
    kfree(pjdq);
err_malloc:
        return err;
}

static int jdq_driver_remove(struct platform_device *pdev)
{
    struct jdq_device * pjdq = platform_get_drvdata(pdev);
    device_destroy(jdq_class, pjdq->deviceid);
    class_destroy(jdq_class);
    cdev_del(&pjdq->jdq_cdev);
    unregister_chrdev_region(pjdq->deviceid, 1);
    gpiod_put(pjdq->jdq_gpio);
    kfree(pjdq);

     printk("smarthome:jdq_driver rmmod success\n");
    return 0;
}

static const struct of_device_id jdq_of_match[]={
    {
        .compatible = "smarthome_jdq",//这个名字不能写错,驱动就是通过这个名字去设备树找到设备资源的
    },
    {}
};

MODULE_DEVICE_TABLE(of, jdq_of_match);
struct platform_driver jdq_driver = {
    .driver = {
        .name = "smarthome_jdq",
        .owner = THIS_MODULE,
        .of_match_table = jdq_of_match,
    },
    .probe = jdq_driver_probe,
    .remove = jdq_driver_remove,
};

static int jdq_drviver_init(void)
{
    int err;
    err = platform_driver_register(&jdq_driver);
    if(err){
        printk("fail to platform_driver_register");
        return -1;
    }

    return 0;
}

static void jdq_drvier_exit(void)
{
    platform_driver_unregister(&jdq_driver);
    return ;
}

module_init(jdq_drviver_init);
module_exit(jdq_drvier_exit);
MODULE_LICENSE("GPL v2");