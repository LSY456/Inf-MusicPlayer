#ifndef AUDIOMETADATA_H
#define AUDIOMETADATA_H

#include <QString>
#include <QByteArray>
#include <QImage>

// 自定义元数据结构体
struct MusicMetadata {
    QString title;
    QString artist;
    QString album;
    QString genre;

    QString lyrics; // 新增歌词字段
    int year = 0;
    int track = 0;
    int duration = 0; // 秒k
    int bitrate = 0;  // kbps
    int sampleRate = 0; // Hz
    QImage cover;
};

#endif // AUDIOMETADATA_H


