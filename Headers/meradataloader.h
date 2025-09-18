#ifndef MERADATALOADER_H
#define MERADATALOADER_H

#include <QObject>
#include "musicmetadata.h"
#include "metadataparser.h"


// 媒体元数据异步加载优化,使用QThread实现后台加载
class MetadataLoader : public QObject
{
    Q_OBJECT
public:
    explicit MetadataLoader(QObject *parent = nullptr);  // 构造函数声明

public slots:
    void loadMetadata(const QString &filePath);

signals:
    void metadataLoaded(const QString &filePath, const MusicMetadata &metadata);
    void loadFinished();
};

#endif // MERADATALOADER_H
