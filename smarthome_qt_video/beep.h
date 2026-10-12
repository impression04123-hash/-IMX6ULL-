#ifndef BEEP_H
#define BEEP_H

#include <QObject>

class Beep : public QObject
{
    Q_OBJECT
public:
    explicit Beep(QObject *parent = nullptr);
    int beep_write(char *buf);

signals:
private:
    QString beep_device = "/dev/beep_device";
    int beep_fd;
};

#endif // BEEP_H
