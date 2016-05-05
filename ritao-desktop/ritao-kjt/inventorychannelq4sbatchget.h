#ifndef INVENTORYCHANNELQ4SBATCHGET_H
#define INVENTORYCHANNELQ4SBATCHGET_H

#include <QObject>

#include <QDateTime>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class InventoryChannelQ4SBatchGet : public QObject
{
    Q_OBJECT
public:
    enum OptType
    {
        OTNone,
//        OTDownloadIdList,               // 下载区间订单ID列表
        OTDownloading,                  // 商品下载
        OTDownloadEnd,                  // 商品下载结束
        OTDownloadError,                // 商品下载出错
    };

    struct ItemInventoryInfo
    {
        ItemInventoryInfo(const QString &productId, int onlineQty, int wareHouseID) :
            _productId(productId), _onlineQty(onlineQty), _wareHouseID(wareHouseID)
        {}
        QString _productId;             // 商品ID
        int _onlineQty;                 // 独占库存
        int _wareHouseID;               // 出库仓 仓库编号
    };

    struct HandlerData
    {
        int _total;                         // 需要处理的商品总数
        QList<QString> _productList;        // 商品id列表
        int _currentIndex;                  // 当前处理的订单个数位置
        bool _success;                      // 是否处理成功（写入到ERP数据库）
        QStringList _msgList;               // 消息列表
//        QDateTime _dateStart;               // 时间区间的开始时间
//        QDateTime _dateEnd;                 // 时间区间的结束时间
    };

public:
    explicit InventoryChannelQ4SBatchGet(QObject *parent = 0);

    void download();
    void setProductIdList(QList<QString> productIdList);

signals:
    void finished(bool success, const QString &msg);

private slots:
    void sTimeout();
    void sReplyFinished(QNetworkReply *reply);

private:
//    void downloadProductIdList();
    void downloadNextItems();
//    void setProductGetFromKJTTime();                  // 设置ERP数据库中的订单下载时间区间（系统参数表）
    void insertItem2ERPByJson(const QJsonObject &json);        // 向ERP中插入订单数据
    QDateTime convertKjtTime(const QString &kjtTime);           // 将跨镜通返回的日期类型转换为QDateTime。后面跨境通将修改返回的格式，到时需修改此处

private:
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    HandlerData _hData;

};

#endif // INVENTORYCHANNELQ4SBATCHGET_H
