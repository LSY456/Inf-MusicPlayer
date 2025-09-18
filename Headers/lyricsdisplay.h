// lyricsdisplay.h
#ifndef LYRICSDISPLAY_H
#define LYRICSDISPLAY_H

#include <QLabel>
#include <QScrollArea>
#include <QTimer>
#include <QVector>
#include <QRegularExpression>

//struct LyricLine {
//    qint64 time; // 毫秒
//    QString text;
//};

class LyricsDisplay : public QScrollArea
{
    Q_OBJECT
public:
    explicit LyricsDisplay(QWidget *parent = nullptr);

    void loadLyrics(const QString &lyrics);
    void setCurrentPosition(qint64 position);
    void clear();

    void setHighlightColor(const QColor &color);
    void setNormalColor(const QColor &color);
    void setFontSize(int size);

protected:
    void resizeEvent(QResizeEvent *event) override;

private:

    struct LyricLine {
            qint64 time;
            QString text;
        };

    void updateLyricsDisplay();
    void scrollToCurrentLine();
    QString formatLyricLine(const QString &line, bool isCurrent) const;

    QLabel *lyricsLabel;
    QVector<LyricLine> lyricLines;
    int currentLine = -1;
    QColor highlightColor = QColor("#FF69B4");
    QColor normalColor = QColor("#FFFFFF");
    int fontSize = 16;

};

#endif // LYRICSDISPLAY_H
