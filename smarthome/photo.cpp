#include "photo.h"
#include "ui_photo.h"
#include "v4l2.h"
#include <QDebug>

Photo::Photo(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Photo)
{
    ui->setupUi(this);
}

Photo::~Photo()
{
    delete ui;
}



void Photo::init_photo_album(int index)
{
    refresh_filelist(index);
}

/**
 * @brief Photo::init_photo_album
 * 每次进来都可重新获取到当前的文件列表
 */
void Photo::refresh_filelist(int index)
{
    dir.setPath("./photo/"); // 设置目录路径
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot); // 过滤条件，只获取文件
    dir.setNameFilters(QStringList() << "*.jpg"); // 文件名过滤，只获取jpg文件
    dir.setSorting(QDir::Time | QDir::Reversed);  // 根据修改时间进行排序，逆序排列
    file_list = dir.entryList(); // 获取文件列表

    qDebug()<<"refresh_filelist:index = "<<index;
    qDebug()<<"refresh_filelist:size = "<<file_list.size();

    //先显示一张图片
    if(file_list.size() > 0){
        current_index = index;
        if(current_index >= file_list.size()){
            current_index = 0;
        }

        QString file_path = dir.filePath(file_list[current_index]); // 获取文件路径
        qDebug()<<"file_path:"<<file_path;

        QImageReader reader(file_path);
        reader.setFormat(QByteArray("RGB16"));
        QImage image = reader.read();// 创建图片对象
        ui->photo_text_label->setPixmap(QPixmap::fromImage(image)); // 在控件上显示图片

    }else{
        ui->photo_text_label->setText("无图片");
    }
}

/**
 * @brief Photo::on_back_tovideo_button_clicked
 * 返回视频页
 */
void Photo::on_back_tovideo_button_clicked()
{
    /*视频页*/
    emit backto_video_page();
}



/**
 * @brief Photo::on_previous_button_clicked
 * 上一张
 */
void Photo::on_previous_button_clicked()
{
    if(file_list.size() > 0){
        current_index--; // 更新当前显示的文件索引
        if (current_index < 0) {
            current_index = file_list.size()-1;
        }

        QString file_path = dir.filePath(file_list[current_index]); // 获取文件路径
        qDebug()<<"file_path:"<<file_path;

        QImageReader reader(file_path);
        reader.setFormat(QByteArray("RGB16"));
        QImage image = reader.read();// 创建图片对象
        ui->photo_text_label->setPixmap(QPixmap::fromImage(image)); // 在控件上显示图片


    }
}

/**
 * @brief Photo::on_next_button_clicked
 * 下一张
 */
void Photo::on_next_button_clicked()
{

    if(file_list.size() > 0){
        current_index++; // 更新当前显示的文件索引
        if (current_index >= file_list.size()) {
            current_index = 0;
        }

        QString file_path = dir.filePath(file_list[current_index]); // 获取文件路径
        qDebug()<<"file_path:"<<file_path;

        QImageReader reader(file_path);
        reader.setFormat(QByteArray("RGB16"));
        QImage image = reader.read();// 创建图片对象
        ui->photo_text_label->setPixmap(QPixmap::fromImage(image)); // 在控件上显示图片


    }
}

/**
 * @brief Photo::on_delete_button_clicked
 * 删除当前照片
 */
void Photo::on_delete_button_clicked()
{
     if(file_list.size() > 0){
         QString file_path = dir.filePath(file_list[current_index]); // 获取文件路径

         // 创建一个 QFile 对象并打开文件
         QFile file(file_path);
         // 检查文件是否存在
         if (!file.exists()) {
             qDebug() << "The file does not exist:" << file_path;
             return;
         }

         // 删除文件
         bool success = file.remove();
         if (success) {
             qDebug() << "File deleted successfully:" << file_path;
             //显示下一张,因为已经删掉了,这个方法内会重新获取列表,所以还是传这个current_index
             refresh_filelist(current_index);
         } else {
             qDebug() << "Failed to delete the file:" << file_path;
         }
     }

     return;
}
