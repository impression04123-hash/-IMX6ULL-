#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <asm-generic/gpio.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/delay.h>

#define I2C_DRIVER_NAME             "gt1151q"
#define GT1151Q_PRODUCT_ID_ADDR     0x8140
#define MAX_SUPPORT_POINTS          5
#define TOUCH_SCREEN_WIDTH          800
#define TOUCH_SCREEN_HEIGHT         480
#define GT1151Q_TOUCH_STATUS        0x814E
#define GT911_REAL_CMD_ADDR         0x8040
#define GT1151Q_TOUCH_POINT_ADDR    0x8150

struct gt1151q_device{
    int rst_pin;
    int int_pin;
    struct i2c_client * client;
    struct input_dev *input;
};

/**
 * @brief:  读取gt1151q寄存器数据
 * @param:  client - [参数说明] 
 * @param:  regaddr - [寄存器地址] 
 * @param:  regval - [参数说明] 
 * @return: int - [0成功 其他失败] 
 */
int read_gt1151q_reg(struct i2c_client *client,uint16_t regaddr,uint8_t *regval)
{
    int err;
    struct i2c_msg msg[2];
    uint8_t rx_buf[1];
    uint8_t tx_buf[]= {regaddr >> 8,regaddr & 0xff};

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
        printk("fail to read_gt1151q_reg:i2c_transfer\n");
        return err;
    }

    //将读取到的数据通过指针返回
    *regval = rx_buf[0];

    return 0;
}

/**
 * @brief:  向gt1151q寄存器写入数据
 * @param:  client - [参数说明] 
 * @param:  regaddr - [参数说明] 
 * @param:  regval - [参数说明] 
 * @return: int - [返回值说明] 
 */
int write_gt1151q_reg(struct i2c_client *client,uint16_t regaddr,uint8_t regval)
{
    int err;
    struct i2c_msg msg;
    uint8_t tx_buf[]= {regaddr >> 8,regaddr & 0xff,regval};

    /* 发送要读取的寄存器首地址以及要写入的数据 */
    msg.addr = client->addr;
    msg.flags = !I2C_M_RD;
    msg.len = sizeof(tx_buf)/sizeof(tx_buf[0]);
    msg.buf = tx_buf;//写什么

    err = i2c_transfer(client->adapter, &msg, 1);
    if(err < 0){
        printk("fail to write_gt1151q_reg:i2c_transfer\n");
        return err;
    }

    return 0;
}

/**
 * @brief:  根据数据手册需要将中断引脚弄成输入方向
 * @param:  gt_dev - [参数说明] 
 */
void gt1151q_device_init(struct gt1151q_device * gt_dev)
{
    gpio_direction_input(gt_dev->int_pin);
    return;
}

/**
 * @brief:  读取gt1151q的产品id
 * @param:  gt_dev - [参数说明] 
 */
void read_gt1151q_product_id(struct gt1151q_device *gt_dev)
{
    int err;
    int i;
    uint8_t product_id[4];

    for(i = 0;i < 4;i++){
        err = read_gt1151q_reg(gt_dev->client,GT1151Q_PRODUCT_ID_ADDR+i,&product_id[i]);
        if(err){
            return;
        }
    }
    printk("product id hex:");
    for(i = 0; i < 4; i++){
        printk("%x ",product_id[i]);
    }
    printk("\n");

    printk("product id str:GT%s\r\n",product_id);
    return;
}

/**
 * @brief:  获取设备树的gpio属性
 * @param:  gt_dev - [参数说明] 
 * @return: int - [返回值说明] 
 */
int of_parse_gt1151q_dt(struct gt1151q_device *gt_dev)
{
    struct i2c_client * client = gt_dev->client;
    struct device_node *np = client->dev.of_node;

    gt_dev->int_pin = of_get_named_gpio(np,"goodix_int", 0);
    if(!gpio_is_valid(gt_dev->int_pin)){
        printk("fail to of_parse_gt1151q_dt:of_get_named_gpio:int_pin\n");
        return gt_dev->int_pin;
    }
    gt_dev->rst_pin = of_get_named_gpio(np,"goodix_rst", 0);
    if(!gpio_is_valid(gt_dev->rst_pin)){
        printk("fail to of_parse_gt1151q_dt:of_get_named_gpio:rst_pin\n");
        return gt_dev->rst_pin;
    }

    return 0;
}

/**
 * @brief:  清除中断标志
 * @param:  gt_dev - [参数说明] 
 */
void clear_gt1151q_status(struct gt1151q_device * gt_dev)
{
    int err;
    err = write_gt1151q_reg(gt_dev->client,GT1151Q_TOUCH_STATUS,0);
    if(err){
        printk("fail to gt1151q_clear_status:write_gt1151q_reg\n");
        return;
    }
    return;
}

/**
 * @brief:  读取状态寄存器
 * @param:  gt_dev - [参数说明] 
 * @return: int - [返回值说明] 
 */
int read_gt1151q_status(struct gt1151q_device *gt_dev)
{
    int err;
    uint8_t status;

    err = read_gt1151q_reg(gt_dev->client,GT1151Q_TOUCH_STATUS,&status);
    if(err){
        printk("fail to read_gt1151q_status:read_gt1151q_reg\n");
        return err;
    }

    return status;
}

/**
 * @brief:  开始读取坐标
 * @param:  gt_dev - [参数说明] 
 */
void start_gt1151q_real_cmd(struct gt1151q_device * gt_dev)
{
    int err;
    err = write_gt1151q_reg(gt_dev->client,GT911_REAL_CMD_ADDR,0);
    if(err){
        printk("fail to start_gt1151q_real_cmd:write_gt1151q_reg\n");
        return;
    }
    return;
}

/**
 * @brief:  中断处理函数将坐标上传给linux内核
 * @param:  irq - [参数说明] 
 * @param:  data - [参数说明] 
 * @return: irqreturn_t - [返回值说明] 
 */
static irqreturn_t gt1151q_interrupt_handler(int irq, void *data)
{
    struct gt1151q_device * gt_dev = data;
    uint8_t status;

    status = read_gt1151q_status(gt_dev);
    if(status < 0){
        goto err_return;
    }
    
    if(status & (1 << 7)){
        int i,j;
        uint8_t value[6];
        int number = status & 0xf;

        if(number){
            for(i = 0;i < number;i ++){
                for(j = 0;j < 6;j ++){
                    int err;
                    uint16_t point_addr = GT1151Q_TOUCH_POINT_ADDR + i * 8;

                    err = read_gt1151q_reg(gt_dev->client,point_addr + j,&value[j]);
                    if(err){
                        goto err_return;
                    }
                }

                /*开始上报每一个坐标*/
                input_mt_slot(gt_dev->input,i);
                input_mt_report_slot_state(gt_dev->input,MT_TOOL_FINGER,true);
                input_report_abs(gt_dev->input, ABS_MT_POSITION_X, (value[1] << 8) | value[0]);
                input_report_abs(gt_dev->input, ABS_MT_POSITION_Y, (value[3] << 8) | value[2]);
            }
        }else{
            input_mt_slot(gt_dev->input,0);
            input_mt_report_slot_state(gt_dev->input,MT_TOOL_FINGER,false);
        }

                 input_sync(gt_dev->input);
    }
    /*不要指针仿真*/
    input_mt_report_pointer_emulation(gt_dev->input,false);


err_return:
    clear_gt1151q_status(gt_dev);
    return IRQ_HANDLED;
}

/**
 * @brief:  probe函数
 * @param:  client - [参数说明] 
 * @param:  id - [参数说明] 
 * @return: int - [返回值说明] 
 */
static int i2c_gt1151q_probe(struct i2c_client *client,const struct i2c_device_id *id)
{
    int err;
    struct gt1151q_device * gt_dev;
    gt_dev = devm_kzalloc(&client->dev, sizeof(struct gt1151q_device), GFP_KERNEL);
    if(!gt_dev){
        printk("fail to i2c_gt1151q_probe:devm_kzalloc\n");
        return -ENOMEM;
    }
    gt_dev->client = client;
    i2c_set_clientdata(client, gt_dev);

    err = of_parse_gt1151q_dt(gt_dev);
    if(err){
        printk("fail to of_parse_gt1151q_dt\n");
        return err;
    }

    /*设置中断引脚为输入方向(根据datasheet来)*/
    gt1151q_device_init(gt_dev);

    /*测试一下读取产品id*/
    read_gt1151q_product_id(gt_dev);

    /*input设备*/
    gt_dev->input = devm_input_allocate_device(&client->dev);
    if(!gt_dev->input){
        printk("fail to devm_input_allocate_device\n");
        return -ENOMEM;
    }
    gt_dev->input->name = client->name;
    gt_dev->input->id.bustype = BUS_I2C;
    gt_dev->input->dev.parent = &client->dev;

    /*设置上报事件*/
    __set_bit(EV_SYN, gt_dev->input->evbit);
    __set_bit(EV_KEY, gt_dev->input->evbit);
    __set_bit(EV_ABS, gt_dev->input->evbit);
    __set_bit(BTN_TOUCH, gt_dev->input->keybit);

    /* 设置多点触摸 */
    input_mt_init_slots(gt_dev->input, MAX_SUPPORT_POINTS, 0);  /*触摸点的数量 */
    input_set_abs_params(gt_dev->input, ABS_MT_POSITION_X,0, TOUCH_SCREEN_WIDTH, 0, 0);
    input_set_abs_params(gt_dev->input, ABS_MT_POSITION_Y,0, TOUCH_SCREEN_HEIGHT, 0, 0);

    /*注册input设备*/
    err = input_register_device(gt_dev->input);
    if(err){
        printk("fail to input_register_device\n");
        return err;
    }

    /*注册中断之前先清除中断标志*/
    clear_gt1151q_status(gt_dev);

    /*注册中断*/
    err = devm_request_threaded_irq(&client->dev, 
                                    client->irq, 
                                    NULL,
                                    gt1151q_interrupt_handler, 
                                    IRQF_TRIGGER_FALLING | IRQF_ONESHOT, 
                                    client->name, 
                                    gt_dev);
    if(err){
        printk("fail to devm_request_threaded_irq\n");
        return err; 
    }

    /*开始读取坐标命令*/
    start_gt1151q_real_cmd(gt_dev);

    return 0;
}

static int i2c_gt1151q_remove(struct i2c_client *client)
{
    struct gt1151q_device *gt_dev = i2c_get_clientdata(client);
    input_unregister_device(gt_dev->input);
    return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id i2c_gt1151q_of_match[] = {
    {.compatible = "goodix,gt1151q"},
    {},
};
MODULE_DEVICE_TABLE(of, i2c_gt1151q_of_match);
#endif

static const struct i2c_device_id i2c_gt1151q_id[] = {
    {I2C_DRIVER_NAME, 0},
    {}
};
MODULE_DEVICE_TABLE(i2c, i2c_gt1151q_id);

static struct i2c_driver i2c_gt1151q_driver = {
    .id_table = i2c_gt1151q_id,
    .probe = i2c_gt1151q_probe,
    .remove = i2c_gt1151q_remove,
    .driver = {
        .name = I2C_DRIVER_NAME,
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(i2c_gt1151q_of_match),
    },
};

module_i2c_driver(i2c_gt1151q_driver);
MODULE_LICENSE("GPL v2");
