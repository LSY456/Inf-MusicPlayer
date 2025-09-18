#include "metadataparser.h"

// TagLib 头文件
#include <taglib/fileref.h>
#include <taglib/tag.h>
#include <taglib/id3v2tag.h>
#include <taglib/mpegfile.h>
#include <taglib/attachedpictureframe.h>
#include <taglib/flacfile.h>
#include <taglib/flacpicture.h>
#include <taglib/tpropertymap.h>
#include <taglib/unsynchronizedlyricsframe.h> // 用于歌词帧
#include <taglib/xiphcomment.h>  // 添加 XiphComment 的完整定义
#include <taglib/mp4file.h>
#include <taglib/mp4tag.h>
#include <taglib/mp4coverart.h>



//matadataparser::matadataparser()
//{



//}

// 解析元数据函数
MusicMetadata MetadataParser::extractMetadata(const QString &filePath) {
    MusicMetadata data;
    TagLib::FileRef fileRef(filePath.toUtf8().constData());

    if (!fileRef.isNull() && fileRef.tag()) {
        TagLib::Tag *tag = fileRef.tag();
        data.title = QString::fromStdString(tag->title().to8Bit(true));
        data.artist = QString::fromStdString(tag->artist().to8Bit(true));
        data.album = QString::fromStdString(tag->album().to8Bit(true));
        data.genre = QString::fromStdString(tag->genre().to8Bit(true));
        data.year = tag->year();
        data.track = tag->track();

        // 解析时长和音频属性
        if (fileRef.audioProperties()) {
            TagLib::AudioProperties *props = fileRef.audioProperties();
            data.duration = props->lengthInSeconds();
            data.bitrate = props->bitrate();
            data.sampleRate = props->sampleRate();
        }

        // 解析封面（MP3/FLAC）
        if (filePath.endsWith(".mp3")) {
            auto *mpegFile = dynamic_cast<TagLib::MPEG::File*>(fileRef.file());
            if (mpegFile && mpegFile->ID3v2Tag()) {
                auto frames = mpegFile->ID3v2Tag()->frameList("APIC");
                if (!frames.isEmpty()) {
                    auto *frame = dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frames.front());
                    QByteArray imgData(frame->picture().data(), frame->picture().size());
                    data.cover = QImage::fromData(imgData);
                }
            }
        } else if (filePath.endsWith(".flac")) {
            auto *flacFile = dynamic_cast<TagLib::FLAC::File*>(fileRef.file());
            if (flacFile && !flacFile->pictureList().isEmpty()) {
                auto *picture = flacFile->pictureList().front();
                QByteArray imgData(picture->data().data(), picture->data().size());
                data.cover = QImage::fromData(imgData);
            }
        }// 视频封面解析
        else if (filePath.endsWith(".mp4", Qt::CaseInsensitive)) {
            TagLib::MP4::File mp4File(filePath.toUtf8().constData());
            if (mp4File.isValid() && mp4File.tag()) {
                TagLib::MP4::Tag* tag = mp4File.tag();
                TagLib::MP4::ItemListMap items = tag->itemListMap();

                if (items.contains("covr")) {
                    TagLib::MP4::CoverArtList coverList = items["covr"].toCoverArtList();
                    if (!coverList.isEmpty()) {
                        TagLib::MP4::CoverArt coverArt = coverList.front();
                        QByteArray imgData(coverArt.data().data(), coverArt.data().size());
                        data.cover = QImage::fromData(imgData);
                    }
                }
            }
        }
        // 新增歌词解析逻辑
        if (filePath.endsWith(".mp3")) {
            TagLib::MPEG::File mpegFile(filePath.toUtf8().constData());
            if (mpegFile.ID3v2Tag()) {
                // 获取所有歌词帧
                auto frames = mpegFile.ID3v2Tag()->frameList("USLT");
                if (!frames.isEmpty()) {
                    for (auto* frame : frames) {
                        auto lyricsFrame = dynamic_cast<TagLib::ID3v2::UnsynchronizedLyricsFrame*>(frame);
                        if (lyricsFrame) {
                            data.lyrics += QString::fromStdWString(lyricsFrame->text().toWString()) + "\n";
                        }
                    }
                }
            }
        }
        else if (filePath.endsWith(".flac")) {
            auto *flacFile = dynamic_cast<TagLib::FLAC::File*>(fileRef.file());
            if (flacFile && flacFile->xiphComment()) {
                auto commentMap = flacFile->xiphComment()->fieldListMap();
                if (commentMap.contains("LYRICS")) {
                    for (auto& lyric : commentMap["LYRICS"]) {
                        data.lyrics += QString::fromStdString(lyric.to8Bit(true)) + "\n";
                    }
                }
            }
        }
    }

    // 设置默认封面（如果解析失败）
    if (data.cover.isNull()) {
        data.cover = QImage("/home/Inf/Desktop/DtkProject/Inf_player/bg-onelastkiss.jpg");
    }


    // 清理多余空行
    data.lyrics = data.lyrics.trimmed();

    return data;
}

