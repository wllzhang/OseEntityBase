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

// 地图状态信息结构体 - 9元组信息
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
          viewLongitude(116.4), viewLatitude(39.9), viewAltitude(0.0),
          mouseLongitude(116.4), mouseLatitude(39.9), mouseAltitude(0.0) {}
    
    // 获取9元组信息 (a,b,c,x1,y1,z1,x2,y2,z2)
    std::tuple<double,double,double,double,double,double,double,double,double> getTuple() const {
        return std::make_tuple(pitch, heading, range, 
                              viewLongitude, viewLatitude, viewAltitude,
                              mouseLongitude, mouseLatitude, mouseAltitude);
    }
};

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

signals:
    void stateChanged(const MapStateInfo& state);
    void viewPositionChanged(double longitude, double latitude, double altitude);
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
