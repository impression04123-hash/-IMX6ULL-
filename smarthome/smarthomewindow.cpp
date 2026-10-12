#include "smarthomewindow.h"
#include "ui_smarthomewindow.h"
#include <sys/signal.h>
#include <QThread>
#include <QDebug>

Ui::SmartHomeWindow * sr501_ui;
SmartHomeWindow::SmartHomeWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SmartHomeWindow)
{
    ui->setupUi(this);
    sr501_ui = ui;

    /* 视频页 */
    v4l2 = new V4l2(this);
    v4l2->setCursor(Qt::BlankCursor); // 隐藏鼠标指针
    connect(v4l2,&V4l2::backto_main_page,this,&SmartHomeWindow::show_main_page);

    /* 设置滑动条控制范围 */
    ui->horizontalSlider->setRange(0, 100);
    /* 设置滑动条控制步长 */
    ui->horizontalSlider->setSingleStep(0);
    /* 关闭滑块跟踪：滑块仅在用户释放滑块时才发出valueChanged()信号 */
    ui->horizontalSlider->setTracking(false);


    led = new Led(this);
    ui->light_button->setCheckable(true);

    beep = new Beep(this);
    ui->beep_button->setCheckable(true);

    jdq = new Jdq(this);
    ui->doorlock_button->setCheckable(true);

    sg90 = new Sg90(this);

    /**
      开启sr501并且注册信号回调函数
      这里为什么要用信号驱动io呢？
      因为你不知道它什么时候才会有数据，它的数据只有发生中断才会有
      但是你不知道它的中断什么时候有阿，所以就要用这种等着它通知的方式来查询
      中断是核心
      */
    sr501 = new Sr501(this);
    signal(SIGIO,sr501_handler);

    //蓝牙
    bluetoothUart = new BluetoothUart(this);
    connect(bluetoothUart,&BluetoothUart::uart_recv,this,&SmartHomeWindow::uart_handler);
    bluetoothUart->start();

    dht11 = new Dht11(this);
    connect(dht11,&Dht11::readyto_read_dht11data,this,&SmartHomeWindow::dht11_handler);

    mq2ADC =new Mq2ADC(this);
    connect(mq2ADC,&Mq2ADC::readyto_read_mq2data,this,&SmartHomeWindow::mq2_handler);

    ap3216c =new Ap3216c(this);
    connect(ap3216c,&Ap3216c::readyto_read_ap3216cdata,this,&SmartHomeWindow::ap3216c_handler);



}

SmartHomeWindow::~SmartHomeWindow()
{
    delete ui;
}

/**
  接收到蓝牙数据之后控制相应的模块
 * @brief SmartHomeWindow::uart_handler
 * @param module
 * @param value
 */
void SmartHomeWindow::uart_handler(char *module, char *value)
{
    char val;
    int int_value = -1;
    if(strcmp(module,"curtain") == 0){
        int_value = atoi(value);
        if(int_value >= 0 && int_value <= 180){//判断数据是否有效,防止蓝牙乱传
            val = 100.0 / 180 * int_value;
            ui->horizontalSlider->setValue(val);
        }

    }else if(strcmp(module,"door") == 0){
        int_value = atoi(value);
        if(int_value >= 0 && int_value <= 1){
            //val = int_value;
            //jdq->jdq_write(&val);

            bool checked = int_value == 1? true:false;
            ui->doorlock_button->setChecked(checked);
            on_doorlock_button_toggled(checked);
        }

    }else if(strcmp(module,"beep") == 0){
        int_value = atoi(value);
        if(int_value >= 0 && int_value <= 1){
            //val = int_value;
            //beep->beep_write(&val);

            bool checked = int_value == 1? true:false;
            ui->beep_button->setChecked(checked);
            on_beep_button_toggled(checked);
        }

    }else if(strcmp(module,"led") == 0){
        int_value = atoi(value);
        if(int_value >= 0 && int_value <= 1){
            //val = int_value;
            //led->led_write(&val);

            bool checked = int_value == 1? true:false;
            ui->light_button->setChecked(checked);
            on_light_button_toggled(checked);
        }
    }
}

/**
  读取温湿度
 * @brief SmartHomeWindow::dht11_handler
 */
void SmartHomeWindow::dht11_handler()
{
    char data[5] = {0};
    dht11->dht11_read(data);
    ui->tamp_value->setNum(data[2]);
    ui->humi_value->setNum(data[0]);

}

/**
  读取空气
 * @brief SmartHomeWindow::mq2_handler
 */
void SmartHomeWindow::mq2_handler()
{
    char data[1] = {0};
    mq2ADC->mq2_read(data);
    ui->air_value->setNum(data[0]);
}

/**
  读取红外,光照,距离
 * @brief SmartHomeWindow::ap3216c_handler
 */
void SmartHomeWindow::ap3216c_handler()
{
    char data[3] = {0};//data[0]---ir,data[1]---als,data[2]---ps
    ap3216c->ap3216c_read(data);

    ui->ir_value->setNum(data[0]);
    ui->als_value->setNum(data[1]);
    ui->dis_value->setNum(data[2]);
}


/**
  控制灯
 * @brief SmartHomeWindow::on_light_button_toggled
 * @param checked
 */
void SmartHomeWindow::on_light_button_toggled(bool checked)
{
    char buf[1];
    if(checked){
        buf[0] = 1;//这里写入1，逻辑有效1
        led->led_write(buf);
    }else{
        buf[0] = 0;
        led->led_write(buf);
    }
}

/**
  控制门铃
 * @brief SmartHomeWindow::on_beep_button_toggled
 * @param checked
 */
void SmartHomeWindow::on_beep_button_toggled(bool checked)
{
    char buf[1];
    if(checked){
        buf[0] = 1;//这里写入1，逻辑有效1
        beep->beep_write(buf);
    }else{
        buf[0] = 0;
        beep->beep_write(buf);
    }
}

/**
  控制门锁
 * @brief SmartHomeWindow::on_doorlock_button_toggled
 * @param checked
 */
void SmartHomeWindow::on_doorlock_button_toggled(bool checked)
{
    char buf[1];
    if(checked){
        buf[0] = 1;//这里写入1，逻辑有效1
        jdq->jdq_write(buf);
    }else{
        buf[0] = 0;
       jdq->jdq_write(buf);
    }
}

/**
 窗帘进度条
 * @brief SmartHomeWindow::on_horizontalSlider_sliderMoved
 * @param position
 */
void SmartHomeWindow::on_horizontalSlider_sliderMoved(int position)
{
    ui->progressBar->setValue(position);
}

void SmartHomeWindow::on_horizontalSlider_valueChanged(int value)
{
    char buf[1] = {0};
    ui->progressBar->setValue(value);
    buf[0] = ui->progressBar->value();
    buf[0] = buf[0] / 100.0 * buf[0];
    sg90->sg90_write(buf);
}

void SmartHomeWindow::on_horizontalSlider_sliderReleased()
{

}

/**
 sr501人体感应
 这个handler需要弄成静态的所以sr501_ui是全局的
 * @brief SmartHomeWindow::sr501_handler
 * @param signum
 */
void SmartHomeWindow::sr501_handler(int signum)
{
    char buf[1] = {0};
    Sr501::sr501_read(buf);
    sr501_ui->move_value->setNum(buf[0]);
}

/**
 * @brief SmartHomeWindow::on_camera_button_clicked
 * 主页隐藏视频页显示
 */
void SmartHomeWindow::on_camera_button_clicked()
{
    this->hide();//主页隐藏,不释放窗口
    v4l2->show();
}

/**
 * @brief SmartHomeWindow::show_main_page
 * 主页显示视频页隐藏
 */
void SmartHomeWindow::show_main_page()
{
    this->show();
    v4l2->hide();
}
