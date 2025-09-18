#include "fuzzymatcher.h"
#include <QRegularExpression>

// 初始化静态成员
bool FuzzyMatcher::enablePinyin = true;

// 拼音映射表初始化
const QMap<QString, QString> FuzzyMatcher::pinyinMap = FuzzyMatcher::initPinyinMap();

FuzzyMatcher::FuzzyMatcher()
{
}

void FuzzyMatcher::setEnablePinyin(bool enable)
{
    enablePinyin = enable;
}

bool FuzzyMatcher::fuzzyMatch(const QString &pattern, const QString &str)
{
    if (pattern.isEmpty()) {
        return true;
    }

    // 1. 首先尝试直接包含匹配
    if (str.contains(pattern, Qt::CaseInsensitive)) {
        return true;
    }

    // 2. 如果启用拼音匹配，尝试拼音首字母匹配
    if (enablePinyin) {
        QString pinyinInitials = getPinyinInitials(str);
        if (pinyinInitials.contains(pattern, Qt::CaseInsensitive)) {
            return true;
        }
    }

    // 3. 尝试模糊匹配（如 "abc" 匹配 "a_b_c"）
    QString regexPattern;
    for (const QChar &c : pattern) {
        regexPattern += QString("[^%1]*%2").arg(c).arg(c);
    }
    regexPattern += ".*";

    QRegularExpression regex(regexPattern, QRegularExpression::CaseInsensitiveOption);
    return regex.match(str).hasMatch();
}

QString FuzzyMatcher::getPinyinInitials(const QString &str)
{
    QString result;
    for (int i = 0; i < str.length(); ++i) {
        QString ch = str.mid(i, 1);
        if (pinyinMap.contains(ch)) {
            result += pinyinMap.value(ch).left(1).toUpper();
        } else {
            // 非中文保持原样
            result += ch.toUpper();
        }
    }
    return result;
}

QMap<QString, QString> FuzzyMatcher::initPinyinMap()
{
    QMap<QString, QString> map;

    map["阿"] = "a";
    map["啊"] = "a";
    map["爱"] = "ai";
    map["安"] = "an";
    map["按"] = "an";
    // 添加更多汉字...
    map["白"] = "bai";
    map["百"] = "bai";
    map["帮"] = "bang";
    map["包"] = "bao";
    map["北"] = "bei";
    map["本"] = "ben";
    map["比"] = "bi";
    map["边"] = "bian";
    map["别"] = "bie";
    map["不"] = "bu";
    // ... 继续添加更多汉字

    return map;
}
static QString createHighlightPattern(const QString &keyword) {
    // 将关键字转换为正则表达式模式，用于高亮匹配
    QString pattern;
    for (const QChar &ch : keyword) {
        if (!pattern.isEmpty()) {
            pattern += ".*?"; // 非贪婪匹配任意字符
        }
        pattern += "(" + QRegularExpression::escape(ch) + ")";
    }
    return pattern;
}


QString FuzzyMatcher::createHighlightPattern(const QString &keyword)  // 添加类作用域
{
    // 将关键字转换为正则表达式模式，用于高亮匹配
    QString pattern;
    for (const QChar &ch : keyword) {
        if (!pattern.isEmpty()) {
            pattern += ".*?"; // 非贪婪匹配任意字符
        }
        pattern += "(" + QRegularExpression::escape(ch) + ")";
    }
    return pattern;
}
