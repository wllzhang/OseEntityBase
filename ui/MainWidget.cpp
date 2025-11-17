/**
 * @file MainWidget.cpp
 * @brief 主窗口组件实现文件
 * 
 * 实现MainWidget类的所有功能，包括UI创建、事件处理、方案文件管理等
 */

#include "MainWidget.h"
#include "../widgets/OsgMapWidget.h"
#include "ModelDeployDialog.h"
#include "EntityPropertyDialog.h"
#include "EntityManagementDialog.h"
#include "LocationJumpDialog.h"
#include "../plan/planfilemanager.h"
#include "../geo/geoutils.h"
#include <QLabel>
#include <qt_windows.h>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpacerItem>
#include <QStyle>
#include <QDebug>
#include <QToolButton>
#include <QFile>
#include <QApplication>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QtMath>
#include <QMetaObject>
#include <QProgressDialog>
#include <QEventLoop>
#include <QPushButton>
#include <QMap>
#include <QVector>
#include <QMetaObject>

#include "../geo/WeaponMountDialog.h"
#include "../geo/geoentitymanager.h"
#include "../geo/geoutils.h"
#include "../geo/mapstatemanager.h"
#include "../geo/navigationhistory.h"
#include "../geo/waypointentity.h"
#include "../widgets/MapInfoOverlay.h"
#include "../util/AfsimScriptGenerator.h"

#include "BehaviorPlanningDialog.h"


MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent), currentNavIndex(0)
    , is3DMode_(true)
    , isPlacingWaypoint_(false)
    , isPlacingRoute_(false)
    , isPlanningEntityRoute_(false)
    , componentConfigDialog_(nullptr)
    , modelAssemblyDialog_(nullptr)
    , modelDeployDialog_(nullptr)
    , entityManagementDialog_(nullptr)
    , behaviorDialog_(nullptr)
    , planFileManager_(nullptr)
{
    //设置窗口属性
    setWindowTitle("任务规划");
    setMinimumSize(1200, 800);

    loadStyleSheet();

    // 创建方案文件管理器
    planFileManager_ = new PlanFileManager(nullptr, this);  // entityManager将在onMapLoaded中设置
    
    // 禁用自动保存，只保留未保存提示
    planFileManager_->setAutoSaveEnabled(false);
    dialogHoverEntity_ = nullptr;
    
    // 加载最近打开的文件列表
    loadRecentFiles();

    // 创建主垂直布局
    mainVLayout = new QVBoxLayout(this);
    mainVLayout->setSpacing(0);
    mainVLayout->setContentsMargins(0,0,0,0);

    // 创建各个部分
    createToolBar();
    createNavigation();
    createSubNavigation();
    createMapArea();

    // 连接地图加载完成信号
    connect(osgMapWidget_, &OsgMapWidget::mapLoaded, this, &MainWidget::onMapLoaded);

    // 初始化显示
    updateSubNavigation(0);
    planBtn->setChecked(true);
}

MainWidget::~MainWidget()
{
    // 清理对话框
    if (componentConfigDialog_) {
        delete componentConfigDialog_;
    }
    if (modelAssemblyDialog_) {
        delete modelAssemblyDialog_;
    }
    if (modelDeployDialog_) {
        delete modelDeployDialog_;
    }
    if (entityManagementDialog_) {
        delete entityManagementDialog_;
    }
    // planFileManager_由Qt父对象管理，自动删除
}



void MainWidget::createToolBar()
{
    // 创建工具栏
    toolBarWidget = new QWidget;
    toolBarWidget->setFixedHeight(50);
    toolBarWidget->setObjectName("toolBarWidget"); // 设置对象名用于样式选

    // 设置渐变背景
    toolBarWidget->setStyleSheet(
        "QWidget#toolBarWidget {"
        "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
        "   stop:0 #87CEFA, stop:1 #6495ED);"
        "   border: none;"
        "   border-bottom: 1px solid #2C3E50;"
        "}"
    );

    toolBarLayout = new QHBoxLayout(toolBarWidget);
    toolBarLayout->setContentsMargins(15,5,15,5);

    // 添加工具栏按钮
    QPushButton *newPlanBtn = new QPushButton("新建方案");
    newPlanBtn->setStyleSheet("border: none;");
    connect(newPlanBtn, &QPushButton::clicked, this, &MainWidget::onNewPlanClicked);
    
    QPushButton *openPlanBtn = new QPushButton("打开方案");
    openPlanBtn->setStyleSheet("border: none;");
    connect(openPlanBtn, &QPushButton::clicked, this, &MainWidget::onOpenPlanClicked);
    
    QPushButton *savePlanBtn = new QPushButton("保存方案");
    savePlanBtn->setStyleSheet("border: none;");
    connect(savePlanBtn, &QPushButton::clicked, this, &MainWidget::onSavePlanClicked);
    
    QPushButton *savePlanAsBtn = new QPushButton("另存为");
    savePlanAsBtn->setStyleSheet("border: none;");
    connect(savePlanAsBtn, &QPushButton::clicked, this, &MainWidget::onSavePlanAsClicked);
    
    // 前进后退按钮
    returnBtn_ = new QPushButton("← 后退");
    returnBtn_->setStyleSheet("border: none;");
    returnBtn_->setEnabled(false);
    forwardBtn_ = new QPushButton("前进 →");
    forwardBtn_->setStyleSheet("border: none;");
    forwardBtn_->setEnabled(false);
    
    QPushButton *helpBtn = new QPushButton("帮助");
    helpBtn->setStyleSheet("border: none;");

    // 当前方案标签
    planNameLabel_ = new QLabel("当前方案: 未打开");
    planNameLabel_->setStyleSheet("color: white; font-weight: bold; padding: 0 10px;");

    toolBarLayout->addWidget(newPlanBtn);
    toolBarLayout->addWidget(openPlanBtn);
    toolBarLayout->addWidget(savePlanBtn);
    toolBarLayout->addWidget(savePlanAsBtn);
    toolBarLayout->addWidget(returnBtn_);
    toolBarLayout->addWidget(forwardBtn_);
    toolBarLayout->addWidget(helpBtn);
    
    // 添加分隔符
    toolBarLayout->addSpacing(10);
    toolBarLayout->addWidget(planNameLabel_);
    
    // 添加定位跳转按钮
    QPushButton *locationJumpBtn = new QPushButton("定位跳转");
    locationJumpBtn->setStyleSheet("border: none;");
    connect(locationJumpBtn, &QPushButton::clicked, this, &MainWidget::onLocationJumpClicked);
    toolBarLayout->addWidget(locationJumpBtn);
    
    // 添加2D/3D切换按钮
    toggle2D3DBtn_ = new QPushButton("切换到2D");
    toggle2D3DBtn_->setStyleSheet("border: none;");
    connect(toggle2D3DBtn_, &QPushButton::clicked, this, &MainWidget::onToggle2D3D);
    toolBarLayout->addWidget(toggle2D3DBtn_);

    // 添加弹簧
    toolBarLayout->addStretch();

    QWidget *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    spacer->setStyleSheet("border: none;");

    toolBarLayout->addWidget(spacer);

    QPushButton *setBtn = new QPushButton("设置");
    setBtn->setStyleSheet("border: none;");

    toolBarLayout->addWidget(setBtn);

    // 用户信息
    QLabel *userLabel = new QLabel("当前用户: Admin");
    userLabel->setStyleSheet("border: none;");
    toolBarLayout->addWidget(userLabel);

    mainVLayout->addWidget(toolBarWidget);
}


void MainWidget::createNavigation()
{
    // 创建内容水平布局
    contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(0);// 设置按钮间距为20像素
    contentLayout->setContentsMargins(0,0,0,0);

    // 创建主导航栏
    navWidget = new QWidget;
    navWidget->setFixedWidth(120);
    navWidget->setObjectName("navWidget"); // 设置对象名用于样式选
    // 设置背景颜色为蓝色
    navWidget->setStyleSheet(
        "QWidget#navWidget {"
        "background-color: #6495ED;"
        "}"
    );

    navLayout = new QVBoxLayout(navWidget);
    navLayout->setSpacing(0);
    navLayout->setContentsMargins(0, 0, 0, 0);

    // 创建导航按钮
    // 1、方案规划按钮
    planBtn = new QToolButton(this);
    planBtn->setText("方案规划");
   planBtn->setIcon(QIcon(":/images/方案规划.png"));
    planBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    planBtn->setFixedSize(120, 120);
    planBtn->setObjectName("navToolButton");
    planBtn->setIconSize(QSize(64, 64));  // 图标大小

    // 2、资源管理按钮
    resourceBtn = new QToolButton(this);
    resourceBtn->setText("资源管理");
   resourceBtn->setIcon(QIcon(":/images/资源管理.png"));
    resourceBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    resourceBtn->setFixedSize(120, 120);
    resourceBtn->setObjectName("navToolButton");
    resourceBtn->setIconSize(QSize(64, 64));  // 图标大小

    // 3、地图服务按钮
    mapBtn = new QToolButton(this);
    mapBtn->setText("地图服务");
   mapBtn->setIcon(QIcon(":/images/地图.png"));
    mapBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    mapBtn->setFixedSize(120, 120);
    mapBtn->setObjectName("navToolButton");
    mapBtn->setIconSize(QSize(64, 64));  // 图标大小

    // 4、态势标绘按钮
    situationBtn = new QToolButton(this);
    situationBtn->setText("态势标绘");
   situationBtn->setIcon(QIcon(":/images/标绘.png"));
    situationBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    situationBtn->setFixedSize(120, 120);
    situationBtn->setObjectName("navToolButton");
    situationBtn->setIconSize(QSize(64, 64));  // 图标大小

    // 设置为可选中状态
    planBtn->setCheckable(true);
    resourceBtn->setCheckable(true);
    mapBtn->setCheckable(true);
    situationBtn->setCheckable(true);

    // 添加到布局
    navLayout->addWidget(planBtn);
    navLayout->addWidget(resourceBtn);
    navLayout->addWidget(mapBtn);
    navLayout->addWidget(situationBtn);

    // 添加弹簧
    navLayout->addStretch();

    // 连接信号槽
    connect(planBtn, &QPushButton::clicked, this, [this]() { onNavButtonClicked(); });
    connect(resourceBtn, &QPushButton::clicked, this, [this]() { onNavButtonClicked(); });
    connect(mapBtn, &QPushButton::clicked, this, [this]() { onNavButtonClicked(); });
    connect(situationBtn, &QPushButton::clicked, this, [this]() { onNavButtonClicked(); });

    // 添加到主布局
    contentLayout->addWidget(navWidget);
}

void MainWidget::createSubNavigation()
{
    // 创建子导航栏
    subNavWidget = new QWidget;
    subNavWidget->setFixedWidth(120);
    subNavWidget->setObjectName("subNavWidget"); // 设置对象名用于样式选
    // 设置背景颜色为蓝色
    subNavWidget->setStyleSheet(
        "QWidget#subNavWidget {"
        "background-color: #6495ED;"
        "}"
    );

//    subNavWidget->setStyleSheet(
//        "QPushButton {"
//        "    height: 35px;"
//        "    text-align: left;"
//        "    padding-left: 15px;"
//        "    border: none;"
//        "    background-color: #f8f8f8;"
//        "}"
//        "QPushButton:hover {"
//        "    background-color: #e8e8e8;"
//        "}"
//        "QPushButton:pressed {"
//        "    background-color: #d8d8d8;"
//        "}"
//    );

    subNavLayout = new QVBoxLayout(subNavWidget);
    subNavLayout->setSpacing(0);
    subNavLayout->setContentsMargins(0, 0, 0, 0);

    // 创建堆叠窗口用于切换子导航
    subNavStack = new QStackedWidget;
    subNavLayout->addWidget(subNavStack);

    // 1、方案规划子导航
    planSubNav = new QWidget;
    QVBoxLayout *planLayout = new QVBoxLayout(planSubNav);
    planLayout->setSpacing(1);
    planLayout->setContentsMargins(0, 0, 0, 0);

    QToolButton *modelDeployBtn = new QToolButton(this);
    modelDeployBtn->setText("模型部署");
    modelDeployBtn->setIcon(QIcon(":/images/模型部署.png"));
    modelDeployBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    modelDeployBtn->setFixedSize(120, 120);
    modelDeployBtn->setObjectName("navToolButton");
    modelDeployBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *entityManageBtn = new QToolButton(this);
    entityManageBtn->setText("实体管理");
    entityManageBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    entityManageBtn->setFixedSize(120, 120);
    entityManageBtn->setObjectName("navToolButton");
    entityManageBtn->setIcon(QIcon(QStringLiteral(":/images/entity_management.png")));
    entityManageBtn->setIconSize(QSize(64, 64));

    QToolButton *targetMatchBtn = new QToolButton(this);
    targetMatchBtn->setText("行为规划");
    targetMatchBtn->setIcon(QIcon(":/images/行为规划.png"));
    targetMatchBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    targetMatchBtn->setFixedSize(120, 120);
    targetMatchBtn->setObjectName("navToolButton");
    targetMatchBtn->setIconSize(QSize(64, 64));  // 图标大小
    connect(targetMatchBtn, &QToolButton::clicked, this, &MainWidget::onBehaviorPlanningClicked);

    QToolButton *exportPlanBtn = new QToolButton(this);
    exportPlanBtn->setText("导出方案");
    exportPlanBtn->setIcon(QIcon(":/images/导出方案.png"));
    exportPlanBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    exportPlanBtn->setFixedSize(120, 120);
    exportPlanBtn->setObjectName("navToolButton");
    exportPlanBtn->setIconSize(QSize(64, 64));  // 图标大小
    connect(exportPlanBtn, &QToolButton::clicked, this, &MainWidget::onExportPlanClicked);


    planLayout->addWidget(modelDeployBtn);
    planLayout->addWidget(entityManageBtn);
    planLayout->addWidget(targetMatchBtn);
    planLayout->addWidget(exportPlanBtn);
    planLayout->addStretch();

    // 2、资源管理子导航
    resourceSubNav = new QWidget;
    QVBoxLayout *resourceLayout = new QVBoxLayout(resourceSubNav);
    resourceLayout->setSpacing(1);
    resourceLayout->setContentsMargins(0, 0, 0, 0);

    QToolButton *modelComponentBtn = new QToolButton(this);
    modelComponentBtn->setText("模型组件");
    modelComponentBtn->setIcon(QIcon(":/images/模型组件.png"));
    modelComponentBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    modelComponentBtn->setFixedSize(120, 120);
    modelComponentBtn->setObjectName("navToolButton");
    modelComponentBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *modelAssemblyBtn = new QToolButton(this);
    modelAssemblyBtn->setText("模型组装");
    modelAssemblyBtn->setIcon(QIcon(":/images/模型组装.png"));
    modelAssemblyBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    modelAssemblyBtn->setFixedSize(120, 120);
    modelAssemblyBtn->setObjectName("navToolButton");
    modelAssemblyBtn->setIconSize(QSize(64, 64));  // 图标大小

    resourceLayout->addWidget(modelComponentBtn);
    resourceLayout->addWidget(modelAssemblyBtn);
    resourceLayout->addStretch();

    // 3、地图服务子导航
    mapSubNav = new QWidget;
    QVBoxLayout *mapLayout = new QVBoxLayout(mapSubNav);
    mapLayout->setSpacing(1);
    mapLayout->setContentsMargins(0, 0, 0, 0);

    QToolButton *distanceBtn = new QToolButton(this);
    distanceBtn->setText("距离测算");
   distanceBtn->setIcon(QIcon(":/images/距离测量.png"));
    distanceBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    distanceBtn->setFixedSize(120, 120);
    distanceBtn->setObjectName("navToolButton");
    distanceBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *areaBtn = new QToolButton(this);
    areaBtn->setText("面积测算");
   areaBtn->setIcon(QIcon(":/images/面积测算.png"));
    areaBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    areaBtn->setFixedSize(120, 120);
    areaBtn->setObjectName("navToolButton");
    areaBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *angleBtn = new QToolButton(this);
    angleBtn->setText("角度测算");
   angleBtn->setIcon(QIcon(":/images/角度测算.png"));
    angleBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    angleBtn->setFixedSize(120, 120);
    angleBtn->setObjectName("navToolButton");
    angleBtn->setIconSize(QSize(64, 64));  // 图标大小

    mapLayout->addWidget(distanceBtn);
    mapLayout->addWidget(areaBtn);
    mapLayout->addWidget(angleBtn);
    mapLayout->addStretch();

    // 4、态势标绘子导航
    situationSubNav = new QWidget;
    QVBoxLayout *situationLayout = new QVBoxLayout(situationSubNav);
    situationLayout->setSpacing(1);
    situationLayout->setContentsMargins(0, 0, 0, 0);

    QToolButton *pointBtn = new QToolButton(this);
    pointBtn->setText("点");
   pointBtn->setIcon(QIcon(":/images/点.png"));
    pointBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    pointBtn->setFixedSize(120, 120);
    pointBtn->setObjectName("navToolButton");
    pointBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *lineBtn = new QToolButton(this);
    lineBtn->setText("直线");
   lineBtn->setIcon(QIcon(":/images/线.png"));
    lineBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    lineBtn->setFixedSize(120, 120);
    lineBtn->setObjectName("navToolButton");
    lineBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *curveBtn = new QToolButton(this);
    curveBtn->setText("曲线");
   curveBtn->setIcon(QIcon(":/images/曲线.png"));
    curveBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    curveBtn->setFixedSize(120, 120);
    curveBtn->setObjectName("navToolButton");
    curveBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *rectBtn = new QToolButton(this);
    rectBtn->setText("矩形");
   rectBtn->setIcon(QIcon(":/images/画多边形.png"));
    rectBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    rectBtn->setFixedSize(120, 120);
    rectBtn->setObjectName("navToolButton");
    rectBtn->setIconSize(QSize(64, 64));  // 图标大小


    situationLayout->addWidget(pointBtn);
    situationLayout->addWidget(lineBtn);
    situationLayout->addWidget(curveBtn);
    situationLayout->addWidget(rectBtn);
    situationLayout->addStretch();

    // 添加到堆叠窗口
    subNavStack->addWidget(planSubNav);
    subNavStack->addWidget(resourceSubNav);
    subNavStack->addWidget(mapSubNav);
    subNavStack->addWidget(situationSubNav);

    // 连接子导航按钮信号
    // 组件参数配置
    connect(modelComponentBtn, &QPushButton::clicked, this, &MainWidget::onModelComponentBtnClicked);
    // 模型组装
    connect(modelAssemblyBtn, &QPushButton::clicked, this, &MainWidget::onModelAssemblyBtnClicked);
    // 模型部署
    connect(modelDeployBtn, &QPushButton::clicked, this, &MainWidget::onModelDeployBtnClicked);
    // 距离测算
    connect(distanceBtn, &QPushButton::clicked, this, &MainWidget::onDistanceMeasureClicked);
    // 面积测算
    connect(areaBtn, &QPushButton::clicked, this, &MainWidget::onAreaMeasureClicked);
    // 角度测算
    connect(angleBtn, &QPushButton::clicked, this, &MainWidget::onAngleMeasureClicked);

    // 点标绘按钮
    connect(pointBtn, &QPushButton::clicked, this, [this]() {
        if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) {
            qDebug() << "EntityManager未初始化";
            return;
        }
        bool ok=false;
        QString label = QInputDialog::getText(this, "点标绘", "请输入标签，然后在地图上点击位置放置:", QLineEdit::Normal, "标注", &ok);
        if (!ok) return;
        if (label.trimmed().isEmpty()) label = "标注";
        pendingWaypointLabel_ = label.trimmed();
        isPlacingWaypoint_ = true;
        qDebug() << "点标绘模式已启动，请在地图上点击放置点";
    });
    
    // 直线标绘按钮
    connect(lineBtn, &QPushButton::clicked, this, &MainWidget::onLineDrawClicked);

    // 航迹管理按钮（航线标绘）
    connect(entityManageBtn, &QPushButton::clicked, this, [this]() {
        showEntityManagementDialog();
    });
    
    // ... 连接其他子导航按钮 待补充

    // 添加到主布局
    contentLayout->addWidget(subNavWidget);
    mainVLayout->addLayout(contentLayout, 1);
}

void MainWidget::onDistanceMeasureClicked()
{
    if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) {
        QMessageBox::warning(this, "距离测算", "地图或实体管理器未初始化");
        return;
    }

    resetMeasurementModes();

    auto entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        QMessageBox::warning(this, "距离测算", "实体管理器未初始化");
        return;
    }

    isMeasuringDistance_ = true;
    distancePointA_ = nullptr;
    distancePointB_ = nullptr;
    entityManager->setBlockMapNavigation(true);

    QMessageBox::information(this, "距离测算", "请依次点击两个标注点进行测距，右键可取消。");

    distanceLeftClickConn_ = connect(entityManager, &GeoEntityManager::mapLeftClicked, this, [this](QPoint screenPos) {
        if (!isMeasuringDistance_) {
            return;
        }

        WaypointEntity* wp = pickWaypointNearScreenPos(screenPos, 20);
        if (!wp) {
            exitDistanceMeasure("附近未找到标注点，距离测算已退出。");
            return;
        }

        if (!distancePointA_) {
            distancePointA_ = wp;
            return;
        }

        if (wp == distancePointA_) {
            exitDistanceMeasure("请选择两个不同的标注点，距离测算已退出。");
            return;
        }

        distancePointB_ = wp;

        double lon1 = 0.0, lat1 = 0.0, alt1 = 0.0;
        double lon2 = 0.0, lat2 = 0.0, alt2 = 0.0;
        distancePointA_->getPosition(lon1, lat1, alt1);
        distancePointB_->getPosition(lon2, lat2, alt2);
        double meters = computeDistanceMeters(lat1, lon1, lat2, lon2);
        double km = meters / 1000.0;

        QMessageBox::information(
            this,
            "距离测算",
            QString("两点间距离：%1 米（%2 公里）")
                .arg(QString::number(meters, 'f', 1))
                .arg(QString::number(km, 'f', 3)));

        exitDistanceMeasure();
    });

    distanceRightClickConn_ = connect(entityManager, &GeoEntityManager::mapRightClicked, this, [this](QPoint) {
        if (!isMeasuringDistance_) {
            return;
        }
        exitDistanceMeasure("已通过右键退出距离测算。");
    });
}


void MainWidget::onAreaMeasureClicked()
{
    if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) {
        QMessageBox::warning(this, "面积测算", "地图或实体管理器未初始化");
        return;
    }

    resetMeasurementModes();

    auto entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        QMessageBox::warning(this, "面积测算", "实体管理器未初始化");
        return;
    }

    isMeasuringArea_ = true;
    areaMeasurePoints_.clear();
    entityManager->setBlockMapNavigation(true);

    QMessageBox::information(
        this,
        "面积测算",
        "请依次点击三个及以上的标注点组成多边形，再次单击首个点闭合，右键可取消。");

    auto finalizeArea = [this]() {
        const double areaMeters = computePolygonAreaMeters(areaMeasurePoints_);
        if (areaMeters <= 0.0) {
            exitAreaMeasure("所选点无法组成有效的封闭多边形（面积为0），面积测算已退出。");
            return;
        }

        const double areaKm2 = areaMeters / 1'000'000.0;
        QMessageBox::information(
            this,
            "面积测算",
            QString("多边形面积：%1 平方米（%2 平方公里）")
                .arg(QString::number(areaMeters, 'f', 2))
                .arg(QString::number(areaKm2, 'f', 6)));

        exitAreaMeasure();
    };

    areaLeftClickConn_ = connect(entityManager, &GeoEntityManager::mapLeftClicked, this, [this, finalizeArea](QPoint screenPos) {
        if (!isMeasuringArea_) {
            return;
        }

        WaypointEntity* wp = pickWaypointNearScreenPos(screenPos, 20);
        if (!wp) {
            exitAreaMeasure("附近未找到标注点，面积测算已退出。");
            return;
        }

        if (areaMeasurePoints_.isEmpty()) {
            areaMeasurePoints_.append(wp);
            return;
        }

        if (wp == areaMeasurePoints_.first()) {
            if (areaMeasurePoints_.size() < 3) {
                exitAreaMeasure("至少需要三个不同的标注点才能计算面积，面积测算已退出。");
                return;
            }
            finalizeArea();
            return;
        }

        if (areaMeasurePoints_.contains(wp)) {
            QMessageBox::information(this, "面积测算", "该标注点已选择，请选择其他点或单击首点完成测算。");
            return;
        }

        areaMeasurePoints_.append(wp);
    });

    areaRightClickConn_ = connect(entityManager, &GeoEntityManager::mapRightClicked, this, [this](QPoint) {
        if (!isMeasuringArea_) {
            return;
        }
        exitAreaMeasure("已通过右键退出面积测算。");
    });
}

void MainWidget::onLineDrawClicked()
{
    if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) {
        QMessageBox::warning(this, "直线标绘", "地图或实体管理器未初始化");
        return;
    }

    resetMeasurementModes();
    exitLineDrawing();

    isDrawingLine_ = true;
    hasPendingLineStart_ = false;

    QMessageBox::information(this, "直线标绘", "请在地图上依次左键点击两个位置绘制直线，右键取消。");
}


void MainWidget::onAngleMeasureClicked()
{
    if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) {
        QMessageBox::warning(this, "角度测算", "地图或实体管理器未初始化");
        return;
    }

    resetMeasurementModes();

    auto entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        QMessageBox::warning(this, "角度测算", "实体管理器未初始化");
        return;
    }

    isMeasuringAngle_ = true;
    angleBasePoint_ = nullptr;
    angleTargetPoint_ = nullptr;
    entityManager->setBlockMapNavigation(true);

    QMessageBox::information(
        this,
        "角度测算",
        "请依次点击两个标注点（第一个为基准点，第二个为目标点），右键可取消。");

    angleLeftClickConn_ = connect(entityManager, &GeoEntityManager::mapLeftClicked, this, [this](QPoint screenPos) {
        if (!isMeasuringAngle_) {
            return;
        }

        WaypointEntity* wp = pickWaypointNearScreenPos(screenPos, 12);
        if (!wp) {
            exitAngleMeasure("附近未找到标注点，角度测算已退出。");
            return;
        }

        if (!angleBasePoint_) {
            angleBasePoint_ = wp;
            return;
        }

        if (wp == angleBasePoint_) {
            exitAngleMeasure("请选择两个不同的标注点，角度测算已退出。");
            return;
        }

        angleTargetPoint_ = wp;
        showAngleBetweenWaypoints(angleBasePoint_, angleTargetPoint_);
        exitAngleMeasure();
    });

    angleRightClickConn_ = connect(entityManager, &GeoEntityManager::mapRightClicked, this, [this](QPoint) {
        if (!isMeasuringAngle_) {
            return;
        }
        exitAngleMeasure("已通过右键退出角度测算。");
    });
}



double MainWidget::computeDistanceMeters(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg) const
{
    // Haversine 公式：基于球面近似的大圆距离，返回单位：米
    static const double R = 6371000.0; // 地球半径（米）
    auto deg2rad = [](double d){ return d * M_PI / 180.0; };
    double lat1 = deg2rad(lat1Deg);
    double lon1 = deg2rad(lon1Deg);
    double lat2 = deg2rad(lat2Deg);
    double lon2 = deg2rad(lon2Deg);
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;
    double a = sin(dlat/2)*sin(dlat/2) + cos(lat1)*cos(lat2)*sin(dlon/2)*sin(dlon/2);
    double c = 2 * atan2(sqrt(a), sqrt(1.0 - a));
    return R * c;
}

double MainWidget::computePolygonAreaMeters(const QVector<WaypointEntity*>& points) const
{
    if (points.size() < 3) {
        return 0.0;
    }

    double refLon = 0.0, refLat = 0.0, refAlt = 0.0;
    points.first()->getPosition(refLon, refLat, refAlt);
    const double refLatRad = qDegreesToRadians(refLat);
    const double earthRadius = 6371000.0; // 米

    QVector<QPointF> planar;
    planar.reserve(points.size());
    for (WaypointEntity* wp : points) {
        if (!wp) {
            continue;
        }
        double lon = 0.0, lat = 0.0, alt = 0.0;
        wp->getPosition(lon, lat, alt);
        double deltaLonRad = qDegreesToRadians(lon - refLon);
        double deltaLatRad = qDegreesToRadians(lat - refLat);
        double x = deltaLonRad * earthRadius * qCos(refLatRad);
        double y = deltaLatRad * earthRadius;
        planar.append(QPointF(x, y));
    }

    if (planar.size() < 3) {
        return 0.0;
    }

    double area = 0.0;
    for (int i = 0, j = planar.size() - 1; i < planar.size(); j = i++) {
        area += (planar[j].x() * planar[i].y()) - (planar[i].x() * planar[j].y());
    }

    return qFabs(area) * 0.5;
}

void MainWidget::showAngleBetweenWaypoints(WaypointEntity* from, WaypointEntity* to)
{
    if (!from || !to) {
        return;
    }

    double lon1 = 0.0, lat1 = 0.0, alt1 = 0.0;
    double lon2 = 0.0, lat2 = 0.0, alt2 = 0.0;
    from->getPosition(lon1, lat1, alt1);
    to->getPosition(lon2, lat2, alt2);

    const double lat1Rad = qDegreesToRadians(lat1);
    const double lat2Rad = qDegreesToRadians(lat2);
    const double dLonRad = qDegreesToRadians(lon2 - lon1);

    const double y = qSin(dLonRad) * qCos(lat2Rad);
    const double x = qCos(lat1Rad) * qSin(lat2Rad) - qSin(lat1Rad) * qCos(lat2Rad) * qCos(dLonRad);
    double bearingDeg = qRadiansToDegrees(qAtan2(y, x));
    if (bearingDeg < 0.0) {
        bearingDeg += 360.0;
    }

    const double groundDistance = computeDistanceMeters(lat1, lon1, lat2, lon2);
    const double altitudeDiff = alt2 - alt1;
    double pitchDeg = 0.0;
    if (qFabs(groundDistance) < 1e-3) {
        pitchDeg = (altitudeDiff >= 0.0) ? 90.0 : -90.0;
    } else {
        pitchDeg = qRadiansToDegrees(qAtan2(altitudeDiff, groundDistance));
    }

    QMessageBox::information(
        this,
        "角度测算",
        QString("方位角：%1°\n俯仰角：%2°")
            .arg(QString::number(bearingDeg, 'f', 2))
            .arg(QString::number(pitchDeg, 'f', 2))
    );
}

void MainWidget::resetMeasurementModes()
{
    exitDistanceMeasure();
    exitAreaMeasure();
    exitAngleMeasure();
    exitLineDrawing();
}

void MainWidget::disconnectMeasurementConnection(QMetaObject::Connection& connection)
{
    if (connection) {  // 检查连接是否有效
        QObject::disconnect(connection);  // 断开连接
        connection = QMetaObject::Connection();  // 将连接重置为空连接
    }
}

void MainWidget::exitDistanceMeasure(const QString& message)
{
    const bool wasActive = isMeasuringDistance_ || distanceLeftClickConn_ || distanceRightClickConn_;
    if (!wasActive && message.isEmpty()) {
        return;
    }

    if (!message.isEmpty()) {
        QMessageBox::information(this, "距离测算", message);
    }

    // 重置距离测量相关的状态变量
    isMeasuringDistance_ = false;  // 清除距离测量标志
    distancePointA_ = nullptr;     // 清空测量点A
    distancePointB_ = nullptr;     // 清空测量点B

    // 断开与鼠标点击事件相关的信号槽连接
    disconnectMeasurementConnection(distanceLeftClickConn_);   // 断开左键连接
    disconnectMeasurementConnection(distanceRightClickConn_);  // 断开右键连接

    // 恢复地图导航功能（如果没有其他测量模式正在运行）
    if (osgMapWidget_) {
        auto entityManager = osgMapWidget_->getEntityManager();
        // 只有当没有面积测量和角度测量在进行时，才恢复地图导航
        if (entityManager && !isMeasuringArea_ && !isMeasuringAngle_) {
            entityManager->setBlockMapNavigation(false);  // 允许地图导航操作
        }
    }
}

void MainWidget::exitAreaMeasure(const QString& message)
{
    const bool wasActive = isMeasuringArea_ || areaLeftClickConn_ || areaRightClickConn_;
    if (!wasActive && message.isEmpty()) {
        return;
    }

    if (!message.isEmpty()) {
        QMessageBox::information(this, "面积测算", message);
    }

    isMeasuringArea_ = false;
    areaMeasurePoints_.clear();

    disconnectMeasurementConnection(areaLeftClickConn_);
    disconnectMeasurementConnection(areaRightClickConn_);

    if (osgMapWidget_) {
        auto entityManager = osgMapWidget_->getEntityManager();
        if (entityManager && !isMeasuringDistance_ && !isMeasuringAngle_) {
            entityManager->setBlockMapNavigation(false);
        }
    }
}

void MainWidget::exitAngleMeasure(const QString& message)
{
    const bool wasActive = isMeasuringAngle_ || angleLeftClickConn_ || angleRightClickConn_;
    if (!wasActive && message.isEmpty()) {
        return;
    }

    if (!message.isEmpty()) {
        QMessageBox::information(this, "角度测算", message);
    }

    isMeasuringAngle_ = false;
    angleBasePoint_ = nullptr;
    angleTargetPoint_ = nullptr;

    disconnectMeasurementConnection(angleLeftClickConn_);
    disconnectMeasurementConnection(angleRightClickConn_);

    if (osgMapWidget_) {
        auto entityManager = osgMapWidget_->getEntityManager();
        if (entityManager && !isMeasuringDistance_ && !isMeasuringArea_) {
            entityManager->setBlockMapNavigation(false);
        }
    }
}

void MainWidget::exitLineDrawing(const QString& message)
{
    const bool wasActive = isDrawingLine_ || hasPendingLineStart_;
    if (!wasActive && message.isEmpty()) {
        return;
    }

    if (!message.isEmpty()) {
        QMessageBox::information(this, "直线标绘", message);
    }

    isDrawingLine_ = false;
    hasPendingLineStart_ = false;
}

WaypointEntity* MainWidget::pickWaypointNearScreenPos(const QPoint& screenPos, int radiusPx) const
{
    if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) return nullptr;
    auto entityManager = osgMapWidget_->getEntityManager();

    // 以曼哈顿/圆形邻域在像素内搜索，优先返回第一个匹配到的航点
    for (int dy = -radiusPx; dy <= radiusPx; ++dy) {
        for (int dx = -radiusPx; dx <= radiusPx; ++dx) {
            if (dx*dx + dy*dy > radiusPx*radiusPx) continue; // 圆形范围
            QPoint p = screenPos + QPoint(dx, dy);
            GeoEntity* e = entityManager->findEntityAtPosition(p);
            if (!e) continue;
            if (auto wp = dynamic_cast<WaypointEntity*>(e)) {
                return wp;
            }
        }
    }
    return nullptr;
}



void MainWidget::createMapArea()
{
    // 创建OSG地图显示区域
    osgMapWidget_ = new OsgMapWidget(this);

    // 添加到主布局
    contentLayout->addWidget(osgMapWidget_, 1);  // 设置伸缩因子为1，占据剩余空间
}

void MainWidget::onNavButtonClicked()
{
    QToolButton *clickedBtn = qobject_cast<QToolButton*>(sender());
    if (!clickedBtn) return;

    // 取消所有按钮的选中状态
    planBtn->setChecked(false);
    resourceBtn->setChecked(false);
    mapBtn->setChecked(false);
    situationBtn->setChecked(false);

    // 设置当前按钮为选中状态
    clickedBtn->setChecked(true);

    // 更新子导航栏
    if (clickedBtn == planBtn) {
        updateSubNavigation(0);
    } else if (clickedBtn == resourceBtn) {
        updateSubNavigation(1);
    } else if (clickedBtn == mapBtn) {
        updateSubNavigation(2);
    } else if (clickedBtn == situationBtn) {
        updateSubNavigation(3);
    }
}

// 组件参数配置槽函数
void MainWidget::onModelComponentBtnClicked()
{
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (clickedBtn) {
        qDebug() << "子导航按钮点击:" << clickedBtn->text();
    }
    
    // 如果对话框不存在，创建它
    if (!componentConfigDialog_) {
        componentConfigDialog_ = new ComponentConfigDialog(this);
        // 设置对话框为非模态，不阻塞主窗口
        componentConfigDialog_->setModal(false);
        // 设置窗口标志为普通窗口，不阻塞父窗口交互
        componentConfigDialog_->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
        // 连接对话框关闭信号
        connect(componentConfigDialog_, &QDialog::finished, this, [this](int result) {
            Q_UNUSED(result);
            // 对话框关闭时保留指针，下次打开时复用
        });
    }
    
    // 如果对话框已经打开，激活它（将其置于前台）
    if (componentConfigDialog_->isVisible()) {
        componentConfigDialog_->activateWindow();
        componentConfigDialog_->raise();
    } else {
        // 显示非模态对话框，不阻塞主窗口
        componentConfigDialog_->show();
    }
}

// 模型组装槽函数
void MainWidget::onModelAssemblyBtnClicked()
{
    QPushButton *clickedBtn = qobject_cast<QPushButton*>(sender());
    if (clickedBtn) {
        qDebug() << "子导航按钮点击:" << clickedBtn->text();
    }
    
    // 如果对话框不存在，创建它
    if (!modelAssemblyDialog_) {
        modelAssemblyDialog_ = new ModelAssemblyDialog(this);
        // 设置对话框为非模态，不阻塞主窗口
        modelAssemblyDialog_->setModal(false);
        // 设置窗口标志为普通窗口，不阻塞父窗口交互
        modelAssemblyDialog_->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
        // 连接对话框关闭信号
        connect(modelAssemblyDialog_, &QDialog::finished, this, [this](int result) {
            Q_UNUSED(result);
            // 对话框关闭时保留指针，下次打开时复用
        });
    }
    
    // 如果对话框已经打开，激活它（将其置于前台）
    if (modelAssemblyDialog_->isVisible()) {
        modelAssemblyDialog_->activateWindow();
        modelAssemblyDialog_->raise();
    } else {
        // 显示非模态对话框，不阻塞主窗口
        modelAssemblyDialog_->show();
    }
}

// 模型部署槽函数
void MainWidget::onModelDeployBtnClicked()
{
    // 检查是否有打开的方案文件
    if (!planFileManager_ || planFileManager_->getCurrentPlanFile().isEmpty()) {
        // 弹出方案选择对话框（新建或打开）
        QMessageBox msgBox(this);
        msgBox.setWindowTitle("方案管理");
        msgBox.setText("请先创建或打开一个方案文件");
        msgBox.setInformativeText("选择操作：");
        QPushButton *newBtn = msgBox.addButton("新建方案", QMessageBox::ActionRole);
        QPushButton *openBtn = msgBox.addButton("打开方案", QMessageBox::ActionRole);
        QPushButton *cancelBtn = msgBox.addButton("取消", QMessageBox::RejectRole);
        msgBox.exec();
        
        if (msgBox.clickedButton() == newBtn) {
            onNewPlanClicked();
            return;  // 新建方案后，用户需要再次点击"模型部署"
        } else if (msgBox.clickedButton() == openBtn) {
            onOpenPlanClicked();
            return;  // 打开方案后，用户需要再次点击"模型部署"
        } else {
            return;  // 用户取消
        }
    }
    
    // 如果对话框不存在，创建它
    if (!modelDeployDialog_) {
        modelDeployDialog_ = new ModelDeployDialog(this);
        // 设置对话框为非模态，不阻塞主窗口
        modelDeployDialog_->setModal(false);
        // 设置窗口标志为普通窗口，不阻塞父窗口交互
        modelDeployDialog_->setWindowFlags(Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint | Qt::WindowMinMaxButtonsHint);
        // 连接对话框关闭信号
        connect(modelDeployDialog_, &QDialog::finished, this, [this](int result) {
            Q_UNUSED(result);
            // 对话框关闭时保留指针，下次打开时复用
        });
    }
    
    // 如果对话框已经打开，激活它（将其置于前台）
    if (modelDeployDialog_->isVisible()) {
        modelDeployDialog_->activateWindow();
        modelDeployDialog_->raise();
    } else {
        // 显示非模态对话框，不阻塞主窗口
        modelDeployDialog_->show();
    }
}

void MainWidget::onNewPlanClicked()
{
    bool ok;
    QString planName = QInputDialog::getText(this, "新建方案", 
                                              "请输入方案名称:", 
                                              QLineEdit::Normal, 
                                              "新方案", 
                                              &ok);
    if (!ok || planName.trimmed().isEmpty()) {
        return;
    }
    
    QString description = QInputDialog::getText(this, "新建方案", 
                                                "请输入方案描述（可选）:", 
                                                QLineEdit::Normal, 
                                                "", 
                                                &ok);
    if (!ok) {
        description = "";  // 用户取消输入描述，使用空字符串
    }
    
    if (planFileManager_) {
        if (planFileManager_->createPlan(planName.trimmed(), description.trimmed())) {
            QMessageBox::information(this, "成功", QString("方案 '%1' 创建成功").arg(planName));
            qDebug() << "方案创建成功:" << planFileManager_->getCurrentPlanFile();
            refreshEntityManagementDialog();
        } else {
            QMessageBox::warning(this, "错误", "方案创建失败");
        }
    }
}

void MainWidget::onOpenPlanClicked()
{
    QString plansDir = PlanFileManager::getPlansDirectory();
    
    // 创建菜单：显示最近打开的文件
    QMenu menu(this);
    QAction* recentAction = nullptr;
    if (!recentPlanFiles_.isEmpty()) {
        QMenu* recentMenu = menu.addMenu("最近打开");
        addRecentFileMenuItem(recentMenu);
        menu.addSeparator();
    }
    
    QAction* openAction = menu.addAction("打开文件...");
    menu.addSeparator();
    
    QAction* selectedAction = menu.exec(mapToGlobal(QPoint(0, 50)));
    
    auto loadPlanWithProgress = [this](const QString& filePath, bool& cancelledOut) -> bool {
        cancelledOut = false;
        if (!planFileManager_) {
            return false;
        }

        QProgressDialog progressDialog(QString::fromUtf8(u8"正在加载方案，请稍候..."), QString(), 0, 0, this);
        progressDialog.setWindowTitle(QString::fromUtf8(u8"加载方案"));
        QPushButton* cancelButton = new QPushButton(QString::fromUtf8(u8"取消"), &progressDialog);
        progressDialog.setCancelButton(cancelButton);
        progressDialog.setWindowModality(Qt::ApplicationModal);
        progressDialog.setMinimumDuration(0);
        progressDialog.setAutoClose(false);
        progressDialog.show();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

        bool cancelSignalReceived = false;
        bool cancelButtonPressed = false;

        QMetaObject::Connection progressConn;
        QMetaObject::Connection cancelConn;
        QMetaObject::Connection cancelButtonConn;

        if (planFileManager_) {
            progressConn = connect(planFileManager_, &PlanFileManager::loadProgress,
                                   this, [&, defaultMessage = QString::fromUtf8(u8"正在加载方案，请稍候...")](int current, int total, const QString& message) {
                                        if (cancelSignalReceived) {
                                            return;
                                        }
                                        if (total <= 0) {
                                            progressDialog.setRange(0, 0);
                                        } else {
                                            if (progressDialog.maximum() != total) {
                                                progressDialog.setRange(0, total);
                                            }
                                            progressDialog.setValue(qBound(0, current, total));
                                        }
                                        QString text = message.isEmpty() ? defaultMessage : message;
                                        progressDialog.setLabelText(text);
                                        progressDialog.repaint();
                                        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);
                                   });

            cancelConn = connect(planFileManager_, &PlanFileManager::loadCancelled,
                                  this, [&]() {
                                      cancelSignalReceived = true;
                                      progressDialog.setLabelText(QString::fromUtf8(u8"正在取消..."));
                                      progressDialog.repaint();
                                  });

            cancelButtonConn = connect(&progressDialog, &QProgressDialog::canceled, this, [&]() {
                cancelButtonPressed = true;
                progressDialog.setLabelText(QString::fromUtf8(u8"正在取消..."));
                progressDialog.repaint();
                planFileManager_->requestCancelLoad();
            });
        }

        bool result = planFileManager_->loadPlan(filePath);

        progressDialog.close();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents | QEventLoop::ExcludeSocketNotifiers);

        if (planFileManager_) {
            QObject::disconnect(progressConn);
            QObject::disconnect(cancelConn);
            QObject::disconnect(cancelButtonConn);
        }

        cancelledOut = cancelSignalReceived;

        if (cancelSignalReceived) {
            return false;
        }

        return result;
    };

    if (selectedAction == openAction) {
        // 打开文件对话框
        QString filePath = QFileDialog::getOpenFileName(this, 
                                                        "打开方案文件", 
                                                        plansDir, 
                                                        "方案文件 (*.plan.json);;所有文件 (*.*)");
        
        if (!filePath.isEmpty()) {
            bool cancelled = false;
            if (loadPlanWithProgress(filePath, cancelled)) {
                updateRecentFiles(filePath);
                
                // 恢复相机视角
                double lon, lat, alt, heading, pitch, range;
                if (planFileManager_->getCameraViewpoint(lon, lat, alt, heading, pitch, range)) {
                    if (osgMapWidget_) {
                        auto viewer = osgMapWidget_->getViewer();
                        if (viewer) {
                            // 保存当前视角到导航历史（如果存在）
                            if (osgMapWidget_->getNavigationHistory()) {
                                auto mapStateManager = osgMapWidget_->getMapStateManager();
                                if (mapStateManager) {
                                    osgEarth::Viewpoint currentVp = mapStateManager->getCurrentViewpoint("Before Load Plan");
                                    osgMapWidget_->getNavigationHistory()->pushViewpoint(currentVp);
                                }
                            }
                            
                            osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
                            if (em) {
                                osgEarth::Viewpoint vp("Plan", lon, lat, alt, heading, pitch, range);
                                em->setViewpoint(vp, 2.0);
                                qDebug() << "恢复相机视角:" << lon << lat << range;
                            }
                        }
                    }
                }
                
                // 更新工具栏的方案名称
                updatePlanNameLabel();
                refreshEntityManagementDialog();
                
                QMessageBox::information(this, "成功", QString("方案文件 '%1' 加载成功").arg(filePath));
                qDebug() << "方案加载成功:" << filePath;
            } else if (!cancelled) {
                QMessageBox::warning(this, "错误", "方案文件加载失败");
            }
        }
    } else if (selectedAction && selectedAction != recentAction) {
        // 检查是否是最近文件菜单项
        QString filePath = selectedAction->data().toString();
        if (!filePath.isEmpty() && QFile::exists(filePath)) {
            bool cancelled = false;
            if (loadPlanWithProgress(filePath, cancelled)) {
                updateRecentFiles(filePath);
                
                // 恢复相机视角
                double lon, lat, alt, heading, pitch, range;
                if (planFileManager_->getCameraViewpoint(lon, lat, alt, heading, pitch, range)) {
                    if (osgMapWidget_) {
                        auto viewer = osgMapWidget_->getViewer();
                        if (viewer) {
                            // 保存当前视角到导航历史（如果存在）
                            if (osgMapWidget_->getNavigationHistory()) {
                                auto mapStateManager = osgMapWidget_->getMapStateManager();
                                if (mapStateManager) {
                                    osgEarth::Viewpoint currentVp = mapStateManager->getCurrentViewpoint("Before Load Plan");
                                    osgMapWidget_->getNavigationHistory()->pushViewpoint(currentVp);
                                }
                            }
                            
                            osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
                            if (em) {
                                osgEarth::Viewpoint vp("Plan", lon, lat, alt, heading, pitch, range);
                                em->setViewpoint(vp, 2.0);
                                qDebug() << "恢复相机视角:" << lon << lat << range;
                            }
                        }
                    }
                }
                
                // 更新工具栏的方案名称
                updatePlanNameLabel();
                refreshEntityManagementDialog();
                
                QMessageBox::information(this, "成功", QString("方案文件 '%1' 加载成功").arg(filePath));
                qDebug() << "方案加载成功:" << filePath;
            } else if (!cancelled) {
                QMessageBox::warning(this, "错误", "方案文件加载失败");
            }
        }
    }
}

void MainWidget::onSavePlanClicked()
{
    if (!planFileManager_) {
        QMessageBox::warning(this, "错误", "方案文件管理器未初始化");
        return;
    }
    
    QString currentPlanFile = planFileManager_->getCurrentPlanFile();
    if (currentPlanFile.isEmpty()) {
        QMessageBox::warning(this, "提示", "当前没有打开的方案文件");
        return;
    }
    
    // 保存当前相机视角（mapStateManager 必然存在）
    if (osgMapWidget_) {
        auto mapStateManager = osgMapWidget_->getMapStateManager();
        if (mapStateManager) {
            const auto& state = mapStateManager->getCurrentState();
            // 验证状态值是否有效后再保存
            bool isValid = true;
            if (qIsNaN(state.viewLongitude) || qIsInf(state.viewLongitude) ||
                qIsNaN(state.viewLatitude) || qIsInf(state.viewLatitude) ||
                state.viewLongitude < -180.0 || state.viewLongitude > 180.0 ||
                state.viewLatitude < -90.0 || state.viewLatitude > 90.0) {
                isValid = false;
                qDebug() << "保存方案: 相机视角经纬度无效，跳过保存";
            }
            if (isValid) {
                planFileManager_->setCameraViewpoint(
                    state.viewLongitude,
                    state.viewLatitude,
                    state.viewAltitude,
                    state.heading,
                    state.pitch,
                    state.range
                );
            }
        }
    }
    
    if (planFileManager_->savePlan()) {
        QMessageBox::information(this, "成功", "方案保存成功");
        qDebug() << "方案保存成功:" << currentPlanFile;
    } else {
        QMessageBox::warning(this, "错误", "方案保存失败");
    }
}

void MainWidget::onSavePlanAsClicked()
{
    if (!planFileManager_) {
        QMessageBox::warning(this, "错误", "方案文件管理器未初始化");
        return;
    }
    
    QString plansDir = PlanFileManager::getPlansDirectory();
    QString filePath = QFileDialog::getSaveFileName(this, 
                                                    "另存为方案文件", 
                                                    plansDir, 
                                                    "方案文件 (*.plan.json);;所有文件 (*.*)");
    
    if (filePath.isEmpty()) {
        return;
    }
    
    // 确保文件扩展名正确
    if (!filePath.endsWith(".plan.json")) {
        filePath += ".plan.json";
    }
    
    // 保存当前相机视角（mapStateManager 必然存在）
    if (osgMapWidget_) {
        auto mapStateManager = osgMapWidget_->getMapStateManager();
        if (mapStateManager) {
            const auto& state = mapStateManager->getCurrentState();
            // 验证状态值是否有效后再保存
            bool isValid = true;
            if (qIsNaN(state.viewLongitude) || qIsInf(state.viewLongitude) ||
                qIsNaN(state.viewLatitude) || qIsInf(state.viewLatitude) ||
                state.viewLongitude < -180.0 || state.viewLongitude > 180.0 ||
                state.viewLatitude < -90.0 || state.viewLatitude > 90.0) {
                isValid = false;
                qDebug() << "保存方案: 相机视角经纬度无效，跳过保存";
            }
            if (isValid) {
                planFileManager_->setCameraViewpoint(
                    state.viewLongitude,
                    state.viewLatitude,
                    state.viewAltitude,
                    state.heading,
                    state.pitch,
                    state.range
                );
            }
        }
    }
    
    if (planFileManager_->savePlan(filePath)) {
        updateRecentFiles(filePath);
        QMessageBox::information(this, "成功", "方案保存成功");
        qDebug() << "方案另存为成功:" << filePath;
    } else {
        QMessageBox::warning(this, "错误", "方案保存失败");
    }
}


void MainWidget::onExportPlanClicked()
{
    if (!planFileManager_) {
        QMessageBox::warning(this, "错误", "方案文件管理器未初始化");
        return;
    }

    QString currentPlanFile = planFileManager_->getCurrentPlanFile();
    if (currentPlanFile.isEmpty()) {
        QMessageBox::warning(this, "错误", "当前没有打开的方案文件");
        return;
    }

    // 获取实体管理器
    GeoEntityManager* entityManager = nullptr;
    if (osgMapWidget_) {
        entityManager = osgMapWidget_->getEntityManager();
    }

    if (!entityManager) {
        QMessageBox::warning(this, "错误", "实体管理器未初始化");
        return;
    }

    // 选择保存路径
    QString defaultFileName = QFileInfo(currentPlanFile).baseName() + "_afsim.txt";
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "保存AFSIM脚本",
        defaultFileName,
        "文本文件 (*.txt);;所有文件 (*.*)"
    );

    if (filePath.isEmpty()) {
        return;  // 用户取消
    }

    // 生成脚本
    AfsimScriptGenerator generator(entityManager, planFileManager_);
    if (generator.generateScript(filePath)) {
        QMessageBox::information(this, "成功", QString("AFSIM脚本已生成:\n%1").arg(filePath));
    } else {
        QMessageBox::warning(this, "错误", "AFSIM脚本生成失败");
    }
}



void MainWidget::updateRecentFiles(const QString& filePath)
{
    // 移除重复项
    recentPlanFiles_.removeAll(filePath);
    
    // 添加到列表开头
    recentPlanFiles_.prepend(filePath);
    
    // 限制数量
    while (recentPlanFiles_.size() > MAX_RECENT_FILES) {
        recentPlanFiles_.removeLast();
    }
    
    // 保存到配置文件
    saveRecentFiles();
}

void MainWidget::loadRecentFiles()
{
    QSettings settings;
    int count = settings.beginReadArray("RecentPlanFiles");
    recentPlanFiles_.clear();
    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        QString filePath = settings.value("path").toString();
        if (QFile::exists(filePath)) {
            recentPlanFiles_.append(filePath);
        }
    }
    settings.endArray();
}

void MainWidget::saveRecentFiles()
{
    QSettings settings;
    settings.beginWriteArray("RecentPlanFiles");
    for (int i = 0; i < recentPlanFiles_.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("path", recentPlanFiles_[i]);
    }
    settings.endArray();
}

void MainWidget::addRecentFileMenuItem(QMenu* menu)
{
    for (const QString& filePath : recentPlanFiles_) {
        QFileInfo fileInfo(filePath);
        QString displayName = fileInfo.fileName();
        QAction* action = menu->addAction(displayName);
        action->setData(filePath);
        action->setToolTip(filePath);
    }
}


void MainWidget::updateSubNavigation(int navIndex)
{
    currentNavIndex = navIndex;
    subNavStack->setCurrentIndex(navIndex);
}

void MainWidget::loadStyleSheet()
{
    QFile qssFile(":/qss/myStyleSheet.qss");

    if (qssFile.open(QFile::ReadOnly)) {
//        this->setStyleSheet(qssFile.readAll());
        QString styleSheet = QLatin1String(qssFile.readAll());
        qApp->setStyleSheet(styleSheet);
        qssFile.close();
    } else {
        qDebug() << "QSS加载失败";
     }
}

void MainWidget::onMapLoaded()
{
    if (!osgMapWidget_) return;
    
    auto entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        qDebug() << "EntityManager未初始化";
        return;
    }
    
    // 设置PlanFileManager的entityManager
    if (planFileManager_) {
        planFileManager_->setEntityManager(entityManager);
        qDebug() << "PlanFileManager的EntityManager已设置";
    }
    
    // 设置PlanFileManager到OsgMapWidget
    if (osgMapWidget_ && planFileManager_) {
        osgMapWidget_->setPlanFileManager(planFileManager_);
        qDebug() << "PlanFileManager已设置到OsgMapWidget";
        
        // 更新工具栏的方案名称
        updatePlanNameLabel();
    }
    
    // 连接导航历史信号到工具栏按钮
    if (osgMapWidget_ && osgMapWidget_->getNavigationHistory()) {
        auto navHistory = osgMapWidget_->getNavigationHistory();
        connect(navHistory, &NavigationHistory::historyStateChanged, this, [this](bool canBack, bool canForward) {
            if (returnBtn_) returnBtn_->setEnabled(canBack);
            if (forwardBtn_) forwardBtn_->setEnabled(canForward);
        });
        
        // 连接前进后退按钮
        if (returnBtn_) {
            connect(returnBtn_, &QPushButton::clicked, this, [this]() {
                if (osgMapWidget_ && osgMapWidget_->getNavigationHistory()) {
                    auto viewer = osgMapWidget_->getViewer();
                    if (viewer) {
                        auto mapStateManager = osgMapWidget_->getMapStateManager();
                        if (mapStateManager) {
                            osgEarth::Viewpoint currentVp = mapStateManager->getCurrentViewpoint("Current");
                            osgEarth::Viewpoint backVp;
                            if (osgMapWidget_->getNavigationHistory()->goBack(currentVp, backVp)) {
                                osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
                                if (em) {
                                    em->setViewpoint(backVp, 1.0);
                                    qDebug() << "导航后退到视角";
                                }
                            }
                        }
                    }
                }
            });
        }
        
        if (forwardBtn_) {
            connect(forwardBtn_, &QPushButton::clicked, this, [this]() {
                if (osgMapWidget_ && osgMapWidget_->getNavigationHistory()) {
                    auto viewer = osgMapWidget_->getViewer();
                    if (viewer) {
                        auto mapStateManager = osgMapWidget_->getMapStateManager();
                        if (mapStateManager) {
                            osgEarth::Viewpoint currentVp = mapStateManager->getCurrentViewpoint("Current");
                            osgEarth::Viewpoint forwardVp;
                            if (osgMapWidget_->getNavigationHistory()->goForward(currentVp, forwardVp)) {
                                osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
                                if (em) {
                                    em->setViewpoint(forwardVp, 1.0);
                                    qDebug() << "导航前进到视角";
                                }
                            }
                        }
                    }
                }
            });
        }
    }
    
    // 连接方案文件变化信号
    if (planFileManager_) {
        connect(planFileManager_, &PlanFileManager::planFileChanged, this, &MainWidget::updatePlanNameLabel);
        connect(planFileManager_, &PlanFileManager::planDataChanged, this, &MainWidget::updatePlanNameLabel);
        connect(planFileManager_, &PlanFileManager::planSaved, this, &MainWidget::updatePlanNameLabel);
        connect(planFileManager_, &PlanFileManager::planLoaded, this, [this](const QString&){
            refreshEntityManagementDialog();
            if (behaviorDialog_) {
                behaviorDialog_->refreshEntities();
            }
        }, Qt::UniqueConnection);
    }
 
    // 连接实体双击事件 - 打开属性编辑对话框
    connect(entityManager, &GeoEntityManager::entityDoubleClicked, this, [this](GeoEntity* entity) {
        focusEntity(entity);
        openEntityPropertyDialog(entity);
        if (osgMapWidget_) {
            osgMapWidget_->synthesizeMouseRelease(Qt::LeftButton);
        }
    });

    connect(entityManager, &GeoEntityManager::entityCreated, this, &MainWidget::onEntityCreated, Qt::UniqueConnection);
    connect(entityManager, &GeoEntityManager::entityRemoved, this, &MainWidget::onEntityRemoved, Qt::UniqueConnection);
    connect(entityManager, &GeoEntityManager::entitySelected, this, &MainWidget::onEntitySelected, Qt::UniqueConnection);
    connect(entityManager, &GeoEntityManager::entityDeselected, this, &MainWidget::onEntityDeselected, Qt::UniqueConnection);

    // 连接实体右键事件 - 显示右键菜单
    connect(entityManager, &GeoEntityManager::entityRightClicked, this, [this, entityManager](GeoEntity* entity, QPoint screenPos) {
        if (!entity || !planFileManager_) {
            return;
        }

        // 创建右键菜单

        QMenu menu(this);
        QAction* editAction = menu.addAction("编辑属性");
        QAction* routePlanAction = menu.addAction("航线规划");
        QAction* weaponMountAction = menu.addAction("武器挂载");
        menu.addSeparator();
        QAction* deleteAction = menu.addAction("删除");
        
        // 转换为全局坐标
        QPoint globalPos = osgMapWidget_->mapToGlobal(screenPos);
        WaypointEntity* waypointEntity = dynamic_cast<WaypointEntity*>(entity);

        if (waypointEntity) {
            QMenu waypointMenu(this);
            QAction* deleteWaypointAction = waypointMenu.addAction("删除航迹点");
            QAction* selectedAction = waypointMenu.exec(globalPos);
            if (osgMapWidget_) {
                osgMapWidget_->synthesizeMouseRelease(Qt::RightButton);
            }
            if (selectedAction == deleteWaypointAction) {
                int ret = QMessageBox::question(this, "确认删除", "确定要删除选中的航迹点吗？", QMessageBox::Yes | QMessageBox::No);
                if (ret == QMessageBox::Yes) {
                    if (entityManager->removeWaypointEntity(waypointEntity)) {
                        if (planFileManager_) {
                            planFileManager_->markPlanModified();
                        }
                    }
                }
            }
            return;
        }

        QAction* selectedAction = menu.exec(globalPos);
        if (osgMapWidget_) {
            osgMapWidget_->synthesizeMouseRelease(Qt::RightButton);
        }
        
        if (selectedAction == routePlanAction) {
            // 开始为实体规划航线
            if (isPlanningEntityRoute_) {
                QMessageBox::information(this, "提示", "已有航线规划正在进行中，请先完成或取消当前航线规划");
                return;
            }
            
            // 获取实体位置作为第一个航点
            double entityLon, entityLat, entityAlt;
            entity->getPosition(entityLon, entityLat, entityAlt);
            
            // 创建航点组
            QString groupId = entityManager->createWaypointGroup(QString("route_%1").arg(entity->getUid()));
            
            // 添加第一个航点（实体位置）
            entityManager->addWaypointToGroup(groupId, entityLon, entityLat, entityAlt);
            
            // 绑定航线到实体
            entityManager->bindRouteToEntity(groupId, entity->getUid());
            
            // 保存航线组ID到实体属性
            entity->setProperty("routeGroupId", groupId);
            
            // 设置状态
            isPlanningEntityRoute_ = true;
            entityRouteUid_ = entity->getUid();
            entityRouteGroupId_ = groupId;
            
            QMessageBox::information(this, "航线规划", 
                QString("已开始为实体 '%1' 规划航线\n第一个航点已设置为实体位置\n请在地图上左键点击添加航点，右键结束规划").arg(entity->getName()));
            qDebug() << "[EntityRoute] 开始为实体规划航线:" << entity->getUid() << "组ID:" << groupId;
        } else if (selectedAction == editAction) {
            openEntityPropertyDialog(entity);
            if (osgMapWidget_) {
                osgMapWidget_->setFocus();
            }
        } else if (selectedAction == weaponMountAction) {
            // 打开武器挂载对话框
            WeaponMountDialog* dialog = new WeaponMountDialog(entity, this);
            dialog->setAttribute(Qt::WA_DeleteOnClose);
            if (dialog->exec() == QDialog::Accepted) {
                // 保存方案
                planFileManager_->savePlan();
            }
            // 对话框关闭后，恢复焦点到地图窗口，确保相机控制正常
            if (osgMapWidget_) {
                osgMapWidget_->setFocus();
            }
        } else if (selectedAction == deleteAction) {
            deleteEntityWithConfirm(entity);
        }
    });
    
    // 订阅空白处点击，用于点标绘放置
    connect(entityManager, &GeoEntityManager::mapLeftClicked, this, [this, entityManager](QPoint screenPos) {        
        if (isDrawingLine_) {
            auto mapStateManager = osgMapWidget_->getMapStateManager();
            if (!mapStateManager) {
                QMessageBox::warning(this, "直线标绘", "地图状态管理器未初始化，无法获取坐标。");
                exitLineDrawing();
                return;
            }
            double lon = 0.0, lat = 0.0, alt = 0.0;
            mapStateManager->getGeoCoordinatesFromScreen(screenPos, lon, lat, alt);
            if (!hasPendingLineStart_) {
                lineStartLon_ = lon;
                lineStartLat_ = lat;
                lineStartAlt_ = alt;
                hasPendingLineStart_ = true;
                qDebug() << "[Line] 已记录起点:" << lon << lat << alt;
                return;
            }

            const double deltaLon = qFabs(lon - lineStartLon_);
            const double deltaLat = qFabs(lat - lineStartLat_);
            if (deltaLon < 1e-6 && deltaLat < 1e-6) {
                QMessageBox::information(this, "直线标绘", "两个点重合，请选择不同的位置。");
                return;
            }

            bool ok = false;
            QString lineName = QInputDialog::getText(
                this,
                "直线标绘",
                "请输入直线名称：",
                QLineEdit::Normal,
                "直线",
                &ok);
            if (!ok) {
                QMessageBox::information(this, "直线标绘", "已取消直线创建。");
                hasPendingLineStart_ = false;
                return;
            }
            lineName = lineName.trimmed();

            auto line = entityManager->addLineEntity(lineName,
                                                     lineStartLon_, lineStartLat_, lineStartAlt_,
                                                     lon, lat, alt);
            if (!line) {
                QMessageBox::warning(this, "直线标绘", "直线创建失败。");
            } else {
                if (planFileManager_) {
                    planFileManager_->markPlanModified();
                }
                entityManager->setSelectedEntity(line);
            }
            exitLineDrawing();
            return;
        }

        if (isPlacingWaypoint_) {
            double lon=0, lat=0, alt=0;
            // 统一使用 MapStateManager 获取坐标信息（mapStateManager 必然存在）
            auto mapStateManager = osgMapWidget_->getMapStateManager();
            mapStateManager->getGeoCoordinatesFromScreen(screenPos, lon, lat, alt);
            auto wp = entityManager->addStandaloneWaypoint(lon, lat, alt, pendingWaypointLabel_);
            if (!wp) QMessageBox::warning(this, "点标绘", "创建失败。");
            isPlacingWaypoint_ = false;
        }

        // 独立航线标绘：左键连加
        if (isPlacingRoute_ && !currentWaypointGroupId_.isEmpty()) {
            double lon=0, lat=0, alt=0;
            // 统一使用 MapStateManager 获取坐标信息（mapStateManager 必然存在）
            auto mapStateManager = osgMapWidget_->getMapStateManager();
            mapStateManager->getGeoCoordinatesFromScreen(screenPos, lon, lat, alt);
            auto wp = entityManager->addWaypointToGroup(currentWaypointGroupId_, lon, lat, alt);
            qDebug() << "[Route] 添加航点:" << lon << lat << alt << (wp?"成功":"失败");
        }
        
        // 实体航线规划：左键连加
        if (isPlanningEntityRoute_ && !entityRouteGroupId_.isEmpty()) {
            double lon=0, lat=0, alt=0;
            // 统一使用 MapStateManager 获取坐标信息（mapStateManager 必然存在）
            auto mapStateManager = osgMapWidget_->getMapStateManager();
            mapStateManager->getGeoCoordinatesFromScreen(screenPos, lon, lat, alt);
            auto wp = entityManager->addWaypointToGroup(entityRouteGroupId_, lon, lat, alt);
            qDebug() << "[EntityRoute] 添加航点:" << lon << lat << alt << (wp?"成功":"失败");
        }
    });

    // 航线标绘：右键结束并生成路线
    connect(entityManager, &GeoEntityManager::mapRightClicked, this, [this, entityManager](QPoint /*screenPos*/) {
        if (osgMapWidget_) {
            osgMapWidget_->synthesizeMouseRelease(Qt::RightButton);
        }
        if (isDrawingLine_) {
            exitLineDrawing("已取消直线标绘。");
        }
        // 独立航线标绘：右键结束并生成路线
        if (isPlacingRoute_ && !currentWaypointGroupId_.isEmpty()) {
            qDebug() << "[Route] 右键结束，准备生成路线，groupId=" << currentWaypointGroupId_;
            // 选择路径算法
            QStringList items; items << "linear" << "bezier";
            bool okSel = false;
            QString choice = QInputDialog::getItem(this, "生成航线", "选择生成算法:", items, 0, false, &okSel);
            if (!okSel || choice.isEmpty()) choice = "linear";

            bool ok = entityManager->generateRouteForGroup(currentWaypointGroupId_, choice);
            if (!ok) {
                QMessageBox::warning(this, "航线标绘", "生成路线失败（点数不足或错误）。");
                qDebug() << "[Route] 生成路线失败";
            } else {
                qDebug() << "[Route] 生成路线成功:" << choice;
            }
            isPlacingRoute_ = false;
            currentWaypointGroupId_.clear();
        }
        
        // 实体航线规划：右键结束并生成路线
        if (isPlanningEntityRoute_ && !entityRouteGroupId_.isEmpty()) {
            qDebug() << "[EntityRoute] 右键结束，准备生成路线，groupId=" << entityRouteGroupId_;
            
            // 检查航点数量（至少需要2个点）
            auto groupInfo = entityManager->getWaypointGroup(entityRouteGroupId_);
            if (groupInfo.waypoints.size() < 2) {
                QMessageBox::warning(this, "航线规划", "航线至少需要2个航点（包括起始点）");
                return;
            }
            
            // 选择路径算法
            QStringList items; items << "linear" << "bezier";
            bool okSel = false;
            QString choice = QInputDialog::getItem(this, "生成航线", "选择生成算法:", items, 0, false, &okSel);
            if (!okSel || choice.isEmpty()) choice = "linear";

            bool ok = entityManager->generateRouteForGroup(entityRouteGroupId_, choice);
            if (!ok) {
                QMessageBox::warning(this, "航线规划", "生成路线失败（点数不足或错误）。");
                qDebug() << "[EntityRoute] 生成路线失败";
            } else {
                qDebug() << "[EntityRoute] 生成路线成功:" << choice;
                // 保存路线类型到实体属性
                if (entityManager->getEntity(entityRouteUid_)) {
                    entityManager->getEntity(entityRouteUid_)->setProperty("routeType", choice);
                }
                // 标记方案有未保存更改
                if (planFileManager_) {
                    emit planFileManager_->planDataChanged();
                }
            }
            
            // 结束规划状态
            isPlanningEntityRoute_ = false;
            entityRouteUid_.clear();
            entityRouteGroupId_.clear();
        }
    });
    
    qDebug() << "地图事件连接完成";

    if (behaviorDialog_) {
        behaviorDialog_->setEntityManager(entityManager);
        behaviorDialog_->refreshEntities();
    }
}

void MainWidget::onToggle2D3D()
{
    if (!osgMapWidget_) return;
    
    if (is3DMode_) {
        // 切换到2D模式
        is3DMode_ = false;
        toggle2D3DBtn_->setText("切换到3D");
        osgMapWidget_->setMode2D();
        qDebug() << "切换到2D模式";
    } else {
        // 切换到3D模式
        is3DMode_ = true;
        toggle2D3DBtn_->setText("切换到2D");
        osgMapWidget_->setMode3D();
        qDebug() << "切换到3D模式";
    }
}

void MainWidget::onLocationJumpClicked()
{
    if (!osgMapWidget_) {
        QMessageBox::warning(this, "错误", "地图未初始化，无法跳转");
        return;
    }

    osgViewer::Viewer* viewer = osgMapWidget_->getViewer();
    if (!viewer) {
        QMessageBox::warning(this, "错误", "地图查看器未初始化，无法跳转");
        return;
    }

    // 从 MapStateManager 获取当前窗口中心的经纬度、高度和视距
    double currentLongitude = 116.3974; // 默认值：北京经度
    double currentLatitude = 39.9093;   // 默认值：北京纬度
    double currentAltitude = 0.0;
    double currentRange = 10000000.0;   // 默认值：10公里

    auto mapStateManager = osgMapWidget_->getMapStateManager();
    if (mapStateManager) {
        const auto& state = mapStateManager->getCurrentState();
        currentLongitude = state.viewLongitude;
        currentLatitude = state.viewLatitude;
        currentAltitude = state.viewAltitude;
        currentRange = state.range;
        
        // 如果高度接近0（椭球面高度），使用默认高度
        if (currentAltitude < 100.0) {
            currentAltitude = 0.0;
        }
    }

    // 显示定位跳转对话框，使用当前窗口中心作为默认值
    LocationJumpDialog dialog(currentLongitude, currentLatitude, currentAltitude, currentRange, this);
    if (dialog.exec() != QDialog::Accepted) {
        return; // 用户取消
    }

    // 获取输入的经纬度
    double longitude = dialog.getLongitude();
    double latitude = dialog.getLatitude();
    double altitude = dialog.getAltitude();
    double range = dialog.getRange();

    // 获取当前的航向角和俯仰角（保持当前视角）
    double heading = 0.0;
    double pitch = -45.0; // 默认俯仰角

    // 从 MapStateManager 获取当前状态（重用之前获取的 mapStateManager）
    if (mapStateManager) {
        const MapStateInfo& state = mapStateManager->getCurrentState();
        heading = state.heading;
        pitch = state.pitch;
        
        // 如果俯仰角太接近水平（大于-10度），设置为默认的-45度
        if (pitch > -10.0) {
            pitch = -45.0;
        }
    }

    // 获取 EarthManipulator 用于设置视点
    osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
    if (!em) {
        QMessageBox::warning(this, "错误", "无法获取地图操作器，跳转失败");
        return;
    }

    // 创建新的视点并跳转
    osgEarth::Viewpoint vp("Location Jump", longitude, latitude, altitude, heading, pitch, range);
    em->setViewpoint(vp, 2.0); // 2秒动画过渡

    qDebug() << "跳转到位置: 经度" << longitude << "纬度" << latitude << "高度" << altitude << "视距" << range;
}

void MainWidget::updatePlanNameLabel()
{
    if (!planNameLabel_ || !planFileManager_) {
        return;
    }
    
    QString currentPlan = planFileManager_->getCurrentPlanFile();
    if (!currentPlan.isEmpty()) {
        QFileInfo fileInfo(currentPlan);
        QString planName = fileInfo.baseName();
        
        // 检查是否有未保存的更改
        if (planFileManager_->hasUnsavedChanges()) {
            planNameLabel_->setText(QString("当前方案: %1 *未保存").arg(planName));
            planNameLabel_->setStyleSheet("color: #FFA500; font-weight: bold; padding: 0 10px;"); // 橙色提示
        } else {
            planNameLabel_->setText(QString("当前方案: %1").arg(planName));
            planNameLabel_->setStyleSheet("color: white; font-weight: bold; padding: 0 10px;");
        }
    } else {
        planNameLabel_->setText("当前方案: 未打开");
        planNameLabel_->setStyleSheet("color: white; font-weight: bold; padding: 0 10px;");
    }
}

void MainWidget::showEntityManagementDialog()
{
    auto entityManager = osgMapWidget_ ? osgMapWidget_->getEntityManager() : nullptr;
    if (!entityManager) {
        QMessageBox::information(this, "提示", "地图尚未加载或实体管理器未初始化");
        return;
    }

    if (!entityManagementDialog_) {
        entityManagementDialog_ = new EntityManagementDialog(this);
        entityManagementDialog_->setModal(false);
        entityManagementDialog_->setWindowModality(Qt::NonModal);

        connect(entityManagementDialog_, &EntityManagementDialog::requestFocus,
                this, &MainWidget::onEntityFocusRequested);
        connect(entityManagementDialog_, &EntityManagementDialog::requestEdit,
                this, &MainWidget::onEntityEditRequested);
        connect(entityManagementDialog_, &EntityManagementDialog::requestDelete,
                this, &MainWidget::onEntityDeleteRequested);
        connect(entityManagementDialog_, &EntityManagementDialog::requestVisibilityChange,
                this, &MainWidget::onEntityVisibilityChanged);
        connect(entityManagementDialog_, &EntityManagementDialog::requestSelection,
                this, &MainWidget::onEntitySelectionRequested);
        connect(entityManagementDialog_, &EntityManagementDialog::requestRefresh,
                this, &MainWidget::onEntityRefreshRequested);
        connect(entityManagementDialog_, &EntityManagementDialog::requestWeaponQuantityChange,
                this, &MainWidget::onEntityWeaponQuantityChanged);
        connect(entityManagementDialog_, &EntityManagementDialog::requestHover,
                this, [this](const QString& uid, bool hovered){
                    auto entityManager = osgMapWidget_ ? osgMapWidget_->getEntityManager() : nullptr;
                    if (!entityManager) {
                        return;
                    }
                    GeoEntity* entity = uid.isEmpty() ? nullptr : entityManager->getEntity(uid);
                    if (hovered) {
                        if (dialogHoverEntity_ && dialogHoverEntity_ != entity) {
                            dialogHoverEntity_->setHovered(false);
                        }
                        dialogHoverEntity_ = entity;
                        if (dialogHoverEntity_) {
                            dialogHoverEntity_->setHovered(true);
                        }
                    } else {
                        if (!entity || entity == dialogHoverEntity_) {
                            if (dialogHoverEntity_) {
                                dialogHoverEntity_->setHovered(false);
                                dialogHoverEntity_ = nullptr;
                            }
                        } else if (entity) {
                            entity->setHovered(false);
                        }
                    }
                });
        connect(entityManagementDialog_, &QDialog::finished, this, [this](int){
            if (dialogHoverEntity_) {
                dialogHoverEntity_->setHovered(false);
                dialogHoverEntity_ = nullptr;
            }
            if (osgMapWidget_) {
                osgMapWidget_->setFocus(Qt::OtherFocusReason);
            }
        });
    }

    refreshEntityManagementDialog();
    entityManagementDialog_->show();
    entityManagementDialog_->raise();
    entityManagementDialog_->activateWindow();
}

void MainWidget::refreshEntityManagementDialog()
{
    if (!entityManagementDialog_) {
        return;
    }
    auto entityManager = osgMapWidget_ ? osgMapWidget_->getEntityManager() : nullptr;
    if (!entityManager) {
        entityManagementDialog_->refresh(QList<GeoEntity*>(), QMap<QString, QList<RouteGroupData>>(), QString());
        return;
    }

    const QList<GeoEntity*> entities = entityManager->getAllEntities();
    QMap<QString, QList<RouteGroupData>> entityRouteMap;
    for (GeoEntity* entity : entities) {
        if (!entity) {
            continue;
        }
        QString groupId = entityManager->getRouteGroupIdForEntity(entity->getUid());
        if (groupId.isEmpty()) {
            continue;
        }
        GeoEntityManager::WaypointGroupInfo groupInfo = entityManager->getWaypointGroup(groupId);
        RouteGroupData data;
        data.groupId = groupInfo.groupId;
        data.groupName = groupInfo.name.isEmpty() ? groupInfo.groupId : groupInfo.name;
        for (auto* wp : groupInfo.waypoints) {
            if (wp) {
                data.waypoints.append(wp);
            }
        }
        entityRouteMap[entity->getUid()].append(data);
    }

    QString selectedUid;
    if (GeoEntity* selected = entityManager->getSelectedEntity()) {
        selectedUid = selected->getUid();
    }
    entityManagementDialog_->refresh(entities, entityRouteMap, selectedUid);
}

void MainWidget::focusEntity(GeoEntity* entity)
{
    if (!entity || !osgMapWidget_) {
        return;
    }

    osgViewer::Viewer* viewer = osgMapWidget_->getViewer();
    if (!viewer) {
        return;
    }

    double lon = 0.0, lat = 0.0, alt = 0.0;
    entity->getPosition(lon, lat, alt);

    // 从 MapStateManager 获取当前状态
    double heading = 0.0;
    double pitch = -45.0;
    double range = 10000000.0; // 默认值

    auto mapState = osgMapWidget_->getMapStateManager();
    if (mapState) {
        const auto& state = mapState->getCurrentState();
        heading = state.heading;
        pitch = state.pitch;
        if (pitch > -10.0) {
            pitch = -45.0;
        }
        range = qMax(3000.0, state.range * 0.5);
    }

    // 获取 EarthManipulator 用于设置视点
    osgEarth::Util::EarthManipulator* em = GeoUtils::getEarthManipulator(viewer);
    if (!em) {
        return;
    }

    std::string nameStd = entity->getName().toStdString();
    osgEarth::Viewpoint vp(nameStd.c_str(), lon, lat, alt, heading, pitch, range);
    em->setViewpoint(vp, 1.0);

    if (auto entityManager = osgMapWidget_->getEntityManager()) {
        entityManager->setSelectedEntity(entity);
    }

    osgMapWidget_->setFocus(Qt::OtherFocusReason);
    refreshEntityManagementDialog();
}

void MainWidget::focusEntityByUid(const QString& uid)
{
    if (uid.isEmpty()) {
        return;
    }
    auto entityManager = osgMapWidget_ ? osgMapWidget_->getEntityManager() : nullptr;
    if (!entityManager) {
        return;
    }
    focusEntity(entityManager->getEntity(uid));
}

void MainWidget::openEntityPropertyDialog(GeoEntity* entity)
{
    if (!entity || !planFileManager_) {
        return;
    }

    EntityPropertyDialog* dialog = new EntityPropertyDialog(entity, planFileManager_, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->exec();

    refreshEntityManagementDialog();
}

void MainWidget::deleteEntityWithConfirm(GeoEntity* entity)
{
    if (!entity || !planFileManager_ || !osgMapWidget_) {
        return;
    }

    auto entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        return;
    }

    int ret = QMessageBox::question(this, "确认删除",
                                   QString("确定要删除实体 '%1' 吗？").arg(entity->getName()),
                                   QMessageBox::Yes | QMessageBox::No);
    if (ret != QMessageBox::Yes) {
        return;
    }

    planFileManager_->removeEntityFromPlan(entity->getUid());
    entityManager->removeEntity(entity->getUid());
    planFileManager_->savePlan();

    refreshEntityManagementDialog();
}

void MainWidget::onEntityFocusRequested(const QString& uid)
{
    focusEntityByUid(uid);
}

void MainWidget::onEntityEditRequested(const QString& uid)
{
    if (uid.isEmpty() || !osgMapWidget_) {
        return;
    }
    GeoEntityManager* entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        return;
    }
    openEntityPropertyDialog(entityManager->getEntity(uid));
}

void MainWidget::onEntityDeleteRequested(const QString& uid)
{
    if (uid.isEmpty() || !osgMapWidget_) {
        return;
    }
    GeoEntityManager* entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        return;
    }
    deleteEntityWithConfirm(entityManager->getEntity(uid));
}

void MainWidget::onEntityVisibilityChanged(const QString& uid, bool visible)
{
    if (uid.isEmpty() || !osgMapWidget_) {
        return;
    }
    GeoEntityManager* entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        return;
    }
    if (!entityManager->setEntityVisible(uid, visible)) {
        return;
    }

    if (planFileManager_) {
        planFileManager_->markPlanModified();
    }

    refreshEntityManagementDialog();
}

void MainWidget::onEntitySelectionRequested(const QString& uid)
{
    if (uid.isEmpty() || !osgMapWidget_) {
        return;
    }
    GeoEntityManager* entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        return;
    }
    GeoEntity* entity = entityManager->getEntity(uid);
    if (!entity) {
        return;
    }
    entityManager->setSelectedEntity(entity);
    refreshEntityManagementDialog();
}

void MainWidget::onEntityRefreshRequested()
{
    refreshEntityManagementDialog();
}

void MainWidget::onEntityCreated(GeoEntity* entity)
{
    Q_UNUSED(entity);
    refreshEntityManagementDialog();
    if (behaviorDialog_) {
        behaviorDialog_->refreshEntities();
    }
}

void MainWidget::onEntityRemoved(const QString& uid)
{
    Q_UNUSED(uid);
    refreshEntityManagementDialog();
    if (behaviorDialog_) {
        behaviorDialog_->refreshEntities();
    }
}

void MainWidget::onEntitySelected(GeoEntity* entity)
{
    if (entityManagementDialog_) {
        entityManagementDialog_->setSelectedUid(entity ? entity->getUid() : QString());
    }
    if (osgMapWidget_) {
        osgMapWidget_->synthesizeMouseRelease(Qt::LeftButton);
    }
}

void MainWidget::onEntityDeselected()
{
    if (entityManagementDialog_) {
        entityManagementDialog_->setSelectedUid(QString());
    }
}

void MainWidget::onEntityWeaponQuantityChanged(const QString& entityUid,
                                               const QString& weaponId,
                                               const QString& weaponName,
                                               int quantity)
{
    if (!osgMapWidget_) {
        return;
    }
    auto entityManager = osgMapWidget_->getEntityManager();
    if (!entityManager) {
        return;
    }
    GeoEntity* entity = entityManager->getEntity(entityUid);
    if (!entity) {
        return;
    }

    QJsonObject mounts = entity->getProperty("weaponMounts").toJsonObject();
    QJsonArray weapons = mounts["weapons"].toArray();
    bool updated = false;
    for (int i = 0; i < weapons.size(); ++i) {
        QJsonObject weaponObj = weapons.at(i).toObject();
        const QString currentId = weaponObj["weaponId"].toString();
        const QString currentName = weaponObj["weaponName"].toString();
        if ((!weaponId.isEmpty() && currentId == weaponId) ||
            (weaponId.isEmpty() && currentName == weaponName)) {
            weaponObj["quantity"] = quantity;
            weapons[i] = weaponObj;
            updated = true;
            break;
        }
    }

    if (!updated) {
        return;
    }

    mounts["weapons"] = weapons;
    entity->setProperty("weaponMounts", mounts);

    if (planFileManager_) {
        planFileManager_->markPlanModified();
    }

    refreshEntityManagementDialog();
}

void MainWidget::onBehaviorPlanningClicked()
{
    if (!planFileManager_) {
        QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"请先创建或打开方案"));
        return;
    }
    GeoEntityManager* entityManager = osgMapWidget_ ? osgMapWidget_->getEntityManager() : nullptr;
    if (!entityManager) {
        QMessageBox::information(this, QString::fromUtf8(u8"提示"), QString::fromUtf8(u8"地图尚未加载或实体管理器不可用"));
        return;
    }

    if (!behaviorDialog_) {
        behaviorDialog_ = new BehaviorPlanningDialog(this);
    }
    behaviorDialog_->setEntityManager(entityManager);
    behaviorDialog_->setPlanFileManager(planFileManager_);
    behaviorDialog_->refreshEntities();
    behaviorDialog_->show();
    behaviorDialog_->raise();
    behaviorDialog_->activateWindow();
}

