#ifndef MONITOR_H
#define MONITOR_H

#include <QMainWindow>
#include <QImage>
#include <QThread>
#include "rtspplayer.h"

namespace Ui {
class Monitor;
}

class Monitor : public QMainWindow
{
    Q_OBJECT

public:
    explicit Monitor(QWidget *parent = nullptr);
    ~Monitor();

    void PlayStart();
    void PlayStop();
    void PlayProcess();

private slots:
    void on_return_button_clicked();
    void on_play_button_clicked();//播放/停止按钮

    void SlotOpenUrlResult(int result);
    void SlotGetOneFrame(QImage img);

signals:
    void backto_main_page(void);
    void SigPlayStart();

private:
    Ui::Monitor *ui;
    QString      m_playUrl;
    MyFFmpeg    *m_ffmpeg = nullptr;
    RTSPPlayer  *m_player = nullptr;
    QImage       m_image;
    QThread     *m_playThread = nullptr;
};

#endif // MONITOR_H
