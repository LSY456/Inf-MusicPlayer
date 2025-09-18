QT += core gui multimedia multimediawidgets widgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

SOURCES += \
    main.cpp \
    player.cpp \
    metadataparser.cpp \
    meradataloader.cpp \
    thumbnailgenerator.cpp \
    fuzzymatcher.cpp \
    lyricsdisplay.cpp

HEADERS += \
    clickableslider.h \
    player.h \
    musicmetadata.h \
    metadataparser.h \
    meradataloader.h \
    thumbnailgenerator.h \
    fuzzymatcher.h \
    lyricsdisplay.h \
    audiocapture.h

FORMS += \
    player.ui 

RESOURCES += \
    resources.qrc 

CONFIG += c++11 link_pkgconfig

PKGCONFIG += taglib

# 启用 Qt6 多媒体扩展
DEFINES += QT_MULTIMEDIA_LIB
