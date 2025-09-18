#ifndef WAVEFORM_H
#define WAVEFORM_H


// waveform.h
#pragma once
#include <QWidget>
#include <QVector>
#include <QPainter>

class WaveformWidget : public QWidget {
    Q_OBJECT
public:
    explicit WaveformWidget(QWidget *parent = nullptr) : QWidget(parent) {}

public slots:
    void updateWaveform(const QVector<qreal> &samples) {
        m_samples = samples;
        update();
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        painter.setPen(QPen(Qt::green, 2));

        if (m_samples.isEmpty()) return;

        int width = this->width();
        int height = this->height();

        for (int i = 0; i < m_samples.size(); ++i) {
            int x = i * width / m_samples.size();
            int y = height/2 * (1 - m_samples[i]);
            painter.drawLine(x, height/2, x, y);
        }
    }

private:
    QVector<qreal> m_samples;
};

#endif // WAVEFORM_H
