#ifndef SCENPLAN_H
#define SCENPLAN_H

#include <QToolBar>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QScrollBar>
#include <QPainter>
#include <QStackedWidget>
#include <QFile>
#include <QInputDialog>
#include <QMessageBox>
#include "ComponentConfigDialog.h"
#include "ModelAssemblyDialog.h"
#include "../widgets/OsgMapWidget.h"

// 前向声明
class GeoEntity;
class PlanFileManager;
class ModelDeployDialog;


class MainWidget : public QWidget
{
    Q_OBJECT

public:
    MainWidget(QWidget *parent = nullptr);
    ~MainWidget();

private slots:
    void onNavButtonClicked();
    void onModelComponentBtnClicked();
    void onModelAssemblyBtnClicked();
    void onModelDeployBtnClicked();
    void onMapLoaded();
    void onToggle2D3D();
    void onNewPlanClicked();
    void onOpenPlanClicked();
    void onSavePlanClicked();
    void onSavePlanAsClicked();

private:
    void createToolBar();
    void createNavigation();
    void createSubNavigation();
    void createMapArea();
    void updateSubNavigation(int navIndex);
    void loadStyleSheet();


    // 主布局组件
//    QWidget *centralWidget;
    QVBoxLayout *mainVLayout;
    QHBoxLayout *contentLayout;

    // 工具栏
    QWidget *toolBarWidget;
    QHBoxLayout *toolBarLayout;

    // 导航栏
    QWidget *navWidget;
    QVBoxLayout *navLayout;
    QToolButton *planBtn, *resourceBtn, *mapBtn, *situationBtn;

    // 子导航栏
    QWidget *subNavWidget;
    QVBoxLayout *subNavLayout;
    QStackedWidget *subNavStack;

    // 子导航按钮组
    QWidget *planSubNav, *resourceSubNav, *mapSubNav, *situationSubNav;

    // 地图区域
    OsgMapWidget *osgMapWidget_;  // OSG地图控件

    // 对话框指针（使用成员变量避免重复创建，并支持非模态显示）
    ComponentConfigDialog *componentConfigDialog_;
    ModelAssemblyDialog *modelAssemblyDialog_;
    ModelDeployDialog *modelDeployDialog_;
    
    // 方案文件管理器
    PlanFileManager *planFileManager_;
    
    // 最近打开的文件列表（最多10个）
    QStringList recentPlanFiles_;
    static const int MAX_RECENT_FILES = 10;
    
    // 辅助方法
    void updateRecentFiles(const QString& filePath);
    void loadRecentFiles();
    void saveRecentFiles();
    void addRecentFileMenuItem(QMenu* menu);

    // 当前选中的导航索引
    int currentNavIndex;
    
    // 2D/3D切换按钮
    QPushButton *toggle2D3DBtn_;
    bool is3DMode_;
    
    // 点标绘：是否进入放置模式与待放置标签
    bool isPlacingWaypoint_;
    QString pendingWaypointLabel_;
    
    // 航线标绘：是否处于连点模式
    bool isPlacingRoute_;
    QString currentWaypointGroupId_;

};

#endif // MAINWIDGET_H
