#ifndef LED_H
#define LED_H

#include <QObject>

class Led : public QObject
{
    Q_OBJECT
public:
    explicit Led(QObject *parent = nullptr);
    int led_write(char *buf);

signals:
private:
    QString led_device = "/dev/led_device";
    int led_fd;
};

#endif // LED_H
