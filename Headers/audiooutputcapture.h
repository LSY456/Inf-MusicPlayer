#ifndef AUDIOOUTPUTCAPTURE_H
#define AUDIOOUTPUTCAPTURE_H

#include <QAudioSink>
#include <QMediaDevices>
#include <QDebug>

class AudioOutputCapture : public QObject {
    Q_OBJECT
public:
    explicit AudioOutputCapture(QObject *parent = nullptr) : QObject(parent) {
        // 配置音频格式（需与播放内容匹配）
        QAudioFormat format;
        format.setSampleRate(44100);
        format.setChannelCount(2); // 立体声
        format.setSampleFormat(QAudioFormat::Float);

        // 获取默认输出设备
        QAudioDevice outputDevice = QMediaDevices::defaultAudioOutput();

        // 创建音频接收器
        audioSink = new QAudioSink(outputDevice, format, this);
        connect(audioSink, &QAudioSink::stateChanged, this, [](QAudio::State state) {
            qDebug() << "Audio Output State:" << state;
        });

        // 创建虚拟设备接收数据
        ioDevice = audioSink->start();
        connect(ioDevice, &QIODevice::readyRead, this, &AudioOutputCapture::readData);
    }

private slots:
    void readData() {
        QByteArray buffer = ioDevice->readAll();
        processOutputAudio(buffer); // 处理输出音频数据
    }

private:
    QAudioSink *audioSink;
    QIODevice *ioDevice;

    void processOutputAudio(const QByteArray &data) {
        // 转换为浮点数样本（假设格式为Float）
        const float *samples = reinterpret_cast<const float*>(data.constData());
        int numSamples = data.size() / sizeof(float);

        // 转换为单声道（平均值）
        QVector<qreal> mono;
        for (int i = 0; i < numSamples; i += 2) {
            mono.append((samples[i] + samples[i+1]) / 2.0);
        }

        emit outputDataReady(mono);
    }

signals:
    void outputDataReady(const QVector<qreal> &samples);
};

#endif // AUDIOOUTPUTCAPTURE_H
