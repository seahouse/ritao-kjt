#ifndef ORDERUPLOAD_H
#define ORDERUPLOAD_H

#include <QObject>

#include <QQueue>

class QTimer;
class QNetworkAccessManager;
class QNetworkReply;

class OrderUpload : public QObject
{
    Q_OBJECT

public:
    enum OptType
    {
        OTNone,
        OTOrderUploading,               // 订单上传
        OTOrderUploadEnd,               // 订单上传结束
        OTOrderUploadError,             // 订单上传出错
    };

public:
    explicit OrderUpload(QObject *parent = 0);

    void upload();

signals:
    void finished(bool success, const QString &msg);

public slots:

private slots:
    void sTimeout();

    void sReplyFinished(QNetworkReply *reply);

private:
    void uploadNextOrder();

private:
    QQueue<int> _orderIdQueue;
    int _currentHandlerId;
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
};

#endif // ORDERUPLOAD_H
