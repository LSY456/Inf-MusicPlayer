#ifndef MATADATAPARSER_H
#define MATADATAPARSER_H
#include <QObject>
#include "musicmetadata.h"
#include <taglib.h>
#include <tag.h>


class MetadataParser
{
public:
    static MusicMetadata extractMetadata(const QString &filePath);

private:
    QString parseLyrics(TagLib::Tag *tag);
};

#endif // MATADATAPARSER_H
