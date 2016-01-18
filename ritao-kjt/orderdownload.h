#ifndef ORDERDOWNLOAD_H
#define ORDERDOWNLOAD_H

#include <QObject>

#include <QDateTime>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class OrderDownload : public QObject
{
    Q_OBJECT

public:
    enum OptType
    {
        OTNone,
        OTOrderDownloading,             // 订单下载
        OTOrderDownloadEnd,             // 订单下载结束
        OTOrderDownloadError,           // 订单下载出错
    };

    struct OrderHandlerData
    {
        int _currentOrderId;
        QString _currentOrderNumber;

        QDateTime _dateStart;               // 时间区间的开始时间
        QDateTime _dateEnd;                 // 时间区间的结束时间
    };

public:
    explicit OrderDownload(QObject *parent = 0);

    void download();

signals:
    void finished(bool success, const QString &msg);

private slots:
    void sTimeout();
    void sReplyFinished(QNetworkReply *reply);

private:
    void downloadNextOrder();

private:
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    OrderHandlerData _ohData;
};

#endif // ORDERDOWNLOAD_H
