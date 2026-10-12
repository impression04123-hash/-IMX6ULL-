#include <linux/init.h>
#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <asm/uaccess.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/regmap.h>
  

  
struct rc522_device{
    struct regmap *regmap;
    struct spi_device *spidev;
    int rst_gpio;
    enum of_gpio_flags rst_gpio_flag;
};
  
extern struct miscdevice rc522_device;
  
static const struct of_device_id rc522_dt_ids[] = {
    { .compatible = "smarthome_rc522"},
    { }
};
MODULE_DEVICE_TABLE(of, rc522_dt_ids);
  
static int rc522_open(struct inode *inode, struct file *fp)
{
    int reset_level;
    struct spi_device *spi = container_of(rc522_device.parent,struct spi_device,dev);
    struct rc522_device *rc522 = spi_get_drvdata(spi);
  
    reset_level = rc522->rst_gpio_flag & OF_GPIO_ACTIVE_LOW  ?  0 : 1;
  
    gpio_direction_output(rc522->rst_gpio,reset_level);
    mdelay(100);
    gpio_direction_output(rc522->rst_gpio,!reset_level);
    mdelay(100);
  
    return 0;
}
  
//写寄存器的值:寄存器地址，寄存器数据  
static ssize_t rc522_write(struct file *filp, const char __user *buf,
        size_t count, loff_t *f_pos)
{
    int err;
    uint8_t reg[2];
    struct spi_device *spi = container_of(rc522_device.parent,struct spi_device,dev);
    struct rc522_device *rc522 = spi_get_drvdata(spi);
  
    if(count != 2){
        printk("Inalid data len:%d\n",count);
        return -EINVAL;
    }
  
    if(copy_from_user(reg,buf,count)){
        printk("Fail to copy_from_user\n");
        return -EFAULT;
    }
  
    /*|0|regaddr|0|*/
    reg[0] = (reg[0] & 0x3f) << 1;
    err = regmap_write(rc522->regmap,reg[0],reg[1]);
    if(err){
        printk("Fail to regmap write\n");
        return err;
    }
  
    return sizeof(reg);
}
  
//输入  buf内容:寄存器地址，0  
//输出  buf内容:寄存器地址，寄存器数据  
ssize_t rc522_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
    int err;
    uint8_t reg[2];
    uint32_t regval = 0;
    struct spi_device *spi = container_of(rc522_device.parent,struct spi_device,dev);
    struct rc522_device *rc522 = spi_get_drvdata(spi);
  
    if(size != 2){
        printk("Inalid data len:%d\n",size);
        return -EINVAL;
    }
  
    if(copy_from_user(reg,buf,size)){
        printk("Fail to copy_from_user\n");
        return -EFAULT;
    }
  
    /*|1|regaddr|0|*/
    reg[0] = ((reg[0] & 0x3f) << 1);
  
    err = regmap_read(rc522->regmap,reg[0],&regval);
    if(err){
        printk("Fail to regmap_read");
        return err;
    }
      
    if(copy_to_user((void *)&buf[1], &regval,1)){
        printk("Fail to copy_to_user\n");
        return -EFAULT;
    }
  
    return sizeof(reg);
}
  
static int rc522_release(struct inode *inode, struct file *fp)
{
    return 0;
}
  
static const struct file_operations rc522_fops = {
    .owner  = THIS_MODULE,
    .open   = rc522_open,
    .release= rc522_release,
    .write = rc522_write,
    .read  = rc522_read,
};
  
struct miscdevice rc522_device = {
    .minor = MISC_DYNAMIC_MINOR,
    .name  = "rc522",
    .fops  = &rc522_fops,
};
  
static const struct regmap_config rc522_regmap_config = {
    .reg_bits   = 8,
    .val_bits   = 8,
    .cache_type = REGCACHE_NONE,
    .read_flag_mask = 0x80  
};
  
static int rc522_probe(struct spi_device *spi)
{
    int err;
    int rst_gpio;
    struct rc522_device *rc522;
    enum of_gpio_flags gpio_flag;
    struct device_node *np = spi->dev.of_node;
  
    printk("rc522 probe\n");
  
    spi->bits_per_word = 8;
    spi->mode = SPI_MODE_0;//SPI_MODE_3  
    err = spi_setup(spi);
    if(err){
        printk("Fail to spi_setup\n");
        return err;
    }
  
    rc522 = devm_kmalloc(&spi->dev,sizeof(*rc522),GFP_KERNEL);
    if(!rc522){
        printk("Fail to devm_kmalloc\n");
        return -ENOMEM;
    }
    rc522->spidev = spi;
    spi_set_drvdata(spi,rc522);
  
    rst_gpio = of_get_named_gpio_flags(np,"rst-gpios",0,&gpio_flag);
    if(!gpio_is_valid(rst_gpio)){
        printk("Fail to of_get_named_gpio_flags for rst-gpio\n");
        return rst_gpio;
    }
  
    printk("rst_gpio:%d,flag:%d\n",rst_gpio,gpio_flag);
  
    err = devm_gpio_request(&spi->dev, rst_gpio,"RC522-RST");
    if(err){
        printk("Fail to devm_gpio_request,gpio:%d\n",rst_gpio);
        return err;
    }
  
    rc522->rst_gpio = rst_gpio;
    rc522->rst_gpio_flag = gpio_flag;
  
    /*这侧regmap的spi子系统*/
    rc522->regmap = regmap_init_spi(spi,&rc522_regmap_config);
    if (IS_ERR(rc522->regmap)){
        printk("Fail to regmap_init_spi\n");
        return PTR_ERR(rc522->regmap);
    }
  
    rc522_device.parent = &spi->dev;
    err = misc_register(&rc522_device);
    if(err){
        printk("Fail to misc_register\n");
        return err;
    }
  
    return 0;
}
  
static int rc522_remove(struct spi_device *spi)
{
    struct rc522_device *rc522 = spi_get_drvdata(spi);
  
    printk("rc522 remove\n");
    misc_deregister(&rc522_device);
    regmap_exit(rc522->regmap);
  
    return 0;
}
  
static struct spi_driver rc522_driver = {
    .driver = {
        .name   = "rc522",
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(rc522_dt_ids),
    },
    .probe      = rc522_probe,
    .remove     = rc522_remove,
};
  
module_spi_driver(rc522_driver);
MODULE_LICENSE("GPL v2");
