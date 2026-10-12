#ifndef MYFFMPEG_H
#define MYFFMPEG_H

#include <QObject>
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libswscale/swscale.h>
#include <libavutil/frame.h>
}

#include <QObject>
#include <QImage>
#include <QDebug>

class MyFFmpeg : public QObject
{
    Q_OBJECT
public:
    explicit MyFFmpeg(QObject *parent = nullptr);
    ~MyFFmpeg();

    void MyFFmpegSetUrl(QString rtspUrl);
    int MyFFmpegInit();
    void MyFFmpegDestroy();
    int MyFFmpepReadFrame();
    void MyFFmpegInfo();

signals:
    void MyFFmpegSigGetOneFrame(QImage img);

private:
    int              m_videoIndex;
    QString          m_rtspUrl;
    AVCodec         *m_AVCodec;
    AVFormatContext *m_AVFormatContext;
    AVCodecContext  *m_AVCodecContext;
    AVFrame         *m_AVFrame;
    AVFrame         *m_AVFrameRGB;
    AVPacket        *m_AVPacket;
    SwsContext      *m_SwsContext;
    uint8_t         *m_OutBuffer;
};

#endif // MYFFMPEG_H
