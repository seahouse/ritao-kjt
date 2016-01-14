#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QMainWindow>
#include <QQueue>
#include <QMap>
#include <QDateTime>

#include "global.h"

class QNetworkAccessManager;
class QNetworkReply;
class OrderCreateKJTToERP;
class OrderUpload;

namespace Ui {
class MainWindow;
}

struct ReplyData
{
    QString _code;
    QString _desc;
};

/// 跨境通订单信息
struct OrderCreateKJTToERPData
{
    ReplyData _replyData;           // 通用返回信息
    int _total;                     // 时间区间内新建订单的总数
    QList<int> _orderIdList;        // 时间区间内新建订单的id列表
    int _currentIndex;              // 当前处理的订单个数位置
    bool _success;                  // 是否处理成功（写入到ERP数据库）
    QDateTime _dateStart;           // 时间区间的开始时间
    QDateTime _dateEnd;             // 时间区间的结束时间
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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum SynchronizeType
    {
        STNone,
        STProductCreate,                // 获取ERP的商品信息，上传到跨境通
        STOrderUpload,                  // 上传ERP订单到跨境通
        STOrderCreateKJTToERP,          // 获取时间区间内的的订单id列表
        STOrderInfoBatchGet,            // 获取订单详细信息并写入ERP数据库
    };



public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void sStart();
    void on_pushButton_clicked();

    void sReplyFinished(QNetworkReply *reply);

    void on_pushButton_2_clicked();
    void sTimeout();

    void on_pushButton_3_clicked();
    void sOrderCreateKJTToERPFinished(bool error, const QString &msg);

    void on_pushButton_4_clicked();
    void sOrderUploadFinished(bool success, const QString &msg);

private:
    void synchronizeProductCreate();                        // 同步商品: 将ERP商品上传到跨境通
    void orderInfoBatchGet();

    void parseReply(const QByteArray &data);
    void insertOrder2ERPByJson(const QJsonObject &json);    // 向ERP中插入订单数据
    QDateTime convertKjtTime(const QString &kjtTime);       // 将跨镜通返回的日期类型转换为QDateTime。后面跨境通将修改返回的格式，到时需修改此处
    void setOrderGetFromKJTTime();                          // 设置ERP数据库中的订单下载时间区间（系统参数表）
    void output(const QString &msg, MsgType type = MTDebug);          // 将信息输出
private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *_manager;
    SynchronizeType _synchronizeType;
    QTimer *_timer;

    QQueue<int> _productIdQueue;
    int _currentLocalId;

//    QMap<QString, QString> _paramsMap;
    OrderCreateKJTToERPData _orderCreateKJTToERPData;

    OrderUpload *_orderUpload;
//    OrderCreateKJTToERP *_orderCreateKJTToERP;

};

#endif // MAINWINDOW_H
