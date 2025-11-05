QT       += core gui opengl
CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets sql

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES -= QT_NO_DEBUG_OUTPUT
msvc:QMAKE_CXXFLAGS += -execution-charset:utf-8
msvc:QMAKE_CXXFLAGS += -source-charset:utf-8

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

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
    ui/ComponentConfigDialog.cpp \
    ui/MainWidget.cpp \
    ui/ModelAssemblyDialog.cpp \
    ui/ModelDeployDialog.cpp \
    ui/EntityPropertyDialog.cpp \
    main.cpp \
    widgets/OsgMapWidget.cpp \
    OsgQt/GraphicsWindowQt.cpp \
    OsgQt/QGraphicsViewAdapter.cpp \
    OsgQt/QWidgetImage.cpp \
    geo/geoentity.cpp \
    geo/geoentitymanager.cpp \
    geo/imageentity.cpp \
    geo/mapstatemanager.cpp \
    geo/waypointentity.cpp \
    geo/geoutils.cpp \
    util/databaseutils.cpp \
    plan/planfilemanager.cpp \
    widgets/draggablelistwidget.cpp \
    widgets/imageviewerwindow.cpp

HEADERS += \
    ui/ComponentConfigDialog.h \
    ui/MainWidget.h \
    ui/ModelAssemblyDialog.h \
    ui/ModelDeployDialog.h \
    ui/EntityPropertyDialog.h \
    widgets/OsgMapWidget.h \
    OsgQt/GraphicsWindowQt.h \
    OsgQt/QGraphicsViewAdapter.h \
    OsgQt/QWidgetImage.h \
    geo/geoentity.h \
    geo/geoentitymanager.h \
    geo/imageentity.h \
    geo/mapstatemanager.h \
    geo/waypointentity.h \
    geo/geoutils.h \
    util/databaseutils.h \
    plan/planfilemanager.h \
    widgets/draggablelistwidget.h \
    widgets/imageviewerwindow.h

FORMS += \
    ui/MainWidget.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

 

RESOURCES += \
    res.qrc
