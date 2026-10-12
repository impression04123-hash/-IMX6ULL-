#ifndef SR501_H
#define SR501_H

#include <QObject>

class Sr501 : public QObject
{
    Q_OBJECT
public:
    explicit Sr501(QObject *parent = nullptr);
    static int sr501_read(char *buf);

signals:
private:
    QString sr501_device = "/dev/sr501_device";
    int sr501_fd;
    int sr501_flags;

};

#endif // SR501_H
