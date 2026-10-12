#ifndef RTSPPLAYER_H
#define RTSPPLAYER_H

#include <QObject>
#include "myffmpeg.h"

class RTSPPlayer : public QObject
{
    Q_OBJECT
public:
    explicit RTSPPlayer(MyFFmpeg *ffmpeg = nullptr,QObject *parent = nullptr);
    void SetPlayerUrl(QString playerUrl);//播放地址从url传到player再传到ffmpeg

    enum PlayerState {
       FFmpegInitFail = 0,
       FFmpegInitSucc,
       FFmpegStoped
    };

signals:
    void SigOpenUrlResult(int result);//发送init ffmpeg的结果

public slots:
    void PlayerStart();
    void PlayerStop();

private:
    volatile bool   m_stopped;				// 停止播放标识，为true时停止播放，退出播放循环
    QString         m_playerUrl;			// 播放url
    MyFFmpeg        *m_ffmpeg;				// MyFFmepg指针

};

#endif // RTSPPLAYER_H
