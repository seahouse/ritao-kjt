#include "global.h"

QMap<QString, QString> g_paramsMap;
//g_paramsMap.insert("1", "2");
//g_paramsMap["appid"] = kjt_appid;
//g_paramsMap["version"] = kjt_version;               // 由接口提供方指定的接口版本
//g_paramsMap["format"] = kjt_format;               // 接口返回结果类型:json

void urlencodePercentConvert(QString &str)
{
    str.replace("%20", "+");
    str.replace("%2A", "*");
    str.replace("%28", "(");
}

//QByteArray toPercentEncoding_exclude = "+*()";
