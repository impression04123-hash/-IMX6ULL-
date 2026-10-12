QT       += core gui

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
    mq2adc.cpp \
    myslider.cpp \
    photo.cpp \
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
    mq2adc.h \
    myslider.h \
    photo.h \
    sg90.h \
    smarthomewindow.h \
    sr501.h \
    v4l2.h \
    v4l2camera.h

FORMS += \
    photo.ui \
    smarthomewindow.ui \
    v4l2.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
