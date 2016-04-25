#include "orderupload.h"

#include "global.h"
#include "configglobal.h"

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
#include <QFile>

OrderUpload::OrderUpload(QObject *parent) : QObject(parent),
    _optType(OTNone),
    _success(true)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void OrderUpload::upload()
{
    _orderIdQueue.clear();
    _msgList.clear();

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
        emit finished(_success, _msgList.join(', ') + ".");
        break;
    case OTOrderUploadError:
        emit finished(_success, _msg);
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
        _msgList.append("上传订单结束");
        _timer->start(1000);
        return;
    }

    _ohData._currentOrderId = _orderIdQueue.dequeue();

    QSqlQuery query;
    query.prepare(tr("select * from 订单 "
                     "where 订单KID=:id "));
    query.bindValue(":id", _ohData._currentOrderId);
    if (!query.exec())
    {
        _success = false;
//        _optType = OTOrderUploadError;
//        _msg = tr("订单");
//        qFatal(query.lastError().text().toStdString().c_str());
        qInfo() << query.lastError().text();
        _timer->start(1000);
        return;
    }

    if (query.first())
    {
        QMap<QString, QString> paramsMap(g_paramsMap);
        paramsMap["method"] = "Order.SOCreate";             // 由接口提供方指定的接口标识符
        paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
        paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

        QJsonObject json;

        json["SaleChannelSysNo"] = g_config.kjtSaleschannelsysno();                // 渠道编号
        _ohData._currentOrderNumber = query.value(tr("订单号")).toString();
        json["MerchantOrderID"] = _ohData._currentOrderNumber;

        /// 获取订单出库仓库在 Kjt平台的编号
        int warehouseID = 51;
        QSqlQuery queryWarehouseID(tr("select * from 系统参数 where 参数分组='跨境通' and 参数名称='跨境通仓库编码'"));
        if (queryWarehouseID.first())
            warehouseID = queryWarehouseID.value(tr("参数内容")).toString().toInt();
        json["WarehouseID"] = warehouseID;

        QJsonObject payInfoObject;              // 订单支付信息
        payInfoObject["ProductAmount"] = query.value(tr("商品总金额")).toDouble();           // 商品总金额
        payInfoObject["ShippingAmount"] = query.value(tr("配送费用")).toDouble();           // 运费总金额
        payInfoObject["TaxAmount"] = query.value(tr("税金")).toDouble();                  // 商品行邮税总金额
        payInfoObject["CommissionAmount"] = query.value(tr("支付手续费")).toDouble();        // 下单支付产生的手续费
        /// 支付方式编号  112: 支付宝 114: 财付通 117: 银联支付 118: 微信支付
        payInfoObject["PayTypeSysNo"] = query.value(tr("支付方式")).toInt();                // 支付方式编号
        payInfoObject["PaySerialNumber"] = query.value(tr("支付流水号")).toString();         // 支付流水号
        json["PayInfo"] = payInfoObject;

        QJsonObject shippingInfoObject;         // 订单配送信息
        shippingInfoObject["ReceiveName"] = query.value(tr("收货人")).toString();          // 收件人姓名
        shippingInfoObject["ReceivePhone"] = query.value(tr("手机号码")).toString();        // 收件人电话
        shippingInfoObject["ReceiveAddress"] = query.value(tr("收货地址")).toString();      // 收件人收货地址
        /// 根据“区域ID”查找“区域”中的“地区编码”
        /// 地区编码参考： http://www.stats.gov.cn/tjsj/tjbz/xzqhdm/201401/t20140116_501070.html
        QString receiveAreaCode;
        QSqlQuery queryAreaCode(tr("select * from 区域 where 区域KID=") + QString::number(query.value(tr("收货区域ID")).toInt()));
        if (queryAreaCode.first())
            receiveAreaCode = queryAreaCode.value(tr("地区编码")).toString();
        shippingInfoObject["ReceiveAreaCode"] = receiveAreaCode;   // query.value(tr("")).toString();   // 收货地区编号
        shippingInfoObject["ShipTypeID"] = query.value(tr("物流公司")).toString();              // 订单物流运输公司编号
        shippingInfoObject["ReceiveAreaName"] = query.value(tr("注册地址")).toString();         // 收件省市区名称
        json["ShippingInfo"] = shippingInfoObject;

        QJsonObject authenticationInfoObject;       // 下单用户实名认证信息
        authenticationInfoObject["Name"] = query.value(tr("个人姓名")).toString();      // 下单用户真实姓名
        authenticationInfoObject["IDCardType"] = query.value(tr("发票类型")).toString();
        authenticationInfoObject["IDCardNumber"] = query.value(tr("纳税人识别号")).toString();    // 下单用户证件编号
        authenticationInfoObject["PhoneNumber"] = query.value(tr("注册电话")).toString();       // 下单用户联系电话
        authenticationInfoObject["Email"] = query.value(tr("电子邮件")).toString();     // 下单用户电子邮件
        json["AuthenticationInfo"] = authenticationInfoObject;

        QJsonArray itemListObject;              // 订单中购买商品列表
        QSqlQuery queryItemList;
        queryItemList.prepare(tr("select * from 订单商品 where 订单ID=:orderId"));
        queryItemList.bindValue(":orderId", _ohData._currentOrderId);
        if (queryItemList.exec())
        {
            while (queryItemList.next())
            {
                QJsonObject itemObject;

                /// 获取商品中的 KJT 商品 ID,
                int productIDERP = queryItemList.value(tr("商品ID")).toInt();
                QString productIDKJT;
                QSqlQuery queryItem;
                queryItem.prepare(tr("select * from 商品 where 商品KID=:productIDERP "));
                queryItem.bindValue(":productIDERP", productIDERP);
                if (queryItem.exec())
                    if (queryItem.first())
                        productIDKJT = queryItem.value("p31").toString();

                itemObject["ProductID"] = productIDKJT;                                       // KJT 商品 ID
                itemObject["Quantity"] = queryItemList.value(tr("购买数量")).toInt();           // 购买数量
                itemObject["SalePrice"] = queryItemList.value(tr("销售单价")).toDouble();       // 商品价格
                itemObject["TaxPrice"] = queryItemList.value(tr("税金")).toDouble();

                itemListObject.append(itemObject);
            }
        }
        json["ItemList"] = itemListObject;

        qInfo() << tr("下单用户真实姓名: ") << query.value(tr("个人姓名")).toString();

        QJsonDocument jsonDoc(json);
//        QFile file("11.txt");
//        if (file.open(QIODevice::WriteOnly))
//        {
//            QTextStream out(&file);
//            out << jsonDoc.toJson(QJsonDocument::Compact);
//            file.close();
//        }
        qDebug() << jsonDoc.toJson(QJsonDocument::Compact);

        paramsMap["data"] = jsonDoc.toJson(QJsonDocument::Compact);
        qDebug() << paramsMap;

        QString params;
        QMapIterator<QString, QString> i(paramsMap);
        while (i.hasNext())
        {
            i.next();
            params.append(i.key()).append("=").append(i.value().toUtf8().toPercentEncoding()).append("&");
        }

        urlencodePercentConvert(params);
        qDebug() << params;
        QString sign = QCryptographicHash::hash(QString(params + g_config.kjtSecretkey()).toLatin1(), QCryptographicHash::Md5).toHex();
        params.append("sign=").append(sign);

        QNetworkRequest req;
        req.setUrl(QUrl(g_config.kjtUrl()));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        _manager->post(req, params.toLatin1());
    }
    else
    {
        _success = false;
        qInfo() << "上传的订单（id: " + QString::number(_ohData._currentOrderId) + "）不存在";
        uploadNextOrder();
    }
}

void OrderUpload::outputSOWarehouse()
{
    QString url = "http://localhost:3548/RitaoKJTCallback.aspx";
//    QString url = "http://kjt.ritaoshimao.com/OrderSOOutputWarehouse.aspx";

    QMap<QString, QString> paramsMap(g_paramsMap);
//    paramsMap["method"] = "Order.SOOutputWarehouse";                // 由接口提供方指定的接口标识符
    paramsMap["method"] = "Inventory.ChannelQ4SAdjustRequest";                // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    json["MerchantOrderID"] = "018524818500";      // 商家订单编号
    /// 订单物流运输公司编号
    /// 1=顺丰 2=圆通 84=如风达
    json["ShipTypeID"] = 1;
    json["TrackingNumber"] = "123456";
    json["CommitTime"] = "20151117020101";

    QJsonDocument jsonDoc(json);
    paramsMap["data"] = jsonDoc.toJson(QJsonDocument::Compact);

    QString params;
    QMapIterator<QString, QString> i(paramsMap);
    while (i.hasNext())
    {
        i.next();
        params.append(i.key()).append("=").append(i.value().toLatin1().toPercentEncoding()).append("&");
    }

    qDebug() << params;
    QString sign = QCryptographicHash::hash(QString(params + g_config.kjtSecretkey()).toLatin1(), QCryptographicHash::Md5).toHex();
    params.append("sign=").append(sign);

    QFile file("1.txt");
    QByteArray bb;
    if (file.open(QIODevice::ReadOnly))
    {
        bb = file.readAll();
        file.close();
    }

    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    _manager->post(req, bb);
}

void OrderUpload::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();
    qInfo() << replyData;

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    QString code = json.value("Code").toString("-99");
    QString desc = json.value("Desc").toString();

    QString opt;
    switch (_optType) {
    case OTOrderUploading:
        opt = tr("新增订单");
        if (code == "0")
        {
            QSqlQuery query;

            QJsonObject data = json.value("Data").toObject();
            /// 将跨境通的“订单号”存入订单表
            int orderIdKJT = data.value("SOSysNo").toInt();
            query.prepare(tr("update 订单 set 第三方订单号=:orderIdKJT "
                             "where 订单KID=:id "));
            query.bindValue(":orderIdKJT", QString::number(orderIdKJT));
            query.bindValue(":id", _ohData._currentOrderId);
            if (!query.exec())
                qInfo() << tr("更新订单的第三方订单号: ") << query.lastError().text();
            /// 将跨境通的“Kjt计算的运费金额”存入订单表
            /// 将跨境通的“商品跨贸税总金额”存入订单表
            double shippingAmount = data.value("ShippingAmount").toDouble();
            double taxAmount = data.value("TaxAmount").toDouble();
            query.prepare(tr("update 订单 set 订单保价=:shippingAmount, 税金=:taxAmount "
                             "where 订单KID=:id "));
            query.bindValue(":shippingAmount", shippingAmount);
            query.bindValue(":taxAmount", taxAmount);
            query.bindValue(":id", _ohData._currentOrderId);
            if (!query.exec())
                qInfo() << tr("更新订单的Kjt计算的运费金额到订单保价: ") << query.lastError().text();

            /// 记录同步数据，并进行下一个跨境通同步
            query.prepare(tr("update 数据同步 set 跨境通处理=1 "
                             "where 跨境通=1 and 同步指令='新增' and 同步表名='订单' and 同步主键KID=:id "));
            query.bindValue(":id", _ohData._currentOrderId);
            if (!query.exec())
                qInfo() << tr("更新数据同步的跨境通处理: ") << query.lastError().text();

            _timer->start(1000);
        }
        else
        {
            _success = false;
            /// 不记录此错误，继续下一个订单
//            _optType = OTOrderUploadError;
//            _msg = tr("上传订单错误：") + desc;
            _msgList.append("上传订单错误: (" + _ohData._currentOrderNumber + ")" + desc);
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
