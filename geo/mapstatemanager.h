/**
 * @file mapstatemanager.h
 * @brief 地图状态管理器头文件
 * 
 * 定义MapStateManager类和MapStateInfo结构体，用于监控和管理地图状态信息
 */

#ifndef MAPSTATEMANAGER_H
#define MAPSTATEMANAGER_H

#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QCursor>
#include <tuple>
#include <osgViewer/Viewer>
#include <osgEarth/GeoData>
#include <osgEarth/Viewpoint>
#include <osgEarthUtil/EarthManipulator>
#include <osgEarth/MapNode>
#include <osgUtil/LineSegmentIntersector>

/**
 * @ingroup managers
 * @brief 默认高度常量（米）
 * 
 * 当无法从地形获取高度或高度接近0（椭球面高度）时使用的默认高度值
 */
namespace MapStateConstants {
    constexpr double DEFAULT_ALTITUDE_METERS = 10000.0;  // 默认高度：10000米
}

/**
 * @ingroup managers
 * @brief 地图状态信息结构体
 * 
 * 包含地图的9元组信息 (a,b,c,x1,y1,z1,x2,y2,z2)：
 * - a,b,c: 相机参数（俯仰角、航向角、距离）
 * - x1,y1,z1: 视角位置（经度、纬度、高度）
 * - x2,y2,z2: 鼠标位置（经度、纬度、高度）
 */
struct MapStateInfo {
    double pitch;           // a: 俯仰角 (pitch) - 相机上下倾斜角度
    double heading;         // b: 航向角 (heading) - 相机左右旋转角度
    double range;           // c: 距离 (range) - 相机到焦点的距离
    double viewLongitude;   // x1: 视角当前经度
    double viewLatitude;    // y1: 视角当前纬度
    double viewAltitude;    // z1: 视角当前高度
    double mouseLongitude;  // x2: 鼠标当前经度
    double mouseLatitude;   // y2: 鼠标当前纬度
    double mouseAltitude;   // z2: 鼠标当前高度
    
    MapStateInfo() 
        : pitch(-90.0), heading(0.0), range(100000.0),
          viewLongitude(116.4), viewLatitude(39.9), viewAltitude(MapStateConstants::DEFAULT_ALTITUDE_METERS),
          mouseLongitude(116.4), mouseLatitude(39.9), mouseAltitude(MapStateConstants::DEFAULT_ALTITUDE_METERS) {}
    
    // 获取9元组信息 (a,b,c,x1,y1,z1,x2,y2,z2)
    std::tuple<double,double,double,double,double,double,double,double,double> getTuple() const {
        return std::make_tuple(pitch, heading, range, 
                              viewLongitude, viewLatitude, viewAltitude,
                              mouseLongitude, mouseLatitude, mouseAltitude);
    }
};

/**
 * @ingroup managers
 * @brief 地图状态管理器
 * 
 * 监控和管理地图的状态信息，包括相机参数和鼠标位置。
 * 通过信号槽机制实时通知状态变化。
 * 
 * 主要功能：
 * - 监控相机参数（俯仰角、航向角、距离）
 * - 监控视角位置（经纬度和高度）
 * - 监控鼠标位置（经纬度和高度）
 * - 提供9元组状态信息
 * - 处理鼠标事件和滚轮事件
 * 
 * @note 使用定时器定期更新状态信息
 */
class MapStateManager : public QObject
{
    Q_OBJECT

public:
    explicit MapStateManager(osgViewer::Viewer* viewer, QObject* parent = nullptr);
    ~MapStateManager();
    
    // 获取9元组信息
    std::tuple<double,double,double,double,double,double,double,double,double> getStateTuple() const;
    
    // 获取单个状态信息
    double getPitch() const;           // 俯仰角
    double getHeading() const;         // 航向角
    double getRange() const;          // 距离
    QPointF getViewPosition() const;   // 视角位置
    QPointF getMousePosition() const;  // 鼠标位置
    osgEarth::GeoPoint getViewGeoPosition() const;  // 视角地理坐标
    osgEarth::GeoPoint getMouseGeoPosition() const; // 鼠标地理坐标
    
    // 获取当前状态
    const MapStateInfo& getCurrentState() const { return currentState_; }
    
    /**
     * @brief 根据屏幕坐标获取地理坐标（统一接口，避免重复调用）
     * @param screenPos 屏幕坐标
     * @param longitude 输出经度
     * @param latitude 输出纬度
     * @param altitude 输出高度（如果转换失败，使用默认高度）
     * @return 成功返回true，失败返回false（但altitude会设置为默认值）
     */
    bool getGeoCoordinatesFromScreen(QPoint screenPos, double& longitude, double& latitude, double& altitude);

signals:
    /** @brief 状态整体变化通知（含相机与鼠标） */
    void stateChanged(const MapStateInfo& state);
    /** @brief 视角位置变化通知 */
    void viewPositionChanged(double longitude, double latitude, double altitude);
    /** @brief 鼠标地理位置变化通知 */
    void mousePositionChanged(double longitude, double latitude, double altitude);

public slots:
    // 鼠标事件处理槽函数
    void onMousePress(QMouseEvent* event);
    void onMouseMove(QMouseEvent* event);
    void onMouseRelease(QMouseEvent* event);
    void onWheelEvent(QWheelEvent* event);

private:
    osgViewer::Viewer* viewer_;
    MapStateInfo currentState_;
    
    // 高程查询相关
    osgEarth::MapNode* mapNode_;
    
    // 内部方法
    void updateState();  // 更新所有状态信息
    void updateMouseGeoPosition(QPoint mousePos);  // 更新鼠标地理坐标
    void initializeMapNode();  // 初始化MapNode
    osgEarth::Util::EarthManipulator* getEarthManipulator() const;
};

#endif // MAPSTATEMANAGER_H
