#include "player.h"
#include "./ui_player.h"
#include <QStyle>
#include <QVideoWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QListWidgetItem>
#include <QTextStream>
#include <QFile>
#include <QDockWidget>
#include <QShortcut>
#include <QDebug>
#include <QStyle>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QRandomGenerator>
#include <QIcon>
#include <QTimer>   
#include <QScreen>
#include <QGuiApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QImage>
#include <QMenu>
#include <QMediaMetaData>
#include <QVideoFrame>
#include <QVideoSink>
#include <QDataStream>
#include <QPainter>
#include <QPixmap>
#include <QInputDialog>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QThread>
#include <QElapsedTimer>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "fuzzymatcher.h"
#include <QProcess>
#include <QPropertyAnimation>
#include "audiocapture.h"


Player::Player(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Player)
    , coverLabel(new QLabel)
    , playMode(Sequential)
{
    ui->setupUi(this);


    // 设置应用图标
    setWindowIcon(QIcon(":/icons/player.svg"));

    // 初始化视频窗口
    videoWidget = new QVideoWidget(this);


    // 确保视频控件在布局最上层
    videoWidget->raise();
    videoWidget->setAttribute(Qt::WA_AlwaysStackOnTop);

    //搜索框
    setupSearchBox();

    // 设置封面标签的对象名
    coverLabel->setObjectName("coverLabel");

    // 初始化歌词显示
    lyricsDisplay = new LyricsDisplay(this);

    lyricsDisplay->setFontSize(16);
    lyricsDisplay->setHighlightColor(QColor("#0062ff"));


    // 修改视频布局
    ui->videoLayout->addWidget(lyricsDisplay, 1);
    lyricsDisplay->hide(); // 初始隐藏

    // 初始化音乐封面
//    QLabel *coverLabel = new QLabel("你好你好");
    coverLabel->setAlignment(Qt::AlignCenter);
//    coverLabel->setFixedSize(300, 300);
    ui->videoLayout->addWidget(coverLabel, 0);

    ui->videoLayout->addWidget(videoWidget, 1);


    // 初始化默认封面
    defaultCover = QPixmap("/home/Inf/Desktop/DtkProject/Inf_player/bg-onelastkiss.jpg");
    defaultCover.setDevicePixelRatio(devicePixelRatio());


    // 构造函数中初始化时显示封面
    setCoverVisible(true);
    isCoverVisible = true;  // 确保状态一致

    // 初始化音视频同步系统
    m_avSyncTimer.start();

    // 初始化媒体播放器和音频输出
    mediaPlayer = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    mediaPlayer->setAudioOutput(audioOutput);
    mediaPlayer->setVideoOutput(videoWidget);

    // 设置初始音量
    audioOutput->setVolume(ui->volumeSlider->value() / 100.0);

    // 设置按钮图标
    ui->playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
    ui->stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
    ui->forwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekForward));
    ui->backwardButton->setIcon(style()->standardIcon(QStyle::SP_MediaSeekBackward));
    ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    ui->previousButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
    ui->nextButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
    ui->playModeButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));

    updatePlayModeIcon();  // 更新播放模式图标
 

    // 连接音量按钮点击信号
    connect(ui->volumeButton, &QPushButton::clicked, this, &Player::toggleMute);

    // 修改静音快捷键连接
    QShortcut *muteShortcut = new QShortcut(Qt::Key_M, this);
    connect(muteShortcut, &QShortcut::activated, this, &Player::toggleMute);

    // 进度条相关连接
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, &Player::updatePosition);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &Player::updateDuration);
    connect(ui->progressSlider, &QSlider::sliderMoved, this, &Player::setPosition);

    // 音量控制相关连接
    connect(ui->volumeSlider, &QSlider::valueChanged, this, &Player::setVolume);

    // 播放状态改变时更新图标
    connect(mediaPlayer, &QMediaPlayer::playbackStateChanged, this, &Player::updatePlayIcon);

    // 初始化菜单指针
    playbackRateMenu = nullptr;
    rateGroup = nullptr;

    // 创建播放列表
    playlistDock = new QDockWidget(this);
    playlistDock->setTitleBarWidget(new QWidget());
    playlistWidget = new QListWidget(playlistDock);
    playlistDock->setWidget(playlistWidget);
    addDockWidget(Qt::RightDockWidgetArea, playlistDock);

    // 根据屏幕分辨率设置播放列表最小宽度
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        int screenWidth = screen->availableGeometry().width();
        // 设置为屏幕宽度的 15%，但不小于 200 像素，不大于 400 像素
        int minWidth = qBound(200, screenWidth / 6, 400);
        playlistDock->setMinimumWidth(minWidth);
    }

    // 设置播放列表样式
    playlistWidget->setIconSize(QSize(48, 48));
    playlistWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #2b2b2b;
            border: none;
        }
        QListWidget::item {
            color: #ffffff;
            padding: 8px;
            border-bottom: 1px solid #3a3a3a;
            height : 60px;
        }
        QListWidget::item:selected {
            background-color: #3a3a3a;
        }
        QListWidget::item:hover {
            background-color: #323232;
        }
    )");

    // 创建播放列表切换按钮
    togglePlaylistButton = new QPushButton(this);
    togglePlaylistButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    togglePlaylistButton->setToolTip("显示/隐藏播放列表");
    togglePlaylistButton->setFixedSize(32, 32);
    togglePlaylistButton->setFocusPolicy(Qt::NoFocus);
    togglePlaylistButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            padding: 1px;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 30);
        }
    )");

    // 将按钮添加到控制栏
    ui->horizontalLayout->addWidget(togglePlaylistButton);

    connect(togglePlaylistButton, &QPushButton::clicked, this, &Player::togglePlaylist);

    // 连接播放列表信号
    connect(playlistWidget, &QListWidget::itemDoubleClicked,
            this, &Player::playlistItemDoubleClicked);

    // 设置默认播放列表路径
    QDir appDataDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!appDataDir.exists()) {
        appDataDir.mkpath(".");
    }
    defaultPlaylistFile = appDataDir.filePath("default.m3u");

    // 加载默认播放列表
    loadDefaultPlaylist();

    createMenus();
    createPlaybackRateMenu();

    // 添加错误处理连接

    connect(mediaPlayer, &QMediaPlayer::errorOccurred, this, [this](QMediaPlayer::Error error, const QString &errorString) {
        if (error != QMediaPlayer::NoError) {
            QMessageBox::warning(this, "播放错误", "播放出错：" + errorString);
        }
    });

    // 添加媒体状态变化处理
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        switch (status) {
            case QMediaPlayer::LoadedMedia:
                // 媒体加载完成，可以开始播放
                ui->playButton->setEnabled(true);
                break;
            case QMediaPlayer::InvalidMedia:
                // 无效媒体
                QMessageBox::warning(this, "错误", "无效的媒体文件");
                break;
            default:
                break;
        }
    });

    // 添加删除快捷键
    QShortcut *deleteShortcut = new QShortcut(QKeySequence::Delete, playlistWidget);
    connect(deleteShortcut, &QShortcut::activated, this, &Player::removeFromPlaylist);

    // 设置播放列表右键菜单
    playlistWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(playlistWidget, &QWidget::customContextMenuRequested,
            this, &Player::showPlaylistContextMenu);

    isFullScreen = false;

    // 为视频窗口添加双击事件
    videoWidget->setMouseTracking(true);
    videoWidget->installEventFilter(this);

    // 设置鼠标跟踪
    videoWidget->setMouseTracking(true);

    // 添加快捷键
    QShortcut *spaceShortcut = new QShortcut(Qt::Key_Space, this);
    connect(spaceShortcut, &QShortcut::activated, this, &Player::on_playButton_clicked);

    QShortcut *leftShortcut = new QShortcut(Qt::Key_Left, this);
    connect(leftShortcut, &QShortcut::activated, this, &Player::on_backwardButton_clicked);

    QShortcut *rightShortcut = new QShortcut(Qt::Key_Right, this);
    connect(rightShortcut, &QShortcut::activated, this, &Player::on_forwardButton_clicked);

    QShortcut *escShortcut = new QShortcut(Qt::Key_Escape, this);
    connect(escShortcut, &QShortcut::activated, this, [this]() {
        if (isFullScreen) {
            toggleFullScreen();
        }
    });

    // 启用拖拽排序
    playlistWidget->setDragDropMode(QAbstractItemView::InternalMove);
    playlistWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // 添加播放列表项变化监听
    connect(playlistWidget->model(), &QAbstractItemModel::rowsMoved,
            this, &Player::saveDefaultPlaylist);

    // 初始化播放模式
    playMode = Sequential;

    // 加载设置
    loadSettings();

//    // 添加播放模式菜单
//    QMenu *playMenu = ui->menubar->addMenu("播放(&P)");


    mediaPlayer->setPlaybackRate(1.0); // 确保播放速率正常


    // 媒体播放结束时的处理
    connect(mediaPlayer, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) {
            switch (playMode) {
                case SingleLoop:
                    mediaPlayer->setPosition(0);
                    mediaPlayer->play();
                    break;
                case Loop:
                    if (playlistWidget->count() > 0) {
                        int nextRow = (playlistWidget->currentRow() + 1) % playlistWidget->count();
                        playlistWidget->setCurrentRow(nextRow);
                        playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
                    }
                    break;
                case Sequential:
                    if (playlistWidget->currentRow() + 1 < playlistWidget->count()) {
                        playNext();
                    } else {
                        mediaPlayer->stop();
                    }

//                    playNext();
                    break;

                case Random:
                    if (playlistWidget->count() > 0) {
                        int randomRow = QRandomGenerator::global()->bounded(playlistWidget->count());
                        playlistWidget->setCurrentRow(randomRow);
                        playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
                    }
                    break;
                case PauseAfter:
                        mediaPlayer->pause();
                        mediaPlayer->setPosition(mediaPlayer->duration()); // 定位到结尾
                        break;
            }
        }
    });

    // 连接信号
    connect(ui->previousButton, &QPushButton::clicked, this, &Player::playPrevious);
    connect(ui->nextButton, &QPushButton::clicked, this, &Player::playNext);
    connect(ui->playModeButton, &QPushButton::clicked, this, &Player::togglePlayMode);

    // 设置播放列表接受拖放
    playlistWidget->setAcceptDrops(true);
    playlistWidget->viewport()->installEventFilter(this);

    // 添加更多快捷键
    QShortcut *screenshotShortcut = new QShortcut(Qt::Key_S, this);
    connect(screenshotShortcut, &QShortcut::activated, this, &Player::captureScreenshot);

    QShortcut *volumeUpShortcut = new QShortcut(Qt::Key_Up, this);
    connect(volumeUpShortcut, &QShortcut::activated, this, [this]() {
        int newVolume = qMin(ui->volumeSlider->value() + 5, 100);
        ui->volumeSlider->setValue(newVolume);
    });

    QShortcut *volumeDownShortcut = new QShortcut(Qt::Key_Down, this);
    connect(volumeDownShortcut, &QShortcut::activated, this, [this]() {
        int newVolume = qMax(ui->volumeSlider->value() - 5, 0);
        ui->volumeSlider->setValue(newVolume);
    });

    // 设置历史记录文件路径
    QDir appDataDir2(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    if (!appDataDir2.exists()) {
        appDataDir2.mkpath(".");
    }
    historyFile = appDataDir2.filePath("history.dat");

    // 加载历史记录
    loadPlayHistory();

    // 初始化预览播放器
    previewPlayer = new QMediaPlayer(this);
    previewVideoWidget = new QVideoWidget;
    previewPlayer->setVideoOutput(previewVideoWidget);

    // 创建预览容器
    thumbnailPreview = new QLabel(nullptr);  // 改为顶层窗口
    thumbnailPreview->setFixedSize(160, 90);  // 16:9 缩略图
    thumbnailPreview->setStyleSheet(
        "QLabel {"
        "   background-color: black;"
        "   border: 1px solid #666;"
        "   padding: 1px;"
        "}"
    );
    thumbnailPreview->setWindowFlags(Qt::ToolTip | Qt::FramelessWindowHint);  // 设置为工具提示窗口

    // 创建时间标签
    previewTimeLabel = new QLabel(thumbnailPreview);
    previewTimeLabel->setStyleSheet(
        "QLabel {"
        "   color: white;"
        "   background-color: rgba(0, 0, 0, 128);"
        "   padding: 2px;"
        "}"
    );
    previewTimeLabel->setAlignment(Qt::AlignCenter);

    thumbnailPreview->hide();

    // 为进度条添加鼠标追踪
    ui->progressSlider->setMouseTracking(true);
    ui->progressSlider->installEventFilter(this);

    // 初始化预览关闭定时器
    previewHideTimer = new QTimer(this);
    previewHideTimer->setSingleShot(true);  // 设置为单次触发
    previewHideTimer->setInterval(2000);    // 2秒后触发
    connect(previewHideTimer, &QTimer::timeout, thumbnailPreview, &QLabel::hide);

    // 设置流媒体历史记录文件路径
    streamHistoryFile = appDataDir.filePath("streams.dat");

    // 加载流媒体历史记录
    loadStreamHistory();

    // 加载主题设置
    QSettings settings;
    QString savedTheme = settings.value("theme", "dark").toString();
    if (savedTheme == "custom") {
        QString customThemePath = settings.value("customThemePath").toString();
        if (!customThemePath.isEmpty()) {
            loadStyleSheet(customThemePath);
        } else {
            applyTheme("dark");  // 默认使用深色主题
        }
    } else {
        applyTheme(savedTheme);
    }



    // 初始化背景标签
    bgLabel = new QLabel(this);
    bgLabel->setAlignment(Qt::AlignCenter);
    bgLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    bgLabel->lower(); // 置于最底层


    // 清空缓存
    thumbnailCache.clear();

}



Player::~Player()
{
    delete ui;
}

void Player::on_playButton_clicked()
{
    qDebug() << "播放按钮被点击，当前状态:" << mediaPlayer->playbackState();

    if (!mediaPlayer->source().isEmpty() || playlistWidget->currentItem()) {
        switch (mediaPlayer->playbackState()) {
            case QMediaPlayer::PlayingState:
                qDebug() << "暂停播放";
                mediaPlayer->pause();
                break;
            case QMediaPlayer::PausedState:
                qDebug() << "继续播放";
                mediaPlayer->play();
                break;
            case QMediaPlayer::StoppedState:
                if (mediaPlayer->source().isEmpty() && playlistWidget->currentItem()) {
                    qDebug() << "从播放列表播放新文件";
                    playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
                } else {
                    qDebug() << "从头开始播放";
                    mediaPlayer->setPosition(0);
                    mediaPlayer->play();
                }
                break;
        }
    } else {
        qDebug() << "打开新文件";
        openFile();
    }
}

void Player::on_stopButton_clicked()
{
    mediaPlayer->stop();
    ui->progressSlider->setValue(0);
    ui->currentTimeLabel->setText("00:00");
}

void Player::on_forwardButton_clicked()
{
    // 快进10秒
    qint64 position = mediaPlayer->position() + 10000;
    mediaPlayer->setPosition(position);
}

void Player::on_backwardButton_clicked()
{
    // 快退10秒
    qint64 position = mediaPlayer->position() - 10000;
    if (position < 0) position = 0;
    mediaPlayer->setPosition(position);
}

void Player::updatePosition(qint64 position)
{
//    ui->progressSlider->setValue(position);
//    ui->currentTimeLabel->setText(formatTime(position));

//    // 保存当前播放位置
//    if (!mediaPlayer->source().isEmpty()) {
//        lastPositions[mediaPlayer->source().toString()] = position;
//    }
    ui->progressSlider->setValue(position);
    ui->currentTimeLabel->setText(formatTime(position));

    // 同步歌词
    if (lyricsDisplay->isVisible()) {
        lyricsDisplay->setCurrentPosition(position);
    }

    // 保存当前位置
    if (!mediaPlayer->source().isEmpty()) {
        lastPositions[mediaPlayer->source().toString()] = position;
    }
}

void Player::updateDuration(qint64 duration)
{
    ui->progressSlider->setRange(0, duration);
    ui->totalTimeLabel->setText(formatTime(duration));
}

void Player::setPosition(int position)
{
    if (mediaPlayer->isSeekable()) {
        mediaPlayer->setPosition(position);
    }
}

void Player::setVolume(int volume)
{
    audioOutput->setVolume(volume / 100.0);
    
    // 更新音量图标
    if (volume == 0) {
        ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    } else if (volume < 33) {
        ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
        ui->volumeButton->setToolTip(QString("音量: %1%").arg(volume));
    } else if (volume < 66) {
        ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
        ui->volumeButton->setToolTip(QString("音量: %1%").arg(volume));
    } else {
        ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
        ui->volumeButton->setToolTip(QString("音量: %1%").arg(volume));
    }

    // 如果音量不为0，更新lastVolume
    if (volume > 0) {
        lastVolume = volume;
        audioOutput->setMuted(false);
    }
}

void Player::updatePlayIcon(QMediaPlayer::PlaybackState state)
{
    QPushButton *btn = ui->playButton;
    switch (state) {
        case QMediaPlayer::PlayingState:
            btn->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
            btn->setToolTip("暂停");
            break;
        case QMediaPlayer::PausedState:
            btn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            btn->setToolTip("继续播放");
            break;
        case QMediaPlayer::StoppedState:
            btn->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
            btn->setToolTip("播放");
            break;
    }
}

QString Player::formatTime(qint64 milliseconds)
{
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void Player::createMenus()
{
    QMenu *fileMenu = ui->menubar->addMenu("文件(&F)");

    QAction *openAct = new QAction("打开(&O)", this);
    openAct->setShortcut(QKeySequence::Open);
    openAct->setStatusTip("打开媒体文件");
    connect(openAct, &QAction::triggered, this, &Player::openFile);

    QAction *addToPlaylistAct = new QAction("添加到播放列表(&A)", this);
    connect(addToPlaylistAct, &QAction::triggered, this, &Player::addToPlaylist);

    QAction *removeFromPlaylistAct = new QAction("从播放列表移除(&R)", this);
    connect(removeFromPlaylistAct, &QAction::triggered, this, &Player::removeFromPlaylist);

    QAction *savePlaylistAct = new QAction("保存播放列表(&S)", this);
    connect(savePlaylistAct, &QAction::triggered, this, &Player::savePlaylist);

    QAction *loadPlaylistAct = new QAction("加载播放列表(&L)", this);
    connect(loadPlaylistAct, &QAction::triggered, this, &Player::loadPlaylist);

    fileMenu->addAction(openAct);
    fileMenu->addAction(addToPlaylistAct);
    fileMenu->addAction(removeFromPlaylistAct);
    fileMenu->addSeparator();
    fileMenu->addAction(savePlaylistAct);
    fileMenu->addAction(loadPlaylistAct);

    // 添加播放菜单
    QMenu *playMenu = ui->menubar->addMenu("播放(&P)");
    playbackRateMenu = playMenu->addMenu("播放速度(&R)");
    QMenu *playModeMenu = playMenu->addMenu("播放模式(&M)");

    QActionGroup *modeGroup = new QActionGroup(this);
    QStringList modeNames = {"顺序播放", "列表循环", "单曲循环", "随机播放", "播完暂停"};
    for (int i = 0; i < modeNames.size(); ++i) {
        QAction *action = new QAction(modeNames[i], this);
        action->setCheckable(true);
        action->setData(i);
        modeGroup->addAction(action);
        playModeMenu->addAction(action);
        if (i == playMode) action->setChecked(true);
    }

    connect(modeGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        setPlayMode(static_cast<PlayMode>(action->data().toInt()));
    });


    // 添加视频菜单
    QMenu *audioMenu = ui->menubar->addMenu("音乐(&A)");

    // 添加切换封面可见性的动作
    QAction *toggleCoverAction = new QAction("隐藏封面", this);
    toggleCoverAction->setCheckable(true);
    toggleCoverAction->setChecked(!isCoverVisible); // 同步初始状态
    connect(toggleCoverAction, &QAction::triggered, this, &Player::toggleCoverVisibility);
    audioMenu->addAction(toggleCoverAction);

    // 添加宽高比菜单
//    aspectRatioMenu = audioMenu->addMenu("隐藏封面(&A)");
//    QActionGroup *audioGroup = new QActionGroup(this);

//    struct AspectAudio {
//        QString name;
//        double ratio;
//    };

//    QList<AspectAudio> audios = {
//        {"自动", 0.0},
//        {"4:3", 4.0/3.0},
//        {"16:9", 16.0/9.0},
//        {"1:1", 1.0},
//        {"2.35:1", 2.35}
//    };

//    for (const auto &ratio : audios) {
//        QAction *action = aspectRatioMenu->addAction(ratio.name);
//        action->setCheckable(true);
//        action->setData(ratio.ratio);
//        audioGroup->addAction(action);
//        if (ratio.ratio == 0.0) action->setChecked(true);
//    }

//    connect(audioGroup, &QActionGroup::triggered, this, [this](QAction *action) {
//        setAspectRatio(action->data().toDouble());
//    });



    // 添加视频菜单
    QMenu *videoMenu = ui->menubar->addMenu("视频(&V)");

    // 添加宽高比菜单
    aspectRatioMenu = videoMenu->addMenu("宽高比(&A)");
    QActionGroup *ratioGroup = new QActionGroup(this);

    struct AspectRatio {
        QString name;
        double ratio;
    };

    QList<AspectRatio> ratios = {
        {"自动", 0.0},
        {"4:3", 4.0/3.0},
        {"16:9", 16.0/9.0},
        {"1:1", 1.0},
        {"2.35:1", 2.35}
    };

    for (const auto &ratio : ratios) {
        QAction *action = aspectRatioMenu->addAction(ratio.name);
        action->setCheckable(true);
        action->setData(ratio.ratio);
        ratioGroup->addAction(action);
        if (ratio.ratio == 0.0) action->setChecked(true);
    }

    connect(ratioGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        setAspectRatio(action->data().toDouble());
    });

    // 添加截图功能
    QAction *screenshotAct = videoMenu->addAction("截图(&S)");
    screenshotAct->setShortcut(Qt::Key_S);
    connect(screenshotAct, &QAction::triggered, this, &Player::captureScreenshot);

    // 添加帮助菜单
    QMenu *helpMenu = ui->menubar->addMenu("帮助(&H)");
    // 添加主题菜单到帮助菜单
    themeMenu = helpMenu->addMenu("主题(&T)");
    setupThemeMenu();  // 设置主题菜单内容

    helpMenu->addSeparator();  // 添加分隔线
    QAction *shortcutsAct = new QAction("快捷键(&K)", this);
    connect(shortcutsAct, &QAction::triggered, this, [this]() {
        QString shortcuts =
            "播放/暂停: Space\n"
            "快进: →\n"
            "快退: ←\n"
            "退出全屏: Esc\n"
            "静音: M\n"
            "删除播放列表项: Delete\n"
            "打开文件: Ctrl+O";
        QMessageBox::information(this, "快捷键", shortcuts);
    });

    QAction *aboutAct = new QAction("关于(&A)", this);
    connect(aboutAct, &QAction::triggered, this, [this]() {
        QMessageBox::about(this, "关于播放器",
            "U-Inf Player v1.0\n\n"
            "一个简单的媒体播放器，支持常见的音视频格式。\n"
            "基于 Qt6.5.3 开发。");
    });

    helpMenu->addAction(shortcutsAct);
    helpMenu->addAction(aboutAct);


    fileMenu->addSeparator();
    
    // 修改历史菜单的实现
    QMenu *historyMenu = fileMenu->addMenu("播放历史(&H)");
    connect(historyMenu, &QMenu::aboutToShow, this, [this, historyMenu]() {
        historyMenu->clear();
        
        if (playHistory.isEmpty()) {
            QAction *emptyAct = historyMenu->addAction("暂无播放历史");
            emptyAct->setEnabled(false);
        } else {
            // 添加最近播放的文件
            for (const auto &history : playHistory) {
                QString timeStr = history.playTime.toString("yyyy-MM-dd hh:mm");
                QString text = QString("%1 (%2)").arg(history.fileName, timeStr);

                QAction *action = historyMenu->addAction(text);
                connect(action, &QAction::triggered, this, [this, history]() {
                    playFile(history.filePath);
                    // 恢复上次播放位置
                    QTimer::singleShot(100, this, [this, history]() {
                        mediaPlayer->setPosition(history.lastPosition);
                    });
                });
            }

            historyMenu->addSeparator();
            QAction *clearAct = historyMenu->addAction("清除历史记录");
            connect(clearAct, &QAction::triggered, this, &Player::clearHistory);
        }
    });

    QMenu *streamMenu = fileMenu->addMenu("网络流(&N)");

    QAction *openStreamAct = new QAction("打开网络流(&O)", this);
    connect(openStreamAct, &QAction::triggered, this, &Player::openStreamUrl);

    QAction *addStreamAct = new QAction("添加到播放列表(&A)", this);
    connect(addStreamAct, &QAction::triggered, this, &Player::addStreamToPlaylist);

    // 添加最近播放的流媒体子菜单
    QMenu *recentStreamsMenu = streamMenu->addMenu("最近播放(&R)");
    connect(recentStreamsMenu, &QMenu::aboutToShow, this, [this, recentStreamsMenu]() {
        recentStreamsMenu->clear();
        for (const QString &url : recentStreams) {
            QAction *action = recentStreamsMenu->addAction(url);
            connect(action, &QAction::triggered, this, [this, url]() {
                mediaPlayer->setSource(QUrl(url));
                mediaPlayer->play();
            });
        }
        if (!recentStreams.isEmpty()) {
            recentStreamsMenu->addSeparator();
            QAction *clearAct = recentStreamsMenu->addAction("清除历史记录");
            connect(clearAct, &QAction::triggered, this, [this]() {
                recentStreams.clear();
                saveStreamHistory();
            });
        }
    });

    streamMenu->addAction(openStreamAct);
    streamMenu->addAction(addStreamAct);
    streamMenu->addSeparator();
    streamMenu->addMenu(recentStreamsMenu);


    QMenu* themeMenu = ui->menubar->addMenu(tr("背景"));

    QAction* setBgAction = new QAction(QIcon(":/icons/image.png"), "设置背景", this);
    connect(setBgAction, &QAction::triggered, [this](){
        QString path = QFileDialog::getOpenFileName(this, "选择背景图", "", "Images (*.png *.jpg)");
        if(!path.isEmpty()) applyPureBackground(path);
    });

    QAction* clearBgAction = new QAction("清除背景", this);
    connect(clearBgAction, &QAction::triggered, [this](){
        bgLabel->clear();
        QSettings().remove("pure_bg");
    });

    themeMenu->addAction(setBgAction);
    themeMenu->addAction(clearBgAction);

    // 添加 AI 感知选项
    QAction *aiPerceptionAction = new QAction("AI感知(&I)", this);
    aiPerceptionAction->setShortcut(QKeySequence("Ctrl+Shift+A")); // 设置快捷键
//    connect(aiPerceptionAction, &QAction::triggered, this, &Player::onAiPerceptionTriggered);
    helpMenu->addAction(aiPerceptionAction);

}

void Player::toggleCoverVisibility()
{
    bool visible = !coverLabel->isVisible();
    setCoverVisible(visible);

    // 更新菜单项文字
    QList<QAction*> audioActions = ui->menubar->actions();
    foreach (QAction *action, audioActions) {
        if (action->text() == "音乐(&A)") {
            QMenu *audioMenu = action->menu();
            if (audioMenu && !audioMenu->actions().isEmpty()) {
                audioMenu->actions().first()->setText(visible ? "隐藏封面" : "显示封面");
            }
            break;
        }
    }
}

void Player::setCoverVisible(bool visible)
{
    // 直接操作 coverLabel
    coverLabel->setVisible(visible);

    // 调整布局拉伸比例：封面可见时为1，否则为0
//    coverLabel->setStretch(0, visible ? 1 : 0);
//    ui->videoLayout->setStretch(1, visible ? 0 : 1); // 视频部分反向调整
    // 根据文件类型更新样式
    updatePlaybackAreaStyle();

}

void Player::createPlaybackRateMenu()
{
    if (!playbackRateMenu) return;

    rateGroup = new QActionGroup(this);

    QList<double> rates = {0.5, 0.75, 1.0, 1.25, 1.5, 2.0};
    QList<QString> rateTexts = {"0.5x", "0.75x", "1.0x (正常)", "1.25x", "1.5x", "2.0x"};

    for (int i = 0; i < rates.size(); ++i) {
        QAction *rateAction = new QAction(rateTexts[i], this);
        rateAction->setCheckable(true);
        rateAction->setData(rates[i]);
        if (qFuzzyCompare(rates[i], 1.0)) {
            rateAction->setChecked(true);
        }
        rateGroup->addAction(rateAction);
        playbackRateMenu->addAction(rateAction);
    }

    connect(rateGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        double rate = action->data().toDouble();
        setPlaybackRate(rate);
    });
}

void Player::setPlaybackRate(double rate)
{
    if (mediaPlayer) {
        mediaPlayer->setPlaybackRate(rate);// 确保播放速率正常

    }
}

void Player::openFile()
{
    QStringList fileNames = QFileDialog::getOpenFileNames(this,
        "打开媒体文件",
        "",
        "媒体文件 (*.mp4 *.avi *.mkv *.mp3 *.wav *.flac);;所有文件 (*.*)");

    if (!fileNames.isEmpty()) {
        // 添加到播放列表
        for (const QString& fileName : fileNames) {
            QListWidgetItem* item = new QListWidgetItem(QFileInfo(fileName).fileName());
            item->setData(Qt::UserRole, fileName);
            playlistWidget->addItem(item);
        }

        // 自动保存到默认播放列表
        saveDefaultPlaylist();

        // 播放第一个文件
        playFile(fileNames.first());
        playlistWidget->setCurrentRow(playlistWidget->count() - fileNames.size());
    }
}

void Player::playFile(const QString& filePath)
{



    if (filePath.isEmpty()) return;

        // 初始化播放
        mediaPlayer->setSource(QUrl::fromLocalFile(filePath));
        mediaPlayer->setPlaybackRate(1.0); // 重置播放速率
        m_avSyncTimer.restart();          // 重置同步计时器

        // 文件类型判断
        const bool isVideo = filePath.endsWith(".mp4", Qt::CaseInsensitive)
                          || filePath.endsWith(".avi", Qt::CaseInsensitive)
                          || filePath.endsWith(".mkv", Qt::CaseInsensitive);

        const bool isAudio = !isVideo && (filePath.endsWith(".mp3", Qt::CaseInsensitive)
                          || filePath.endsWith(".wav", Qt::CaseInsensitive)
                          || filePath.endsWith(".flac", Qt::CaseInsensitive));

        // 界面显示控制
        videoWidget->setVisible(isVideo);
        lyricsDisplay->setVisible(isAudio);
        coverLabel->setVisible(false);
        updateCoverMenuState(isAudio); // 更新封面菜单状态



        //元数据处理
        const MusicMetadata metadata = MetadataParser::extractMetadata(filePath);
        updatePlaylistItem(filePath, metadata); // 更新播放列表项

        // 封面显示逻辑
        if (isAudio && !metadata.cover.isNull()) {
            // 加载封面但不显示
            QPixmap cover = QPixmap::fromImage(metadata.cover)
                          .scaled(500, 500, Qt::KeepAspectRatio, Qt::SmoothTransformation);
            coverLabel->setPixmap(cover);
        } else {
            coverLabel->setText(tr("无封面"));
        }

        //  歌词处理
//        if (isAudio) {
//            // 优先使用内嵌歌词
//            if (!metadata.lyrics.isEmpty()) {
//                lyricsDisplay->loadLyrics(metadata.lyrics);
//            } else {
//                // 回退到外部歌词文件
//                const QString lyricsPath = QFileInfo(filePath).path() + "/"
//                                          + QFileInfo(filePath).completeBaseName() + ".lrc";
//                QFile lyricsFile(lyricsPath);
//                if (lyricsFile.open(QIODevice::ReadOnly)) {
//                    lyricsDisplay->loadLyrics(lyricsFile.readAll());
//                } else {
//                    lyricsDisplay->clear();
//                }
//            }
//        }

        if (isAudio) {
            // 优先使用内嵌歌词
            QString lyrics = metadata.lyrics;

            // 如果没有内嵌歌词，查找外部歌词文件
            if (lyrics.isEmpty()) {
                QFileInfo fi(filePath);
                QString basePath = fi.path() + "/" + fi.completeBaseName();

                // 尝试多种可能的歌词文件格式
                const QStringList lyricExtensions = {".lrc", ".txt", ".krc", ".trc"};
                for (const QString &ext : lyricExtensions) {
                    QFile lyricFile(basePath + ext);
                    if (lyricFile.open(QIODevice::ReadOnly)) {
                        lyrics = QString::fromUtf8(lyricFile.readAll());
                        break;
                    }
                }
            }

            // 如果还是空白，显示提示
            if (lyrics.trimmed().isEmpty()) {
                lyrics = "未找到歌词";
            }

            lyricsDisplay->loadLyrics(lyrics);
            lyricsDisplay->show();
        } else {
            lyricsDisplay->hide();
        }

        // 播放控制
        // 恢复播放位置
        if (lastPositions.contains(filePath)) {
            mediaPlayer->setPosition(lastPositions[filePath]);
        }

        mediaPlayer->play();
        setWindowTitle(tr("U-Inf Player - %1").arg(QFileInfo(filePath).fileName()));

        // 高亮播放列表项
        const auto items = playlistWidget->findItems(QFileInfo(filePath).fileName(), Qt::MatchExactly);
        if (!items.isEmpty()) {
            playlistWidget->setCurrentItem(items.first());
        }

        //历史记录
        addToHistory(filePath);
}
//添加文件时触发元数据加载
void Player::addToPlaylist()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
        "添加到播放列表",
        "",
        "媒体文件 (*.mp4 *.avi *.mkv *.mp3 *.wav *.flac);;所有文件 (*.*)");

    for (const QString& file : files) {
//        QListWidgetItem* item = new QListWidgetItem(QFileInfo(file).fileName());
//        item->setData(Qt::UserRole, file);
//        playlistWidget->addItem(item);

        // 创建自定义的播放列表项
        QListWidgetItem* item = new QListWidgetItem();
        item->setData(Qt::UserRole, file);

        // 初始设置默认图标
        item->setIcon(QIcon("/home/Inf/Desktop/DtkProject/Inf_player/musicapp.svg"));

        // 发起异步元数据加载
//        emit requestMetadata(file);

        // 设置默认文本
        item->setText(QFileInfo(file).fileName());

        playlistWidget->addItem(item);

        // 异步加载元数据
        QTimer::singleShot(0, this, [this, file]() {
            MusicMetadata metadata = MetadataParser::extractMetadata(file);
            updatePlaylistItem(file, metadata);
        });
    }

    // 自动保存到默认播放列表
    saveDefaultPlaylist();
}

void Player::playlistItemDoubleClicked(QListWidgetItem* item)
{
    playFile(item->data(Qt::UserRole).toString());
}

void Player::savePlaylist()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        "保存播放列表",
        "",
        "播放列表 (*.m3u)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            for (int i = 0; i < playlistWidget->count(); ++i) {
                out << playlistWidget->item(i)->data(Qt::UserRole).toString() << "\n";
            }
            currentPlaylistFile = fileName;
        }
    }
}

void Player::loadPlaylist()
{
    QString fileName = currentPlaylistFile;
    if (fileName.isEmpty()) {
        fileName = QFileDialog::getOpenFileName(this,
            "加载播放列表",
            "",
            "播放列表 (*.m3u)");
    }

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            playlistWidget->clear();
            QTextStream in(&file);
            while (!in.atEnd()) {
                QString filePath = in.readLine().trimmed();
                if (!filePath.isEmpty() && QFile::exists(filePath)) {
                    QListWidgetItem* item = new QListWidgetItem(QFileInfo(filePath).fileName());
                    item->setData(Qt::UserRole, filePath);
                    playlistWidget->addItem(item);
                }
            }
            currentPlaylistFile = fileName;
        }
    }
}

void Player::removeFromPlaylist()
{
    QList<QListWidgetItem*> items = playlistWidget->selectedItems();
    for (QListWidgetItem* item : items) {
        delete playlistWidget->takeItem(playlistWidget->row(item));
    }

    // 自动保存到默认播放列表
    saveDefaultPlaylist();
}

void Player::togglePlaylist()
{
    playlistDock->setVisible(!playlistDock->isVisible());
    togglePlaylistButton->setIcon(style()->standardIcon(
        playlistDock->isVisible() ? QStyle::SP_FileDialogDetailedView : QStyle::SP_FileDialogListView
    ));
}

void Player::showPlaylistContextMenu(const QPoint &pos)
{
    QList<QListWidgetItem*> selectedItems = playlistWidget->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    QMenu contextMenu(this);

    // 播放选中项
    if (selectedItems.count() == 1) {
        QAction *playAction = new QAction("播放", this);
        connect(playAction, &QAction::triggered, this, [this, selectedItems]() {
            playFile(selectedItems.first()->data(Qt::UserRole).toString());
        });
        contextMenu.addAction(playAction);
        contextMenu.addSeparator();
    }

    // 删除选中项
    QAction *deleteAction = new QAction("删除选中项", this);
    connect(deleteAction, &QAction::triggered, this, [this, selectedItems]() {
        QString message = selectedItems.count() == 1
            ? "确定要删除选中的文件吗？"
            : QString("确定要删除选中的 %1 个文件吗？").arg(selectedItems.count());

        if (QMessageBox::question(this, "确认删除", message) == QMessageBox::Yes) {
            for (QListWidgetItem* item : selectedItems) {
                delete playlistWidget->takeItem(playlistWidget->row(item));
            }
            saveDefaultPlaylist();  // 保存更改
        }
    });
    contextMenu.addAction(deleteAction);

    // 添加排序菜单
    contextMenu.addSeparator();
    QMenu *sortMenu = contextMenu.addMenu("排序");

    QAction *sortByNameAct = new QAction("按名称排序", this);
    connect(sortByNameAct, &QAction::triggered, this, [this]() {
        playlistWidget->sortItems(Qt::AscendingOrder);
        saveDefaultPlaylist();
    });

    QAction *sortByTimeAct = new QAction("按添加时间排序", this);
    connect(sortByTimeAct, &QAction::triggered, this, [this]() {
        // 恢复到添加顺序
        loadDefaultPlaylist();
    });

    sortMenu->addAction(sortByNameAct);
    sortMenu->addAction(sortByTimeAct);

    // 显示文件信息
    if (selectedItems.count() == 1) {
        // 获取文件元数据
//        qDebug() << "元数据标题：" << metadata.title;

        contextMenu.addSeparator();
        QAction *infoAction = new QAction("文件信息", this);
        connect(infoAction, &QAction::triggered, this, [this, selectedItems]() {
            QFileInfo fileInfo(selectedItems.first()->data(Qt::UserRole).toString());
            MusicMetadata metadata = MetadataParser::extractMetadata(selectedItems.first()->data(Qt::UserRole).toString());
            // 将文件大小从字节转换为MB
            double fileSizeMB = fileInfo.size() / (1024.0 * 1024.0);
            QString info = QString("文件名：%1\n标题：%2\n艺术家：%3\n专辑：%4\n大小：%5 MB\n路径：%6\n")
                .arg(fileInfo.fileName())
                .arg(metadata.title)
                .arg(metadata.artist)
                .arg(metadata.album)
//                .arg(fileInfo.size() / 1024)
                .arg(QString::number(fileSizeMB, 'f', 2))  // 单位为MB,保留2位小数
                .arg(fileInfo.absolutePath());
            QMessageBox::information(this, "文件信息", info);
        });
        contextMenu.addAction(infoAction);
    }

    contextMenu.exec(playlistWidget->mapToGlobal(pos));
}

// 添加事件过滤器
bool Player::eventFilter(QObject *obj, QEvent *event)
{

//    if (obj == searchBox) {
//        if (event->type() == QEvent::InputMethod) {
//            QInputMethodEvent *ime = static_cast<QInputMethodEvent*>(event);
//            // 处理输入法事件
//            if (!ime->commitString().isEmpty()) {
//                searchBox->insertPlainText(ime->commitString());
//                performSearch(searchBox->toPlainText());
//            }
//            return true;
//        }
//    }

//    // 处理搜索框的中文输入事件
//    if (obj == searchBox && event->type() == QEvent::InputMethod) {
//        QInputMethodEvent *ime = static_cast<QInputMethodEvent*>(event);
//        if (!ime->commitString().isEmpty()) {
//            // 处理中文输入提交
//            searchBox->setText(searchBox->text() + ime->commitString());
//            performSearch();
//        }
//        return true;
//    }
    if (obj == searchBox && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            performSearch(searchBox->toPlainText().trimmed());
            return true;
        }
    }

    if (obj == videoWidget && event->type() == QEvent::MouseButtonDblClick) {
        toggleFullScreen();
        return true;
    }
    if (obj == playlistWidget->viewport()) {
        if (event->type() == QEvent::DragEnter) {
            QDragEnterEvent *dragEvent = static_cast<QDragEnterEvent*>(event);
            if (dragEvent->mimeData()->hasUrls()) {
                dragEvent->acceptProposedAction();
                return true;
            }
        } else if (event->type() == QEvent::Drop) {
            QDropEvent *dropEvent = static_cast<QDropEvent*>(event);
            QList<QUrl> urls = dropEvent->mimeData()->urls();
            for (const QUrl &url : urls) {
                QString filePath = url.toLocalFile();
                if (QFileInfo(filePath).exists()) {
                    QListWidgetItem* item = new QListWidgetItem(QFileInfo(filePath).fileName());
                    item->setData(Qt::UserRole, filePath);
                    playlistWidget->addItem(item);
                }
            }
            saveDefaultPlaylist();
            return true;
        }
    }
    if (obj == ui->progressSlider) {
        switch (event->type()) {
            case QEvent::MouseMove: {
                // 停止定时器（如果正在运行）
                previewHideTimer->stop();

                QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

                // 计算鼠标位置对应的视频时间
                double ratio = static_cast<double>(mouseEvent->pos().x()) / ui->progressSlider->width();
                qint64 position = ratio * mediaPlayer->duration();

                // 获取进度条的全局位置
               QPoint sliderPos = ui->progressSlider->mapToGlobal(QPoint(0, 0));

               // 计算预览窗口位置
               int previewX = mouseEvent->globalPosition().toPoint().x() - thumbnailPreview->width() / 2;
               int previewY = sliderPos.y() - thumbnailPreview->height() - 5; // 始终在进度条上方5像素

                // 确保预览窗口不会超出屏幕左右边界
                QScreen *screen = QGuiApplication::screenAt(mouseEvent->globalPosition().toPoint());
                if (screen) {
                    QRect screenGeometry = screen->geometry();

                    // 只调整水平方向
                    if (previewX < screenGeometry.left()) {
                        previewX = screenGeometry.left();
                    } else if (previewX + thumbnailPreview->width() > screenGeometry.right()) {
                        previewX = screenGeometry.right() - thumbnailPreview->width();
                    }
                }

                // 更新预览位置
                thumbnailPreview->move(previewX, previewY);

                // 显示预览
                showThumbnailPreview(position);
                break;
            }
            case QEvent::Leave:
                // 启动定时器，2秒后隐藏预览
                previewHideTimer->start();
                break;
            default:
                break;
        }
    }

    return QMainWindow::eventFilter(obj, event);
}

void Player::toggleFullScreen()
{
    if (!isFullScreen) {
        // 进入全屏
        isFullScreen = true;
        setWindowState(windowState() | Qt::WindowFullScreen);
        ui->controlWidget->hide();  // 隐藏控制栏
        playlistDock->hide();
        menuBar()->hide();
    } else {
        // 退出全屏
        isFullScreen = false;
        setWindowState(windowState() & ~Qt::WindowFullScreen);
        ui->controlWidget->show();  // 显示控制栏
        menuBar()->show();
        if (playlistDock->isVisible()) {
            playlistDock->show();
        }
    }
}

// 添加保存默认播放列表函数
void Player::saveDefaultPlaylist()
{
    QFile file(defaultPlaylistFile);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        for (int i = 0; i < playlistWidget->count(); ++i) {
            out << playlistWidget->item(i)->data(Qt::UserRole).toString() << "\n";
        }
    }
}

// 添加加载默认播放列表函数
void Player::loadDefaultPlaylist()
{
//    QFile file(defaultPlaylistFile);
//    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
//        playlistWidget->clear();
//        QTextStream in(&file);
//        while (!in.atEnd()) {
//            QString filePath = in.readLine().trimmed();
//            if (!filePath.isEmpty() && QFile::exists(filePath)) {
//                QListWidgetItem* item = new QListWidgetItem(QFileInfo(filePath).fileName());
//                item->setData(Qt::UserRole, filePath);
//                playlistWidget->addItem(item);
//            }
//        }
//    }
    QFile file(defaultPlaylistFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        playlistWidget->clear();
        QTextStream in(&file);
        while (!in.atEnd()) {
            QString filePath = in.readLine().trimmed();
            if (!filePath.isEmpty() && QFile::exists(filePath)) {
                // 创建列表项并立即加载元数据
                QListWidgetItem* item = new QListWidgetItem();
                item->setData(Qt::UserRole, filePath);

                // 设置初始文本为文件名
                item->setText(QFileInfo(filePath).fileName());

                // 设置默认图标
                item->setIcon(QIcon("/home/Inf/Desktop/DtkProject/Inf_player/musicapp.svg"));

                playlistWidget->addItem(item);

                // 异步加载元数据
                QTimer::singleShot(0, this, [this, filePath]() {
                    MusicMetadata metadata = MetadataParser::extractMetadata(filePath);
                    updatePlaylistItem(filePath, metadata);
                });
            }
        }
    }
}

void Player::saveSettings()
{
    QSettings settings;

    // 保存窗口状态
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // 保存音量
    settings.setValue("volume", ui->volumeSlider->value());

    // 保存播放模式
    settings.setValue("playMode", playMode);

    // 保存播放列表状态
    settings.setValue("playlistVisible", playlistDock->isVisible());

    // 保存最后播放的文件和位置
    if (!mediaPlayer->source().isEmpty()) {
        settings.setValue("lastFile", mediaPlayer->source().toString());
        settings.setValue("lastPosition", mediaPlayer->position());
    }
}

void Player::loadSettings()
{
    QSettings settings;

    // 恢复窗口状态
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // 恢复音量
    ui->volumeSlider->setValue(settings.value("volume", 50).toInt());

    // 恢复播放模式
    playMode = static_cast<PlayMode>(settings.value("playMode", Sequential).toInt());

    // 恢复播放列表显示状态
    playlistDock->setVisible(settings.value("playlistVisible", true).toBool());

    // 恢复最后播放的文件
    QString lastFile = settings.value("lastFile").toString();
    if (!lastFile.isEmpty() && QFile::exists(lastFile)) {
        lastPlayedFile = lastFile;
        QTimer::singleShot(0, this, [this, &settings]() {
            playFile(lastPlayedFile);
            mediaPlayer->setPosition(settings.value("lastPosition", 0).toLongLong());
            mediaPlayer->pause();
        });
    }
}

void Player::closeEvent(QCloseEvent *event)
{
    saveSettings();
    savePlayHistory();
    // 保存主题设置
    QSettings settings;
    settings.setValue("theme", currentTheme);
    QMainWindow::closeEvent(event);
}

void Player::playNext()
{

//    if (playMode == Sequential) {
//        int nextRow = playlistWidget->currentRow() + 1;
//        if (nextRow < playlistWidget->count()) {
//            playlistWidget->setCurrentRow(nextRow);
//            playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
//        } else {
//            mediaPlayer->stop(); // 到达列表末尾停止播放
//        }
//    }


    switch (playMode) {
        case Loop:
        case Sequential: {
            int nextRow = playlistWidget->currentRow() + 1;
            if (nextRow < playlistWidget->count()) {
                playlistWidget->setCurrentRow(nextRow);
                playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
            } else if (playMode == Loop && playlistWidget->count() > 0) {
                // 列表循环模式下回到第一个项目
                playlistWidget->setCurrentRow(0);
                playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
            } else {
                mediaPlayer->stop();
            }
            break;
        }
        case Random:
            if (playlistWidget->count() > 0) {
                int randomRow = QRandomGenerator::global()->bounded(playlistWidget->count());
                playlistWidget->setCurrentRow(randomRow);
                playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
            }
            break;
        default:
            break;
    }

}

void Player::playPrevious()
{
    if (playlistWidget->count() > 0) {
        int prevRow = playlistWidget->currentRow() - 1;
        if (prevRow < 0) {
            prevRow = playMode == Loop ? playlistWidget->count() - 1 : -1;
        }
        if (prevRow >= 0) {
            playlistWidget->setCurrentRow(prevRow);
            playFile(playlistWidget->currentItem()->data(Qt::UserRole).toString());
        }
    }
}

void Player::setPlayMode(PlayMode mode)
{
    playMode = mode;
    updatePlayModeIcon();
}

// 添加切换播放模式的函数
void Player::togglePlayMode()
{
    // 循环切换播放模式
    playMode = static_cast<PlayMode>((static_cast<int>(playMode) + 1) % 5);
    updatePlayModeIcon();
}

// 更新播放模式图标
void Player::updatePlayModeIcon()
{
    QIcon icon;
    QString tooltip;

    switch (playMode) {
        case Sequential:
            icon = QIcon(QStringLiteral("/home/Inf/Desktop/DtkProject/Inf_player/icon_sequential.svg"));
            tooltip = "顺序播放";
            break;
        case Loop:
            icon = QIcon(QStringLiteral("/home/Inf/Desktop/DtkProject/Inf_player/icon_loop.svg"));
            tooltip = "列表循环";
            break;
        case SingleLoop:
            icon = QIcon(QStringLiteral("/home/Inf/Desktop/DtkProject/Inf_player/icon_single.svg"));
            tooltip = "单曲循环";
            break;
        case Random:
            icon = QIcon(QStringLiteral("/home/Inf/Desktop/DtkProject/Inf_player/icon_random.svg"));
            tooltip = "随机播放";
            break;
        case PauseAfter:
            icon = QIcon(QStringLiteral("/home/Inf/Desktop/DtkProject/Inf_player/icon_Audio_play.svg"));
            tooltip = "播完暂停";
            break;
    }

    // 确保图标大小合适
    QSize iconSize(24, 24);
    ui->playModeButton->setIconSize(iconSize);
    ui->playModeButton->setIcon(icon);
    ui->playModeButton->setToolTip(tooltip);
}

void Player::setAspectRatio(double ratio)
{
    videoAspectRatio = ratio;
    if (ratio == 0.0) {
        // 自动模式
        videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
    } else {
        // 固定宽高比模式
        QSize videoSize = mediaPlayer->metaData().value(QMediaMetaData::Resolution).toSize();
        if (!videoSize.isEmpty()) {
            int w = videoWidget->width();
            int h = int(w / ratio);

            // 确保高度不超过视频窗口
            if (h > videoWidget->height()) {
                h = videoWidget->height();
                w = int(h * ratio);
            }

            // 计算居中位置
            int x = (videoWidget->width() - w) / 2;
            int y = (videoWidget->height() - h) / 2;

            // 设置视频显示区域
            videoWidget->setGeometry(x, y, w, h);
        }
    }
}

// 添加窗口大小变化事件处理
void Player::resizeEvent(QResizeEvent *event)
{
//    QMainWindow::resizeEvent(event);
//    if (videoAspectRatio != 0.0) {
//        setAspectRatio(videoAspectRatio);
//    }

    if(!currentBgPath.isEmpty()){
        QPixmap bg(currentBgPath);
        bgLabel->setPixmap(bg.scaled(event->size(), Qt::KeepAspectRatioByExpanding));
    }
    QMainWindow::resizeEvent(event);


}

void Player::captureScreenshot()
{
    if (!mediaPlayer || !mediaPlayer->isPlaying()) return;

    // 获取视频原始分辨率
    QSize videoSize = mediaPlayer->metaData().value(QMediaMetaData::Resolution).toSize();
    if (videoSize.isEmpty()) return;

    // 创建视频帧抓取器
    QVideoSink *videoSink = mediaPlayer->videoSink();
    if (!videoSink) return;

    QVideoFrame frame = videoSink->videoFrame();
    if (!frame.isValid()) return;

    // 将视频帧转换为图像
    QImage image = frame.toImage();
    if (image.isNull()) return;

    QString fileName = QFileDialog::getSaveFileName(this,
        "保存截图",
        QStandardPaths::writableLocation(QStandardPaths::PicturesLocation)
            + "/screenshot.png",
        "图片文件 (*.png *.jpg)");

    if (!fileName.isEmpty()) {
        // 保存原始分辨率的图像
        image.save(fileName);
    }
}

void Player::loadPlayHistory()
{
    QFile file(historyFile);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        // 添加版本检查
        in.setVersion(QDataStream::Qt_6_0);

        // 清除现有历史记录
        playHistory.clear();

        // 读取记录数量
        qint32 count;
        in >> count;

        // 读取每条记录
        for (qint32 i = 0; i < count; ++i) {
            PlayHistory history;
            in >> history;
            playHistory.append(history);
        }
    }
}

void Player::savePlayHistory()
{
    QFile file(historyFile);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        // 设置版本
        out.setVersion(QDataStream::Qt_6_0);

        // 写入记录数量
        out << qint32(playHistory.size());

        // 写入每条记录
        for (const auto &history : playHistory) {
            out << history;
        }
    }
}

void Player::addToHistory(const QString &filePath)
{
    // 检查是否已存在
    auto it = std::find_if(playHistory.begin(), playHistory.end(),
        [&filePath](const PlayHistory &h) { return h.filePath == filePath; });

    PlayHistory history;
    history.filePath = filePath;
    history.fileName = QFileInfo(filePath).fileName();
    history.playTime = QDateTime::currentDateTime();
    history.duration = mediaPlayer->duration();
    history.lastPosition = mediaPlayer->position();

    if (it != playHistory.end()) {
        // 更新现有记录
        *it = history;
    } else {
        // 添加新记录
        playHistory.prepend(history);
        // 限制历史记录数量
        while (playHistory.size() > 50) {
            playHistory.removeLast();
        }
    }

    savePlayHistory();
}

void Player::showHistoryMenu()
{
    QMenu historyMenu(this);

    // 添加最近播放的文件
    for (const auto &history : playHistory) {
        QString timeStr = history.playTime.toString("yyyy-MM-dd hh:mm");
        QString text = QString("%1 (%2)").arg(history.fileName, timeStr);

        QAction *action = historyMenu.addAction(text);
        connect(action, &QAction::triggered, this, [this, history]() {
            playFile(history.filePath);
            // 恢复上次播放位置
            QTimer::singleShot(100, this, [this, history]() {
                mediaPlayer->setPosition(history.lastPosition);
            });
        });
    }

    if (!playHistory.isEmpty()) {
        historyMenu.addSeparator();
        QAction *clearAct = historyMenu.addAction("清除历史记录");
        connect(clearAct, &QAction::triggered, this, &Player::clearHistory);
    }

    historyMenu.exec(QCursor::pos());
}

void Player::clearHistory()
{
    if (QMessageBox::question(this, "确认", "确定要清除所有播放历史记录吗？")
        == QMessageBox::Yes) {
        playHistory.clear();
        savePlayHistory();
    }
}

void Player::showThumbnailPreview(int position)
{
    if (!mediaPlayer || mediaPlayer->source().isEmpty()) {
        thumbnailPreview->hide();
        return;
    }

    // 确保预览播放器设置正确
    if (previewPlayer->source() != mediaPlayer->source()) {
        previewPlayer->setSource(mediaPlayer->source());
        previewPlayer->pause();  // 保持暂停状态
    }

    // 设置预览位置
    previewPlayer->setPosition(position);

    // 等待一小段时间让预览帧准备好
    QTimer::singleShot(50, this, [this, position]() {
        // 获取预览帧
        if (QVideoSink *sink = previewPlayer->videoSink()) {
            QVideoFrame frame = sink->videoFrame();
            if (frame.isValid()) {
                QImage image = frame.toImage();
                if (!image.isNull()) {
                    // 缩放到预览大小
                    image = image.scaled(thumbnailPreview->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

                    // 创建预览图像
                    QImage previewImage(thumbnailPreview->size(), QImage::Format_ARGB32);
                    previewImage.fill(Qt::black);

                    // 居中绘制
                    QPainter painter(&previewImage);
                    int x = (previewImage.width() - image.width()) / 2;
                    int y = (previewImage.height() - image.height()) / 2;
                    painter.drawImage(x, y, image);

                    // 显示时间标签
                    QString timeStr = formatTime(position);
                    previewTimeLabel->setText(timeStr);
                    previewTimeLabel->adjustSize();
                    previewTimeLabel->move(
                        (thumbnailPreview->width() - previewTimeLabel->width()) / 2,
                        thumbnailPreview->height() - previewTimeLabel->height() - 2
                    );

                    thumbnailPreview->setPixmap(QPixmap::fromImage(previewImage));
                    thumbnailPreview->show();
                }
            }
        }
    });
}

void Player::openStreamUrl()
{
    bool ok;
    QString url = QInputDialog::getText(this, "打开网络流",
        "请输入流媒体URL：",
        QLineEdit::Normal,
        "http://", &ok);

    if (ok && !url.isEmpty()) {
        // 验证URL格式
        QUrl streamUrl(url);
        if (!streamUrl.isValid()) {
            QMessageBox::warning(this, "错误", "无效的URL格式");
            return;
        }

        // 添加到最近播放列表
        if (!recentStreams.contains(url)) {
            recentStreams.prepend(url);
            // 限制最近播放列表大小
            while (recentStreams.size() > 10) {
                recentStreams.removeLast();
            }
            saveStreamHistory();
        }

        // 播放流媒体
        mediaPlayer->setSource(QUrl(url));
        mediaPlayer->play();

        // 更新界面
        setWindowTitle("视频播放器 - " + url);
    }
}

void Player::addStreamToPlaylist()
{
    bool ok;
    QString url = QInputDialog::getText(this, "添加网络流",
        "请输入流媒体URL：",
        QLineEdit::Normal,
        "http://", &ok);

    if (ok && !url.isEmpty()) {
        QUrl streamUrl(url);
        if (!streamUrl.isValid()) {
            QMessageBox::warning(this, "错误", "无效的URL格式");
            return;
        }

        // 添加到播放列表
        QListWidgetItem* item = new QListWidgetItem(url);
        item->setData(Qt::UserRole, url);
        playlistWidget->addItem(item);

        // 保存播放列表
        saveDefaultPlaylist();
    }
}

void Player::saveStreamHistory()
{
    QFile file(streamHistoryFile);
    if (file.open(QIODevice::WriteOnly)) {
        QDataStream out(&file);
        out.setVersion(QDataStream::Qt_6_0);
        out << recentStreams;
    }
}

void Player::loadStreamHistory()
{
    QFile file(streamHistoryFile);
    if (file.open(QIODevice::ReadOnly)) {
        QDataStream in(&file);
        in.setVersion(QDataStream::Qt_6_0);
        in >> recentStreams;
    }
}

void Player::setupThemeMenu()
{

    QActionGroup *themeGroup = new QActionGroup(this);

    // 添加预设主题
    struct ThemeInfo {
        QString name;
        QString id;
    };

    QList<ThemeInfo> themes = {
        {"浅色主题", "light"},
        {"深色主题", "dark"},
        {"蓝色主题", "blue"},
        {"自定义主题", "custom"},
        {"跟随系统", "system"}
    };

    for (const auto &theme : themes) {
        QAction *action = themeMenu->addAction(theme.name);
        action->setCheckable(true);
        action->setData(theme.id);
        themeGroup->addAction(action);
        if (theme.id == currentTheme) {
            action->setChecked(true);
        }
    }

    // 添加自定义主题选项
    themeMenu->addSeparator();
    QAction *customThemeAct = themeMenu->addAction("加载自定义主题...");

    // 连接信号
    connect(themeGroup, &QActionGroup::triggered, this, [this](QAction *action) {
        applyTheme(action->data().toString());
    });

    connect(customThemeAct, &QAction::triggered, this, [this]() {
        QString stylePath = QFileDialog::getOpenFileName(this,
            "选择主题文件", "", "样式表文件 (*.qss)");
        if (!stylePath.isEmpty()) {
            loadStyleSheet(stylePath);
        }
    });
}

void Player::applyTheme(const QString &themeName)
{
    if (themeName == "system") {
            // 检测系统主题并应用
            QString systemTheme = detectSystemTheme();
            currentTheme = systemTheme;
            applyTheme(systemTheme);
        } else {
            QString styleSheet;


            if (themeName == "light") {
                styleSheet = R"(
                    QMainWindow {
                        background-color: #dbdbdb;
                        color: #000000;
                    }
                    QMenuBar {
                        background-color: #ffffff;
                        color: #000000;
                    }
                    QSlider::handle:horizontal {
                        background: #2196F3;
                        border-radius: 7px;
                    }
                    QSlider::groove:horizontal {
                        background: #e0e0e0;
                        height: 3px;
                    }
                    QPushButton {
                        background-color: #ffffff;
                        border: 1px solid #dddddd;
                        padding: 5px;
                        border-radius: 3px;
                    }
                    QPushButton:hover {
                        background-color: #f5f5f5;
                    }
                    QTextEdit {
                        background: #ffffff;
                        color: #000000;
                        border: 1px solid #dddddd;
                        border-radius: 12px;
                        padding: 2px 8px;
                    }
                    QTextEdit:focus {
                        border: 1px solid #2196F3;
                    }
                     QVideoWidget, QLabel#coverLabel {
                         background-color: white;
                     }
                     QListWidget {
                         background-color: #ffffff;      /* 白色背景 */
                         border: 1px solid #e0e0e0;     /* 浅灰色边框 */
                         color: #333333;                /* 深灰色文字 */
                     }
                     QListWidget::item {
                         color: #333333;                /* 深灰色文字 */
                         padding: 8px;
                         border-bottom: 1px solid #f0f0f0;  /* 很浅的分隔线 */
                         height: 60px;
                         background-color: #ffffff;     /* 白色项目背景 */
                     }
                     QListWidget::item:selected {
                         background-color: #e3f2fd;     /* 浅蓝色选中背景 */
                         color: #3aaeef;                /* 深蓝色选中文字 */
                         border-bottom: 1px solid #bbdefb;  /* 蓝色分隔线 */
                     }
                     QListWidget::item:hover {
                         background-color: #f5f5f5;      /* 浅灰色悬停背景 */
                     }
                     QPushButton#playModeButton {
                         qproperty-iconSize: 24px;
                     }
                     QPushButton#playModeButton::icon {
                         color: #333333;
                     }

                )";

                // 更新搜索按钮样式
                searchButton->setStyleSheet(R"(
                    QPushButton {
                        border: none;
                        background: transparent;
                    }
                    QPushButton:hover {
                        background: rgba(0, 0, 0, 30);
                    }
                )");
            }
            else if (themeName == "dark") {
                styleSheet = R"(
                    QMainWindow {
                        background-color: #2b2b2b;
                        color: #ffffff;
                    }
                    QMenuBar {
                        background-color: #333333;
                        color: #ffffff;
                    }
                    QMenu {
                        background-color: #333333;
                        color: #ffffff;
                        border: 1px solid #555555;
                    }
                    QMenu::item:selected {
                        background-color: #444444;
                    }
                    QSlider::handle:horizontal {
                        background: #4a9eff;
                        border-radius: 7px;
                    }
                    QSlider::groove:horizontal {
                        background: #555555;
                        height: 3px;
                    }
                    QPushButton {
                        background-color: #3a3a3a;
                        color: #ffffff;
                        border: 1px solid #555555;
                        padding: 5px;
                        border-radius: 3px;
                    }
                    QPushButton:hover {
                        background-color: #444444;
                    }
                    QTextEdit {
                        background: #3a3a3a;
                        color: white;
                        border: 1px solid #555;
                        border-radius: 12px;
                        padding: 2px 8px;
                    }
                    QTextEdit:focus {
                        border: 1px solid #4a9eff;
                    }
                   QListWidget {
                         background-color: #2b2b2b;
                         border: none;
                     }
                   QListWidget::item {
                        color: #ffffff;
                        padding: 8px;
                        border-bottom: 1px solid #3a3a3a;
                         height: 60px;
                     }
                    QListWidget::item:selected {
                         background-color: #3a3a3a;
                    }
                    QListWidget::item:hover {
                       background-color: #323232;
                   }
                     QVideoWidget, QLabel#coverLabel {
                         background-color: black;
                     }
                 QPushButton#playModeButton {
                     qproperty-iconSize: 24px;
                 }
                 QPushButton#playModeButton::icon {
                     color: #ffffff;
                 }
               )";

                // 更新搜索按钮样式
                searchButton->setStyleSheet(R"(
                    QPushButton {
                        border: none;
                        background: transparent;
                    }
                    QPushButton:hover {
                        background: rgba(255, 255, 255, 30);
                    }
                )");
            }
            else if (themeName == "blue") {
                styleSheet = R"(
                    QMainWindow {
                        background-color: #1a237e;
                        color: #ffffff;
                    }
                    QMenuBar {
                        background-color: #283593;
                        color: #ffffff;
                    }
                    QMenu {
                        background-color: #283593;
                        color: #ffffff;
                        border: 1px solid #3949ab;
                    }
                    QMenu::item:selected {
                        background-color: #3949ab;
                    }
                    QSlider::handle:horizontal {
                        background: #42a5f5;
                        border-radius: 7px;
                    }
                    QSlider::groove:horizontal {
                        background: #3949ab;
                        height: 3px;
                    }
                    QPushButton {
                        background-color: #3949ab;
                        color: #ffffff;
                        border: 1px solid #5c6bc0;
                        padding: 5px;
                        border-radius: 3px;
                    }
                    QPushButton:hover {
                        background-color: #5c6bc0;
                    }
                    QTextEdit {
                        background: #1a237e;
                        color: white;
                        border: 1px solid #555;
                        border-radius: 12px;
                        padding: 2px 8px;
                    }
                    QTextEdit:focus {
                        border: 1px solid #3a3a3a;
                    }
                 QListWidget {
                     background-color: #1a237e;  /* 与主窗口相同的深蓝色 */
                     border: 1px solid #3949ab;  /* 边框使用较亮的蓝色 */
                     color: #ffffff;             /* 白色文字 */
                 }
                 QListWidget::item {
                     color: #ffffff;            /* 白色文字 */
                     padding: 8px;
                     border-bottom: 1px solid #3949ab;  /* 项目分隔线使用较亮的蓝色 */
                     height: 60px;
                 }
                 QListWidget::item:selected {
                     background-color: #3949ab;   /* 选中项使用中等蓝色 */
                     color: #ffffff;
                 }
                 QListWidget::item:hover {
                     background-color: #283593;   /* 悬停项使用稍暗的蓝色 */
                 }
                 QVideoWidget, QLabel#coverLabel {
                     background-color: blue;
                 }
                 QPushButton#playModeButton {
                     qproperty-iconSize: 24px;
                 }
                 QPushButton#playModeButton::icon {
                     color: #aaccff;
                 }
                )";

                // 更新搜索按钮样式
                searchButton->setStyleSheet(R"(
                    QPushButton {
                        border: none;
                        background: transparent;
                    }
                    QPushButton:hover {
                        background: rgba(255, 255, 255, 30);
                    }
                )");
            }

    setStyleSheet(styleSheet);
    currentTheme = themeName;

    // 更新播放列表样式
    if (themeName == "dark") {
        setupPlaylistStyle();
    } else {
        playlistWidget->setStyleSheet("");
    }

    // 确保搜索框样式更新
    searchBox->setStyleSheet("");



    // 保存主题设置
    QSettings settings;
    settings.setValue("theme", themeName);
    // 确保在主题切换后更新播放区域
    updatePlaybackAreaStyle();

    }
}

void Player::loadStyleSheet(const QString &stylePath)
{
    QFile file(stylePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString styleSheet = file.readAll();
        setStyleSheet(styleSheet);
        currentTheme = "custom";

        // 保存自定义主题路径
        QSettings settings;
        settings.setValue("customThemePath", stylePath);
    }
}

// 播放列表样式
void Player::setupPlaylistStyle()
{
    playlistWidget->setStyleSheet(R"(
//        QListWidget {
//            background-color: #2b2b2b;
//            border: none;
//        }
//        QListWidget::item {
//            color: #ffffff;
//            padding: 4px;
//            border-bottom: 1px solid #3a3a3a;
//        }
//        QListWidget::item:selected {
//            background-color: #3a3a3a;
//        }
//        QListWidget::item:hover {
//            background-color: #323232;
//        }
//        QListWidget {
//            background-color: #2b2b2b;
//            border: none;
//        }
//        QListWidget::item {
//            color: #ffffff;
//            padding: 4px;
//            border-bottom: 1px solid #3a3a3a;
//        }
//        QListWidget::item:selected {
//            background-color: #3a3a3a;
//        }
//        QListWidget::item:hover {
//            background-color: #323232;
//        }
          QListWidget {
                  background-color: #2b2b2b;
                  border: none;
              }
              QListWidget::item {
                  color: #ffffff;
                  padding: 8px;
                  border-bottom: 1px solid #3a3a3a;
                  height: 60px;
              }
              QListWidget::item:selected {
                  background-color: #3a3a3a;
              }
              QListWidget::item:hover {
                  background-color: #323232;
              }

    )");

    // 启用HTML文本渲染
    playlistWidget->setTextElideMode(Qt::ElideNone);
}

// 添加切换静音的函数
void Player::toggleMute()
{
    if (audioOutput->isMuted()) {
        // 取消静音
        audioOutput->setMuted(false);
        ui->volumeSlider->setValue(lastVolume);
        ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolume));
    } else {
        // 静音
        lastVolume = ui->volumeSlider->value();
        audioOutput->setMuted(true);
        ui->volumeSlider->setValue(0);
        ui->volumeButton->setIcon(style()->standardIcon(QStyle::SP_MediaVolumeMuted));
    }
}


//搜索框
void Player::setupSearchBox()
{
    // 创建搜索容器
    searchWidget = new QWidget(this);
    QHBoxLayout *searchLayout = new QHBoxLayout(searchWidget);
    searchLayout->setContentsMargins(0, 0, 0, 0);
    searchLayout->setSpacing(2);


    // 创建搜索框 - 修改为支持中文输入

    searchBox = new QTextEdit(searchWidget);
    searchBox->setPlaceholderText("搜索播放列表...");
    searchBox->setMaximumHeight(24); // 保持单行高度
    searchBox->setLineWrapMode(QTextEdit::NoWrap);
    searchBox->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    searchBox->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // 设置输入法属性以支持中文输入
    searchBox->setAttribute(Qt::WA_InputMethodEnabled, true);
    searchBox->setInputMethodHints(Qt::ImhNoPredictiveText);   //ImhNoAutoUppercase | Qt::ImhNoPredictiveText

    // 设置字体确保支持中文显示
    QFont font = searchBox->font();
    font.setFamily("Noto Sans CJK S"); // 或者使用其他支持中文的字体
    font.setPixelSize(12);  // 使用像素大小更精确
    searchBox->setFont(font);

    // 调整文档边距
    searchBox->document()->setDocumentMargin(0);

//    searchBox->setStyleSheet(""); // 清空初始样式

//    // 创建搜索框
//    searchBox = new QLineEdit(searchWidget);
//    searchBox->setPlaceholderText("搜索播放列表...");
//    searchBox->setClearButtonEnabled(true);
//    searchBox->setFixedWidth(200);
    searchBox->setStyleSheet(R"(
        QTextEdit {
            background: #3a3a3a;
            color: white;
            border: 1px solid #555;
            border-radius: 12px;
            padding: 2px 8px;
        }
        QTextEdit:focus {
            border: 1px solid #4a9eff;
        }
    )");

    // 创建搜索按钮
    searchButton = new QPushButton(searchWidget);
    searchButton->setIcon(style()->standardIcon(QStyle::SP_FileDialogContentsView));
    searchButton->setFixedSize(24, 24);
    searchButton->setStyleSheet(R"(
        QPushButton {
            border: none;
            background: transparent;
        }
        QPushButton:hover {
            background: rgba(255, 255, 255, 30);
        }
    )");

    // 将控件添加到布局
    searchLayout->addWidget(searchBox);
    searchLayout->addWidget(searchButton);

    // 将搜索框添加到菜单栏右侧
    ui->menubar->setCornerWidget(searchWidget, Qt::TopRightCorner);

    // 连接信号
//    connect(searchBox, &QLineEdit::textChanged, this, &Player::performSearch);
    // 连接textChanged信号
//    connect(searchBox, &QTextEdit::textChanged, this, &Player::performSearch);
//    connect(searchButton, &QPushButton::clicked, this, &Player::performSearch);
//    connect(searchBox, &QLineEdit::returnPressed, this, &Player::performSearch);
    // 连接信号（使用lambda传递参数）
        connect(searchBox, &QTextEdit::textChanged, this, [this]() {
            performSearch(searchBox->toPlainText().trimmed());
        });

        connect(searchButton, &QPushButton::clicked, this, [this]() {
            performSearch(searchBox->toPlainText().trimmed());
        });

    // 添加清除搜索的快捷键
    QShortcut *clearSearchShortcut = new QShortcut(Qt::Key_Escape, searchBox);
    connect(clearSearchShortcut, &QShortcut::activated, this, [this]() {
        searchBox->clear();
        clearSearch();
    });
    // 确保搜索框能响应输入法事件
    searchBox->installEventFilter(this);
}


//搜索逻辑
// 修改后的performSearch函数
void Player::performSearch(const QString& keyword)
{

    // 性能优化：批量更新
    playlistWidget->setUpdatesEnabled(false);

    if (keyword.isEmpty()) {
        clearSearch();
        return;
    }


    // 使用正则表达式进行高亮匹配
        QRegularExpression regex(FuzzyMatcher::createHighlightPattern(keyword),
        QRegularExpression::CaseInsensitiveOption);

    // 过滤逻辑
   for (int i = 0; i < playlistWidget->count(); ++i) {
       QListWidgetItem *item = playlistWidget->item(i);
       QString itemText = item->text();
       QString itemPath = item->data(Qt::UserRole).toString();

       // 使用模糊匹配
       bool match = FuzzyMatcher::fuzzyMatch(keyword, itemText) ||
                    FuzzyMatcher::fuzzyMatch(keyword, itemPath);

       item->setHidden(!match);

   }

   // 选择第一个可见项
   for (int i = 0; i < playlistWidget->count(); ++i) {
       if (!playlistWidget->item(i)->isHidden()) {
           playlistWidget->setCurrentRow(i);
           break;
       }
   }

   playlistWidget->setUpdatesEnabled(true);
   playlistWidget->update();

}

void Player::clearSearch()
{
    // 显示所有项目并移除高亮
   for (int i = 0; i < playlistWidget->count(); ++i) {
       QListWidgetItem *item = playlistWidget->item(i);
       item->setHidden(false);

       // 移除HTML标签，恢复原始文本
       QString plainText = item->text().remove(QRegularExpression("<[^>]*>"));
       item->setText(plainText);
   }
}




// 修改后的inputMethodEvent
void Player::inputMethodEvent(QInputMethodEvent *event)
{
    if (searchBox->hasFocus() && !event->commitString().isEmpty()) {
        // 插入已提交的文本（候选词选定后）
        searchBox->insertPlainText(event->commitString());
        event->accept();
    } else {
        event->ignore();
    }
}

void Player::updateCoverMenuState(bool visible)
{
    QList<QAction*> audioActions = ui->menubar->actions();
    foreach (QAction *action, audioActions) {
        if (action->text() == "音乐(&A)") {
            QMenu *audioMenu = action->menu();
            if (audioMenu && !audioMenu->actions().isEmpty()) {
                QAction *coverAction = audioMenu->actions().first();
                coverAction->setText(visible ? "隐藏封面" : "显示封面");
                coverAction->setChecked(visible);
            }
            break;
        }
    }
}

//处理元数据加载结果
void Player::handleMetadataLoaded(const QString &filePath, const MusicMetadata &metadata)
{
    for (int i = 0; i < playlistWidget->count(); ++i) {
        QListWidgetItem *item = playlistWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            item->setText(metadata.title.isEmpty() ? QFileInfo(filePath).fileName() : metadata.title);
            item->setData(Qt::UserRole + 1, metadata.artist);
            item->setData(Qt::UserRole + 2, metadata.album);

            // 如果有封面，可以在这里处理
            if (!metadata.cover.isNull()) {
                QPixmap cover = QPixmap::fromImage(metadata.cover.scaled(40, 40, Qt::KeepAspectRatio));
                item->setIcon(cover);
            }
            break;
        }
    }
}




void Player::handleThumbnailGenerated(const QString &filePath, const QPixmap &thumbnail)
{
    // 存入缓存
    thumbnailCache.insert(filePath, thumbnail);

    // 更新UI中的对应项
    for (int i = 0; i < playlistWidget->count(); ++i) {
        QListWidgetItem *item = playlistWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            item->setIcon(QIcon(thumbnail));
            break;
        }
    }
}

void Player::handleThumbnailFailed(const QString &filePath, const QString &error)
{
    qDebug() << "Failed to generate thumbnail for" << filePath << ":" << error;
    // 可以设置默认图标或重试逻辑
}


// 在滚动事件中触发
void Player::onPlaylistScrolled()
{
    generateThumbnailForVisibleItems();
}


// 当需要生成缩略图时（如添加新文件或滚动到可见区域）
void Player::generateThumbnailForVisibleItems()
{
    QRect visibleRect = playlistWidget->viewport()->rect();

    for (int i = 0; i < playlistWidget->count(); ++i) {
        QListWidgetItem *item = playlistWidget->item(i);
        QRect itemRect = playlistWidget->visualItemRect(item);

        if (visibleRect.intersects(itemRect)) {
            QString filePath = item->data(Qt::UserRole).toString();

        }
    }
}


void Player::handleVideoFrame(const QVideoFrame &frame)
{
    if(!frame.isValid()) return;

    qint64 frameTime = frame.startTime() / 1000;
    qint64 audioPos = mediaPlayer->position();

    // 丢弃过快视频帧（超过200ms）
    if(frameTime - audioPos > 200) {
        return;
    }

    // 正常渲染
    videoWidget->repaint();
}


void Player::updatePlaylistItem(const QString &filePath, const MusicMetadata &metadata)
{
    for (int i = 0; i < playlistWidget->count(); ++i) {
        QListWidgetItem *item = playlistWidget->item(i);
        if (item->data(Qt::UserRole).toString() == filePath) {
            // 设置图标（封面）
            if (!metadata.cover.isNull()) {
                QPixmap cover = QPixmap::fromImage(metadata.cover)
                    .scaled(48, 48, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                item->setIcon(cover);
            } else {
                item->setIcon(QIcon("/home/Inf/Desktop/DtkProject/Inf_player/musicapp.svg"));
            }

            // 设置文本信息
            QString displayText = metadata.title.isEmpty()
                ? QFileInfo(filePath).fileName()
                : metadata.title;

            if (!metadata.artist.isEmpty() || !metadata.album.isEmpty()) {
                displayText += QString("\n%1 - %2")
                    .arg(metadata.artist.isEmpty() ? "未知艺术家" : metadata.artist)
                    .arg(metadata.album.isEmpty() ? "未知专辑" : metadata.album);
            }


            item->setText(displayText);
            item->setToolTip(displayText); // 添加工具提示

            // 设置文本对齐和换行
//            item->setTextAlignment(Qt::AlignLeft | Qt::AlignVCenter);
            // 设置文本对齐（使用 QVariant::fromValue 转换）
            item->setData(Qt::TextAlignmentRole, QVariant::fromValue(Qt::AlignLeft | Qt::AlignVCenter));
            break;
        }
    }
}

//更新播放区域的样式
void Player::updatePlaybackAreaStyle()
{
    QString style;
    if (currentTheme == "light") {
        style = "background-color: #dbdbdb;";
    } else if (currentTheme == "dark") {
        style = "background-color: #2b2b2b;";
    } else if (currentTheme == "blue") {
        style = "background-color: #1a237e;";
    } else {  // 其他所有情况默认白色
        style = "background-color: #ffffff;";
    }




    videoWidget->setStyleSheet(style);
    coverLabel->setStyleSheet(style);


    // 确保歌词显示区域也有适当的背景色
    if (lyricsDisplay) {
        lyricsDisplay->setStyleSheet("background: transparent;");
    }


}



// 视频首帧捕获
void Player::captureVideoThumbnail(const QString &filePath) {
    QMediaPlayer* thumbnailPlayer = new QMediaPlayer;
    QVideoSink* videoSink = new QVideoSink;
    thumbnailPlayer->setVideoSink(videoSink);

    connect(videoSink, &QVideoSink::videoFrameChanged, this,
        [=, this](const QVideoFrame &frame) {  // 注意捕获 this 指针
            if (!frame.isValid()) return;

            QImage image = frame.toImage();
            if (!image.isNull()) {
                // 正确访问类成员 metadataCache
                metadataCache[filePath].cover = image;

                // 更新封面显示（带缩放）
                QPixmap pixmap = QPixmap::fromImage(image)
                    .scaled(coverLabel->size() * devicePixelRatio(),
                            Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
                pixmap.setDevicePixelRatio(devicePixelRatio());
                coverLabel->setPixmap(pixmap);
            }

            thumbnailPlayer->deleteLater();
        }
    );

    thumbnailPlayer->setSource(QUrl::fromLocalFile(filePath));
    thumbnailPlayer->play();
}

void Player::applyPureBackground(const QString &path)
{
    // 加载图片
    QPixmap bgImage(path);
    if(bgImage.isNull()) return;

    // 直接设置原始图片
    bgLabel->setPixmap(bgImage.scaled(this->size(), Qt::KeepAspectRatioByExpanding));
    bgLabel->adjustSize();

    // 保存路径
    currentBgPath = path;
    QSettings().setValue("pure_bg", path);
}


QString Player::detectSystemTheme()
{
    // 检测系统主题
    QSettings settings;

    // Linux (GNOME/KDE)
    if (QFile::exists("/usr/share/gnome-shell/theme/Adwaita-dark.css")) {
        return "dark";
    }

    //通过gsettings检测主题
    QProcess process;
    process.start("gsettings", QStringList() << "get" << "com.deepin.dde.appearance" << "gtk-theme");
    if (process.waitForFinished(1000)) {
        QString output = process.readAllStandardOutput().trimmed().replace("'", "");
        if (output.contains("dark", Qt::CaseInsensitive)) {
            return "dark";
        }
        if (!output.isEmpty()) {
            return "light";
        }
    }

    // Windows
    #ifdef Q_OS_WIN
    // 在 Windows 上检测系统主题
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD value;
        DWORD size = sizeof(value);
        if (RegQueryValueExA(hKey, "AppsUseLightTheme", nullptr, nullptr, reinterpret_cast<LPBYTE>(&value), &size) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return value == 0 ? "dark" : "light";
        }
        RegCloseKey(hKey);
    }
    #endif

    // 默认返回浅色主题
    return "light";
}



