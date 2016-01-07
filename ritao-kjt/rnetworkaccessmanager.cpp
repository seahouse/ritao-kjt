#include "rnetworkaccessmanager.h"

RNetworkAccessManager g_networkManager;

RNetworkAccessManager::RNetworkAccessManager(QObject *parent) :
    QNetworkAccessManager(parent)
{
    connect(this, SIGNAL(finished(QNetworkReply*)), this, SLOT(sFinished(QNetworkReply*)));
}

void RNetworkAccessManager::sFinished(QNetworkReply *reply)
{

}
