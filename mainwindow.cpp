/**
 * @file mainwindow.cpp
 * @brief 主窗口类实现文件
 * 
 * 实现MainWindow类的所有功能
 */

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "OsgQt/GraphicsWindowQt.h"
#include "imageviewerwindow.h"
#include "geo/geoentitymanager.h"
#include "geo/mapstatemanager.h"
#include "geo/geoutils.h"

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
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QMessageBox>

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
    selectedEntity_ = nullptr;
    entityContextMenu_ = nullptr;
    currentWaypointGroupId_.clear();
    
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
    
    toggleButton_ = new QPushButton("切换到2D");
    QPushButton* btnImages = new QPushButton("战斗机");
    QPushButton* btnPlotPoint = new QPushButton("点标绘");
    QPushButton* btnPlotRoute = new QPushButton("航线标绘");
    
    connect(toggleButton_, &QPushButton::clicked, this, &MainWindow::toggle2D3DMode);
    connect(btnImages, &QPushButton::clicked, this, &MainWindow::openImageViewer);
    connect(btnPlotPoint, &QPushButton::clicked, this, [this]() {
        if (!entityManager_) return;
        bool ok=false;
        QString label = QInputDialog::getText(this, "点标绘", "请输入标签，然后在地图上点击位置放置:", QLineEdit::Normal, "标注", &ok);
        if (!ok) return;
        if (label.trimmed().isEmpty()) label = "标注";
        pendingWaypointLabel_ = label.trimmed();
        isPlacingWaypoint_ = true;
        statusBar()->showMessage("点标绘：请在地图上点击以放置点...", 3000);
    });

    connect(btnPlotRoute, &QPushButton::clicked, this, [this]() {
        if (!entityManager_) return;
        if (isPlacingRoute_) {
            statusBar()->showMessage("航线标绘已在进行中，左键添加点，右键结束。", 2000);
            return;
        }
        currentWaypointGroupId_ = entityManager_->createWaypointGroup("route");
        isPlacingRoute_ = true;
        statusBar()->showMessage("航线标绘：左键依次添加航点，右键结束。", 3000);
    });
    
    layout->addWidget(toggleButton_);
    layout->addWidget(btnImages);
    layout->addWidget(btnPlotPoint);
    layout->addWidget(btnPlotRoute);
    layout->addStretch();
    
    // 将控制面板添加到状态栏
    this->statusBar()->addWidget(controlWidget);
    
    // 渲染定时器
    // 在frame()完成后立即处理延迟删除队列，确保删除在渲染完成后执行
    QTimer *renderTimer = new QTimer(this);
    connect(renderTimer, &QTimer::timeout, [&](){
        viewer_->frame();
        // frame()完成后，立即处理延迟删除队列，确保不在渲染过程中删除
        if (entityManager_) {
            entityManager_->processPendingDeletions();
        }
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
                entityManager_->setViewer(viewer_.get());
                
                // 连接实体管理器的信号
                connect(entityManager_, &GeoEntityManager::entitySelected, 
                        this, [this](GeoEntity* entity) {
                            selectedEntity_ = entity;
                            qDebug() << "MainWindow收到实体选择信号:" << entity->getName();
                        });
                connect(entityManager_, &GeoEntityManager::entityDeselected, 
                        this, [this]() {
                            selectedEntity_ = nullptr;
                            qDebug() << "MainWindow收到实体取消选择信号";
                        });
                connect(entityManager_, &GeoEntityManager::entityRightClicked, 
                        this, [this](GeoEntity* entity, QPoint screenPos) {
                            showEntityContextMenu(screenPos, entity);
                            qDebug() << "MainWindow收到实体右键点击信号:" << entity->getName();
                        });
                
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
                
                // 设置GLWidget的实体管理器
                // 这样GLWidget在处理鼠标事件时会自动通知GeoEntityManager
                gw->getGLWidget()->setEntityManager(entityManager_);
                
                qDebug() << "MapStateManager和GeoEntityManager已设置到GLWidget";
                } else {
                    qDebug() << "无法获取GLWidget，MapStateManager设置失败";
                }
                
                qDebug() << "地图状态管理器初始化完成";
            }

            // 将MapStateManager注入到实体管理器，供其读取当前range
            if (entityManager_ && mapStateManager_) {
                entityManager_->setMapStateManager(mapStateManager_);
            }

            // 订阅空白处点击，用于点标绘放置
            if (entityManager_) {
                // 点标绘：一次性
                connect(entityManager_, &GeoEntityManager::mapLeftClicked, this, [this](QPoint screenPos) {
                    if (isPlacingWaypoint_) {
                        double lon=0, lat=0, alt=0;
                        if (!screenToGeoCoordinates(screenPos, lon, lat, alt)) {
                            QMessageBox::warning(this, "点标绘", "无法将屏幕坐标转换为地理坐标。");
                            isPlacingWaypoint_ = false;
                            return;
                        }
                        auto wp = entityManager_->addStandaloneWaypoint(lon, lat, alt, pendingWaypointLabel_);
                        if (!wp) QMessageBox::warning(this, "点标绘", "创建失败。");
                        isPlacingWaypoint_ = false;
                    }

                    // 航线标绘：左键连加
                    if (isPlacingRoute_ && !currentWaypointGroupId_.isEmpty()) {
                        double lon=0, lat=0, alt=0;
                        if (!screenToGeoCoordinates(screenPos, lon, lat, alt)) return;
                        auto wp = entityManager_->addWaypointToGroup(currentWaypointGroupId_, lon, lat, alt);
                        qDebug() << "[Route] 添加航点:" << lon << lat << alt << (wp?"成功":"失败");
                    }
                });

                // 航线标绘：右键结束并生成路线
                connect(entityManager_, &GeoEntityManager::mapRightClicked, this, [this](QPoint /*screenPos*/) {
                    if (isPlacingRoute_ && !currentWaypointGroupId_.isEmpty()) {
                        qDebug() << "[Route] 右键结束，准备生成路线，groupId=" << currentWaypointGroupId_;
                        // 选择路径算法
                        QStringList items; items << "linear" << "bezier";
                        bool okSel = false;
                        QString choice = QInputDialog::getItem(this, "生成航线", "选择生成算法:", items, 0, false, &okSel);
                        if (!okSel || choice.isEmpty()) choice = "linear";

                        bool ok = entityManager_->generateRouteForGroup(currentWaypointGroupId_, choice);
                        if (!ok) {
                            QMessageBox::warning(this, "航线标绘", "生成路线失败（点数不足或错误）。");
                            qDebug() << "[Route] 生成路线失败";
                        } else {
                            statusBar()->showMessage(QString("航线生成完成（%1）").arg(choice), 2000);
                            qDebug() << "[Route] 生成路线成功";
                        }
                        isPlacingRoute_ = false;
                        currentWaypointGroupId_.clear();
                    }
                });
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
 
    // 根据模式设置不同的视点
    osgEarth::Viewpoint vp;
    if (mode == MAP_MODE_2D) {
        // 2D视角：俯仰角-89.9度，航向角-0.916737度，距离540978，经度116.347，纬度40.0438，高度-1.70909
        vp = osgEarth::Viewpoint("2D View", 116.347, 40.0438, -1.70909, -0.916737, -90, 540978.0);
        settings->setMinMaxPitch(-90.0, -89.0);            // 设置俯仰角范围
        settings->setMinMaxDistance(1000.0, 4605500.0);  // 设置缩放范围
        qDebug() << "设置2D视角：俯仰角-89.9度，航向角-0.916737度，距离540978";
    } else {
        // 3D视角：俯仰角-76.466度，航向角0度，距离1.27252e+07，经度109.257，纬度41.82，高度-38.5648
        vp = osgEarth::Viewpoint("3D View", 109.257, 41.82, -38.5648, 0.0, -76.466, 12725200.0);
        settings->setMinMaxPitch(-90, 90);            // 设置俯仰角范围
        settings->setMinMaxDistance(1000.0, 50000000.0);  // 设置缩放范围
        qDebug() << "设置3D视角：俯仰角-76.466度，航向角0度，距离12725200";
    }
    
    em->setHomeViewpoint(vp);
    
    viewer_->setCameraManipulator(em.get());
    viewer_->home();
    
    qDebug() << "操作器设置完成";
}

void MainWindow::toggle2D3DMode()
{
    if (currentMode_ == MAP_MODE_3D) {
        // 切换到2D模式
        currentMode_ = MAP_MODE_2D;
        toggleButton_->setText("切换到3D");
        setupManipulator(MAP_MODE_2D);
        qDebug() << "切换到2D模式";
    } else {
        // 切换到3D模式
        currentMode_ = MAP_MODE_3D;
        toggleButton_->setText("切换到2D");
        setupManipulator(MAP_MODE_3D);
        qDebug() << "切换到3D模式";
    }
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
    
    // 使用工具函数加载JSON配置
    QString errorMsg;
    QJsonObject config = GeoUtils::loadJsonFile(configPath, &errorMsg);
    
    if (config.isEmpty()) {
        qDebug() << "实体配置加载失败:" << errorMsg;
        return;
    }
    
    if (entityManager_) {
        entityManager_->setEntityConfig(config);
        qDebug() << "实体配置加载完成";
    }
}

bool MainWindow::screenToGeoCoordinates(QPoint screenPos, double& longitude, double& latitude, double& altitude)
{
    // 注意：screenPos 必须是相对于 OSG GLWidget 的本地坐标，不是 MainWindow 坐标
    // 如果是 MainWindow 坐标，需要先转换为 GLWidget 坐标：glWidget->mapFrom(this, windowPos)
    
    return GeoUtils::screenToGeoCoordinates(viewer_.get(), mapNode_.get(), screenPos, longitude, latitude, altitude);
}

void MainWindow::showEntityContextMenu(QPoint screenPos, GeoEntity* entity)
{
    if (!entity) return;
    
    qDebug() << "显示实体上下文菜单:" << entity->getName() << "位置:" << screenPos;
    
    // 创建或更新上下文菜单
    if (!entityContextMenu_) {
        entityContextMenu_ = new QMenu(this);
        
        // 设置Heading动作
        QAction* setHeadingAction = entityContextMenu_->addAction("设置航向角");
        connect(setHeadingAction, &QAction::triggered, [this]() {
            if (!selectedEntity_) return;
            bool ok;
            double currentHeading = selectedEntity_->getHeading();
            double newHeading = QInputDialog::getDouble(this, "设置航向角", 
                QString("当前航向角: %1°\n请输入新的航向角:").arg(currentHeading), 
                currentHeading, -360.0, 360.0, 1, &ok);
            if (ok) {
                selectedEntity_->setHeading(newHeading);
                qDebug() << "设置实体航向角:" << selectedEntity_->getName() << "->" << newHeading << "度";
            }
        });
        
        // 设置高度动作
        QAction* setAltitudeAction = entityContextMenu_->addAction("设置高度");
        connect(setAltitudeAction, &QAction::triggered, [this]() {
            if (!selectedEntity_) return;
            double longitude, latitude, altitude;
            selectedEntity_->getPosition(longitude, latitude, altitude);
            
            bool ok;
            double newAltitude = QInputDialog::getDouble(this, "设置高度", 
                QString("当前高度: %1米\n请输入新的高度:").arg(altitude), 
                altitude, 0.0, 1000000.0, 1000, &ok);
            if (ok) {
                selectedEntity_->setPosition(longitude, latitude, newAltitude);
                qDebug() << "设置实体高度:" << selectedEntity_->getName() << "->" << newAltitude << "米";
            }
        });
        
        // 删除实体动作
        QAction* deleteAction = entityContextMenu_->addAction("删除实体");
        connect(deleteAction, &QAction::triggered, [this]() {
            if (!selectedEntity_) return;
            
            // 关键修复：先保存ID和名称，避免在对话框显示后访问已删除的实体
            QString entityId = selectedEntity_->getId();
            QString entityName = selectedEntity_->getName();
            
            QMessageBox::StandardButton reply = QMessageBox::question(this, "确认删除", 
                QString("确定要删除实体 '%1' 吗？").arg(entityName),
                QMessageBox::Yes | QMessageBox::No);
            if (reply == QMessageBox::Yes) {
                // 关键修复：先清除MainWindow中的引用，避免在删除过程中访问已删除的实体
                selectedEntity_ = nullptr;
                
                // 然后删除实体（removeEntity内部会处理GeoEntityManager中的selectedEntity_）
                entityManager_->removeEntity(entityId);
                qDebug() << "删除实体:" << entityId;
            }
        });
        
        // 显示属性动作
        QAction* showPropertiesAction = entityContextMenu_->addAction("显示属性");
        connect(showPropertiesAction, &QAction::triggered, [this]() {
            if (!selectedEntity_) return;
            QString info = QString("实体信息:\n")
                .append(QString("名称: %1\n").arg(selectedEntity_->getName()))
                .append(QString("类型: %1\n").arg(selectedEntity_->getType()))
                .append(QString("ID: %1\n").arg(selectedEntity_->getId()));
            
            double longitude, latitude, altitude;
            selectedEntity_->getPosition(longitude, latitude, altitude);
            info.append(QString("位置: 经度%1°, 纬度%2°, 高度%3米\n")
                .arg(longitude, 0, 'f', 6)
                .arg(latitude, 0, 'f', 6)
                .arg(altitude, 0, 'f', 2));
            
            info.append(QString("航向角: %1°\n").arg(selectedEntity_->getHeading()));
            info.append(QString("可见性: %1\n").arg(selectedEntity_->isVisible() ? "是" : "否"));
            info.append(QString("选中状态: %1").arg(selectedEntity_->isSelected() ? "是" : "否"));
            
            QMessageBox::information(this, "实体属性", info);
        });

      
    }
    
    // 显示菜单
    qDebug() << "执行上下文菜单显示";
    entityContextMenu_->exec(mapToGlobal(screenPos));
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
    
    // 获取 OSG GLWidget 用于坐标转换
    osgQt::GraphicsWindowQt* gw = dynamic_cast<osgQt::GraphicsWindowQt*>(
        viewer_->getCamera()->getGraphicsContext());
    if (!gw || !gw->getGLWidget()) {
        qDebug() << "无法获取 GLWidget，拖拽操作失败";
        event->ignore();
        return;
    }
    
    QWidget* glWidget = gw->getGLWidget();
    
    // 将 MainWindow 坐标转换为 GLWidget 的本地坐标
    // event->pos() 是相对于 MainWindow 的坐标，需要转换为 GLWidget 坐标
    QPoint dropPos = event->pos();
    QPoint glWidgetPos = glWidget->mapFrom(this, dropPos);
    
    // 检查坐标是否在 GLWidget 范围内
    if (!glWidget->rect().contains(glWidgetPos)) {
        qDebug() << "拖拽位置不在 GLWidget 范围内:" << dropPos << "->" << glWidgetPos;
        event->ignore();
        return;
    }
    
    double longitude, latitude, altitude;
    
    // 使用 GLWidget 的本地坐标进行地理坐标转换
    if (!screenToGeoCoordinates(glWidgetPos, longitude, latitude, altitude)) {
        qDebug() << "无法将拖拽位置转换为地理坐标，使用默认位置";
        // 如果转换失败，使用默认位置
        longitude = 116.4;
        latitude = 39.9;
        altitude = 100000.0;
    }
    
    // 设置实体高度为合理值
    altitude = 100000.0;
    
    if (entityManager_->addEntityFromDrag(text, longitude, latitude, altitude)) {
        
        // 调整相机到实体位置，使用鼠标实际位置
        osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer_.get());
        if (em) {
            // 使用鼠标实际位置作为相机视点
            osgEarth::Viewpoint vp("Entity", longitude, latitude, 0.0, 0.0, -90.0, 1000000.0);
            em->setViewpoint(vp, 2.0);
            qDebug() << "相机已调整到实体位置:" << longitude << latitude;
        }
        
        qDebug() << "实体添加完成，位置:" << longitude << latitude << altitude;
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

 

void MainWindow::onMapStateChanged(const MapStateInfo& state)
{
    auto tuple = state.getTuple();
    // qDebug() << "=== 地图状态变化 ===";
    // qDebug() << "9元组信息 (a,b,c,x1,y1,z1,x2,y2,z2):";
    // qDebug() << "a (Pitch俯仰角):" << std::get<0>(tuple);
    // qDebug() << "b (Heading航向角):" << std::get<1>(tuple);
    // qDebug() << "c (Range距离):" << std::get<2>(tuple);
    // qDebug() << "x1 (视角经度):" << std::get<3>(tuple);
    // qDebug() << "y1 (视角纬度):" << std::get<4>(tuple);
    // qDebug() << "z1 (视角高度):" << std::get<5>(tuple);
    // qDebug() << "x2 (鼠标经度):" << std::get<6>(tuple);
    // qDebug() << "y2 (鼠标纬度):" << std::get<7>(tuple);
    // qDebug() << "z2 (鼠标高度):" << std::get<8>(tuple);
}

void MainWindow::onMousePositionChanged(double longitude, double latitude, double altitude)
{
    // qDebug() << "鼠标位置变化 - 经度:" << longitude << "纬度:" << latitude << "高度:" << altitude;
}

void MainWindow::onViewPositionChanged(double longitude, double latitude, double altitude)
{
    // qDebug() << "视角位置变化 - 经度:" << longitude << "纬度:" << latitude << "高度:" << altitude;
}

