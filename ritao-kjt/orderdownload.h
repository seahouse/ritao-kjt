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
        OTDownloadIdList,               // 下载区间订单ID列表
        OTOrderDownloading,             // 订单下载
        OTOrderDownloadEnd,             // 订单下载结束
        OTOrderDownloadError,           // 订单下载出错
    };

    struct OrderHandlerData
    {
        int _total;                         // 时间区间内新建订单的总数
        QList<int> _orderIdList;            // 时间区间内新建订单的id列表
        int _currentIndex;                  // 当前处理的订单个数位置
        bool _success;                      // 是否处理成功（写入到ERP数据库）
        QStringList _msgList;               // 消息列表
        QDateTime _dateStart;               // 时间区间的开始时间
        QDateTime _dateEnd;                 // 时间区间的结束时间
    };

    /// 订单中购买商品信息
    struct SOItemInfo
    {
        QString _productId;
        QString _productName;
        int _quantity;              // 购买数量
        double _productPrice;       // 商品价格
        double _taxPrice;           // 行邮税率金额
        double _taxRate;            // 行邮税率
        QString _sOItemSysNo;       //
    };

    /// 日志
    struct LogInfo
    {
        QDateTime _optTime;         // 操作时间
        /// 操作类型  -90 订单锁定 -91 订单解锁 1 创建订单 2 订单审核 3 已出库待申报 4 已申报待通关 5 已通关发往顾客 66 订单物流 轨迹
        int _optType;               // 操作类型
        QString _optNote;           // 操作详情
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
    void downloadOrderIdList();
    void downloadNextOrders();
    void setOrderGetFromKJTTime();                  // 设置ERP数据库中的订单下载时间区间（系统参数表）
    void insertOrder2ERPByJson(const QJsonObject &json);        // 向ERP中插入订单数据
    QDateTime convertKjtTime(const QString &kjtTime);           // 将跨镜通返回的日期类型转换为QDateTime。后面跨境通将修改返回的格式，到时需修改此处

private:
    QString _msg;
    QTimer *_timer;
    OptType _optType;

    QNetworkAccessManager *_manager;
    OrderHandlerData _ohData;
};

#endif // ORDERDOWNLOAD_H
