#ifndef SMARTHOMEWINDOW_H
#define SMARTHOMEWINDOW_H

#include <QMainWindow>
#include "led.h"
#include "beep.h"
#include "jdq.h"
#include "sg90.h"
#include "sr501.h"
#include "bluetoothuart.h"
#include "dht11.h"
#include "mq2adc.h"
#include "ap3216c.h"
#include "v4l2.h"

QT_BEGIN_NAMESPACE
namespace Ui { class SmartHomeWindow; }
QT_END_NAMESPACE

class SmartHomeWindow : public QMainWindow
{
    Q_OBJECT

public:
    SmartHomeWindow(QWidget *parent = nullptr);
    ~SmartHomeWindow();

public slots:
    void uart_handler( char* module,  char*  value);
    void dht11_handler(void);
    void mq2_handler(void);
    void ap3216c_handler(void);
    void show_main_page(void);

signals:


private slots:
    void on_light_button_toggled(bool checked);

    void on_beep_button_toggled(bool checked);

    void on_doorlock_button_toggled(bool checked);

    void on_horizontalSlider_sliderMoved(int position);

    void on_horizontalSlider_valueChanged(int value);

    void on_horizontalSlider_sliderReleased();

    void on_camera_button_clicked();

private:
    Ui::SmartHomeWindow *ui;
    Led * led;
    Beep * beep;
    Jdq * jdq;
    Sg90 * sg90;
    Sr501 * sr501;
    BluetoothUart * bluetoothUart;
    Dht11 * dht11;
    Mq2ADC * mq2ADC;
    Ap3216c * ap3216c;
    V4l2 *v4l2;

    static void sr501_handler(int signum);
};
#endif // SMARTHOMEWINDOW_H
