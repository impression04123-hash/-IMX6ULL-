#include "myffmpeg.h"

/**
 * @brief MyFFmpeg::MyFFmpeg
 * @param parent
 * 初始化ffmpeg,获取视频帧
 */
MyFFmpeg::MyFFmpeg(QObject *parent) : QObject(parent)
{
    MyFFmpegInfo();
}

MyFFmpeg::~MyFFmpeg()
{
    MyFFmpegDestroy();
}

void MyFFmpeg::MyFFmpegSetUrl(QString rtspUrl)
{
    m_rtspUrl = rtspUrl;
}

int MyFFmpeg::MyFFmpegInit()
{
    int i;
    int ret = -1;

    // 获取视频播放URL
    QByteArray byteRtspUrl =m_rtspUrl.toLocal8Bit();
    char *pRtspUrl = byteRtspUrl.data();
    qDebug()<<"pRtspUrl" << pRtspUrl;

    // 初始化所有组件，调用该函数后，才能使用复用器和编解码器
    av_register_all();

    // 初始化网络库
    avformat_network_init();

    // 分配AVFormatContext，它是FFMPEG解封装（flv，mp4，rmvb，avi）功能的结构体，
    // 具体可参考https://blog.csdn.net/leixiaohua1020/article/details/14214705
    m_AVFormatContext = avformat_alloc_context();

    // 设置参数
    AVDictionary *options = NULL;
    // 设置传输协议为UDP协议
    av_dict_set(&options, "rtsp_transport", "udp", 0);

    // 设置UDP连接最大延时时间
    av_dict_set(&options, "max_delay", "500", 0);

    // 设置“buffer_size”缓存容量
    av_dict_set(&options, "buffer_size", "2048000", 0);

    // 设置avformat_open_input超时时间为3秒
    av_dict_set(&options, "stimeout", "3000000", 0);


    // 打开网络流或文件流
    ret = avformat_open_input(&m_AVFormatContext, pRtspUrl, NULL, &options);
    if (ret != 0)
    {
        qDebug("Couldn't open input stream, ret=%d\n", ret);
        return ret;
    }

    // 读取流数据包并获取流的相关信息
    if (avformat_find_stream_info(m_AVFormatContext, NULL) < 0)
    {
        qDebug("Couldn't find stream information.\n");
        return -1;
    }

    // 确定流格式是否为视频
    for (i = 0; i < m_AVFormatContext->nb_streams; i++)
    {
        if (m_AVFormatContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            m_videoIndex = i;
            break;
        }
    }

    if (m_videoIndex == -1)
    {
        qDebug("Didn't find a video stream.\n");
        return -1;
    }

    m_AVCodecContext = m_AVFormatContext->streams[m_videoIndex]->codec;

    // 根据编码器的ID号查找对应的解码器
    m_AVCodec = avcodec_find_decoder(m_AVCodecContext->codec_id);
    if (NULL == m_AVCodec)
    {
        qDebug("avcodec_find_decoder AV_CODEC_ID_H264 fail!\n");
        return -1;
    }


    // 配置编码器上下文的参数
    m_AVCodecContext->bit_rate = 0;         //码率
    m_AVCodecContext->time_base.num = 1; // 下面2行设置帧率，每秒25帧
    m_AVCodecContext->time_base.den = 25;
    m_AVCodecContext->frame_number = 1;     //每包一个视频帧

    // Initialize the AVCodecContext to use the given AVCodec
    if (avcodec_open2(m_AVCodecContext, m_AVCodec, NULL) < 0)
    {
        qDebug("avcodec_open2 fail");
        return -1;
    }

    // alloc AVFrame
    m_AVFrame = av_frame_alloc();
    m_AVFrameRGB = av_frame_alloc();

    // 图像色彩空间转换、分辨率缩放、前后图像滤波处理
    m_SwsContext = sws_getContext(m_AVCodecContext->width, m_AVCodecContext->height,
            m_AVCodecContext->pix_fmt, m_AVCodecContext->width, m_AVCodecContext->height,
            AV_PIX_FMT_RGB565, SWS_BICUBIC, NULL, NULL, NULL);

    int bytes = avpicture_get_size(AV_PIX_FMT_RGB565, m_AVCodecContext->width, m_AVCodecContext->height);
    m_OutBuffer = (uint8_t *)av_malloc(bytes * sizeof(uint8_t));

    // 将分配的内存空间给m_AVFrameRGB使用
    avpicture_fill((AVPicture *)m_AVFrameRGB, m_OutBuffer, AV_PIX_FMT_RGB565, m_AVCodecContext->width, m_AVCodecContext->height);

    // 为AVPacket分别内存空间
    int packSize = m_AVCodecContext->width * m_AVCodecContext->height;
    m_AVPacket = (AVPacket *)malloc(sizeof(AVPacket));
    av_new_packet(m_AVPacket, packSize);

    qDebug("============== MyFFmpegInit ok! ====================== ");

    return 0;
}

int MyFFmpeg::MyFFmpepReadFrame()
{
    int ret = -1;
    int getPicture = 0;

    // 获取下一帧数据
    ret = av_read_frame(m_AVFormatContext, m_AVPacket);
    if (ret < 0)
    {
        qDebug("av_read_frame fail!\n");
        return -1;
    }

    if (m_AVPacket->stream_index != m_videoIndex)
    {
        av_free_packet(m_AVPacket);
        return 0;
    }


    //  解码m_AVPacket，Decode the video frame of size avpkt->size from avpkt->data into picture
    ret = avcodec_decode_video2(m_AVCodecContext, m_AVFrame, &getPicture, m_AVPacket);
    if (ret < 0)
    {
        char errbuf[128];
        av_strerror(ret, errbuf, sizeof(errbuf));
        qDebug("avcodec_decode_video2 fail! Error: %s", errbuf);
        av_packet_unref(m_AVPacket);
        return 0;
    }

    // got_picture_ptr Zero if no frame could be decompressed, otherwise, it is nonzero.
    // 判断是否已有视频帧被解码了
    if (getPicture){
        // 对解码视频帧进行缩放、格式转换等操作
        sws_scale(m_SwsContext, (uint8_t const * const *)m_AVFrame->data,
                 m_AVFrame->linesize, 0, m_AVCodecContext->height,
                 m_AVFrameRGB->data, m_AVFrameRGB->linesize);

        // 转换到QImage
        QImage tmmImage((uchar *)m_OutBuffer, m_AVCodecContext->width, m_AVCodecContext->height, QImage::Format_RGB16);
        QImage image = tmmImage.copy();

        // 发送QImage
        emit MyFFmpegSigGetOneFrame(image);
    }

    // 释放资源
    av_free_packet(m_AVPacket);

    return 0;
}

void MyFFmpeg::MyFFmpegDestroy()
{
    // 释放输出缓冲区
    av_free(m_OutBuffer);

    // 释放 RGB 帧
    if (m_AVFrameRGB) {
        av_frame_free(&m_AVFrameRGB);
    }

    // 释放原始帧
    if (m_AVFrame) {
        av_frame_free(&m_AVFrame);
    }

    // 释放缩放上下文
    if (m_SwsContext) {
        sws_freeContext(m_SwsContext);
    }

    // 释放 AVPacket
    if (m_AVPacket) {
        av_packet_unref(m_AVPacket);
        av_free(m_AVPacket);
    }

    // 关闭编解码器上下文
    if (m_AVCodecContext) {
        avcodec_close(m_AVCodecContext);
    }

    // 关闭输入流
    if (m_AVFormatContext) {
        avformat_close_input(&m_AVFormatContext);
    }

    // 释放格式上下文
    if (m_AVFormatContext) {
        avformat_free_context(m_AVFormatContext);
    }

    // 反初始化网络库
    avformat_network_deinit();
}



void MyFFmpeg::MyFFmpegInfo()
{
    qDebug("------------------------------------------------------------------");
    qDebug("%s", avcodec_configuration());
    qDebug("version: %d", avcodec_version());
    qDebug("------------------------------------------------------------------");
}
