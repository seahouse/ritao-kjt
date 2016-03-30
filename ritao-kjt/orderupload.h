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

    struct OrderHandlerData
    {
        int _currentOrderId;
        QString _currentOrderNumber;        
    };

public:
    explicit OrderUpload(QObject *parent = 0);

    void upload();
    void outputSOWarehouse();

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
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    OrderHandlerData _ohData;
    bool _success;                      // 是否处理成功
};

#endif // ORDERUPLOAD_H
