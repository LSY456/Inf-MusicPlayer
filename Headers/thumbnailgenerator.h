#ifndef THUMBNAILGENERATOR_H
#define THUMBNAILGENERATOR_H

#include <QObject>
#include <QPixmap>
#include <QString>

//缩略图异步生成与显示
class ThumbnailGenerator : public QObject
{
    Q_OBJECT
public:
    explicit ThumbnailGenerator(QObject *parent = nullptr);

    // 设置缩略图生成质量
    void setQuality(int quality); // 1-100
    int quality() const;

    // 设置默认缩略图大小
    void setDefaultSize(const QSize &size);
    QSize defaultSize() const;

public slots:
    // 生成缩略图的主要方法
    void generateThumbnail(const QString &filePath, qint64 timeMs = 5000);
    void generateThumbnail(const QString &filePath, const QSize &size, qint64 timeMs = 5000);

signals:
    // 缩略图生成完成信号
    void thumbnailGenerated(const QString &filePath, const QPixmap &thumbnail);
    // 生成失败信号
    void generationFailed(const QString &filePath, const QString &error);

private:
    int m_quality = 85; // 默认JPEG质量
    QSize m_defaultSize = QSize(160, 90); // 默认缩略图大小
};

#endif // THUMBNAILGENERATOR_H
