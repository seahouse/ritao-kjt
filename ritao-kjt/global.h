#ifndef GLOBAL
#define GLOBAL

#include <QString>
#include <QMap>

#ifndef QT_NO_DEBUG
#define kjt_url "http://preapi.kjt.com/open.api"
#else
#define kjt_url "http://preapi.kjt.com/open.api"
#endif

#define kjt_secretkey "kjt@345"      // 由接口提供方分配给接口调用方的验签密钥

#define kjt_appid "seller345"        // 由接口提供方分配给接口调用方的身份标识符
#define kjt_version "1.0"            // 由接口提供方指定的接口版本
#define kjt_format "json"            // 接口返回结果类型:json

extern QMap<QString, QString> g_paramsMap;

/// 信息类型
enum MsgType
{
    MTDebug,
    MTInfo,
};

#endif // GLOBAL

