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
                       "where p1='1' and p2='0' and 同步指令='新增' and 同步表名='订单' "
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
        emit finished(_success, _msgList.join(", ") + ".");
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
        qInfo() << query.lastError().text();
        _timer->start(1000);
        return;
    }

    if (query.first())
    {
        QMap<QString, QString> paramsMap(g_paramsMap);
        paramsMap["service"] = "subAddSaleOrder";             // 服务名称

        QJsonObject json;

        QJsonArray saleOrderList;
        QJsonObject saleOrder;

        saleOrder["uuid"] = QString::number(query.value("订单KID").toInt());              // uuid
        saleOrder["orderCode"] = query.value("订单号").toString();                         //
        saleOrder["hgBarcode"] = "";                                                        //
        saleOrder["printMsg"] = "";                                                     //
        saleOrder["orderTax"] = QString::number(query.value("税金").toDouble());          // 税费
        saleOrder["platFromName"] = "test";                                             // 来源平台名称(OMS指定)
        saleOrder["shopName"] = "test";                                                 // 店铺名称(OMS指定)展示展销请填写连锁店号
        /// WAIT_BUYER_PAY(等待买家付款)
        /// WAIT_SELLER_SEND_GOODS  (等待卖家发货,即:买家已付款)
        /// WAIT_BUYER_CONFIRM_GOODS(等待买家确认收货,即:卖家已发货)
        /// TRADE_BUYER_SIGNED(买家已签收,货到付款专用)
        /// TRADE_FINISHED(交易成功)
        /// TRADE_CLOSED(付款以后用户退款成功，交易自动关闭)
        saleOrder["orderStatus"] = "WAIT_SELLER_SEND_GOODS";                            // 交易状态
        saleOrder["type"] = "fixed";                // 订单类型. fixed(一口价), od(货到付款), ZSZT用于展示展销业务
        saleOrder["createDate"] = query.value("下单日期").toDateTime().toString(date_format_str);              // 下单时间
        saleOrder["updateDate"] = QDateTime::currentDateTime().toString(date_format_str);                          // 更新时间
        saleOrder["payTime"] = query.value("付款日期").toDateTime().toString(date_format_str);                 // 支付时间
        saleOrder["logisticsCompanyCode"] = query.value("物流公司").toString();             // 物流公司编码
        saleOrder["logisticsCompanyName"] = query.value("物流公司").toString();             // 物流公司名称
        saleOrder["logisticsNumber"] = query.value("运单号").toString();                   // 物流单号
        saleOrder["postPrice"] = QString::number(query.value("配送费用").toDouble());       // 邮费
        saleOrder["isDeliveryPay"] = "false";       // 是否货到付款(true/false)
        saleOrder["bunick"] = query.value("收货人").toString();                            // 会员昵称
        saleOrder["invoiceName"] = "";                   // 发票抬头 多张发票，用逗号分隔
        saleOrder["invoiceType"] = "";                   // 发票类型
        saleOrder["invoiceContent"] = "";                // 发票明细
        saleOrder["sellersMessage"] = "";                // 卖家留言
        saleOrder["buyerMessage"] = "";                  // 买家留言
        saleOrder["merchantMessage"] = "";               // 商家留言
        saleOrder["amountReceivable"] = QString::number(query.value("订单应付总额").toDouble());      // 应收金额
        saleOrder["actualPayment"] = QString::number(query.value("订单应付总额").toDouble());         // 实际支付

        QJsonObject receiverOrder;
        receiverOrder["uuid"] = QString::number(query.value("订单KID").toInt());           // uuid
        receiverOrder["orderCode"] = query.value("订单号").toString();                     // 订单号
        receiverOrder["name"] = query.value("收货人").toString();                          // 名字
        receiverOrder["phone"] = "";                    // 固话
        receiverOrder["mobilePhone"] = query.value("手机号码").toString();                 // 移动电话号
        receiverOrder["address"] = query.value("收货地址").toString();                     // 地址
        QString province = "";
        QSqlQuery provinceQuery;
        provinceQuery.prepare("select 省份 from 省份 where 省份KID=:provinceId");
        provinceQuery.bindValue(":provinceId", query.value("收货省份ID").toInt());
        if (provinceQuery.exec() && provinceQuery.first())
            province = provinceQuery.value("省份").toString();
        receiverOrder["province"] = province;           // 省
        QString city = "";
        QSqlQuery cityQuery;
        cityQuery.prepare("select 城市名称 from 城市 where 城市KID=:cityId");
        cityQuery.bindValue(":cityId", query.value("收货城市ID").toInt());
        if (cityQuery.exec() && cityQuery.first())
            city = cityQuery.value("城市名称").toString();
        receiverOrder["city"] = city;                   // 市
        QString district = "";
        QSqlQuery districtQuery;
        districtQuery.prepare("select 区域 from 区域 where 区域KID=:districtId");
        districtQuery.bindValue(":districtId", query.value("收货区域ID").toInt());
        if (districtQuery.exec() && districtQuery.first())
            district = districtQuery.value("区域").toString();
        receiverOrder["district"] = district;           // 市
        receiverOrder["zip"] = query.value("邮政编码").toString();                      // 名字
        saleOrder["receiver"] = receiverOrder;

        QJsonArray detailList;
        QSqlQuery queryDetail;
        queryDetail.prepare(tr("select * from 订单商品 where 订单ID=:orderId"));
        queryDetail.bindValue(":orderId", _ohData._currentOrderId);
        if (queryDetail.exec())
        {
            while (queryDetail.next())
            {
                QJsonObject detail;

                detail["uuid"] = "";                                                    // uuid
                detail["orderCode"] = query.value("订单号").toString();              // 订单编码
                detail["orderDetailCode"] = query.value("订单号").toString();        // uuid
                detail["skuId"] = queryDetail.value("商品编号").toString();                 // 平台SKU编码
                QString outerSkuId;
                QSqlQuery outerSkuIdQuery;
                outerSkuIdQuery.prepare("select p43 from 商品 where 商品KID=:productId");
                outerSkuIdQuery.bindValue(":productId", queryDetail.value("商品ID").toInt());
                if (outerSkuIdQuery.exec() && outerSkuIdQuery.first())
                    outerSkuId = outerSkuIdQuery.value("p43").toString();
                detail["outerSkuId"] = outerSkuId;              // 外部Sku编号
                detail["num"] = QString::number(queryDetail.value("购买数量").toDouble());      // 数量
                detail["title"] = queryDetail.value("商品名称").toString();    // 商品标题
                detail["price"] = QString::number(queryDetail.value("销售单价").toDouble());    // 商品价格
                detail["payment"] = QString::number(queryDetail.value("成交单价").toDouble());  // 单实际金额
                detail["discountPrice"] = QString::number(queryDetail.value("红包金额").toDouble());        // 优惠金额
                detail["totalPrice"] = QString::number(queryDetail.value("付款金额").toDouble());// 应付金额
                detail["adjustPrice"] = "";             // 手工调整金额
                detail["divideOrderPrice"] = "";        // 分摊之后的实付金额
                detail["billPrice"] = "";               // 开票金额
                detail["partMjzDiscount"] = "";         // 优惠分摊

                detailList.append(detail);
            }
        }
        saleOrder["detail"] = detailList;

        saleOrderList.append(saleOrder);
        json["saleOrderList"] = saleOrderList;

        QJsonDocument jsonDoc(json);
        paramsMap["content"] = jsonDoc.toJson(QJsonDocument::Compact);
        QFile file("11.txt");
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << jsonDoc.toJson(QJsonDocument::Compact);
            file.close();
        }

        qDebug() << paramsMap;

        QString params;
        QMapIterator<QString, QString> i(paramsMap);
        while (i.hasNext())
        {
            i.next();
            params.append(i.key()).append("=").append(i.value().toUtf8().toPercentEncoding()).append("&");
        }

        urlencodePercentConvert(params);
        QString secret = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();
        params.append("secret=").append(secret);
//        params.append("secret=");       // 暂时留空
        qDebug() << params;

        QNetworkRequest req;
        req.setUrl(QUrl(g_config.cqdfUrl()));
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
//    QString url = "http://kjt.ritaoshimao.com/RitaoKJTCallback.aspx";

    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Order.SOOutputCustoms";                // 由接口提供方指定的接口标识符
//    paramsMap["method"] = "Inventory.ChannelQ4SAdjustRequest";                // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    json["MerchantOrderID"] = "wfx201604050003";      // 商家订单编号
    /// 订单物流运输公司编号
    /// 1=顺丰 2=圆通 84=如风达
    json["ShipTypeID"] = 1;
    json["TrackingNumber"] = "123456";
    json["CommitTime"] = "20151117020101";
    json["Status"] = "1";
    json["Message"] = "aaaa";

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
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();
    params.append("sign=").append(sign);

//    QFile file("1.txt");
//    QByteArray bb;
//    if (file.open(QIODevice::ReadOnly))
//    {
//        bb = file.readAll();
//        file.close();
//    }

    QNetworkRequest req;
    req.setUrl(QUrl(url));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
//    _manager->post(req, bb);
    _manager->post(req, params.toLatin1());
}

void OrderUpload::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();
    qDebug() << replyData;

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    bool code = json.value("isSuccess").toBool(false);
    QString body = json.value("body").toString();

    QString opt;
    switch (_optType) {
    case OTOrderUploading:
        opt = tr("新增订单");
        if (code == true)
        {
            QSqlQuery query;

//            QJsonObject data = json.value("Data").toObject();
//            /// 将跨境通的“订单号”存入订单表
//            int orderIdKJT = data.value("SOSysNo").toInt();
//            query.prepare(tr("update 订单 set 第三方订单号=:orderIdKJT "
//                             "where 订单KID=:id "));
//            query.bindValue(":orderIdKJT", QString::number(orderIdKJT));
//            query.bindValue(":id", _ohData._currentOrderId);
//            if (!query.exec())
//                qInfo() << tr("更新订单的第三方订单号: ") << query.lastError().text();
//            /// 将跨境通的“Kjt计算的运费金额”存入订单表
//            /// 将跨境通的“商品跨贸税总金额”存入订单表
//            double shippingAmount = data.value("ShippingAmount").toDouble();
//            double taxAmount = data.value("TaxAmount").toDouble();
//            query.prepare(tr("update 订单 set 订单保价=:shippingAmount, 税金=:taxAmount "
//                             "where 订单KID=:id "));
//            query.bindValue(":shippingAmount", shippingAmount);
//            query.bindValue(":taxAmount", taxAmount);
//            query.bindValue(":id", _ohData._currentOrderId);
//            if (!query.exec())
//                qInfo() << tr("更新订单的Kjt计算的运费金额到订单保价: ") << query.lastError().text();

            /// 记录同步数据，并进行下一个跨境通同步
            query.prepare(tr("update 数据同步 set p2='1' "
                             "where p1='1' and 同步指令='新增' and 同步表名='订单' and 同步主键KID=:id "));
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
//            _msg = tr("上传订单错误：") + body;
            _msgList.append("上传订单错误: (" + _ohData._currentOrderNumber + ")" + body);
            _timer->start(1000);
        }
        break;
    default:
        break;
    }

    qInfo() << opt << code << body;
}
