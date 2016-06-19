#ifndef PRODUCTUPLOAD2HG_H
#define PRODUCTUPLOAD2HG_H

#include <QObject>

#include <QQueue>
#include <QDomElement>

class QNetworkReply;
class QTimer;
class QNetworkAccessManager;

class ProductUpload2HG : public QObject
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
        int _currentProductId;          // 商品KID
//        QString _currentOrderNumber;
    };

public:
    explicit ProductUpload2HG(QObject *parent = 0);

    void upload();

signals:
    void finished(bool success, const QString &msg);

private slots:
    void sTimeout();
    void sReplyFinished(QNetworkReply *reply);

private:
    void uploadNextProduct();
    void appendElement(QDomElement &tagParent, const QString &name, const QString &value);

private:
    QQueue<int> _productIdQueue;
    QStringList _msgList;
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    OrderHandlerData _ohData;
    QDomDocument _doc;
};

#endif // PRODUCTUPLOAD2HG_H
