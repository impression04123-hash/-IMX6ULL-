QT       += core gui
QT += network
QT += multimedia
QT += multimediawidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ap3216c.cpp \
    beep.cpp \
    bluetoothuart.cpp \
    dht11.cpp \
    jdq.cpp \
    led.cpp \
    main.cpp \
    monitor.cpp \
    mq2adc.cpp \
    myffmpeg.cpp \
    myslider.cpp \
    photo.cpp \
    rtspplayer.cpp \
    sg90.cpp \
    smarthomewindow.cpp \
    sr501.cpp \
    v4l2.cpp \
    v4l2camera.cpp

HEADERS += \
    ap3216c.h \
    beep.h \
    bluetoothuart.h \
    dht11.h \
    jdq.h \
    led.h \
    monitor.h \
    mq2adc.h \
    myffmpeg.h \
    myslider.h \
    photo.h \
    rtspplayer.h \
    sg90.h \
    smarthomewindow.h \
    sr501.h \
    v4l2.h \
    v4l2camera.h

FORMS += \
    monitor.ui \
    photo.ui \
    smarthomewindow.ui \
    v4l2.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


INCLUDEPATH+=$$PWD/../../ffmpeg/ffmpeg-4.4.5/install/include \
             $$PWD/../../ffmpeg/x264-master/install/include

LIBS += $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libavcodec.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libavdevice.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libavfilter.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libavformat.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libavutil.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libpostproc.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libswresample.so \
        $$PWD/../../ffmpeg/ffmpeg-4.4.5/install/lib/libswscale.so \
        $$PWD/../../ffmpeg/x264-master/install/lib/libx264.so

RESOURCES += \
    resources.qrc
