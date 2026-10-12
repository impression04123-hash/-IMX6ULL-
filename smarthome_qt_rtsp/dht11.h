#ifndef DHT11_H
#define DHT11_H

#include <QObject>
#include <QTimer>

class Dht11 : public QObject
{
    Q_OBJECT
public:
    explicit Dht11(QObject *parent = nullptr);
    void dht11_read(char *buf);

public slots:
     void timeto_read_dht11data(void);

signals:
    void readyto_read_dht11data(void);

private:
    QString dht11_device = "/dev/dht11_device";
    int dht11_fd;
    QTimer * dht11_timer;
};

#endif // DHT11_H
