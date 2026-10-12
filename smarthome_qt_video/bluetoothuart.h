#ifndef BLUETOOTHUART_H
#define BLUETOOTHUART_H

#include <QObject>
#include <QThread>

class BluetoothUart : public QThread
{
    Q_OBJECT
public:
    explicit BluetoothUart(QObject *parent = nullptr);
    void run();

signals:
    void uart_recv(char* , char* );


private:
    QString uart_device = "/dev/ttymxc2";
    int uart_fd;
};

#endif // BLUETOOTHUART_H
