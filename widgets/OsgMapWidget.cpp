/**
 * @file OsgMapWidget.cpp
 * @brief OSG地图Widget组件实现文件
 * 
 * 实现OsgMapWidget类的所有功能，包括地图加载、视图切换、拖拽处理等
 */

#include "OsgMapWidget.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QResizeEvent>
#include <QShowEvent>

#include <osgDB/ReadFile>
#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarth/Viewpoint>
#include <osg/Camera>
#include <osgViewer/Viewer>
#include <osg/GraphicsContext>
#include <algorithm>

#include "../geo/geoentitymanager.h"
#include "../geo/mapstatemanager.h"
#include "../geo/geoutils.h"
#include "../plan/planfilemanager.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>

OsgMapWidget::OsgMapWidget(QWidget *parent)
    : QWidget(parent)
    , viewer_(nullptr)
    , root_(nullptr)
    , mapNode_(nullptr)
    , gw_(nullptr)
    , entityManager_(nullptr)
    , mapStateManager_(nullptr)
    , planFileManager_(nullptr)
{
    // 启用拖放功能
    setAcceptDrops(true);
    // 设置布局
    setLayout(new QVBoxLayout(this));
    layout()->setContentsMargins(0, 0, 0, 0);
    
    // 初始化OSG
    root_ = new osg::Group;
    viewer_ = new osgViewer::Viewer;
    
    // 创建GraphicsWindow
    osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
    traits->width = std::max(width(), 100);  // 避免为0
    traits->height = std::max(height(), 100);
    traits->windowDecoration = false;
    traits->doubleBuffer = true;

    osg::ref_ptr<osg::Camera> camera = new osg::Camera;
    gw_ = new osgQt::GraphicsWindowQt(traits.get());
    camera->setGraphicsContext(gw_);
    camera->setClearColor(osg::Vec4(0.5f, 0.7f, 1.0f, 1.0f));
    
    setupCamera();
    
    viewer_->setCamera(camera);
    viewer_->setSceneData(root_);
    viewer_->setThreadingModel(osgViewer::Viewer::SingleThreaded);
    
    // 获取GLWidget并添加到布局
    QGLWidget* glWidget = gw_->getGLWidget();
    layout()->addWidget(glWidget);
    
    // 初始化查看器
    initializeViewer();
    
    // 定时器用于刷新渲染（但不在构造函数中启动）
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        if (viewer_) {
            viewer_->frame();
            // frame()完成后，立即处理延迟删除队列，确保不在渲染过程中删除
            if (entityManager_) {
                entityManager_->processPendingDeletions();
            }
        }
    });
    // 不在这里启动，等窗口显示后再启动
    
    qDebug() << "OsgMapWidget初始化完成";
}

OsgMapWidget::~OsgMapWidget()
{
    timer_->stop();
}

void OsgMapWidget::initializeViewer()
{
    // 延迟加载地图
    QTimer::singleShot(100, this, [this]() {
        loadMap();
        setupManipulator();
    });
}

void OsgMapWidget::setupCamera()
{
    osg::Camera* camera = viewer_->getCamera();
    if (!camera) return;
    
    double aspectRatio = 1.0 * width() / height();
    camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1e7);
    camera->setViewport(0, 0, width(), height());
    
    qDebug() << "相机设置完成";
}

void OsgMapWidget::loadMap()
{
    // 清除现有场景
    root_->removeChildren(0, root_->getNumChildren());
    
    QString earthPath = ":/earth/my.earth";
    
    // 使用工具函数将Qt资源路径转换为文件路径
    QString errorMsg;
    QString filePath = GeoUtils::convertResourcePathToFile(earthPath, &errorMsg);
    if (filePath.isEmpty()) {
        qDebug() << "无法转换资源路径:" << errorMsg;
        return;
    }
    
    qDebug() << "加载地图:" << earthPath;
    
    osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(filePath.toStdString());
    if (node.valid()) {
        root_->addChild(node);
        mapNode_ = osgEarth::MapNode::findMapNode(node.get());
        if (mapNode_) {
            qDebug() << "地图加载成功";
            
            // 初始化实体管理器
            if (!entityManager_) {
                entityManager_ = new GeoEntityManager(root_.get(), mapNode_.get(), this);
                entityManager_->setViewer(viewer_.get());
                qDebug() << "实体管理器初始化完成";
            }
            
            // 初始化地图状态管理器
            if (!mapStateManager_) {
                mapStateManager_ = new MapStateManager(viewer_.get(), this);
                qDebug() << "地图状态管理器初始化完成";
                
                // 设置GLWidget的管理器
                if (gw_ && gw_->getGLWidget()) {
                    gw_->getGLWidget()->setMapStateManager(mapStateManager_);
                    gw_->getGLWidget()->setEntityManager(entityManager_);
                    qDebug() << "管理器已设置到GLWidget";
                }
                
                // 将MapStateManager注入到EntityManager
                if (entityManager_ && mapStateManager_) {
                    entityManager_->setMapStateManager(mapStateManager_);
                }
            }
        } else {
            qDebug() << "警告：未找到MapNode";
        }
    } else {
        qDebug() << "地图加载失败";
    }
}

void OsgMapWidget::setupManipulator()
{
    // 默认设置为3D模式
    setMode3D();
    
    // 发送地图加载完成信号
    emit mapLoaded();
}

void OsgMapWidget::setMode2D()
{
    if (!viewer_ || !mapNode_) {
        qDebug() << "OsgMapWidget: Viewer或MapNode未初始化，无法设置2D模式";
        return;
    }
    
    // 使用EarthManipulator来显示地图
    osg::ref_ptr<osgEarth::Util::EarthManipulator> em = new osgEarth::Util::EarthManipulator();
    
    // 设置操作器参数
    osgEarth::Util::EarthManipulator::Settings* settings = em->getSettings();
    settings->setMinMaxPitch(-90.0, -89.0);            // 设置俯仰角范围
    settings->setMinMaxDistance(1000.0, 4605500.0);  // 设置缩放范围
    
    // 2D视角：俯仰角-89.9度，航向角-0.916737度，距离540978，经度116.347，纬度40.0438，高度-1.70909
    osgEarth::Viewpoint vp = osgEarth::Viewpoint("2D View", 116.347, 40.0438, -1.70909, -0.916737, -90, 540978.0);
    em->setHomeViewpoint(vp);
    
    viewer_->setCameraManipulator(em.get());
    viewer_->home();
    
    qDebug() << "设置为2D模式";
}

void OsgMapWidget::setMode3D()
{
    if (!viewer_ || !mapNode_) {
        qDebug() << "OsgMapWidget: Viewer或MapNode未初始化，无法设置3D模式";
        return;
    }
    
    // 使用EarthManipulator来显示地图
    osg::ref_ptr<osgEarth::Util::EarthManipulator> em = new osgEarth::Util::EarthManipulator();
    
    // 设置操作器参数
    osgEarth::Util::EarthManipulator::Settings* settings = em->getSettings();
    settings->setMinMaxPitch(-90, 90);            // 设置俯仰角范围
    settings->setMinMaxDistance(1000.0, 50000000.0);  // 设置缩放范围
    
    // 3D视角：俯仰角-76.466度，航向角0度，距离1.27252e+07，经度109.257，纬度41.82，高度-38.5648
    osgEarth::Viewpoint vp = osgEarth::Viewpoint("3D View", 109.257, 41.82, -38.5648, 0.0, -76.466, 12725200.0);
    em->setHomeViewpoint(vp);
    
    viewer_->setCameraManipulator(em.get());
    viewer_->home();
    
    qDebug() << "设置为3D模式";
}

void OsgMapWidget::showEvent(QShowEvent* event)
{
    // 窗口显示后才启动渲染定时器
    if (timer_ && !timer_->isActive()) {
        timer_->start(16); // ~60 FPS
        qDebug() << "启动OSG渲染定时器";
    }
    QWidget::showEvent(event);
}

void OsgMapWidget::resizeEvent(QResizeEvent* event)
{
    if (viewer_ && viewer_->getCamera()) {
        osg::Camera* camera = viewer_->getCamera();
        camera->setViewport(0, 0, event->size().width(), event->size().height());
        
        double aspectRatio = 1.0 * event->size().width() / event->size().height();
        camera->setProjectionMatrixAsPerspective(30.0, aspectRatio, 1.0, 1e7);
    }
    
    QWidget::resizeEvent(event);
}

void OsgMapWidget::setPlanFileManager(PlanFileManager* planFileManager)
{
    planFileManager_ = planFileManager;
}

void OsgMapWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasText()) {
        QString text = event->mimeData()->text();
        if (text.startsWith("modeldeploy:")) {
            event->acceptProposedAction();
            qDebug() << "OsgMapWidget: 接受拖拽:" << text;
            return;
        }
    }
    event->ignore();
}

void OsgMapWidget::dropEvent(QDropEvent* event)
{
    if (!entityManager_) {
        qDebug() << "OsgMapWidget: EntityManager为空，无法处理拖拽";
        event->ignore();
        return;
    }
    
    QString text = event->mimeData()->text();
    if (!text.startsWith("modeldeploy:")) {
        event->ignore();
        return;
    }
    
    // 解析拖拽数据格式: "modeldeploy:{modelId}:{modelName}"
    QStringList parts = text.split(":");
    if (parts.size() < 3) {
        qDebug() << "OsgMapWidget: 无效的拖拽数据格式:" << text;
        event->ignore();
        return;
    }
    
    QString modelId = parts[1];
    QString modelName = parts[2];
    qDebug() << "OsgMapWidget: 拖拽模型 ID:" << modelId << "名称:" << modelName;
    
    // 获取 GLWidget 用于坐标转换
    QWidget* glWidget = gw_ ? gw_->getGLWidget() : nullptr;
    if (!glWidget) {
        qDebug() << "OsgMapWidget: 无法获取 GLWidget，拖拽操作失败";
        event->ignore();
        return;
    }
    
    // 将 OsgMapWidget 坐标转换为 GLWidget 的本地坐标
    QPoint dropPos = event->pos();
    QPoint glWidgetPos = glWidget->mapFrom(this, dropPos);
    
    // 检查坐标是否在 GLWidget 范围内
    if (!glWidget->rect().contains(glWidgetPos)) {
        qDebug() << "OsgMapWidget: 拖拽位置不在 GLWidget 范围内:" << dropPos << "->" << glWidgetPos;
        event->ignore();
        return;
    }
    
    // 转换为地理坐标 - 统一使用 MapStateManager 获取坐标信息（mapStateManager_ 必然存在）
    double longitude, latitude, altitude;
    mapStateManager_->getGeoCoordinatesFromScreen(glWidgetPos, longitude, latitude, altitude);
    
    // 创建实体（高度已经通过 MapStateManager 设置为默认高度）
    GeoEntity* entity = entityManager_->createEntity(
        "aircraft",
        modelName,
        QJsonObject(),
        longitude,
        latitude,
        altitude
    );
    
    if (entity) {
        // 设置模型ID和方案文件关联
        entity->setProperty("modelId", modelId);
        
        // 如果有PlanFileManager，添加到方案
        if (planFileManager_) {
            planFileManager_->addEntityToPlan(entity);
            qDebug() << "OsgMapWidget: 实体已添加到方案";
        }
        
        // 调整相机到实体位置，参考mainwindow.cpp的实现
        osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer_.get());
        if (em) {
            // 使用实体位置作为相机视点
            osgEarth::Viewpoint vp("Entity", longitude, latitude, 0.0, 0.0, -90.0, 1000000.0);
            em->setViewpoint(vp, 2.0);
            qDebug() << "OsgMapWidget: 相机已调整到实体位置:" << longitude << latitude;
        }
        
        qDebug() << "OsgMapWidget: 实体创建成功，位置:" << longitude << latitude << altitude;
        event->acceptProposedAction();
    } else {
        qDebug() << "OsgMapWidget: 实体创建失败";
        event->ignore();
    }
}