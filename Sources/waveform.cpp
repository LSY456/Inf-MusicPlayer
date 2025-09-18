// waveform.cpp
#include "waveform.h"
#include <QPainter>

WaveformWidget::WaveformWidget(QWidget *parent) : QWidget(parent) {}

void WaveformWidget::updateWaveform(const QVector<qreal> &samples) {
    m_samples = samples;
    update(); // 触发重绘
}

void WaveformWidget::paintEvent(QPaintEvent *) {
    QPainter painter(this);
    painter.fillRect(rect(), Qt::black);

    if (m_samples.isEmpty()) return;

    painter.setPen(QPen(Qt::cyan, 2));
    qreal xStep = width() / static_cast<qreal>(m_samples.size());

    for (int i = 0; i < m_samples.size(); ++i) {
        qreal y = height() / 2 * (1 - m_samples[i]);
        painter.drawLine(i * xStep, height()/2, i * xStep, y);
    }
}
