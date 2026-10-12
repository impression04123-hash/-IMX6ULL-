#ifndef PHOTO_H
#define PHOTO_H

#include <QMainWindow>
#include <QDir>
#include <QImageReader>
#include <QImage>

namespace Ui {
class Photo;
}

class Photo : public QMainWindow
{
    Q_OBJECT

public:
    explicit Photo(QWidget *parent = nullptr);
    ~Photo();

    void refresh_filelist(int index = 0);

public slots:
    void init_photo_album(int index = 0);

signals:
    void backto_video_page(void);

private slots:
    void on_back_tovideo_button_clicked();

    void on_previous_button_clicked();

    void on_next_button_clicked();

    void on_delete_button_clicked();

private:
    Ui::Photo *ui;
    QDir dir; // 目录对象
    QStringList file_list; // 存储文件列表
    int current_index = 0; // 当前显示的文件索引
};

#endif // PHOTO_H
