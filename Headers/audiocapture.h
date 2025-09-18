#ifndef AUDIOCAPTURE_H
#define AUDIOCAPTURE_H

#include <QAudioSource>
#include <QIODevice>
#include <QMediaDevices>
#include <QDebug>

class AudioCapture : public QObject {
    Q_OBJECT
public:
    explicit AudioCapture(QObject *parent = nullptr) : QObject(parent) {
        // 配置音频格式
        QAudioFormat format;
        format.setSampleRate(44100);        // 采样率
        format.setChannelCount(1);          // 单声道
        format.setSampleFormat(QAudioFormat::Int16); // 16位有符号整数

        // 获取默认输入设备
        QAudioDevice inputDevice = QMediaDevices::defaultAudioInput();

        // 创建音频源
        audioSource = new QAudioSource(inputDevice, format, this);
        connect(audioSource, &QAudioSource::stateChanged, this, [](QAudio::State state) {
            qDebug() << "Audio Input State:" << state;
        });

        // 启动捕获
        ioDevice = audioSource->start();
        connect(ioDevice, &QIODevice::readyRead, this, &AudioCapture::readData);
    }

private slots:
    void readData() {
        QByteArray buffer = ioDevice->readAll();
        processRawAudio(buffer); // 处理原始音频数据
    }

private:
    QAudioSource *audioSource;
    QIODevice *ioDevice;

    void processRawAudio(const QByteArray &data) {
        // 转换为16位整数样本
        const qint16 *samples = reinterpret_cast<const qint16*>(data.constData());
        int numSamples = data.size() / sizeof(qint16);

        // 转换为浮点数并归一化到[-1, 1]
        QVector<qreal> normalized;
        for (int i = 0; i < numSamples; ++i) {
            normalized.append(samples[i] / 32768.0);
        }

        emit audioDataReady(normalized);
    }

signals:
    void audioDataReady(const QVector<qreal> &samples);
};

#endif // AUDIOCAPTURE_H
