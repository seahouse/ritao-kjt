#include "orderdownload.h"

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
#include <QFile>

OrderDownload::OrderDownload(QObject *parent) : QObject(parent),
    _optType(OTNone)
{
    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

void OrderDownload::download()
{
    _optType = OTDownloadIdList;
    _ohData._success = true;

    _timer->start(1000);
}

void OrderDownload::sTimeout()
{
    _timer->stop();

    switch (_optType) {
    case OTDownloadIdList:
        downloadOrderIdList();
        break;
    case OTOrderDownloading:
        downloadNextOrders();
        break;
    case OTOrderDownloadEnd:
        emit finished(_ohData._success, _ohData._msgList.join("\r\n"));
        break;
    case OTOrderDownloadError:
        emit finished(false, _msg);
    default:
        break;
    }
}

void OrderDownload::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();

    QJsonObject json(QJsonDocument::fromJson(replyData).object());
    QString code = json.value("Code").toString("-99");
    QString desc = json.value("Desc").toString();

    QString opt;
    switch (_optType) {
    case OTDownloadIdList:
        if (code == "0")
        {
            /// 读取订单id列表
            QJsonObject data = json.value("Data").toObject();
            _ohData._total = data.value("Total").toInt();
            QJsonArray orderIdListArray = data.value("OrderIDList").toArray();
            QList<int> orderIdList;
            for (int i = 0; i < orderIdListArray.size(); i++)
                orderIdList.append(orderIdListArray.at(i).toInt());
            _ohData._orderIdList = orderIdList;
            _ohData._currentIndex = 0;
            qDebug() << "orderIdList:" << _ohData._orderIdList;

            _optType = OTOrderDownloading;
            _timer->start(1000);
        }
        else
        {
            _optType = OTOrderDownloadError;
            _msg = tr("下载区间订单ID列表错误：") + desc;
            _timer->start(1000);
        }
        break;
    case OTOrderDownloading:
        /// 解析数据，写入ERP数据库，并继续请求数据
        if ("0" == code)
        {
            QJsonObject data = json.value("Data").toObject();
            QJsonArray orderListArray = data.value("OrderList").toArray();
            for (int i = 0; i < orderListArray.size(); i++)
                insertOrder2ERPByJson(orderListArray.at(i).toObject());
        }
        _timer->start(1000);
        break;
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

void OrderDownload::downloadOrderIdList()
{
    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Order.OrderIDQuery";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    /// 获取上次运行的结束日期，作为本次的起始日期
    QDateTime dateStart;
    QSqlQuery query;
    query.prepare(tr("select 参数内容Date from 系统参数 where 参数分组='跨境通' and 参数名称='订单下载结束时间'"));
    if (!query.exec())
        qInfo() << query.lastError().text();
    else if (query.first())
        dateStart = query.value(tr("参数内容Date")).toDateTime();

    if (!dateStart.isValid())
        dateStart = QDateTime(QDate(1900, 1, 1));
    qDebug() << dateStart;
    _ohData._dateStart = dateStart;
    _ohData._dateEnd = QDateTime::currentDateTime();

    QJsonObject json;
    json["OrderDateBegin"] = dateStart.toString("yyyy-MM-dd hh:mm:ss");
    json["OrderDateEnd"] = _ohData._dateEnd.toString("yyyy-MM-dd hh:mm:ss");
    QJsonDocument jsonDoc(json);
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

    params.append(kjt_secretkey);
    params.replace("%20", "+");     // 将空格的%20修改为+
    qDebug() << params;
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

    QString url;
    url.append(kjt_url).append("?").append(params).append("&sign=").append(sign);

    qDebug() << url;
    _manager->get(QNetworkRequest(QUrl(url)));
}

void OrderDownload::downloadNextOrders()
{
    if (_ohData._currentIndex >= _ohData._total)
    {
        setOrderGetFromKJTTime();   // 执行结束，修改系统参数的时间

        _optType = OTOrderDownloadEnd;
        if (_ohData._success)
            _ohData._msgList.append(tr("下载订单结束."));
        _timer->start(1000);
        return;
    }

    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Order.OrderInfoBatchGet";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    QJsonArray orderIdListArray;
    foreach (int orderId, _ohData._orderIdList.mid(_ohData._currentIndex, 20)) {
        orderIdListArray.append(QJsonValue(orderId));
    }
    _ohData._currentIndex += 20;
    json["OrderIDList"] = orderIdListArray;
    QJsonDocument jsonDoc(json);
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

    params.append(kjt_secretkey);
    params.replace("%20", "+");     // 将空格的%20修改为+
    qDebug() << params;
    QString sign = QCryptographicHash::hash(params.toLatin1(), QCryptographicHash::Md5).toHex();

    QString url;
    url.append(kjt_url).append("?").append(params).append("&sign=").append(sign);

    qDebug() << url;
    _manager->get(QNetworkRequest(QUrl(url)));
}

void OrderDownload::setOrderGetFromKJTTime()
{
    if (!_ohData._success) return;
    if (!_ohData._dateStart.isValid() || !_ohData._dateEnd.isValid()) return;

    QSqlQuery query;
    query.prepare(tr("update 系统参数 set 参数内容Date=:dateStart where 参数分组='跨境通' and 参数名称='订单下载起始时间'"));
    query.bindValue(":dateStart", _ohData._dateStart);
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
    }

    query.prepare(tr("update 系统参数 set 参数内容Date=:dateStart where 参数分组='跨境通' and 参数名称='订单下载结束时间'"));
    query.bindValue(":dateStart", _ohData._dateEnd);
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
    }
}

void OrderDownload::insertOrder2ERPByJson(const QJsonObject &json)
{
    int orderId = json.value("OrderID").toInt();        // Kjt 系统订单号
    int merchantSysNo = json.value("MerchantSysNo").toInt();    //
    QString merchantOrderID = json.value("MerchantOrderID").toString();     // 第三方商家订单号
    /// 跨镜通将会把日期修改为 yyyy-MM-dd hh:mm:ss 形式。到时再修改此处
    QDateTime orderDate = convertKjtTime(json.value("OrderDate").toString());           // 订单时间
    /// 订单状态码规则
    /// -4 系统作废
    /// -1 作废
    /// 0 待审核
    /// 1 待出库
    /// 4 已出库待申报
    /// 41 已申报待通关
    /// 45 已通关发往顾客
    /// 5 订单完成
    /// 6 申报失败订单作废
    /// 65 通关失败订单作废
    /// 7 订单拒收
    int sOStatusCode = json.value("SOStatusCode").toInt();              // 订单当前状态代码
    QString sOStatusDescription = json.value("SOStatusDescription").toString();     // 订单当前状态描述
    int tradeType = json.value("TradeType").toInt();                    // 贸易类型  0：直邮 1：自贸
    int warehouseID = json.value("WarehouseID").toInt();                // 仓库编号  51 = 浦东机场自贸仓 52 = 洋山港自贸仓 53 = 外高桥自贸仓
    QDateTime auditTime = convertKjtTime(json.value("AuditTime").toString());       // 审核时间
    QDateTime sOOutStockTime = convertKjtTime(json.value("SOOutStockTime").toString());         // 出库时间
    QDateTime sOOutCustomsTime = convertKjtTime(json.value("SOOutCustomsTime").toString());     // 出区时间
    int saleChannelSysNo = json.value("SaleChannelSysNo").toInt();      // 分销渠道编号
    QString saleChannelName = json.value("SaleChannelName").toString(); // 分销渠道名称

    QJsonObject foreignExchangePurchasingInfoObject = json.value("ForeignExchangePurchasingInfo").toObject();   // 订单支付信息
    /// 购汇状态代码  -3：购汇异常 0：未购汇 1：待购汇 2：购汇中 3：购汇完成
    int statusCode = foreignExchangePurchasingInfoObject.value("StatusCode").toInt();       // 购汇状态代码
    QString statusDescrption = foreignExchangePurchasingInfoObject.value("StatusDescrption").toString();        // 购汇状态描述
    QString purchasingCurrencyCode = foreignExchangePurchasingInfoObject.value("PurchasingCurrencyCode").toString();        // 购汇币种代码，如 EUR, JPY, AUD 等
    double purchasingAmt = foreignExchangePurchasingInfoObject.value("PurchasingAmt").toDouble();       // 购汇金额
    QString purchasingException = foreignExchangePurchasingInfoObject.value("PurchasingException").toString();  // 购汇异常

    QJsonObject payInfoObject = json.value("PayInfo").toObject();       // 订单支付信息
    double productAmount = payInfoObject.value("ProductAmount").toDouble();     // 商品总金额，保留 2 位小数（如 120.50 或 100.00，无费用时为 0）
    double shippingAmount = payInfoObject.value("ShippingAmount").toDouble();   // 运费总金额，保留 2 位小数
    double taxAmount = payInfoObject.value("TaxAmount").toDouble();     // 商品行邮税总金额，保留 2 位小数
    double commissionAmount = payInfoObject.value("CommissionAmount").toDouble();       // 下单支付产生的手续费，保留 2 位小数
    /// 支付方式编号  112: 支付宝 114: 财付通 117: 银联支付 118: 微信支付
    int payTypeSysNo = payInfoObject.value("PayTypeSysNo").toInt();     // 支付方式编号
    QString paySerialNumber = payInfoObject.value("PaySerialNumber").toString();        // 支付流水号，不能重复，订单申报必要信息
    int payStatusCode = payInfoObject.value("PayStatusCode").toInt();   // 支付状态码  0:未支付 1:已支付未审核 2:已支付审核通过

    QJsonObject shippingInfoObject = json.value("ShippingInfo").toObject();     // 订单配送信息
    QString receiveName = shippingInfoObject.value("ReceiveName").toString();           // 收件人姓名
    QString receivePhone = shippingInfoObject.value("ReceivePhone").toString();         // 收件人电话
    QString receiveAddress = shippingInfoObject.value("ReceiveAddress").toString();     // 收件人收货地址，不包含省市区名称
    /// 收货地区编号，至少需要到市级别（根据中华人民共和国国家统计局提供的《最新县及县以上行政区划代码（截止 2013 年 8月 31 日）》
    /// http://www.stats.gov.cn/tjsj/tjbz/xzqhdm/201401/t20140116_501070.html
    QString receiveAreaCode = shippingInfoObject.value("ReceiveAreaCode").toString();   // 收货地区编号
    QString receiveZip = shippingInfoObject.value("ReceiveZip").toString();             // 收件地邮政编码
    QString shipTypeID = shippingInfoObject.value("ShipTypeID").toString();             // 订单物流运输公司编号，参见附录《跨境通自贸仓支持的配送方式列表》
    QString senderName = shippingInfoObject.value("SenderName").toString();             // 发件人姓名
    QString senderTel = shippingInfoObject.value("SenderTel").toString();               // 发件人电话
    QString senderCompanyName = shippingInfoObject.value("SenderCompanyName").toString();               // 发件人公司
    QString senderAddr = shippingInfoObject.value("SenderAddr").toString();             // 发件人地址
    QString senderZip = shippingInfoObject.value("SenderZip").toString();               // 发件地邮编
    QString senderCity = shippingInfoObject.value("SenderCity").toString();             // 发件地城市
    QString senderProvince = shippingInfoObject.value("SenderProvince").toString();     // 发件地省
    QString senderCountry = shippingInfoObject.value("SenderCountry").toString();       // 发件地国家，例如：USA（代表美国）CHN（代表中国）
    QString receiveAreaName = shippingInfoObject.value("ReceiveAreaName").toString();   // 收件省市区名称（省市区名称之间用半角逗号,隔开，如 上海,上海市,静安区）
    QString trackingNumber = shippingInfoObject.value("TrackingNumber").toString();     // 订单物流运单号

    QJsonObject sOAuthenticationInfoObject = json.value("SOAuthenticationInfo").toObject();         // 下单用户实名认证信息
    QString name = sOAuthenticationInfoObject.value("Name").toString();             // 下单用户真实姓名
    int iDCardType = sOAuthenticationInfoObject.value("IDCardType").toInt();        // 下单用户证件类型（0 – 身份证）
    QString iDCardNumber = sOAuthenticationInfoObject.value("IDCardNumber").toString();             // 下单用户证件编号
    QString phoneNumber = sOAuthenticationInfoObject.value("PhoneNumber").toString();               // 下单用户联系电话
    QString email = sOAuthenticationInfoObject.value("Email").toString();           // 下单用户电子邮件
    QString address = sOAuthenticationInfoObject.value("Address").toString();       // 下单用户联系地址

    QJsonArray itemListObject = json.value("ItemList").toArray();               // 订单中购买商品列表
    QList<SOItemInfo> sOItemInfoList;
    for (int i = 0; i < itemListObject.size(); i++)
    {
        QJsonObject sOItemInfoObject = itemListObject.at(i).toObject();
        SOItemInfo sOitemInfo;
        sOitemInfo._productName = sOItemInfoObject.value("ProductName").toString();             // 商品名称
        sOitemInfo._productId = sOItemInfoObject.value("ProductID").toString();                 // KJT 商品 ID
        sOitemInfo._quantity = sOItemInfoObject.value("Quantity").toInt();                      //
        sOitemInfo._productPrice = sOItemInfoObject.value("ProductPrice").toDouble();
        sOitemInfo._taxPrice = sOItemInfoObject.value("TaxPrice").toDouble();
        sOitemInfo._taxRate = sOItemInfoObject.value("TaxRate").toDouble();
        sOitemInfo._sOItemSysNo = sOItemInfoObject.value("SOItemSysNo").toInt();

        sOItemInfoList.append(sOitemInfo);
    }

    QJsonArray logsArray = json.value("Logs").toArray();                        // 订单日志
    QList<LogInfo> logInfoList;
    for (int i = 0; i < logsArray.size(); i++)
    {
        QJsonObject logObject = logsArray.at(i).toObject();
        LogInfo logInfo;
        logInfo._optTime = convertKjtTime(logObject.value("OptTime").toString());
        logInfo._optType = logObject.value("OptType").toInt();
        logInfo._optNote = logObject.value("OptNote").toString();

        logInfoList.append(logInfo);
    }

    qDebug() << orderId << merchantOrderID << paySerialNumber << qrand();
//    if (10022053 != orderId) return;

    /// 如果“第三方商家订单号”这个字段不为空，则表示是其他平台上传的订单，此处不做下载处理
    if (!merchantOrderID.isNull()) return;

    QSqlQuery query;
    query.prepare(tr("select * from 订单 where 订单类型=:OrderType and 第三方订单号=:MerchantOrderID"));
    query.bindValue(":OrderType", "kjt");
    query.bindValue(":MerchantOrderID", QString::number(orderId));
    if (!query.exec())
    {
        qInfo() << query.lastError().text();
        _ohData._success = false;
    }
    /// 已存在此订单编号，直接返回
    if (query.first())
    {
        qInfo() << tr("此订单已存在。kjt系统订单号：")  + QString::number(orderId);
        return;
    }

    query.prepare(tr("insert into 订单("
                     "订单号, 订单类型, 第三方订单号, 下单日期, 跨境通订单状态, "
                     "贸易类型, 审核日期, 发货日期, 商品总金额, 配送费用, "
                     "税金, 支付手续费, 支付方式, 支付流水号, 付款状态, "
                     "收货人, 手机号码, 收货地址, 邮政编码, 发件人姓名, "
                     "发件人电话, 发件人地址, 发件地邮编, 注册地址, 运单号, 个人姓名, "
                     "纳税人识别号, 注册电话, 电子邮件, 获取时间 "
                     ") values ("
                     ":OrderID, :OrderType, :MerchantOrderID, :OrderDate, :SOStatusCode, "
                     ":TradeType, :AuditTime, :SOOutStockTime, :ProductAmount, :ShippingAmount, "
                     ":TaxAmount, :CommissionAmount, :PayTypeSysNo, :PaySerialNumber, :PayStatusCode, "
                     ":ReceiveName, :ReceivePhone, :ReceiveAddress, :ReceiveZip, :SenderName, "
                     ":SenderTel, :SenderAddr, :SenderZip, :ReceiveAreaName, :TrackingNumber, :Name, "
                     ":IDCardNumber, :PhoneNumber, :Email, :GetTime "
                     ")"));

    /// 订单号规则
    /// 01（表示主订单） + 两位随机数 + 当前时间的日（两位） + 当前时间的月+70（两位） + 四位随机数
    QString orderNumber = QString("%1%2%3%4%5")
            .arg("01")
            .arg(QString::number(qrand() % 100), 2, '0')
            .arg(QDate::currentDate().toString("dd"))
            .arg(QString::number(QDate::currentDate().month() + 70))
            .arg(QString::number(qrand() % 10000), 4, '0');
    query.bindValue(":OrderID", orderNumber);
    query.bindValue(":OrderType", "kjt");               // 固定为 kjt
    query.bindValue(":MerchantOrderID", orderId);
    query.bindValue(":OrderDate", orderDate);
    query.bindValue(":SOStatusCode", sOStatusCode);

    query.bindValue(":TradeType", tradeType);
    query.bindValue(":AuditTime", auditTime);
    query.bindValue(":SOOutStockTime", sOOutStockTime);
    query.bindValue(":ProductAmount", productAmount);
    query.bindValue(":ShippingAmount", shippingAmount);

    query.bindValue(":TaxAmount", taxAmount);
    query.bindValue(":CommissionAmount", commissionAmount);
    query.bindValue(":PayTypeSysNo", payTypeSysNo);
    query.bindValue(":PaySerialNumber", paySerialNumber.isNull() ? "" : paySerialNumber);
    query.bindValue(":PayStatusCode", payStatusCode);

    query.bindValue(":ReceiveName", receiveName);
    query.bindValue(":ReceivePhone", receivePhone.isNull() ? "" : receivePhone);
    query.bindValue(":ReceiveAddress", receiveAddress);
    query.bindValue(":ReceiveZip", receiveZip);
    query.bindValue(":ReceiveAreaName", receiveAreaName);
    query.bindValue(":SenderName", senderName.isNull() ? "" : senderName);

    query.bindValue(":SenderTel", senderTel.isNull() ? "" : senderTel);
    query.bindValue(":SenderAddr", senderAddr.isNull() ? "" : senderAddr);
    query.bindValue(":SenderZip", senderZip.isNull() ? "" : senderZip);
    query.bindValue(":TrackingNumber", trackingNumber.isNull() ? "" : trackingNumber);
    query.bindValue(":Name", name);

    query.bindValue(":IDCardNumber", iDCardNumber);
    query.bindValue(":PhoneNumber", phoneNumber.isNull() ? "" : phoneNumber);
    query.bindValue(":Email", email);
    query.bindValue(":GetTime", QDateTime::currentDateTime());              // 同步时间为当前时间
    if (!query.exec())
    {
//        qFatal(query.lastError().text().toStdString().c_str());
        qInfo() << query.lastError().text();
        _ohData._success = false;
    }
    else
    {
        /// 插入订单商品数据
        int parentId = query.lastInsertId().toInt();        // 订单id
        if (parentId > 0)
        {
            QSqlQuery queryItemInfo;
            foreach (SOItemInfo sOitemInfo, sOItemInfoList) {
                queryItemInfo.prepare(tr("insert into 订单商品 ("
                                         "订单id, 商品名称, 商品编号, 购买数量, 销售单价, "
                                         "税金 "
                                         ") values ("
                                         ":OrderId, :ProductName, :ProductID, :Quantity, :ProductPrice, "
                                         ":TaxPrice "
                                         ")"));
                queryItemInfo.bindValue(":OrderId", parentId);
                queryItemInfo.bindValue(":ProductName", sOitemInfo._productName.isNull() ? "" : sOitemInfo._productName);
                queryItemInfo.bindValue(":ProductID", sOitemInfo._productId.isNull() ? "" : sOitemInfo._productId);
                queryItemInfo.bindValue(":Quantity", sOitemInfo._quantity);
                queryItemInfo.bindValue(":ProductPrice", sOitemInfo._productPrice);
                queryItemInfo.bindValue(":TaxPrice", sOitemInfo._taxPrice);

                if (!queryItemInfo.exec())
                {
                    qInfo() << queryItemInfo.lastError().text();
                    _ohData._success = false;
                }
            }
        }

        /// 写入数据同步记录
        if (parentId > 0)
        {
//            QSqlQuery queryDataSynchronize;
//            queryDataSynchronize.prepare(tr("insert into 数据同步 ("
//                                            "ERP, ERP, "
//                                            ")"));
        }
    }

}

QDateTime OrderDownload::convertKjtTime(const QString &kjtTime)
{
    /// 跨镜通将会把日期修改为 yyyy-MM-dd hh:mm:ss 形式。到时再修改此处
    qint64 utcTime = kjtTime.mid(kjtTime.indexOf("(") + 1, 13).toLongLong();
    return QDateTime::fromMSecsSinceEpoch(utcTime);      // 订单时间
}
