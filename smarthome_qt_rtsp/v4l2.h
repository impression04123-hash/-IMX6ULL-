#ifndef V4L2_H
#define V4L2_H

#include <QMainWindow>
#include <QImage>
#include "photo.h"
#include "v4l2camera.h"

namespace Ui {
class V4l2;
}

class V4l2 : public QMainWindow
{
    Q_OBJECT

public:
    explicit V4l2(QWidget *parent = nullptr);
    ~V4l2();

public slots:
    void show_video_page(void);
    void set_open_button_text(bool start);
    void show_video(QImage image);
    void show_currentphoto();

signals:
    void backto_main_page(void);
    void start_photo_album(int);


private slots:
    void on_photo_button_clicked();

    void on_back_tomain_button_clicked();

private:
    Ui::V4l2 *ui;
    Photo *photo;
    V4l2Camera *v4l2Camera;
};

#endif // V4L2_H
