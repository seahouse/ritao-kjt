#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QQueue>
#include <QMap>

class QNetworkAccessManager;
class QNetworkReply;
class OrderCreateKJTToERP;

namespace Ui {
class MainWindow;
}

struct ReplyData
{
    QString _code;
    QString _desc;
};

struct OrderCreateKJTToERPData
{
    ReplyData _replyData;
    int _total;
    QList<int> _orderIdList;
    int _currentIndex;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum SynchronizeType
    {
        STNone,
        STProductCreate,
        STOrderCreateKJTToERP,
        STOrderInfoBatchGet,
    };

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_pushButton_clicked();

    void sReplyFinished(QNetworkReply *reply);

    void on_pushButton_2_clicked();
    void sTimeout();

    void on_pushButton_3_clicked();
    void sOrderCreateKJTToERPFinished(bool error, const QString &msg);

private:
    void synchronizeProductCreate();
    void orderInfoBatchGet();

    void parseReply(const QByteArray &data);

private:
    Ui::MainWindow *ui;

    QNetworkAccessManager *_manager;
    SynchronizeType _synchronizeType;
    QTimer *_timer;

    QQueue<int> _productIdQueue;
    int _currentLocalId;

//    QMap<QString, QString> _paramsMap;
    OrderCreateKJTToERPData _orderCreateKJTToERPData;

    OrderCreateKJTToERP *_orderCreateKJTToERP;
};

#endif // MAINWINDOW_H
