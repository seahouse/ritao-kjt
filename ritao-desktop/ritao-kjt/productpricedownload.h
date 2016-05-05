#ifndef PRODUCTPRICEDOWNLOAD_H
#define PRODUCTPRICEDOWNLOAD_H

#include <QObject>

#include <QDateTime>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class ProductPriceDownload : public QObject
{
    Q_OBJECT

public:
    enum OptType
    {
        OTNone,
        OTDownloadIdList,               // 下载区间订单ID列表
        OTProductDownloading,           // 商品下载
        OTProductDownloadEnd,           // 商品下载结束
        OTProductDownloadError,         // 商品下载出错
    };

    struct ProductSummary
    {
        ProductSummary(const QString &productId, int status, const QDateTime &changeDate) :
            _productId(productId), _status(status), _changeDate(changeDate)
        {}
        QString _productId;             // 商品ID
        int _status;                    // 商品状态。0:上架不展示; 1:上架; 2:下架; 3:初始; -1:已作废
        QDateTime _changeDate;          // 发生商品创建、修改或订阅的最后时间
    };

    struct ProductHandlerData
    {
        int _total;                         // 时间区间内新建商品的总数
        QList<ProductSummary> _productList; // 时间区间内新建商品的id列表
        int _currentIndex;                  // 当前处理的订单个数位置
        bool _success;                      // 是否处理成功（写入到ERP数据库）
        QStringList _msgList;               // 消息列表
        QDateTime _dateStart;               // 时间区间的开始时间
        QDateTime _dateEnd;                 // 时间区间的结束时间
    };

public:
    explicit ProductPriceDownload(QObject *parent = 0);

    void download();

signals:
    void finished(bool success, const QString &msg);

private slots:
    void sTimeout();
    void sReplyFinished(QNetworkReply *reply);

private:
    void downloadProductIdList();
    void downloadNextProducts();
    void setProductGetFromKJTTime();                  // 设置ERP数据库中的订单下载时间区间（系统参数表）
    void insertProduct2ERPByJson(const QJsonObject &json);        // 向ERP中插入订单数据
    QDateTime convertKjtTime(const QString &kjtTime);           // 将跨镜通返回的日期类型转换为QDateTime。后面跨境通将修改返回的格式，到时需修改此处

private:
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    ProductHandlerData _phData;
};

#endif // PRODUCTPRICEDOWNLOAD_H
