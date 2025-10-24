QT  += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# Set source code encoding to UTF-8
CODECFORTR = UTF-8
CODECFORSRC = UTF-8

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
QMAKE_CXXFLAGS += /source-charset:utf-8 /execution-charset:utf-8
# OSG库配置
OSGDIR = E:/osgqtlib/

CONFIG(release, debug|release) {
    LIBS += -L$${OSGDIR}/lib/ \
        -lOpenThreads \
        -losg \
        -losgAnimation \
        -losgDB \
        -losgEarth \
        -losgEarthAnnotation \
        -losgEarthFeatures \
        -losgEarthSplat \
        -losgEarthSymbology \
        -losgEarthUtil \
        -losgFX \
        -losgGA \
        -losgManipulator \
        -losgParticle \
        -losgPresentation \
        -losgShadow \
        -losgSim \
        -losgTerrain \
        -losgText \
        -losgUI \
        -losgUtil \
        -losgViewer \
        -losgVolume \
        -losgWidget \
        -losgdb_osgearth_feature_ogr \
        -losgdb_osgearth_feature_tfs \
        -losgdb_osgearth_feature_wfs \
        -losgdb_osgearth_feature_xyz \
        -losgdb_osgearth_gdal
} else {
    LIBS += -L$${OSGDIR}/lib/ \
        -lOpenThreadsd \
        -losgAnimationd \
        -losgDBd \
        -losgEarthAnnotationd \
        -losgEarthFeaturesd \
        -losgEarthSplatd \
        -losgEarthSymbologyd \
        -losgEarthUtild \
        -losgEarthd \
        -losgFXd \
        -losgGAd \
        -losgManipulatord \
        -losgParticled \
        -losgPresentationd \
        -losgShadowd \
        -losgSimd \
        -losgTerraind \
        -losgTextd \
        -losgUId \
        -losgUtild \
        -losgViewerd \
        -losgVolumed \
        -losgWidgetd \
        -losgd \
        -losgdb_osgearth_feature_ogrd \
        -losgdb_osgearth_feature_tfsd \
        -losgdb_osgearth_feature_wfsd \
        -losgdb_osgearth_feature_xyzd \
        -losgdb_osgearth_gdald
}

INCLUDEPATH += $${OSGDIR}/include
DEPENDPATH += $${OSGDIR}/include

 
SOURCES += \
    OsgQt/GraphicsWindowQt.cpp \
    OsgQt/QGraphicsViewAdapter.cpp \
    OsgQt/QWidgetImage.cpp \
    main.cpp \
    mainwindow.cpp \
    imageviewerwindow.cpp \
    draggablelistwidget.cpp \
    geo/geoentitymanager.cpp \
    geo/geoentity.cpp \
    geo/imageentity.cpp \
    geo/mapstatemanager.cpp

HEADERS += \
    OsgQt/GraphicsWindowQt.h \
    OsgQt/QGraphicsViewAdapter.h \
    OsgQt/QWidgetImage.h \
    mainwindow.h \
    imageviewerwindow.h \
    draggablelistwidget.h \
    geo/geoentitymanager.h \
    geo/geoentity.h \
    geo/imageentity.h \
    geo/mapstatemanager.h

FORMS += \
    mainwindow.ui

