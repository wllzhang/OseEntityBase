#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "OsgQt/GraphicsWindowQt.h"
#include "imageviewerwindow.h"
#include "geo/geoentitymanager.h"
#include "geo/mapstatemanager.h"

#include <QDebug>
#include <QTimer>
#include <QPushButton>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QFileInfo>
#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>

#include <osgDB/ReadFile>
#include <osgEarth/MapNode>
#include <osgGA/TrackballManipulator>
#include <osgGA/StateSetManipulator>
#include <osgGA/TerrainManipulator>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarth/Viewpoint>
#include <osgEarth/SpatialReference>
#include <osgEarth/Profile>
#include <osgEarth/GeoData>
#include <osg/Geode>
#include <osg/Shape>
#include <osg/ShapeDrawable>

#include <cmath>


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentMode_(MAP_MODE_3D)
{
    ui->setupUi(this);
    
    // 启用拖放功能
    setAcceptDrops(true);
    
    // 只使用3D进行测试
    earth3DPath_ = "E:/osgqtlib/osgEarthmy_osgb/earth/my.earth";
    
    // 检查文件是否存在
    if (!QFileInfo(earth3DPath_).exists()) {
        qDebug() << "警告: 3D地图文件未找到:" << earth3DPath_;
    }
    
    qDebug() << "3D地图路径:" << earth3DPath_;
    
    // 初始化OSG组件
    root_ = new osg::Group;
    viewer_ = new osgViewer::Viewer;
    imageViewerWindow_ = nullptr;
    entityManager_ = nullptr;
    mapStateManager_ = nullptr;
    
    // 初始化查看器
    initializeViewer();
    
    // 延迟加载地图
    QTimer::singleShot(100, this, [this]() {
        loadMap(earth3DPath_);
        setupManipulator(MAP_MODE_3D);
        
    });
    
    // 创建控制面板
    QWidget* controlWidget = new QWidget;
    QHBoxLayout* layout = new QHBoxLayout(controlWidget);
    
    QPushButton* btn3D = new QPushButton("3D模式");
    QPushButton* btnImages = new QPushButton("战斗机");
    QPushButton* btnTestHeading = new QPushButton("测试旋转");
    
    connect(btn3D, &QPushButton::clicked, this, &MainWindow::switchTo3DMode);
    connect(btnImages, &QPushButton::clicked, this, &MainWindow::openImageViewer);
    connect(btnTestHeading, &QPushButton::clicked, this, &MainWindow::testSetHeading);
    
    layout->addWidget(btn3D);
    layout->addWidget(btnImages);
    layout->addWidget(btnTestHeading);
    layout->addStretch();
    
    // 将控制面板添加到状态栏
    this->statusBar()->addWidget(controlWidget);
    
    // 渲染定时器
    QTimer *renderTimer = new QTimer(this);
    connect(renderTimer, &QTimer::timeout, [&](){
        viewer_->frame();
    });
    renderTimer->start(1000/60);
    
}

MainWindow::~MainWindow()
{
    if (imageViewerWindow_) {
        delete imageViewerWindow_;
    }
    delete ui;
}

void MainWindow::initializeViewer()
{
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = this->width();
    traits->height = this->height();
    traits->windowDecoration = false;
    traits->doubleBuffer = true;

    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    camera->setGraphicsContext(new osgQt::GraphicsWindowQt(traits.get()));
    camera->setClearColor(osg::Vec4(0.5f, 0.7f, 1.0f, 1.0f));
    
    setupCamera();
    
    viewer_->setCamera(camera);
    viewer_->setSceneData(root_);
    viewer_->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>(camera->getGraphicsContext());
    this->setCentralWidget(gw->getGLWidget());
}

void MainWindow::setupCamera()
{
    osg::Camera* camera = viewer_->getCamera();
    if (!camera) return;
    
    double aspectRatio = 1.0 * this->width() / this->height();
    camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1e7);
    camera->setViewport(0, 0, this->width(), this->height());
    
    qDebug() << "相机设置完成";
}

void MainWindow::loadMap(const QString& earthFile)
{
    root_->removeChildren(0, root_->getNumChildren());
    
    QFileInfo fileInfo(earthFile);
    if (!fileInfo.exists()) {
        qDebug() << "地图文件未找到:" << earthFile;
        return;
    }
    
    qDebug() << "加载地图:" << earthFile;
    
    osg::ref_ptr<osg::Node> mapNode = osgDB::readNodeFile(earthFile.toStdString());
    if (mapNode.valid()) {
        root_->addChild(mapNode);
        
        // 查找MapNode
        mapNode_ = osgEarth::MapNode::findMapNode(mapNode.get());
        if (mapNode_) {
            // 初始化实体管理器
            if (!entityManager_) {
                entityManager_ = new GeoEntityManager(root_.get(), mapNode_.get(), this);
                loadEntityConfig();
            }
            
            // 初始化地图状态管理器
            if (!mapStateManager_) {
                mapStateManager_ = new MapStateManager(viewer_.get(), this);
                connect(mapStateManager_, &MapStateManager::stateChanged, 
                        this, &MainWindow::onMapStateChanged);
                connect(mapStateManager_, &MapStateManager::viewPositionChanged, 
                        this, &MainWindow::onViewPositionChanged);
                connect(mapStateManager_, &MapStateManager::mousePositionChanged, 
                        this, &MainWindow::onMousePositionChanged);
                
                // ===== 设置GLWidget的MapStateManager =====
                // 获取GLWidget并设置MapStateManager
                // 这样GLWidget的鼠标事件处理函数会直接调用MapStateManager
                osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>(viewer_->getCamera()->getGraphicsContext());
                if (gw && gw->getGLWidget()) {
                    // 设置GLWidget的地图状态管理器
                    // 这样GLWidget在处理鼠标事件时会自动通知MapStateManager
                    gw->getGLWidget()->setMapStateManager(mapStateManager_);
                    qDebug() << "MapStateManager已设置到GLWidget";
                } else {
                    qDebug() << "无法获取GLWidget，MapStateManager设置失败";
                }
                
                qDebug() << "地图状态管理器初始化完成";
            }
            
            qDebug() << "地图加载成功";
        } else {
            qDebug() << "未找到MapNode";
        }
    } else {
        qDebug() << "地图加载失败";
    }
}

void MainWindow::setupManipulator(MapMode mode)
{
    currentMode_ = mode;
    setupCamera();
    
    // 使用EarthManipulator来显示地图，但设置合适的初始位置
    osg::ref_ptr<osgEarth::Util::EarthManipulator> em = new osgEarth::Util::EarthManipulator();
    
    // 设置操作器参数
    osgEarth::Util::EarthManipulator::Settings* settings = em->getSettings();
    settings->setMinMaxDistance(1000.0, 10000000000.0);  // 设置缩放范围
    settings->setMinMaxPitch(-89.0, 89.0);            // 设置俯仰角范围
    
    // 设置初始视点，看向北京（红色立方体位置）
    // 参数：名称, 经度, 纬度, 高度, 航向角, 俯仰角, 距离
    osgEarth::Viewpoint vp("Beijing", 116.4, 39.9, 0.0, 0.0, -90.0, 100000.0);
    em->setHomeViewpoint(vp);
    
    viewer_->setCameraManipulator(em.get());
    viewer_->home();
    
    qDebug() << "操作器设置完成";
}

void MainWindow::switchTo3DMode()
{
    loadMap(earth3DPath_);
    setupManipulator(MAP_MODE_3D);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    
    // 更新相机视口和投影
    if (viewer_ && viewer_->getCamera()) {
        osg::Camera* camera = viewer_->getCamera();
        camera->setViewport(new osg::Viewport(0, 0, this->width(), this->height()));
        
        setupCamera();
    }
}


void MainWindow::loadEntityConfig()
{
    QString configPath = "E:/osgqtlib/osgEarthmy_osgb/images_config.json";
    QFile file(configPath);
    
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开实体配置文件:" << configPath;
        return;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        return;
    }
    
    QJsonObject config = doc.object();
    if (entityManager_) {
        entityManager_->setEntityConfig(config);
        qDebug() << "实体配置加载完成";
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasText()) {
        QString text = event->mimeData()->text();
        if (text.startsWith("aircraft:")) {
            event->acceptProposedAction();
            qDebug() << "接受拖拽:" << text;
            return;
        }
    }
    event->ignore();
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (!entityManager_) {
        event->ignore();
        return;
    }
    
    QString text = event->mimeData()->text();
    if (!text.startsWith("aircraft:")) {
        event->ignore();
        return;
    }
    
    QPoint dropPos = event->pos();
    double longitude = 116.4 + (dropPos.x() - this->width()/2) * 0.01;
    double latitude = 39.9 + (this->height()/2 - dropPos.y()) * 0.01;
    double altitude = 100000.0;
    
    if (entityManager_->addEntityFromDrag(text, longitude, latitude, altitude)) {
        
        // 调整相机到实体位置
        osgEarth::Util::EarthManipulator* em = dynamic_cast<osgEarth::Util::EarthManipulator*>(viewer_->getCameraManipulator());
        if (em) {
            osgEarth::Viewpoint vp("Entity", longitude, latitude, 0.0, 0.0, -45.0, 50000.0);
            em->setViewpoint(vp, 2.0);
            qDebug() << "相机已调整到实体位置:" << longitude << latitude;
        }
        
        qDebug() << "实体添加完成";
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::openImageViewer()
{
    if (!imageViewerWindow_) {
        imageViewerWindow_ = new ImageViewerWindow(this);
    }
    
    imageViewerWindow_->show();
    imageViewerWindow_->raise();
    imageViewerWindow_->activateWindow();
}

void MainWindow::testSetHeading()
{
    qDebug() << "=== 测试设置实体旋转角度 ===";
    
    if (!entityManager_) {
        qDebug() << "实体管理器未初始化";
        return;
    }
    
    // 测试设置第一个实体的旋转角度
    QStringList entityList = entityManager_->getEntityIds();
    if (!entityList.isEmpty()) {
        QString entityId = entityList.first();
        double headingDegrees = 45.0;  // 设置45度旋转角度
        
        qDebug() << "设置实体旋转角度:";
        qDebug() << "实体ID:" << entityId;
        qDebug() << "旋转角度:" << headingDegrees << "度";
        
        GeoEntity* entity = entityManager_->getEntity(entityId);
        if (entity) {
            entity->setHeading(headingDegrees);
            qDebug() << "实体旋转角度设置完成";
        } else {
            qDebug() << "未找到指定的实体:" << entityId;
        }
    } else {
        qDebug() << "没有找到任何实体";
    }
}

void MainWindow::onMapStateChanged(const MapStateInfo& state)
{
    auto tuple = state.getTuple();
    qDebug() << "=== 地图状态变化 ===";
    qDebug() << "9元组信息 (a,b,c,x1,y1,z1,x2,y2,z2):";
    qDebug() << "a (Pitch俯仰角):" << std::get<0>(tuple);
    qDebug() << "b (Heading航向角):" << std::get<1>(tuple);
    qDebug() << "c (Range距离):" << std::get<2>(tuple);
    qDebug() << "x1 (视角经度):" << std::get<3>(tuple);
    qDebug() << "y1 (视角纬度):" << std::get<4>(tuple);
    qDebug() << "z1 (视角高度):" << std::get<5>(tuple);
    qDebug() << "x2 (鼠标经度):" << std::get<6>(tuple);
    qDebug() << "y2 (鼠标纬度):" << std::get<7>(tuple);
    qDebug() << "z2 (鼠标高度):" << std::get<8>(tuple);
}

void MainWindow::onMousePositionChanged(double longitude, double latitude, double altitude)
{
    qDebug() << "鼠标位置变化 - 经度:" << longitude << "纬度:" << latitude << "高度:" << altitude;
}

void MainWindow::onViewPositionChanged(double longitude, double latitude, double altitude)
{
    qDebug() << "视角位置变化 - 经度:" << longitude << "纬度:" << latitude << "高度:" << altitude;
}

