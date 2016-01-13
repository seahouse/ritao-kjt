#include "orderupload.h"

#include "global.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <QDebug>
#include <QTimer>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QCryptographicHash>
#include <QNetworkAccessManager>
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

OrderUpload::OrderUpload(QObject *parent) : QObject(parent),
    _optType(OTNone)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void OrderUpload::upload()
{
    /// 获取需要新增到跨境通的商品KID列表
    QSqlQuery query(tr("select 同步主键KID from 数据同步 "
                       "where 跨境通=1 and 跨境通处理=0 and 同步指令='新增' and 同步表名='订单' "
                       "order by 数据同步KID "));
    while (query.next())
        _orderIdQueue.enqueue(query.value(tr("同步主键KID")).toInt());

    qDebug() << _orderIdQueue;

    _optType = OTOrderUploading;
    _timer->start(1000);
}

void OrderUpload::sTimeout()
{
    _timer->stop();

    switch (_optType) {
    case OTOrderUploading:
        uploadNextOrder();
        break;
    case OTOrderUploadEnd:
        emit finished(true, tr("上传订单结束。"));
        break;
    default:
        break;
    }
}

void OrderUpload::uploadNextOrder()
{
    if (_orderIdQueue.isEmpty())
    {
        _optType = OTOrderUploadEnd;
        _timer->start(1000);
        return;
    }

    _currentHandlerId = _orderIdQueue.dequeue();

    QSqlQuery query;
    query.prepare(tr("select * from 订单 "
                     "where 订单KID=:id "));
    query.bindValue(":id", _currentHandlerId);
    if (!query.exec())
    {
//        _optType = OTOrderUploadError;
//        _msg = tr("订单");
//        qFatal(query.lastError().text().toStdString().c_str());
        qInfo() << query.lastError().text();
        _timer->start(1000);
        return;
    }

    if (query.first())
    {
        QString url = "http://preapi.kjt.com/open.api";     //接口测试地址
        QString secretkey = "kjt@345";              // 由接口提供方分配给接口调用方的验签密钥

        QMap<QString, QString> paramsMap(g_paramsMap);
        paramsMap["method"] = "Order.SOCreate";             // 由接口提供方指定的接口标识符
        paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
        paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

        QJsonObject json;

        json["SaleChannelSysNo"] = 1106;                // 渠道编号
        json["MerchantOrderID"] = query.value(tr("订单号")).toString();

        /// 获取订单出库仓库在 Kjt平台的编号
        int warehouseID = 51;
        QSqlQuery queryWarehouseID(tr("select * from 系统参数 where 参数分组='跨境通' and 参数名称='跨境通仓库编码'"));
        if (queryWarehouseID.first())
            warehouseID = queryWarehouseID.value(tr("参数内容")).toString().toInt();
        json["WarehouseID"] = warehouseID;

        QJsonArray jsonArray;
        jsonArray.append(QJsonValue("345JPA018400002"));
        json["ProductIDs"] = jsonArray;
        QJsonDocument jsonDoc(json);
        qDebug() << jsonDoc.toJson(QJsonDocument::Compact);

        paramsMap["data"] = jsonDoc.toJson(QJsonDocument::Compact);
        qDebug() << paramsMap;

        QString params;
        QMapIterator<QString, QString> i(paramsMap);
        while (i.hasNext())
        {
            i.next();
            params.append(i.key()).append("=").append(i.value().toLatin1().toPercentEncoding()).append("&");
        }

        params.append(secretkey);
        qDebug() << params;
        QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

        url.append("?").append(params).append("&sign=").append(sign);

        qDebug() << url;
        _manager->get(QNetworkRequest(QUrl(url)));
    }

}

void OrderUpload::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    QString code = json.value("Code").toString("-99");
    QString desc = json.value("Desc").toString();

    QString opt;
    switch (_optType) {
    case OTOrderUploading:
        opt = tr("新增商品");
        if (code == "0")
        {
            /// 记录同步数据，并进行下一个跨境通同步
            QSqlQuery query;
            query.prepare(tr("update 数据同步 set 跨境通处理=1 "
                             "where 跨境通=1 and 同步指令='新增' and 同步表名='订单' and 同步主键KID=:id "));
            query.bindValue(":id", _currentHandlerId);
            query.exec();
            _timer->start(1000);
        }
        break;
//    case STOrderCreateKJTToERP:
//        opt = tr("获取订单");
//        _orderCreateKJTToERPData._replyData._code = code;
//        _orderCreateKJTToERPData._replyData._desc = desc;
//        if ("0" == code)
//        {
//            /// 读取订单id列表
//            QJsonObject data = json.value("Data").toObject();
//            _orderCreateKJTToERPData._total = data.value("Total").toInt();
//            QJsonArray orderIdListArray = data.value("OrderIDList").toArray();
//            QList<int> orderIdList;
//            for (int i = 0; i < orderIdListArray.size(); i++)
//                orderIdList.append(orderIdListArray.at(i).toInt());
//            _orderCreateKJTToERPData._orderIdList = orderIdList;
//            qDebug() << "orderIdList:" << _orderCreateKJTToERPData._orderIdList;

//            _orderCreateKJTToERPData._currentIndex = 0;
//            orderInfoBatchGet();
//        }
//        break;
//    case STOrderInfoBatchGet:
//        opt = tr("下载订单");
//        /// 解析数据，写入ERP数据库，并继续请求数据
//        if ("0" == code)
//        {
//            QJsonObject data = json.value("Data").toObject();
//            QJsonArray orderListArray = data.value("OrderList").toArray();
//            for (int i = 0; i < orderListArray.size(); i++)
//                insertOrder2ERPByJson(orderListArray.at(i).toObject());
//        }
//        _timer->start(1000);
//        break;
    default:
        break;
    }

    qInfo() << opt << code << desc;
}
