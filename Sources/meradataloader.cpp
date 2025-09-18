#include "meradataloader.h"

MetadataLoader::MetadataLoader(QObject *parent)  // 构造函数实现
    : QObject(parent)
{
    // 初始化代码（如果需要）
}

void MetadataLoader::loadMetadata(const QString &filePath)
{
    MusicMetadata metadata = MetadataParser::extractMetadata(filePath);
    emit metadataLoaded(filePath, metadata);
    emit loadFinished();
}
