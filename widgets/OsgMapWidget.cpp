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
#include "../geo/navigationhistory.h"
#include "../geo/basemapmanager.h"
#include "../plan/planfilemanager.h"
#include "MapInfoOverlay.h"
#include <osgEarth/Map>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include <QApplication>
#include <QCursor>
#include <QMouseEvent>

OsgMapWidget::OsgMapWidget(QWidget *parent)
    : QWidget(parent)
    , viewer_(nullptr)
    , root_(nullptr)
    , mapNode_(nullptr)
    , gw_(nullptr)
    , entityManager_(nullptr)
    , mapStateManager_(nullptr)
    , planFileManager_(nullptr)
    , mapInfoOverlay_(nullptr)
    , navigationHistory_(nullptr)
    , baseMapManager_(nullptr)
{
    // 启用拖放功能
    setAcceptDrops(true);
    setFocusPolicy(Qt::StrongFocus);
    setFocus(Qt::OtherFocusReason);
    // 设置布局（使用堆叠布局，让信息叠加层在最上层）
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    
    // 创建导航历史管理器
    navigationHistory_ = new NavigationHistory(this);
    
    // 创建信息叠加层（在最上层，覆盖OSG地图）
    mapInfoOverlay_ = new MapInfoOverlay(this);
    // 注意：MapInfoOverlay内部会处理鼠标事件穿透，这里不需要设置
    // 导航历史信号和按钮已移到MainWidget的工具栏，这里不再连接
    
    // 初始化OSG
    root_ = new osg::Group;
    viewer_ = new osgViewer::Viewer;
    viewer_->setKeyEventSetsDone(0);
    
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
    mainLayout->addWidget(glWidget);
    
    // 信息叠加层不覆盖整个窗口，只用来管理子控件
    // 所有子控件都直接作为OsgMapWidget的子控件（类似QMapControl的做法）
    mapInfoOverlay_->setParent(this);
    mapInfoOverlay_->hide();  // MapInfoOverlay本身不显示
    
    // 设置信息面板为OsgMapWidget的直接子控件（初始隐藏，等地图加载完成后再显示）
    QWidget* infoPanel = mapInfoOverlay_->getInfoPanel();
    if (infoPanel) {
        infoPanel->setParent(this);  // 设置正确的parent
        infoPanel->raise();  // 确保在最上层
        infoPanel->hide();  // 初始隐藏，等地图加载完成后再显示
    }
    
    // 设置指北针widget为OsgMapWidget的直接子控件（初始隐藏，等地图加载完成后再显示）
    QWidget* compassWidget = mapInfoOverlay_->getCompassWidget();
    if (compassWidget) {
        compassWidget->setParent(this);  // 设置正确的parent
        compassWidget->raise();  // 确保在最上层
        compassWidget->hide();  // 初始隐藏，等地图加载完成后再显示
    }
    
    // 设置比例尺widget为OsgMapWidget的直接子控件（初始隐藏，等地图加载完成后再显示）
    QWidget* scaleWidget = mapInfoOverlay_->getScaleWidget();
    if (scaleWidget) {
        scaleWidget->setParent(this);  // 设置正确的parent
        scaleWidget->raise();  // 确保在最上层
        scaleWidget->hide();  // 初始隐藏，等地图加载完成后再显示
    }
    
    // 延迟更新位置，确保OSG先初始化（但不显示，等地图加载完成后再显示）
    QTimer::singleShot(500, this, [this]() {
        if (mapInfoOverlay_ && width() > 0 && height() > 0) {
            mapInfoOverlay_->updateOverlayWidgetsPosition(width(), height());
        }
    });
    
    // 初始化查看器
    initializeViewer();
    
    // 定时器用于刷新渲染（但不在构造函数中启动）
    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        if (viewer_) {
            viewer_->frame();
            // qDebug() << "viewer done?" << viewer_->done();
            // frame()完成后，立即处理延迟删除队列，确保不在渲染过程中删除
            if (entityManager_) {
                entityManager_->processPendingDeletions();
            }

            // 关键：OpenGL一帧完成后，强制刷新叠加控件，避免缩放时的拖影/重影
            if (mapInfoOverlay_) {
                QWidget* infoPanel = mapInfoOverlay_->getInfoPanel();
                QWidget* compass = mapInfoOverlay_->getCompassWidget();
                QWidget* scale = mapInfoOverlay_->getScaleWidget();
                if (infoPanel) infoPanel->update();
                if (compass) compass->update();
                if (scale) scale->update();
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
    
    qDebug() << "开始创建地图（使用BaseMapManager）";
    
    // 创建osgEarth Map对象（对应my.earth的配置）
    osg::ref_ptr<osgEarth::Map> map = new osgEarth::Map();
    map->setName("OpenStreetMap Globe");
    
    // 创建底图管理器
    baseMapManager_ = new BaseMapManager(map.get(), this);
    
    // 默认无底图（对应my.earth的默认状态）
    // 用户可以后续通过底图管理对话框添加底图图层
    
    // 创建MapNode
    mapNode_ = new osgEarth::MapNode(map.get());
    
    if (mapNode_) {
        // 将MapNode添加到场景根节点
        root_->addChild(mapNode_.get());
        
        qDebug() << "地图创建成功 - MapNode已创建";
        
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
        
        // 检查OSG渲染状态
        if (viewer_ && viewer_->getCamera()) {
            qDebug() << "OSG相机已初始化，清除颜色:" 
                     << viewer_->getCamera()->getClearColor().r() << ","
                     << viewer_->getCamera()->getClearColor().g() << ","
                     << viewer_->getCamera()->getClearColor().b();
        }
    } else {
        qDebug() << "错误：MapNode创建失败";
    }
}

void OsgMapWidget::synthesizeMouseRelease(Qt::MouseButton button)
{
    if (!gw_) {
        return;
    }

    osgQt::GLWidget* glWidget = gw_->getGLWidget();
    if (!glWidget) {
        return;
    }

    QPoint globalPos = QCursor::pos();
    QPoint widgetPos = glWidget->mapFromGlobal(globalPos);

    Qt::MouseButtons buttons = Qt::NoButton;
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;

    QMouseEvent releaseEvent(QEvent::MouseButtonRelease,
                             widgetPos,
                             globalPos,
                             button,
                             buttons,
                             modifiers);

    QApplication::sendEvent(glWidget, &releaseEvent);
}

void OsgMapWidget::setupManipulator()
{
    // 默认设置为3D模式
    setMode3D();
    
    // 连接信息叠加层（地图加载完成后）
    if (mapInfoOverlay_ && mapStateManager_) {
        mapInfoOverlay_->setMapStateManager(mapStateManager_);
        mapInfoOverlay_->updateAllInfo();
        
        // 连接状态变化信号，记录导航历史（使用防抖，避免频繁记录）
        QTimer* debounceTimer = new QTimer(this);
        debounceTimer->setSingleShot(true);
        debounceTimer->setInterval(1000); // 1秒防抖，只在视角稳定后记录
        
        connect(mapStateManager_, &MapStateManager::stateChanged, this, [debounceTimer]() {
            debounceTimer->stop();
            debounceTimer->start();
        });
        
        connect(debounceTimer, &QTimer::timeout, this, [this]() {
            if (navigationHistory_ && mapStateManager_) {
                osgEarth::Viewpoint vp = mapStateManager_->getCurrentViewpoint("Auto Save");
                navigationHistory_->pushViewpoint(vp);
            }
        });
        
        // OSG地图加载完成后，延迟显示信息面板（确保地图已经渲染）
        QTimer::singleShot(500, this, [this]() {
            // 检查地图是否已正确加载
            bool mapReady = mapNode_ && mapStateManager_ && viewer_;
            qDebug() << "检查地图渲染状态:"
                     << "mapNode=" << (mapNode_ ? "✓" : "✗")
                     << "mapStateManager=" << (mapStateManager_ ? "✓" : "✗")
                     << "viewer=" << (viewer_ ? "✓" : "✗");
            
            if (mapReady && width() > 0 && height() > 0) {
                // 更新位置
                if (mapInfoOverlay_) {
                    mapInfoOverlay_->updateOverlayWidgetsPosition(width(), height());
                }
                
                // 显示叠加控件
                auto showWidget = [](QWidget* widget, const QString& name) {
                    if (widget) {
                        widget->show();
                        widget->raise();
                        qDebug() << name << "已显示";
                    }
                };
                
                showWidget(mapInfoOverlay_->getInfoPanel(), "信息面板");
                showWidget(mapInfoOverlay_->getCompassWidget(), "指北针");
                showWidget(mapInfoOverlay_->getScaleWidget(), "比例尺");
            } else {
                qDebug() << "警告：地图未准备就绪，信息面板延迟显示";
                // 如果地图未准备就绪，再延迟1秒后重试
                QTimer::singleShot(1000, this, [this]() {
                    auto showWidget = [](QWidget* widget) { if (widget) widget->show(); };
                    showWidget(mapInfoOverlay_->getInfoPanel());
                    showWidget(mapInfoOverlay_->getCompassWidget());
                    showWidget(mapInfoOverlay_->getScaleWidget());
                    qDebug() << "信息面板强制显示（延迟超时）";
                });
            }
        });
    }
    
    // 发送地图加载完成信号
    emit mapLoaded();
}

void OsgMapWidget::setMode2D()
{
    if (!viewer_ || !mapNode_) {
        qDebug() << "OsgMapWidget: Viewer或MapNode未初始化，无法设置2D模式";
        return;
    }
    
    // 保存当前视角到导航历史
    if (navigationHistory_ && mapStateManager_) {
        osgEarth::Viewpoint currentVp = mapStateManager_->getCurrentViewpoint("Before 2D Mode");
        navigationHistory_->pushViewpoint(currentVp);
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
    
    // 保存当前视角到导航历史
    if (navigationHistory_ && mapStateManager_) {
        osgEarth::Viewpoint currentVp = mapStateManager_->getCurrentViewpoint("Before 3D Mode");
        navigationHistory_->pushViewpoint(currentVp);
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
    
    // 更新所有叠加控件的位置
    if (mapInfoOverlay_) {
        mapInfoOverlay_->updateOverlayWidgetsPosition(width(), height());
    }
    
    QWidget::resizeEvent(event);
}

void OsgMapWidget::setPlanFileManager(PlanFileManager* planFileManager)
{
    planFileManager_ = planFileManager;
    
    // 方案名称已移到MainWidget工具栏，这里不再处理
    if (mapInfoOverlay_ && planFileManager_) {
        mapInfoOverlay_->setPlanFileManager(planFileManager_);
    }
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

bool OsgMapWidget::switchBaseMap(const QString& mapName)
{
    // 此方法已废弃，现在使用叠加模式
    // 保留此方法以保持兼容性，但实际功能已由BaseMapDialog管理
    Q_UNUSED(mapName);
    qDebug() << "OsgMapWidget: switchBaseMap已废弃，请使用BaseMapDialog管理底图";
    return false;
}