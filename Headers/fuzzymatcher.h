#ifndef FUZZYMATCHER_H
#define FUZZYMATCHER_H

#include <QString>
#include <QStringList>

class FuzzyMatcher
{
public:
    FuzzyMatcher();

    // 设置是否启用拼音匹配
    static void setEnablePinyin(bool enable);

    // 模糊匹配函数
    static bool fuzzyMatch(const QString &pattern, const QString &str);

    // 获取拼音首字母
    static QString getPinyinInitials(const QString &str);

    static QString createHighlightPattern(const QString &keyword);  // 在类声明中添加

private:
    // 中文转拼音映射表
    static QMap<QString, QString> initPinyinMap();
    static const QMap<QString, QString> pinyinMap;

    static bool enablePinyin;
};

#endif // FUZZYMATCHER_H
