/**
 * @file sr501_driver.c
 * @author your name (you@domain.com)
 * @brief   1:要记得释放irq,需要连参数一起释放和删除timer,就是probe中的资源都要释放了
 *          2:gpiod_get这里要注意方向,这里是in的方向
 * 
 * @version 0.1
 * @date 2024-08-04
 * 
 * 
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/cdev.h>
#include <linux/gpio/consumer.h>

static struct class * sr501_class;

struct sr501_device
{
    dev_t deviceid;
    struct cdev sr501_cdev;
    struct device * sr501_dev;
    struct gpio_desc * sr501_gpio;
    struct fasync_struct *sr501_fasync;
    struct timer_list sr501_timer;
    int irq;
};

static irqreturn_t sr501_handler(int irq, void *dev_id)
{
    struct sr501_device *psr501 = (struct sr501_device *)dev_id;

    unsigned long expires = jiffies + msecs_to_jiffies(200);
    mod_timer(&psr501->sr501_timer, expires);
    return IRQ_HANDLED;
}

static void timeout_handler(unsigned long data)

{   
    struct sr501_device *psr501 = (struct sr501_device *)data;

    // 需要确保 fasync_struct 指针在使用前是有效的
    if (psr501->sr501_fasync) {
        kill_fasync(&psr501->sr501_fasync, SIGIO, POLL_IN);   //异步通知
    }
    return;
}

static ssize_t sr501_device_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
    int err;
    char status = -1;//不要用int会有问题

    struct sr501_device * psr501 = filp->private_data;
    status = gpiod_get_value(psr501->sr501_gpio);

    err = copy_to_user(buf, &status, 1);
    if (err) {
        return -EFAULT;  // 如果复制失败，返回 -EFAULT
    }

    return 1;
}

#if 0
static ssize_t sr501_device_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    char status;

    struct sr501_device * psr501 = filp->private_data;
    gpiod_direction_output(psr501->sr501_gpio, 0);//设置输出方向

    err = copy_from_user(&status, buf, 1);
    gpiod_set_value(psr501->sr501_gpio,status);//也可以不要上面的设置输出方向直接gpiod_direction_output(psr501->sr501_gpio, status);

    return 1;
}
#endif

static int sr501_device_open (struct inode *node, struct file *filp)
{

    struct sr501_device * psr501 = container_of(node->i_cdev, struct sr501_device, sr501_cdev);
    filp->private_data = psr501;//将psr501放入file中供ioctl使用
  
    return 0;
}

static int sr501_device_fasync (int fd, struct file *filp, int on)
{
    int ret;
    struct sr501_device * psr501 = filp->private_data;

    ret = fasync_helper(fd, filp, on, &psr501->sr501_fasync);
    if(ret < 0){
        return ret;
    }
    return 0;
}

static int sr501_device_release (struct inode *node, struct file *filp)
{
    //printk("sr501_device_release funcation run ...\n");
    //struct sr501_device * psr501 = filp->private_data;

    // 释放 fasync 结构，防止内存泄漏或非法访问
    sr501_device_fasync(-1, filp, 0);
    return 0;
}

static const struct file_operations sr501_device_ops = {
    .owner = THIS_MODULE,
    .open = sr501_device_open,
    .release = sr501_device_release,
    .read = sr501_device_read,
    .fasync = sr501_device_fasync,
};

static int sr501_driver_probe(struct platform_device * pdev)
{
    int err = 0;
    struct sr501_device * psr501;
    //分配
    psr501 = kmalloc(sizeof(*psr501), GFP_KERNEL);
    if(!psr501){
        err = -ENOMEM;
        goto err_malloc;
    }
    // 初始化 fasync 结构指针为 NULL
    psr501->sr501_fasync = NULL;
    
    //将字符设备添加到platform中这个是给其他函数用,比如remove方法用的
    platform_set_drvdata(pdev, psr501);

    //从设备树中获取gpio资源,在设备树中，必定有一属性名为"sr501-gpios"或"sr501-gpio"
    psr501->sr501_gpio = gpiod_get(&pdev->dev, "sr501", GPIOD_IN);//这里要搞成IN否则中断不会触发
    if (IS_ERR(psr501->sr501_gpio)) {
        printk("Fail to get GPIO for sr501\n");
        goto err_gpio_get;
    }

    err = alloc_chrdev_region(&psr501->deviceid, 0, 1, "sr501_device");
    if(err){
        printk("fail to alloc_chrdev_region\n");
        goto err_alloc_chrdev_region;
    }

    cdev_init(&psr501->sr501_cdev,&sr501_device_ops);
    err = cdev_add(&psr501->sr501_cdev,psr501->deviceid,1);
    if(err){
        printk("fail to cdev_add\n");
        goto err_cdev_add;
    }

    sr501_class = class_create(THIS_MODULE,"sr501_class");
    if(IS_ERR(sr501_class)){
        err = PTR_ERR(sr501_class);
        printk("fail to class create!\n");
        goto err_class_create;
    }

    psr501->sr501_dev = device_create(sr501_class,NULL,psr501->deviceid,NULL,"sr501_device");
    if(IS_ERR(psr501->sr501_dev)){
        err = PTR_ERR(psr501->sr501_dev);
        printk("fail to device_create");
        goto err_device_create;
    }

    psr501->irq = gpiod_to_irq(psr501->sr501_gpio);
    printk("irq = %d\n",psr501->irq);
    err = request_irq(psr501->irq, sr501_handler, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "sr501_irq", psr501);
    if(err){
        printk("fail to request irq!\n");
        goto err_irq;
    }

    setup_timer(&psr501->sr501_timer, timeout_handler, (unsigned long)psr501);
    psr501->sr501_timer.expires = 0;
	add_timer(&psr501->sr501_timer);

    printk("smarthome:sr501_driver insmod success\n");

    return err;

err_irq:
err_device_create:
    class_destroy(sr501_class);
err_class_create:
    cdev_del(&psr501->sr501_cdev);
err_cdev_add:
    unregister_chrdev_region(psr501->deviceid,1);
err_alloc_chrdev_region:
    gpiod_put(psr501->sr501_gpio);
err_gpio_get:
    kfree(psr501);
err_malloc:
        return err;
}

static int sr501_driver_remove(struct platform_device *pdev)
{
    struct sr501_device * psr501 = platform_get_drvdata(pdev);

    free_irq(psr501->irq, psr501);
    del_timer(&psr501->sr501_timer);
    device_destroy(sr501_class, psr501->deviceid);
    class_destroy(sr501_class);
    cdev_del(&psr501->sr501_cdev);
    unregister_chrdev_region(psr501->deviceid, 1);
    gpiod_put(psr501->sr501_gpio);
    kfree(psr501);

    printk("smarthome:sr501_driver rmmod success\n");
    return 0;
}

static const struct of_device_id sr501_of_match[]={
    {
        .compatible = "smarthome_sr501",//这个名字不能写错,驱动就是通过这个名字去设备树找到设备资源的
    },
    {}
};

MODULE_DEVICE_TABLE(of, sr501_of_match);
struct platform_driver sr501_driver = {
    .driver = {
        .name = "smarthome_sr501",
        .owner = THIS_MODULE,
        .of_match_table = sr501_of_match,
    },
    .probe = sr501_driver_probe,
    .remove = sr501_driver_remove,
};

static int sr501_drviver_init(void)
{
    int err;
    err = platform_driver_register(&sr501_driver);
    if(err){
        printk("fail to platform_driver_register");
        return -1;
    }

    return 0;
}

static void sr501_drvier_exit(void)
{
    platform_driver_unregister(&sr501_driver);
    return ;
}

module_init(sr501_drviver_init);
module_exit(sr501_drvier_exit);
MODULE_LICENSE("GPL v2");