#ifndef KJTSYNCHRONIZE_H
#define KJTSYNCHRONIZE_H

#include <QObject>

class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class KJTSynchronize : public QObject
{
    Q_OBJECT
public:
    enum OptType
    {
        OTNone,
        OTDownloadIdList,               // 下载区间ID列表
        OTDownloading,                  // 下载中
        OTDownloadEnd,                  // 下载结束
        OTUploading,                    // 上传中
        OTUploadEnd,                    // 上传结束
        OTError,                        // 出错
    };

public:
    explicit KJTSynchronize(QObject *parent = 0);

signals:
    void finished(bool success, const QString &msg);

//private slots:
//    void sTimeout();
//    void sReplyFinished(QNetworkReply *reply);

private:
    QTimer *_timer;
    OptType _optType;
    QNetworkAccessManager *_manager;
};

#endif // KJTSYNCHRONIZE_H
