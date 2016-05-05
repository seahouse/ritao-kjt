#ifndef RNETWORKACCESSMANAGER_H
#define RNETWORKACCESSMANAGER_H

#include <QNetworkAccessManager>

class RNetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT
public:
    RNetworkAccessManager(QObject *parent = 0);

private slots:
    void sFinished(QNetworkReply *reply);
};

extern RNetworkAccessManager g_networkManager;

#endif // RNETWORKACCESSMANAGER_H
