#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "ordercreatekjttoerp.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <time.h>
#include <QSqlQuery>
#include <QFile>
#include <QTimer>
#include <QSqlError>

#include "global.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    _synchronizeType(STNone),
    _currentLocalId(-1)
{
    ui->setupUi(this);

    qsrand(time(NULL));

    _manager = new QNetworkAccessManager;
    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));

    _timer = new QTimer;
    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    g_paramsMap["appid"] = kjt_appid;
    g_paramsMap["version"] = kjt_version;               // 由接口提供方指定的接口版本
    g_paramsMap["format"] = kjt_format;               // 接口返回结果类型:json

    _orderCreateKJTToERP = new OrderCreateKJTToERP();
    connect(_orderCreateKJTToERP, SIGNAL(finished(bool,QString)), this, SLOT(sOrderCreateKJTToERPFinished(bool, QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()
{
    QString url = "http://preapi.kjt.com/open.api";     //接口测试地址
    QString secretkey = "kjt@345";              // 由接口提供方分配给接口调用方的验签密钥

    QMap<QString, QString> paramsMap;
    paramsMap["appid"] = "seller345";           // 由接口提供方分配给接口调用方的身份标识符
    paramsMap["method"] = "Product.ProudctInfoBatchGet";    // 由接口提供方指定的接口标识符
    paramsMap["version"] = "1.0";               // 由接口提供方指定的接口版本
    paramsMap["format"] = "json";               // 接口返回结果类型:json
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
//    qsrand(time(NULL));
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    QJsonArray jsonArray;
    jsonArray.append(QJsonValue("345JPA018400002"));
    json["ProductIDs"] = jsonArray;
    json["SaleChannelSysNo"] = 1106;
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

void MainWindow::sReplyFinished(QNetworkReply *reply)
{
    QByteArray replyData = reply->readAll();
    ui->textEdit->setText(replyData);

    parseReply(replyData);

//    switch (_synchronizeType) {
//    case STProductCreate:
//        parseReply(replyData);
//        break;
//    case STOrderCreateKJTToERP:
//        break;
//    default:
//        break;
//    }
}

void MainWindow::on_pushButton_2_clicked()
{
    _synchronizeType = STProductCreate;
    _productIdQueue.clear();

    /// 获取需要新增到跨境通的商品KID列表
    QSqlQuery query(tr("select 同步主键KID from 数据同步 "
                       "where 跨境通=1 and 跨境通处理=0 and 同步指令='新增' and 同步表名='商品' "
                       "order by 数据同步KID "));
    while (query.next())
        _productIdQueue.enqueue(query.value(tr("同步主键KID")).toInt());

    qDebug() << _productIdQueue;
    synchronizeProductCreate();
}

void MainWindow::sTimeout()
{
    _timer->stop();
}

void MainWindow::synchronizeProductCreate()
{
    if (_productIdQueue.isEmpty()) return;

    _currentLocalId = _productIdQueue.dequeue();

    QSqlQuery query;
    query.prepare(tr("select * from 商品 "
                     "where 商品KID=:id "));
    query.bindValue(":id", _currentLocalId);
    if (!query.exec())
    {
        qFatal(query.lastError().text().toStdString().c_str());
        return;
    }

    if (query.first())
    {
        qDebug() << query.value(tr("商品KID")).toInt() << "\t" << query.value(tr("商品名称")).toString();

        QString url = "http://preapi.kjt.com/open.api";     //接口测试地址
        QString secretkey = "kjt@345";              // 由接口提供方分配给接口调用方的验签密钥

        QMap<QString, QString> paramsMap;
        paramsMap["appid"] = "seller345";           // 由接口提供方分配给接口调用方的身份标识符
        paramsMap["method"] = "Product.ProductCreate";    // 由接口提供方指定的接口标识符
        paramsMap["version"] = "1.0";               // 由接口提供方指定的接口版本
        paramsMap["format"] = "json";               // 接口返回结果类型:json
        paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
        paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

        QJsonObject json;
        json["IsSettledDown"] = query.value(tr("入住商品")).toInt();     // 是否为入驻商品。0 = 否 1 = 是
        json["MerchantProductID"] = query.value(tr("商品KID")).toString();
        json["ProductName"] = query.value(tr("商品名称")).toString();
        json["BriefName"] = "HW"; // query.value(tr("品名简称")).toString();    // 商品简称
        json["BrandCode"] = "950"; // query.value(tr("商品品牌ID")).toString();    // 品牌编号 code
        json["C3Code"] = "A46"; // query.value(tr("商品分类ID")).toString();
        json["ProductTradeType"] = query.value(tr("贸易类型")).toInt();
        json["OriginCode"] = "JP";  // query.value(tr("产地")).toString();      // 产地，两位字母
        json["ProductDesc"] = query.value(tr("商品简述")).toString();

        QJsonObject productPriceInfoJsonObject;
        productPriceInfoJsonObject["CurrentPrice"] = query.value(tr("销售价")).toDouble();
        json["ProductPriceInfo"] = productPriceInfoJsonObject;

        QJsonObject productEntryInfoJsonObject;
        productEntryInfoJsonObject["ProductNameEN"] = query.value(tr("商品英文名称")).toString();
        productEntryInfoJsonObject["Specifications"] = tr("30＊10片");   // ?? 30*10 // query.value(tr("商品规格")).toString();
        productEntryInfoJsonObject["TaxUnit"] = "g";   // query.value(tr("计税单位")).toString(); // 计税单位, 不能为空!!
        productEntryInfoJsonObject["CustomsCode"] = "2244"; // query.value(tr("关区代码")).toString();  // 海关关区根据商品所入仓库对应的四位数关区代码填写 2244 – 直邮进口模式 2216 – 浦东机场自贸模式 2249 – 洋山港自贸模式 2218 – 外高桥自贸模式
        productEntryInfoJsonObject["StoreType"] = 0;    // query.value(tr("运输方式")).toString();
        productEntryInfoJsonObject["ApplyUnit"] = "123";    // query.value(tr("申报单位")).toString();  // 申报单位, 不能为空
        productEntryInfoJsonObject["ApplyQty"] = 123;   // query.value(tr("申报数量")).toString();  // 申报数量, 不能为空
        productEntryInfoJsonObject["GrossWeight"] = 12.0;   // query.value(tr("商品毛重")).toDouble();
        productEntryInfoJsonObject["SuttleWeight"] = 10.0;  // query.value(tr("商品净重")).toDouble();
        json["ProductEntryInfo"] = productEntryInfoJsonObject;

        QJsonObject productMaintainInfoJsonObject;
        productMaintainInfoJsonObject["ProductModel"] = "123";  // query.value(tr("商品型号")).toString();
        productMaintainInfoJsonObject["Weight"] = 10.0;     // query.value(tr("商品物流重量")).toDouble();
        productMaintainInfoJsonObject["Length"] = query.value(tr("长度")).toDouble();
        productMaintainInfoJsonObject["Width"] = query.value(tr("宽度")).toDouble();
        productMaintainInfoJsonObject["Height"] = query.value(tr("高度")).toDouble();
        json["ProductMaintainInfo"] = productMaintainInfoJsonObject;


        QJsonDocument jsonDoc(json);
        QFile file("11.txt");
        if (file.open(QIODevice::WriteOnly))
        {
            QTextStream out(&file);
            out << jsonDoc.toJson(QJsonDocument::Compact);
            file.close();
        }
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
//        params.append(secretkey);
        QString sign = QCryptographicHash::hash(QString(params + secretkey).toLatin1(), QCryptographicHash::Md5).toHex();
        params.append("sign=").append(sign);
        qDebug() << params.toLatin1();

//        url.append("?").append(params).append("&sign=").append(sign);

        qDebug() << url;
        QNetworkRequest req;
        req.setUrl(QUrl(url));
        req.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
        req.setHeader(QNetworkRequest::ContentLengthHeader, params.toLatin1().length());
        _manager->post(req, params.toLatin1());

    }
}

void MainWindow::orderInfoBatchGet()
{
    _synchronizeType = STOrderInfoBatchGet;
    if (_orderCreateKJTToERPData._currentIndex >= _orderCreateKJTToERPData._total) return;

    QMap<QString, QString> paramsMap(g_paramsMap);
    paramsMap["method"] = "Order.OrderInfoBatchGet";    // 由接口提供方指定的接口标识符
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    QJsonArray orderIdListArray;
    foreach (int orderId, _orderCreateKJTToERPData._orderIdList.mid(_orderCreateKJTToERPData._currentIndex, 20)) {
        orderIdListArray.append(QJsonValue(orderId));
    }
    _orderCreateKJTToERPData._currentIndex += 20;
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

void MainWindow::parseReply(const QByteArray &data)
{
    QJsonObject json(QJsonDocument::fromJson(data).object());
    QString code = json.value("Code").toString("-99");
    QString desc = json.value("Desc").toString();

    QString opt;
    switch (_synchronizeType) {
    case STProductCreate:
        opt = tr("新增商品");
        if (code == "0")
        {
            /// 记录同步数据，并进行下一个跨境通同步
            QSqlQuery query;
            query.prepare(tr("update 数据同步 set 跨境通处理=1 "
                             "where 跨境通=1 and 同步指令='新增' and 同步表名='商品' and 同步主键KID=:id "));
            query.bindValue(":id", _currentLocalId);
            query.exec();
            synchronizeProductCreate();
        }
        break;
    case STOrderCreateKJTToERP:
        opt = tr("下载订单");
        _orderCreateKJTToERPData._replyData._code = code;
        _orderCreateKJTToERPData._replyData._desc = desc;
        if ("0" == code)
        {
            /// 读取订单id列表
            QJsonObject data = json.value("Data").toObject();
            _orderCreateKJTToERPData._total = data.value("Total").toInt();
            QJsonArray orderIdListArray = data.value("OrderIDList").toArray();
            QList<int> orderIdList;
            for (int i = 0; i < orderIdListArray.size(); i++)
                orderIdList.append(orderIdListArray.at(i).toInt());
            _orderCreateKJTToERPData._orderIdList = orderIdList;
            qDebug() << "orderIdList:" << _orderCreateKJTToERPData._orderIdList;

            _orderCreateKJTToERPData._currentIndex = 0;
            orderInfoBatchGet();
        }
        break;
    case STOrderInfoBatchGet:
        opt = tr("下载订单");
        /// 解析数据，写入ERP数据库，并继续请求数据
        if ("0" == code)
        {
            QJsonObject data = json.value("Data").toObject();
            QJsonArray orderListArray = data.value("OrderList").toArray();
            for (int i = 0; i < orderListArray.size(); i++)
                insertOrder2ERPByJson(orderListArray.at(i).toObject());
        }
        orderInfoBatchGet();
        break;
    default:
        break;
    }

    qInfo() << opt << code << desc;
}

void MainWindow::on_pushButton_3_clicked()
{
    _synchronizeType = STOrderCreateKJTToERP;

    _orderCreateKJTToERP->run(ui->dateTimeEdit->dateTime(), ui->dateTimeEdit_2->dateTime());

    QMap<QString, QString> paramsMap;
    paramsMap["appid"] = kjt_appid;
    paramsMap["method"] = "Order.OrderIDQuery";    // 由接口提供方指定的接口标识符
    paramsMap["version"] = kjt_version;               // 由接口提供方指定的接口版本
    paramsMap["format"] = kjt_format;               // 接口返回结果类型:json
    paramsMap["timestamp"] = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");       // 调用方时间戳，格式为“4 位年+2 位月+2 位日+2 位小时(24 小时制)+2 位分+2 位秒”
//    qsrand(time(NULL));
    paramsMap["nonce"] = QString::number(100000 + qrand() % (999999 - 100000)); // QString::number(100000 + qrand() % (999999 - 100000));

    QJsonObject json;
    json["OrderDateBegin"] = "2015-01-01 00:00:00";
    json["OrderDateEnd"] = "2016-01-01 00:00:00";
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

void MainWindow::sOrderCreateKJTToERPFinished(bool error, const QString &msg)
{
    qDebug() << error << msg;
}

void MainWindow::insertOrder2ERPByJson(const QJsonObject &json)
{
    int orderId = json.value("OrderID").toInt();        // Kjt 系统订单号
    int merchantSysNo = json.value("MerchantSysNo").toInt();    //
    QString merchantOrderID = json.value("MerchantOrderID").toString();     // 第三方商家订单号
    QString orderDate = json.value("OrderDate").toString();     // 订单时间 ?? 解析规则 ??
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
    int sOStatusCode = json.value("SOStatusCode").toInt();      // 订单当前状态代码
    QString sOStatusDescription = json.value("SOStatusDescription").toString(); // 订单当前状态描述
    int tradeType = json.value("TradeType").toInt();            // 贸易类型  0：直邮 1：自贸
    int warehouseID = json.value("WarehouseID").toInt();        // 仓库编号  51 = 浦东机场自贸仓 52 = 洋山港自贸仓 53 = 外高桥自贸仓
    QString auditTime = json.value("AuditTime").toString();     // 审核时间
    QString sOOutStockTime = json.value("SOOutStockTime").toString();   // 出库时间
    QString sOOutCustomsTime = json.value("SOOutCustomsTime").toString();   // 出区时间
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
    int payStatusCode = payInfoObject.value("PayStatusCode").toInt();   // 0:未支付 1:已支付未审核 2:已支付审核通过

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
        sOitemInfo._quantity = sOItemInfoObject.value("Quantity").toInt();
        sOitemInfo._productPrice = sOItemInfoObject.value("ProductPrice").toDouble();
        sOitemInfo._taxPrice = sOItemInfoObject.value("TaxPrice").toDouble();
        sOitemInfo._taxRate = sOItemInfoObject.value("TaxRate").toDouble();
        sOitemInfo._sOItemSysNo = sOItemInfoObject.value("SOItemSysNo").toInt();

        sOItemInfoList.append(sOitemInfo);
    }

    QJsonArray logsArray = json.value("Logs").toArray();
    QList<LogInfo> logInfoList;
    for (int i = 0; i < logsArray.size(); i++)
    {
        QJsonObject logObject = logsArray.at(i).toObject();
        LogInfo logInfo;
        logInfo._optTime = logObject.value("OptTime").toString();
        logInfo._optType = logObject.value("OptType").toInt();
        logInfo._optNote = logObject.value("OptNote").toString();
    }

    qDebug() << orderId;
}
