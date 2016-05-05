#include "kjtsynchronize.h"

#include <QTimer>
#include <QNetworkAccessManager>

KJTSynchronize::KJTSynchronize(QObject *parent) : QObject(parent),
    _optType(OTNone)
{
    _timer = new QTimer;
//    connect(_timer, SIGNAL(timeout()), this, SLOT(sTimeout()));

    _manager = new QNetworkAccessManager;
//    connect(_manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(sReplyFinished(QNetworkReply*)));
}

