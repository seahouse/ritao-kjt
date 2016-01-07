#ifndef ORDERCREATEKJTTOERP_H
#define ORDERCREATEKJTTOERP_H

#include <QObject>

class OrderCreateKJTToERP : public QObject
{
    Q_OBJECT
public:
    OrderCreateKJTToERP(QObject *parent = 0);

    void run(const QDateTime &dateTimeStart, const QDateTime &dateTimeEnd);

signals:
    void finished(bool error, const QString &msg);
};

#endif // ORDERCREATEKJTTOERP_H
