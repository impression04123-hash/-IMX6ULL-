#include "monitor.h"
#include "ui_monitor.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavformat/version.h>
#include <libavutil/time.h>
#include <libavutil/mathematics.h>
}

Monitor::Monitor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::Monitor)
{
    ui->setupUi(this);

    m_ffmpeg = new MyFFmpeg;
    connect(m_ffmpeg, SIGNAL(MyFFmpegSigGetOneFrame(QImage)), this, SLOT(SlotGetOneFrame(QImage)));

    ui->label_video->setStyleSheet("QLabel{border:2px solid green}");
}

Monitor::~Monitor()
{
    delete ui;
}

void Monitor::PlayStart()
{
    this->PlayStop();//结束之之前的线程

    m_playThread = new QThread();
    m_player = new RTSPPlayer(m_ffmpeg);
    m_player->SetPlayerUrl(m_playUrl);

    connect(this, SIGNAL(SigPlayStart()), m_player, SLOT(PlayerStart()));
    connect(m_player, SIGNAL(SigOpenUrlResult(int)), this, SLOT(SlotOpenUrlResult(int)));

    m_player->moveToThread(m_playThread);
    m_playThread->start();
}

void Monitor::PlayStop()
{
    if (m_player){
        m_player->PlayerStop();
    }

    if (m_playThread){
        m_playThread->quit();
        m_playThread->wait(1000);
        delete m_playThread;
        m_playThread = nullptr;
    }

    if (m_player){
        delete m_player;
        m_player = nullptr;
    }
}

void Monitor::PlayProcess()
{
    this->PlayStart();
    emit SigPlayStart();
}

/**
 * @brief Monitor::on_return_button_clicked
 * 返回主界面
 */
void Monitor::on_return_button_clicked()
{
    /*返回主页*/
   emit backto_main_page();
}

/**
 * @brief Monitor::on_play_button_clicked
 * 播放或者停止按钮
 */
void Monitor::on_play_button_clicked()
{
    if (ui->play_button->text().compare("播放") == 0) {
        //因为做不出一个软件盘,所以直接在代码写死了流地址
        m_playUrl = "rtsp://192.168.1.40:554/stream_ch1.h264";

        ui->play_button->setEnabled(false);
        this->PlayProcess();
    } else if (ui->play_button->text().compare("停止") == 0) {
        ui->play_button->setEnabled(false);
        this->PlayStop();
    }
}

/**
 * @brief Monitor::SlotOpenUrlResult
 * @param result
 * 接收到RTSPPlayer的init ffmpeg的信号之后修改按钮
 */
void Monitor::SlotOpenUrlResult(int result)
{
    if(result == RTSPPlayer::FFmpegInitFail){
        this->PlayStop();
        ui->play_button->setText("停止");
    }else if (result == RTSPPlayer::FFmpegInitSucc){
        ui->play_button->setText("停止");
    }
    else if (result == RTSPPlayer::FFmpegStoped){
        ui->play_button->setText("播放");
    }

    ui->play_button->setEnabled(true);
}

/**
 * @brief Monitor::SlotGetOneFrame
 * @param img
 * 一帧一帧的显示
 */
void Monitor::SlotGetOneFrame(QImage img)
{
     //将图像按比例缩放
     //QImage m_Image = img.scaled(QSize(800, 480),Qt::KeepAspectRatio);
     ui->label_video->setPixmap(QPixmap::fromImage(img));
}
