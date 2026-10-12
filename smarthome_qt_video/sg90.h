#ifndef SG90_H
#define SG90_H

#include <QObject>

class Sg90 : public QObject
{
    Q_OBJECT
public:
    explicit Sg90(QObject *parent = nullptr);
    int sg90_write(char *buf);

signals:
private:
    QString sg90_device = "/dev/sg90_device";
    int sg90_fd;
};

#endif // SG90_H
