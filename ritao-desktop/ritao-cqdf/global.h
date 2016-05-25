#ifndef GLOBAL
#define GLOBAL

#include <QString>
#include <QMap>

#ifndef QT_NO_DEBUG
//#define kjt_url "http://preapi.kjt.com/open.api"
//#define kjt_secretkey "kjt@345"      // 由接口提供方分配给接口调用方的验签密钥
//#define kjt_appid "seller345"        // 由接口提供方分配给接口调用方的身份标识符
//#define kjt_saleschannelsysno  "1106"   // 渠道编号

//#define kjt_url "http://api.kjt.com/open.api"
//#define kjt_secretkey "ecf48175ec0341a695ee938592bdfdcb"      // 由接口提供方分配给接口调用方的验签密钥
//#define kjt_appid "seller1383"        // 由接口提供方分配给接口调用方的身份标识符
//#define kjt_saleschannelsysno  "73"   // 渠道编号

//#define kjt_version "1.0"            // 由接口提供方指定的接口版本
//#define kjt_format "json"            // 接口返回结果类型:json
#else
#define kjt_url "http://api.kjt.com/open.api"
#define kjt_secretkey "ecf48175ec0341a695ee938592bdfdcb"      // 由接口提供方分配给接口调用方的验签密钥
#define kjt_appid "seller1383"        // 由接口提供方分配给接口调用方的身份标识符
#define kjt_saleschannelsysno  "73"   // 渠道编号

#define kjt_version "1.0"            // 由接口提供方指定的接口版本
#define kjt_format "json"            // 接口返回结果类型:json
#endif

#define date_format_str     "yyyy-MM-dd hh:mm:ss"
#define hg_receiver         "CQITC"

extern QMap<QString, QString> g_paramsMap;

/// 信息类型
enum MsgType
{
    MTDebug,
    MTInfo,
};

/// 将转换后的code，继续替换%字符
void urlencodePercentConvert(QString &str);
//extern QByteArray toPercentEncoding_exclude;

#endif // GLOBAL

