#include "v4l2.h"
#include "ui_v4l2.h"
#include <QDebug>

V4l2::V4l2(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::V4l2)
{
    ui->setupUi(this);

    //打开或者停止采集按钮
    ui->open_button->setCheckable(true);
    ui->open_button->setChecked(true);  // 初始状态为选中

    /*相册页*/
    photo = new Photo(this);
    photo->setCursor(Qt::BlankCursor); // 隐藏鼠标指针
    connect(photo,&Photo::backto_video_page,this,&V4l2::show_video_page);
    connect(this,&V4l2::start_photo_album,photo,&Photo::init_photo_album);

    v4l2Camera = new V4l2Camera(this);
    connect(ui->open_button, SIGNAL(clicked(bool)), v4l2Camera, SLOT(set_thread_start(bool)));
    connect(ui->open_button, SIGNAL(clicked(bool)), this, SLOT(set_open_button_text(bool)));

    //默认一进来这个页面就开始采集
    ui->open_button->setText("停止");
    v4l2Camera->start();
    connect(v4l2Camera,&V4l2Camera::image_ready,this,&V4l2::show_video);

    /*拍照按钮*/
    connect(ui->takephoto_button, SIGNAL(clicked()), v4l2Camera, SLOT(take_photo()));
    connect(v4l2Camera,&V4l2Camera::image_photo,this,&V4l2::show_currentphoto);
}

V4l2::~V4l2()
{
    delete ui;
}


/**
 * @brief V4l2::on_photo_button_clicked
 * 本页隐藏相册页显示
 */
void V4l2::on_photo_button_clicked()
{
    this->hide();
    photo->show();

    //当每次点击这个进入相册页的时候就初始化一次相册的数据
    emit start_photo_album(0);
}

/**
 * @brief V4l2::on_back_tomain_button_clicked
 * 返回主页
 */
void V4l2::on_back_tomain_button_clicked()
{
    /*返回主页*/
   emit backto_main_page();
}

/**
 * @brief V4l2::show_video_page
 * 本页显示相册页隐藏
 */
void V4l2::show_video_page()
{
    this->show();
    photo->hide();
}

/**
 * @brief V4l2::set_open_button_text
 * @param start
 * 改变开始/停止采集按钮文本
 */
void V4l2::set_open_button_text(bool start)
{
    if(start){
        ui->open_button->setText("停止");
    }else{
        ui->open_button->setText("开始");
    }
}

/**
 * @brief V4l2::show_video
 * 显示图像
 */
void V4l2::show_video(QImage image)
{
    ui->video_text_label->setPixmap(QPixmap::fromImage(image));
}

/**
 * @brief V4l2::show_currentphoto
 * 拍照效果
 */
void V4l2::show_currentphoto()
{
    ui->video_text_label->clear();
}

