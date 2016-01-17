#ifndef PRODUCTUPLOAD_H
#define PRODUCTUPLOAD_H

#include <QObject>

#include <QQueue>

class QNetworkReply;
class QTimer;
class QNetworkAccessManager;

class ProductUpload : public QObject
{
    Q_OBJECT

public:
    enum OptType
    {
        OTNone,
        OTProductUploading,             // 商品上传中
        OTProductUploadEnd,             // 商品上传结束
        OTProductUploadError,           // 商品上传出错
    };

    struct OrderHandlerData
    {
        int _currentProductId;
//        QString _currentOrderNumber;
    };

public:
    explicit ProductUpload(QObject *parent = 0);

    void upload();

signals:
    void finished(bool success, const QString &msg);

private slots:
    void sTimeout();
    void sReplyFinished(QNetworkReply *reply);

private:
    void uploadNextProduct();

private:
    QQueue<int> _productIdQueue;
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    OrderHandlerData _ohData;
};

#endif // PRODUCTUPLOAD_H
