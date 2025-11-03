/**
 * @file mainwindow.h
 * @brief 主窗口类头文件
 * 
 * 定义应用程序的主窗口MainWindow，负责UI显示和用户交互
 */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgGA/OrbitManipulator>
#include <osgGA/TerrainManipulator>
#include <osgEarth/MapNode>
#include "geo/mapstatemanager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 前置声明
class QLabel;
class QPushButton;
class QResizeEvent;
class QMenu;
class QMouseEvent;
class ImageViewerWindow;
class GeoEntityManager;
class MapStateManager;
class GeoEntity;

/**
 * @brief 地图模式枚举
 */
enum MapMode {
    MAP_MODE_2D,  ///< 2D地图模式
    MAP_MODE_3D   ///< 3D地图模式
};

/**
 * @brief 主窗口类
 * 
 * 应用程序的主窗口，负责UI显示和用户交互。
 * 集成了Qt和OSG，提供3D地图显示和实体管理功能。
 * 
 * 主要功能：
 * - 3D地图显示和视角控制
 * - 2D/3D模式切换
 * - 实体拖拽添加
 * - 实体右键菜单
 * - 地图状态监控
 * 
 * @note 通过信号槽机制与 GeoEntityManager 和 MapStateManager 通信
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void toggle2D3DMode();  // 2D/3D切换
    void openImageViewer();
    void onMapStateChanged(const MapStateInfo& state);  // 地图状态变化
    void onViewPositionChanged(double longitude, double latitude, double altitude);   // 视角位置变化
    void onMousePositionChanged(double longitude, double latitude, double altitude);  // 鼠标位置变化

protected:
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void initializeViewer();
    void loadMap(const QString& earthFile);
    void setupCamera();
    void setupManipulator(MapMode mode);
    void loadEntityConfig();
    bool screenToGeoCoordinates(QPoint screenPos, double& longitude, double& latitude, double& altitude);
    void showEntityContextMenu(QPoint screenPos, GeoEntity* entity);

private:
    Ui::MainWindow *ui;
    osg::ref_ptr<osg::Group> root_;
    osg::ref_ptr<osgViewer::Viewer> viewer_;
    osg::ref_ptr<osgGA::TrackballManipulator> trackballManipulator_;
    osg::ref_ptr<osgGA::TerrainManipulator> terrainManipulator_;
    osg::ref_ptr<osgEarth::MapNode> mapNode_;

    MapMode currentMode_;
    QString earth2DPath_;
    QString earth3DPath_;
    
    // 切换按钮
    QPushButton* toggleButton_;

    
    // 图片查看器窗口
    ImageViewerWindow* imageViewerWindow_;
    
    // 实体管理器
    GeoEntityManager* entityManager_;
    
    // 地图状态管理器
    MapStateManager* mapStateManager_;
    
    // 实体选择管理
    GeoEntity* selectedEntity_;
    QMenu* entityContextMenu_;

    // 航线规划：当前选中的航点组ID
    QString currentWaypointGroupId_;

    // 点标绘：是否进入放置模式与待放置标签
    bool isPlacingWaypoint_ = false;
    QString pendingWaypointLabel_;

    // 航线标绘：是否处于连点模式
    bool isPlacingRoute_ = false;
};
#endif // MAINWINDOW_H
