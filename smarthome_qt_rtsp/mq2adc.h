#ifndef MQ2ADC_H
#define MQ2ADC_H

#include <QObject>
#include <QTimer>

/**
  MQ2是空气传感器,采用adc采集
  相应的驱动是linux中自带的驱动
 * @brief The Mq2ADC class
 */
class Mq2ADC : public QObject
{
    Q_OBJECT
public:
    explicit Mq2ADC(QObject *parent = nullptr);
    void mq2_read(char *buf);

public slots:
     void timeto_read_mq2data(void);

signals:
    void readyto_read_mq2data(void);

private:
    QString mq2_device = "/sys/bus/iio/devices/iio:device0/in_voltage1_raw";
    FILE * mq2_fd;
    QTimer * mq2_timer;

};

#endif // MQ2ADC_H
