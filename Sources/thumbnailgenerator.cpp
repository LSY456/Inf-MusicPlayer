#include "thumbnailgenerator.h"
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QVideoSink>
#include <QImage>
#include <QDebug>
#include <QEventLoop>
#include <QTimer>
#include <QVideoFrame>

ThumbnailGenerator::ThumbnailGenerator(QObject *parent)
    : QObject(parent)
{
}

void ThumbnailGenerator::setQuality(int quality)
{
    m_quality = qBound(1, quality, 100);
}

int ThumbnailGenerator::quality() const
{
    return m_quality;
}

void ThumbnailGenerator::setDefaultSize(const QSize &size)
{
    if (size.isValid()) {
        m_defaultSize = size;
    }
}

QSize ThumbnailGenerator::defaultSize() const
{
    return m_defaultSize;
}

void ThumbnailGenerator::generateThumbnail(const QString &filePath, qint64 timeMs)
{
    generateThumbnail(filePath, m_defaultSize, timeMs);
}

void ThumbnailGenerator::generateThumbnail(const QString &filePath, const QSize &size, qint64 timeMs)
{
    if (filePath.isEmpty() || !size.isValid()) {
        emit generationFailed(filePath, "Invalid parameters");
        return;
    }

    QMediaPlayer *player = new QMediaPlayer(this);
    QVideoWidget *videoWidget = new QVideoWidget;
    player->setVideoOutput(videoWidget);
    player->setSource(QUrl::fromLocalFile(filePath));

    // 使用智能指针确保资源释放
    QScopedPointer<QMediaPlayer> playerGuard(player);
    QScopedPointer<QVideoWidget> videoWidgetGuard(videoWidget);

    // 等待媒体加载的事件循环
    QEventLoop loadLoop;
    QTimer loadTimeout;
    loadTimeout.setSingleShot(true);

    QObject::connect(player, &QMediaPlayer::mediaStatusChanged, &loadLoop, [&](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            loadLoop.quit();
        } else if (status == QMediaPlayer::InvalidMedia) {
            loadLoop.quit();
        }
    });

    QObject::connect(&loadTimeout, &QTimer::timeout, &loadLoop, &QEventLoop::quit);
    loadTimeout.start(5000); // 5秒超时

    loadLoop.exec();

    if (player->mediaStatus() == QMediaPlayer::InvalidMedia) {
        emit generationFailed(filePath, "Failed to load media");
        return;
    }

    if (loadTimeout.isActive()) {
        loadTimeout.stop();
    } else {
        // 超时情况
        emit generationFailed(filePath, "Media loading timeout");
        return;
    }

    // 定位到指定时间
    player->setPosition(timeMs);

    // 等待定位完成
    QEventLoop seekLoop;
    QTimer seekTimeout;
    seekTimeout.setSingleShot(true);

    bool seekDone = false;
    QObject::connect(player, &QMediaPlayer::positionChanged, &seekLoop, [&](qint64 pos) {
        if (qAbs(pos - timeMs) < 100) { // 允许100ms误差
            seekDone = true;
            seekLoop.quit();
        }
    });

    QObject::connect(&seekTimeout, &QTimer::timeout, &seekLoop, &QEventLoop::quit);
    seekTimeout.start(3000); // 3秒超时

    seekLoop.exec();

    if (!seekDone) {
        emit generationFailed(filePath, "Seek operation timeout");
        return;
    }

    // 获取视频帧
    QVideoSink *sink = player->videoSink();
    if (!sink) {
        emit generationFailed(filePath, "No video sink available");
        return;
    }

    QVideoFrame frame = sink->videoFrame();
    if (!frame.isValid()) {
        emit generationFailed(filePath, "Invalid video frame");
        return;
    }

    // 转换为图像
    QImage image = frame.toImage();
    if (image.isNull()) {
        emit generationFailed(filePath, "Failed to convert frame to image");
        return;
    }

    // 缩放并转换为缩略图
    QPixmap thumbnail = QPixmap::fromImage(image.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    if (thumbnail.isNull()) {
        emit generationFailed(filePath, "Failed to create thumbnail");
        return;
    }

    emit thumbnailGenerated(filePath, thumbnail);
}
