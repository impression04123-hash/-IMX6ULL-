#include "linux/err.h"
#include "linux/slab.h"
#include <linux/module.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/pwm.h>
#include <linux/uaccess.h>
#include <linux/platform_device.h>

static struct class * sg90_class;

struct sg90_device
{
    dev_t deviceid;
    struct cdev sg90_cdev;
    struct device * sg90_dev;
    struct pwm_device *sg90_pwm_device;
};

static ssize_t sg90_device_read (struct file *filp, char __user *buf, size_t size, loff_t *offset)
{
    return 0;
}

static ssize_t sg90_device_write (struct file *filp, const char __user *buf, size_t size, loff_t *offset)
{
    int err;
    unsigned char data[1];
    struct sg90_device * psg90 = filp->private_data;

    err = copy_from_user(data, buf, size);
    if (err) {
        return -EFAULT;  // 如果复制失败，返回 -EFAULT
    }

    pwm_config(psg90->sg90_pwm_device, 500000 + data[0] * 100000 /9, 20000000);//转到任意的角度

    return 1;
}

static int sg90_device_open (struct inode *node, struct file *filp)
{

    struct sg90_device * psg90 = container_of(node->i_cdev, struct sg90_device, sg90_cdev);
    filp->private_data = psg90;//将psg90放入file中供ioctl使用
    
    pwm_config(psg90->sg90_pwm_device, 500000, 20000000);   /* 配置PWM：0.5ms，0度，周期：20000000ns = 20ms */
    pwm_set_polarity(psg90->sg90_pwm_device, PWM_POLARITY_NORMAL); /* 设置输出极性：占空比为高电平 */
    pwm_enable(psg90->sg90_pwm_device);    /* 使能PWM输出 */

    return 0;
}


static int sg90_device_release (struct inode *node, struct file *filp)
{

#if 0
    struct sg90_device * psg90 = filp->private_data;
    
    pwm_config(psg90->sg90_pwm_device, 500000, 20000000);   /* 配置PWM：0.5ms，0度，周期：20000000ns = 20ms */
    pwm_free(psg90->sg90_pwm_device);    /* 使能PWM输出 */
#endif
    return 0;
}

static const struct file_operations sg90_device_ops = {
    .owner = THIS_MODULE,
    .open = sg90_device_open,
    .release = sg90_device_release,
    .read = sg90_device_read,
    .write = sg90_device_write,
};

static int sg90_driver_probe(struct platform_device * pdev)
{
    int err = 0;
    struct sg90_device * psg90;
    //分配
    psg90 = kmalloc(sizeof(*psg90), GFP_KERNEL);
    if(!psg90){
        err = -ENOMEM;
        goto err_malloc;
    }

    err = alloc_chrdev_region(&psg90->deviceid, 0, 1, "sg90_device");
    if(err){
        printk("fail to alloc_chrdev_region\n");
        goto err_alloc_chrdev_region;
    }

    cdev_init(&psg90->sg90_cdev,&sg90_device_ops);
    err = cdev_add(&psg90->sg90_cdev,psg90->deviceid,1);
    if(err){
        printk("fail to cdev_add\n");
        goto err_cdev_add;
    }

    sg90_class = class_create(THIS_MODULE,"sg90_class");
    if(IS_ERR(sg90_class)){
        err = PTR_ERR(sg90_class);
        printk("fail to class create!\n");
        goto err_class_create;
    }

    psg90->sg90_dev = device_create(sg90_class,NULL,psg90->deviceid,NULL,"sg90_device");
    if(IS_ERR(psg90->sg90_dev)){
        err = PTR_ERR(psg90->sg90_dev);
        printk("fail to device_create");
        goto err_device_create;
    }

    psg90->sg90_pwm_device = devm_of_pwm_get(&pdev->dev, pdev->dev.of_node, NULL);
    if(IS_ERR(psg90->sg90_pwm_device)){
        printk(KERN_ERR" pwm_test,get pwm  error!!\n");
        goto err_devm_pwm_get;
    }

    //将字符设备添加到platform中这个是给其他函数用,比如remove方法用的
    platform_set_drvdata(pdev, psg90);

    printk("smarthome:sg90_driver insmod success\n");
    return err;

err_devm_pwm_get:
    device_destroy(sg90_class, psg90->deviceid);
err_device_create:
    class_destroy(sg90_class);
err_class_create:
    cdev_del(&psg90->sg90_cdev);
err_cdev_add:
    unregister_chrdev_region(psg90->deviceid,1);
err_alloc_chrdev_region:
    kfree(psg90);
err_malloc:
        return err;
}

static int sg90_driver_remove(struct platform_device *pdev)
{
    struct sg90_device * psg90 = platform_get_drvdata(pdev);

    pwm_free(psg90->sg90_pwm_device);    
    device_destroy(sg90_class, psg90->deviceid);
    class_destroy(sg90_class);
    cdev_del(&psg90->sg90_cdev);
    unregister_chrdev_region(psg90->deviceid, 1);
    kfree(psg90);

     printk("smarthome:sg90_driver rmmod success\n");
    return 0;
}

static const struct of_device_id sg90_of_match[]={
    {
        .compatible = "smarthome_sg90",//这个名字不能写错,驱动就是通过这个名字去设备树找到设备资源的
    },
    {}
};

MODULE_DEVICE_TABLE(of, sg90_of_match);
struct platform_driver sg90_driver = {
    .driver = {
        .name = "smarthome_sg90",
        .owner = THIS_MODULE,
        .of_match_table = sg90_of_match,
    },
    .probe = sg90_driver_probe,
    .remove = sg90_driver_remove,
};

static int sg90_drviver_init(void)
{
    int err;
    err = platform_driver_register(&sg90_driver);
    if(err){
        printk("fail to platform_driver_register");
        return -1;
    }

    return 0;
}

static void sg90_drvier_exit(void)
{
    platform_driver_unregister(&sg90_driver);
    return ;
}

module_init(sg90_drviver_init);
module_exit(sg90_drvier_exit);
MODULE_LICENSE("GPL v2");