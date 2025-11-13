/**
 * @file MainWidget.h
 * @brief 主窗口组件头文件
 * 
 * 定义MainWidget类，应用程序的主窗口，整合所有功能模块
 */

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
#include <QLabel>
#include "ComponentConfigDialog.h"
#include "ModelAssemblyDialog.h"
#include "../geo/waypointentity.h"
#include "../widgets/OsgMapWidget.h"

// 前向声明
class GeoEntity;
class PlanFileManager;
class ModelDeployDialog;
class WeaponMountDialog;
class EntityManagementDialog;
class WaypointEntity;

/**
 * @brief 应用程序主窗口
 * 
 * 整合所有功能模块的主窗口，包括：
 * - 工具栏和导航栏
 * - OSG地图显示区域
 * - 方案文件管理（新建、打开、保存、另存为）
 * - 模型组件配置对话框
 * - 模型组装对话框
 * - 模型部署对话框
 * - 2D/3D视图切换
 * - 最近文件管理
 * 
 * 主要功能：
 * - 管理UI布局和导航
 * - 协调各个功能模块
 * - 处理方案文件的创建、打开、保存
 * - 管理对话框的显示（非模态）
 * - 处理实体双击和右键菜单
 */
class MainWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父widget
     */
    MainWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MainWidget();

private slots:
    /**
     * @brief 导航按钮点击槽函数
     */
    void onNavButtonClicked();
    
    /**
     * @brief 模型组件按钮点击槽函数
     * 
     * 显示或激活模型组件配置对话框（非模态）
     */
    void onModelComponentBtnClicked();
    
    /**
     * @brief 模型组装按钮点击槽函数
     * 
     * 显示或激活模型组装对话框（非模态）
     */
    void onModelAssemblyBtnClicked();
    
    /**
     * @brief 模型部署按钮点击槽函数
     * 
     * 显示模型部署对话框，用于拖拽部署实体到地图
     */
    void onModelDeployBtnClicked();
    
    /**
     * @brief 地图加载完成槽函数
     * 
     * 地图加载完成后，设置实体管理器和方案文件管理器
     */
    void onMapLoaded();
    
    /**
     * @brief 2D/3D切换按钮点击槽函数
     */
    void onToggle2D3D();
    
    /**
     * @brief 更新工具栏中的方案名称标签
     */
    void updatePlanNameLabel();
    
    /**
     * @brief 新建方案槽函数
     */
    void onNewPlanClicked();
    
    /**
     * @brief 打开方案槽函数
     */
    void onOpenPlanClicked();
    
    /**
     * @brief 保存方案槽函数
     */
    void onSavePlanClicked();
    
    /**
     * @brief 另存为方案槽函数
     */
    void onSavePlanAsClicked();

    void onEntityFocusRequested(const QString& uid);
    void onEntityEditRequested(const QString& uid);
    void onEntityDeleteRequested(const QString& uid);
    void onEntityVisibilityChanged(const QString& uid, bool visible);
    void onEntitySelectionRequested(const QString& uid);
    void onEntityRefreshRequested();
    void onEntityCreated(GeoEntity* entity);
    void onEntityRemoved(const QString& uid);
    void onEntitySelected(GeoEntity* entity);
    void onEntityDeselected();
    void onEntityWeaponQuantityChanged(const QString& entityUid,
                                       const QString& weaponId,
                                       const QString& weaponName,
                                       int quantity);

    /**
     * @brief 导出方案槽函数（生成AFSIM脚本）
     */
    void onExportPlanClicked();

    /**
     * @brief 距离测算按钮点击
     */
    void onDistanceMeasureClicked();

    /**
     * @brief 面积测算按钮点击：进入多点选择面积测算模式
     */
    void onAreaMeasureClicked();

    /**
     * @brief 角度测算按钮点击：进入方位/俯仰角计算模式
     */
    void onAngleMeasureClicked();

    /**
     * @brief 直线绘制按钮点击：进入直线标绘模式
     */
    void onLineDrawClicked();


private:
    /**
     * @brief 创建工具栏
     */
    void createToolBar();
    
    /**
     * @brief 创建导航栏
     */
    void createNavigation();
    
    /**
     * @brief 创建子导航栏
     */
    void createSubNavigation();
    
    /**
     * @brief 创建地图区域
     */
    void createMapArea();
    
    /**
     * @brief 更新子导航栏显示
     * @param navIndex 导航索引
     */
    void updateSubNavigation(int navIndex);
    
    /**
     * @brief 加载样式表
     */
    void loadStyleSheet();
    
    /**
     * @brief 更新最近文件列表
     * @param filePath 文件路径
     */
    void updateRecentFiles(const QString& filePath);
    
    /**
     * @brief 从设置中加载最近文件列表
     */
    void loadRecentFiles();
    
    /**
     * @brief 保存最近文件列表到设置
     */
    void saveRecentFiles();
    
    /**
     * @brief 添加最近文件菜单项
     * @param menu 菜单指针
     */
    void addRecentFileMenuItem(QMenu* menu);

    void showEntityManagementDialog();
    void refreshEntityManagementDialog();
    void focusEntity(GeoEntity* entity);
    void focusEntityByUid(const QString& uid);
    void openEntityPropertyDialog(GeoEntity* entity);
    void deleteEntityWithConfirm(GeoEntity* entity);

    // 主布局组件
    QVBoxLayout *mainVLayout;        // 主垂直布局
    QHBoxLayout *contentLayout;       // 内容水平布局

    // 工具栏
    QWidget *toolBarWidget;          // 工具栏widget
    QHBoxLayout *toolBarLayout;      // 工具栏布局

    // 导航栏
    QWidget *navWidget;              // 导航栏widget
    QVBoxLayout *navLayout;          // 导航栏布局
    QToolButton *planBtn;            // 方案规划按钮
    QToolButton *resourceBtn;        // 资源管理按钮
    QToolButton *mapBtn;             // 地图按钮
    QToolButton *situationBtn;       // 态势标绘按钮

    // 子导航栏
    QWidget *subNavWidget;           // 子导航栏widget
    QVBoxLayout *subNavLayout;       // 子导航栏布局
    QStackedWidget *subNavStack;     // 子导航栏堆叠widget

    // 子导航按钮组
    QWidget *planSubNav;             // 方案规划子导航
    QWidget *resourceSubNav;         // 资源管理子导航
    QWidget *mapSubNav;              // 地图子导航
    QWidget *situationSubNav;         // 态势标绘子导航

    // 地图区域
    OsgMapWidget *osgMapWidget_;    // OSG地图控件

    // 对话框指针（使用成员变量避免重复创建，并支持非模态显示）
    ComponentConfigDialog *componentConfigDialog_;    // 组件配置对话框
    ModelAssemblyDialog *modelAssemblyDialog_;         // 模型组装对话框
    ModelDeployDialog *modelDeployDialog_;            // 模型部署对话框
    EntityManagementDialog *entityManagementDialog_;  // 实体管理对话框
    
    // 方案文件管理器
    PlanFileManager *planFileManager_;
    
    // 最近打开的文件列表（最多10个）
    QStringList recentPlanFiles_;
    static const int MAX_RECENT_FILES = 10;  // 最近文件最大数量

    // 当前选中的导航索引
    int currentNavIndex;
    
    // 2D/3D切换按钮
    QPushButton *toggle2D3DBtn_;     // 2D/3D切换按钮
    bool is3DMode_;                  // 当前是否为3D模式
    
    // 工具栏按钮
    QPushButton *returnBtn_;         // 后退按钮
    QPushButton *forwardBtn_;        // 前进按钮
    QLabel *planNameLabel_;          // 当前方案名称标签
    
    // 点标绘：是否进入放置模式与待放置标签
    bool isPlacingWaypoint_;         // 是否正在放置航点
    QString pendingWaypointLabel_;   // 待放置的航点标签
    
    // 航线标绘：是否处于连点模式
    bool isPlacingRoute_;            // 是否正在放置航线（独立航线）
    QString currentWaypointGroupId_; // 当前航点组ID
    
    // 实体航线规划相关
    bool isPlanningEntityRoute_;     // 是否正在为实体规划航线
    QString entityRouteUid_;     // 正在规划航线的实体UID
    QString entityRouteGroupId_;     // 实体航线组ID
    GeoEntity* dialogHoverEntity_;

    // 地图服务相关
    bool isMeasuringDistance_ = false;               // 是否处于测距模式
    WaypointEntity* distancePointA_ = nullptr;  // 起点标绘点
    WaypointEntity* distancePointB_ = nullptr;  // 终点标绘点
    bool isMeasuringArea_ = false;                   // 是否处于面积测算模式
    QVector<class WaypointEntity*> areaMeasurePoints_; // 面积测算已选航点
    bool isMeasuringAngle_ = false;                  // 是否处于角度测算模式
    class WaypointEntity* angleBasePoint_ = nullptr; // 角度测算起点
    class WaypointEntity* angleTargetPoint_ = nullptr;// 角度测算终点

    bool isDrawingLine_ = false;                     // 是否处于直线绘制模式
    bool hasPendingLineStart_ = false;               // 是否已记录直线起点
    double lineStartLon_ = 0.0;
    double lineStartLat_ = 0.0;
    double lineStartAlt_ = 0.0;

    QMetaObject::Connection distanceLeftClickConn_;
    QMetaObject::Connection distanceRightClickConn_;
    QMetaObject::Connection areaLeftClickConn_;
    QMetaObject::Connection areaRightClickConn_;
    QMetaObject::Connection angleLeftClickConn_;
    QMetaObject::Connection angleRightClickConn_;

    // 重置所有测量模式，一次性退出所有正在进行的测量操作
    void resetMeasurementModes();
    // 安全地断开信号槽连接，避免重复断开和空连接操作
    void disconnectMeasurementConnection(QMetaObject::Connection& connection);
    // 退出距离测量模式
    void exitDistanceMeasure(const QString& message = QString());
    // 退出面积测量模式
    void exitAreaMeasure(const QString& message = QString());
    // 退出角度测量模式
    void exitAngleMeasure(const QString& message = QString());
    // 退出直线绘制模式
    void exitLineDrawing(const QString& message = QString());
    // 计算多边形的面积
    double computePolygonAreaMeters(const QVector<class WaypointEntity*>& points) const;
    // 计算两个点的角度
    void showAngleBetweenWaypoints(class WaypointEntity* from, class WaypointEntity* to);

    // 使用 Haversine 公式计算两经纬度点的大圆距离（米）
    double computeDistanceMeters(double lat1Deg, double lon1Deg, double lat2Deg, double lon2Deg) const;

    /**
     * @brief 在屏幕坐标附近模糊选择一个航点（标绘点）
     * @param screenPos 鼠标点击的屏幕坐标
     * @param radiusPx 搜索半径（像素）
     * @return 找到的航点指针，未找到返回nullptr
     */
    WaypointEntity* pickWaypointNearScreenPos(const QPoint& screenPos, int radiusPx) const;


};

#endif // MAINWIDGET_H
