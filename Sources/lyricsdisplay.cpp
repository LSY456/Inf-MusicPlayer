// lyricsdisplay.cpp
#include "lyricsdisplay.h"
#include <QTextDocument>
#include <QScrollBar>
#include <QDebug>
#include <QVBoxLayout>
#include <QPropertyAnimation>
#include <QGraphicsDropShadowEffect>

LyricsDisplay::LyricsDisplay(QWidget *parent)
    : QScrollArea(parent)
{
//    lyricsLabel = new QLabel(this);
//    lyricsLabel->setAlignment(Qt::AlignCenter);
//    lyricsLabel->setWordWrap(true);
//    lyricsLabel->setTextFormat(Qt::RichText);
//    lyricsLabel->setStyleSheet("background: transparent;");

//    setWidget(lyricsLabel);
//    setWidgetResizable(true);
//    setFrameShape(QFrame::NoFrame);
//    setStyleSheet("background: transparent; border: none;");

    // 创建一个容器widget用于居中
    QWidget *container = new QWidget(this);
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);

    // 创建歌词标签
    lyricsLabel = new QLabel(container);
    lyricsLabel->setAlignment(Qt::AlignCenter);
    lyricsLabel->setWordWrap(true);
    lyricsLabel->setTextFormat(Qt::RichText);
    lyricsLabel->setStyleSheet("background: transparent;");

    // 添加垂直弹簧使内容垂直居中
    containerLayout->addStretch();
    containerLayout->addWidget(lyricsLabel);
    containerLayout->addStretch();

    setWidget(container);
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("background: transparent; border: none;");


    // 添加背景效果
   QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect;
   shadowEffect->setBlurRadius(20);
   shadowEffect->setColor(QColor(0, 0, 0, 100));
   shadowEffect->setOffset(0, 3);
   container->setGraphicsEffect(shadowEffect);

   // 设置歌词标签样式
//   lyricsLabel->setStyleSheet(R"(
//       QLabel {
//           background-color: rgba(30, 30, 30, 180);
//           border-radius: 12px;
//           padding: 20px;
//           margin: 15px;
//       }
//   )");
   // 启用抗锯齿
   lyricsLabel->setAttribute(Qt::WA_TranslucentBackground);
}

void LyricsDisplay::loadLyrics(const QString &lyrics)
{
//    lyricLines.clear();
//    currentLine = -1;

    QStringList lines = lyrics.split('\n', Qt::SkipEmptyParts);
    QRegularExpression timeRegex("\\[(\\d+):(\\d+)\\.(\\d+)\\]");

    clear();
    QRegularExpression regex(R"((\[\d{2}:\d{2}\.\d{2}\]))");

    // 增加错误歌词行过滤
    QStringList validLines;
    for (const auto &line : lyrics.split('\n')) {
        if (line.contains(regex) || line.trimmed().isEmpty()) {
            validLines << line;
        } else {
            qWarning() << "Invalid lyric line:" << line;
        }
    }

    for (const QString &line : lines) {
        QRegularExpressionMatchIterator it = timeRegex.globalMatch(line);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            int minutes = match.captured(1).toInt();
            int seconds = match.captured(2).toInt();
            int milliseconds = match.captured(3).toInt();

            qint64 time = (minutes * 60 + seconds) * 1000 + milliseconds * 10;

            QString text = line.mid(match.capturedEnd());
            if (!text.isEmpty()) {
                lyricLines.append({time, text.trimmed()});
            }
        }
    }

    // 自动检测LRC格式
//   if (lyrics.contains(QRegularExpression("\\[\\d+:\\d+\\.\\d+\\]"))) {
//       // 处理标准LRC格式
//       QStringList lines = lyrics.split('\n', Qt::SkipEmptyParts);
//       QRegularExpression timeRegex("\\[(\\d+):(\\d+)\\.(\\d+)\\]");
//       // ...原有LRC解析逻辑
//   } else {
//       // 处理纯文本歌词
//       QStringList lines = lyrics.split('\n', Qt::SkipEmptyParts);
//       for (const QString &line : lines) {
//           lyricLines.append({0, line.trimmed()});
//       }
//   }

    updateLyricsDisplay();
}


// 添加精确同步逻辑
void LyricsDisplay::setCurrentPosition(qint64 position)
{
//    int newLine = -1;
//    for (int i = 0; i < lyricLines.size(); ++i) {
//        if (lyricLines[i].time <= position) {
//            newLine = i;
//        } else {
//            break;
//        }
//    }

//    if (newLine != currentLine) {
//        currentLine = newLine;
//        updateLyricsDisplay();
//        scrollToCurrentLine();
//    }

    // 二分查找当前歌词行
   long long low = 0;
   long long high = lyricLines.size() - 1;
   long long newLine = -1;

   while (low <= high) {
       long long mid = (low + high) / 2;
       if (lyricLines[mid].time <= position) {
           newLine = mid;
           low = mid + 1;
       } else {
           high = mid - 1;
       }
   }

   // 处理空白行
   while (newLine >= 0 && lyricLines[newLine].text.isEmpty()) {
       newLine--;
   }

   if (newLine != currentLine) {
       currentLine = newLine;
       updateLyricsDisplay();
       scrollToCurrentLine();
   }
}

void LyricsDisplay::clear()
{
    lyricLines.clear();
    currentLine = -1;
    lyricsLabel->setText("");
}

void LyricsDisplay::setHighlightColor(const QColor &color)
{
    highlightColor = color;
    updateLyricsDisplay();
}

void LyricsDisplay::setNormalColor(const QColor &color)
{
    normalColor = color;
    updateLyricsDisplay();
}

void LyricsDisplay::setFontSize(int size)
{
    fontSize = size;
    updateLyricsDisplay();
}

void LyricsDisplay::resizeEvent(QResizeEvent *event)
{
    QScrollArea::resizeEvent(event);
    updateLyricsDisplay();
}

void LyricsDisplay::updateLyricsDisplay()
{


//    QString html;
//        html += "<html><body style='text-align:center;'>";

//        for (int i = 0; i < lyricLines.size(); ++i) {
//            bool isCurrent = (i == currentLine);
//            html += formatLyricLine(lyricLines[i].text, isCurrent);
//            html += "<br>";
//        }

//        html += "</body></html>";
//        lyricsLabel->setText(html);

//        // 调整容器大小以确保居中
//        QWidget *container = widget();
//        container->adjustSize();

//        // 计算所需的最小高度
//        int minHeight = lyricLines.size() * (lyricsLabel->fontMetrics().height() + 5);
//        container->setMinimumHeight(minHeight);
    QString html;
    html += "<html><body style='text-align:center;'>";

    for (int i = 0; i < lyricLines.size(); ++i) {
        bool isCurrent = (i == currentLine);
        QString lineText = lyricLines[i].text;
        html += formatLyricLine(lineText, isCurrent);
        // 仅非空行添加换行
        if (!lineText.isEmpty()) {
            html += "<br>";
        }
    }

    html += "</body></html>";
    lyricsLabel->setText(html);

    // 调整容器大小以确保居中
    QWidget *container = widget();
    container->adjustSize();

    // 计算所需的最小高度
    int minHeight = lyricLines.size() * (lyricsLabel->fontMetrics().height() + 5);
    container->setMinimumHeight(minHeight);


}

void LyricsDisplay::scrollToCurrentLine()
{
//    if (currentLine >= 0 && currentLine < lyricLines.size()) {
//        int lineHeight = lyricsLabel->fontMetrics().height() + 5;
//        int targetY = currentLine * lineHeight - height() / 2 + lineHeight;

//        // 确保滚动位置不会超出范围
//        targetY = qMax(0, qMin(targetY, widget()->height() - height()));
//        verticalScrollBar()->setValue(targetY);
//    }


    if (currentLine >= 0 && currentLine < lyricLines.size()) {
        QPropertyAnimation *animation = new QPropertyAnimation(verticalScrollBar(), "value");
        animation->setDuration(800);
        animation->setEasingCurve(QEasingCurve::OutQuint);

        int lineHeight = lyricsLabel->fontMetrics().height() + 15;
        int targetY = currentLine * lineHeight - height()/2 + lineHeight/2;
        targetY = qBound(0, targetY, widget()->height() - height());

        animation->setStartValue(verticalScrollBar()->value());
        animation->setEndValue(targetY);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }

}

QString LyricsDisplay::formatLyricLine(const QString &line, bool isCurrent) const
{

    if (line.isEmpty()) {
        // 空行使用更大的间距
        return "<div style='margin-bottom: 1.5em;'></div>";
    }


    QString color = isCurrent ? highlightColor.name() : normalColor.name();
    int size = isCurrent ? fontSize + 4 : fontSize;

    QLinearGradient gradient(0, 0, 0, 1);
    gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    gradient.setColorAt(0, isCurrent ? highlightColor.lighter(120) : normalColor);
    gradient.setColorAt(1, isCurrent ? highlightColor.darker(120) : normalColor.darker());

    QString shadow = isCurrent ?
        "text-shadow: 0 0 8px rgba(%1, 150), 0 0 3px rgba(%1, 180);"
        : "text-shadow: 0 0 5px rgba(%1, 80);";

    return QString("<span style='color:%1; font-size:%2pt;'>%3</span>")
           .arg(color)
           .arg(size)
           .arg(line.toHtmlEscaped());
}



