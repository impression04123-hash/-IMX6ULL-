#ifndef JDQ_H
#define JDQ_H

#include <QObject>

class Jdq : public QObject
{
    Q_OBJECT
public:
    explicit Jdq(QObject *parent = nullptr);
    int jdq_write(char *buf);

signals:

private:
    QString jdq_device = "/dev/jdq_device";
    int jdq_fd;

};

#endif // JDQ_H
