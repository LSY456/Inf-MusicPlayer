#ifndef CLICKABLESLIDER_H
#define CLICKABLESLIDER_H

#include <QSlider>
#include <QMouseEvent>
#include <QStyle>

class ClickableSlider : public QSlider
{
    Q_OBJECT
public:
    explicit ClickableSlider(QWidget *parent = nullptr) : QSlider(parent) {
        setOrientation(Qt::Horizontal);  // 设置默认方向为水平
    }

protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() == Qt::LeftButton) {
            double pos = event->pos().x() / (double)width();
            setValue(pos * (maximum() - minimum()) + minimum());
            emit sliderMoved(value());
            event->accept();
        }
        QSlider::mousePressEvent(event);
    }
};

#endif // CLICKABLESLIDER_H
