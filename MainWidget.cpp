#include "MainWidget.h"
#include "OsgMapWidget.h"
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

#include "geo/geoentitymanager.h"
#include "geo/geoutils.h"

MainWidget::MainWidget(QWidget *parent)
    : QWidget(parent), currentNavIndex(0)
    , is3DMode_(true)
    , isPlacingWaypoint_(false)
    , isPlacingRoute_(false)
    , componentConfigDialog_(nullptr)
    , modelAssemblyDialog_(nullptr)
{
    //设置窗口属性
    setWindowTitle("任务规划");
    setMinimumSize(1200, 800);

    loadStyleSheet();

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
        "   stop:0 #6495ED, stop:1 #0066cc);"
        "   border: none;"
        "   border-bottom: 1px solid #2C3E50;"
        "}"
    );

    toolBarLayout = new QHBoxLayout(toolBarWidget);
    toolBarLayout->setContentsMargins(15,5,15,5);

    // 添加工具栏按钮
    QPushButton *newBtn = new QPushButton("新建");
    newBtn->setStyleSheet("border: none;");
    QPushButton *saveBtn = new QPushButton("保存");
    saveBtn->setStyleSheet("border: none;");
    QPushButton *returnBtn = new QPushButton("后退");
    returnBtn->setStyleSheet("border: none;");
    QPushButton *forwardBtn = new QPushButton("前进");
    forwardBtn->setStyleSheet("border: none;");
    QPushButton *helpBtn = new QPushButton("帮助");
    helpBtn->setStyleSheet("border: none;");

    // 设置样式 待补充


    toolBarLayout->addWidget(newBtn);
    toolBarLayout->addWidget(saveBtn);
    toolBarLayout->addWidget(returnBtn);
    toolBarLayout->addWidget(forwardBtn);
    toolBarLayout->addWidget(helpBtn);
    
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
        "background-color: #0066cc;"
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
        "background-color: #0066cc;"
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
//    modelDeployBtn->setIcon(QIcon(":/images/模型部署.png"));
    modelDeployBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    modelDeployBtn->setFixedSize(120, 120);
    modelDeployBtn->setObjectName("navToolButton");
    modelDeployBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *trackManageBtn = new QToolButton(this);
    trackManageBtn->setText("航迹管理");
//    trackManageBtn->setIcon(QIcon(":/images/航线绘制.png"));
    trackManageBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    trackManageBtn->setFixedSize(120, 120);
    trackManageBtn->setObjectName("navToolButton");
    trackManageBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *targetMatchBtn = new QToolButton(this);
    targetMatchBtn->setText("目标匹配");
//    targetMatchBtn->setIcon(QIcon(":/images/目标匹配.png"));
    targetMatchBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    targetMatchBtn->setFixedSize(120, 120);
    targetMatchBtn->setObjectName("navToolButton");
    targetMatchBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *exportPlanBtn = new QToolButton(this);
    exportPlanBtn->setText("导出方案");
//    exportPlanBtn->setIcon(QIcon(":/images/导出方案.png"));
    exportPlanBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    exportPlanBtn->setFixedSize(120, 120);
    exportPlanBtn->setObjectName("navToolButton");
    exportPlanBtn->setIconSize(QSize(64, 64));  // 图标大小

    planLayout->addWidget(modelDeployBtn);
    planLayout->addWidget(trackManageBtn);
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
//    modelComponentBtn->setIcon(QIcon(":/images/模型组件.png"));
    modelComponentBtn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    modelComponentBtn->setFixedSize(120, 120);
    modelComponentBtn->setObjectName("navToolButton");
    modelComponentBtn->setIconSize(QSize(64, 64));  // 图标大小

    QToolButton *modelAssemblyBtn = new QToolButton(this);
    modelAssemblyBtn->setText("模型组装");
//    modelAssemblyBtn->setIcon(QIcon(":/images/模型组装.png"));
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
    
    // 航迹管理按钮（航线标绘）
    connect(trackManageBtn, &QPushButton::clicked, this, [this]() {
        if (!osgMapWidget_ || !osgMapWidget_->getEntityManager()) {
            qDebug() << "EntityManager未初始化";
            return;
        }
        auto entityManager = osgMapWidget_->getEntityManager();
        if (isPlacingRoute_) {
            qDebug() << "航线标绘已在进行中，左键添加点，右键结束。";
            return;
        }
        currentWaypointGroupId_ = entityManager->createWaypointGroup("route");
        isPlacingRoute_ = true;
        qDebug() << "航线标绘模式已启动，左键依次添加航点，右键结束";
    });
    
    // ... 连接其他子导航按钮

    // 添加到主布局
    contentLayout->addWidget(subNavWidget);
    mainVLayout->addLayout(contentLayout, 1);
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
    
    // 订阅空白处点击，用于点标绘放置
    connect(entityManager, &GeoEntityManager::mapLeftClicked, this, [this, entityManager](QPoint screenPos) {
        if (isPlacingWaypoint_) {
            double lon=0, lat=0, alt=0;
            auto viewer = osgMapWidget_->getViewer();
            auto mapNode = osgMapWidget_->getMapNode();
            if (!GeoUtils::screenToGeoCoordinates(viewer, mapNode, screenPos, lon, lat, alt)) {
                QMessageBox::warning(this, "点标绘", "无法将屏幕坐标转换为地理坐标。");
                isPlacingWaypoint_ = false;
                return;
            }
            auto wp = entityManager->addStandaloneWaypoint(lon, lat, alt, pendingWaypointLabel_);
            if (!wp) QMessageBox::warning(this, "点标绘", "创建失败。");
            isPlacingWaypoint_ = false;
        }

        // 航线标绘：左键连加
        if (isPlacingRoute_ && !currentWaypointGroupId_.isEmpty()) {
            double lon=0, lat=0, alt=0;
            auto viewer = osgMapWidget_->getViewer();
            auto mapNode = osgMapWidget_->getMapNode();
            if (!GeoUtils::screenToGeoCoordinates(viewer, mapNode, screenPos, lon, lat, alt)) return;
            auto wp = entityManager->addWaypointToGroup(currentWaypointGroupId_, lon, lat, alt);
            qDebug() << "[Route] 添加航点:" << lon << lat << alt << (wp?"成功":"失败");
        }
    });

    // 航线标绘：右键结束并生成路线
    connect(entityManager, &GeoEntityManager::mapRightClicked, this, [this, entityManager](QPoint /*screenPos*/) {
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
    });
    
    qDebug() << "地图事件连接完成";
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
