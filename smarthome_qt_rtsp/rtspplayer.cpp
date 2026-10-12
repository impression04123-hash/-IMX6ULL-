#include "rtspplayer.h"
#include <QThread>

RTSPPlayer::RTSPPlayer(MyFFmpeg *ffmpeg,QObject *parent) : QObject(parent), m_ffmpeg(ffmpeg)
{

}

void RTSPPlayer::SetPlayerUrl(QString playerUrl)
{
    this->m_playerUrl = playerUrl;
}

void RTSPPlayer::PlayerStart()
{
    if (m_ffmpeg == NULL){
        emit SigOpenUrlResult(RTSPPlayer::FFmpegInitFail);
        return;
    }

    m_ffmpeg->MyFFmpegSetUrl(this->m_playerUrl);
    int ret = m_ffmpeg->MyFFmpegInit();
    if (ret != 0) {
        emit SigOpenUrlResult(RTSPPlayer::FFmpegInitFail);
        return;
    }

    emit SigOpenUrlResult(RTSPPlayer::FFmpegInitSucc);

    m_stopped = false;

    while (1) {
        if (m_stopped){
            qDebug("--------------- stop ----------------");
            break;
        }

        if (!m_ffmpeg) {
            break;
        }

        if (m_ffmpeg->MyFFmpepReadFrame() < 0) {
            qDebug("--------------- get frame fail, stop -----------");
            break;
        }

        QThread::msleep(0.02);
    }

    m_ffmpeg->MyFFmpegDestroy();
    emit SigOpenUrlResult(RTSPPlayer::FFmpegStoped);
    qDebug("---------------- quit -----------------");

    return;
}

void RTSPPlayer::PlayerStop()
{
    this->m_stopped = true;
}
