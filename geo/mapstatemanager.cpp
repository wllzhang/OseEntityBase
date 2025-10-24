#include "mapstatemanager.h"
#include <QDebug>
#include <QApplication>
#include <osgEarth/SpatialReference>
#include <osgEarth/MapNode>
#include <osgEarth/Utils>
#include <osgEarth/GeoData>
#include <osgEarth/Units>
#include <osgUtil/LineSegmentIntersector>
#include <osgEarth/MapNode>

MapStateManager::MapStateManager(osgViewer::Viewer* viewer, QObject* parent)
    : QObject(parent)
    , viewer_(viewer)
    , mapNode_(nullptr)
{
    qDebug() << "MapStateManager初始化完成";
    
    // 初始化MapNode
    initializeMapNode();
    
    // 更新初始状态
    updateState();
}

MapStateManager::~MapStateManager()
{
    qDebug() << "MapStateManager析构完成";
}

std::tuple<double,double,double,double,double,double,double,double,double> MapStateManager::getStateTuple() const
{
    return currentState_.getTuple();
}

double MapStateManager::getPitch() const
{
    return currentState_.pitch;
}

double MapStateManager::getHeading() const
{
    return currentState_.heading;
}

double MapStateManager::getRange() const
{
    return currentState_.range;
}

QPointF MapStateManager::getViewPosition() const
{
    return QPointF(currentState_.viewLongitude, currentState_.viewLatitude);
}

QPointF MapStateManager::getMousePosition() const
{
    return QPointF(currentState_.mouseLongitude, currentState_.mouseLatitude);
}

osgEarth::GeoPoint MapStateManager::getViewGeoPosition() const
{
    return osgEarth::GeoPoint(osgEarth::SpatialReference::get("wgs84"), 
                             currentState_.viewLongitude, currentState_.viewLatitude, currentState_.viewAltitude);
}

osgEarth::GeoPoint MapStateManager::getMouseGeoPosition() const
{
    return osgEarth::GeoPoint(osgEarth::SpatialReference::get("wgs84"), 
                             currentState_.mouseLongitude, currentState_.mouseLatitude, currentState_.mouseAltitude);
}

void MapStateManager::onMousePress(QMouseEvent* event)
{
    qDebug() << "MapStateManager::onMousePress 被调用" << event->pos();
    // 更新鼠标地理坐标
    updateMouseGeoPosition(event->pos());
    // 更新状态
    updateState();
}

void MapStateManager::onMouseMove(QMouseEvent* event)
{
    qDebug() << "MapStateManager::onMouseMove 被调用" << event->pos();
    // 更新鼠标地理坐标
    updateMouseGeoPosition(event->pos());
    // 更新状态
    updateState();
    
    // 发出状态变化信号
    emit stateChanged(currentState_);
}

void MapStateManager::onMouseRelease(QMouseEvent* event)
{
    qDebug() << "MapStateManager::onMouseRelease 被调用" << event->pos();
    // 更新鼠标地理坐标
    updateMouseGeoPosition(event->pos());
    // 更新状态
    updateState();
}

void MapStateManager::onWheelEvent(QWheelEvent* event)
{
    Q_UNUSED(event)
    // 更新状态
    updateState();
    
    // 发出状态变化信号
    emit stateChanged(currentState_);
    
    qDebug() << "滚轮事件，当前距离:" << currentState_.range;
}

// 更新所有状态信息
void MapStateManager::updateState()
{
    if (!viewer_ || !viewer_->getCamera()) {
        return;
    }
    
    try {
        osgEarth::Util::EarthManipulator* manip = getEarthManipulator();
        if (manip) {
            // 获取当前视点
            osgEarth::Viewpoint vp = manip->getViewpoint();
            
            // 更新距离
            auto rangeOpt = vp.range();
            if (rangeOpt.isSet()) {
                currentState_.range = rangeOpt.get().as(osgEarth::Units::METERS);
            }
            
            // 更新角度信息
            auto pitchOpt = vp.pitch();
            if (pitchOpt.isSet()) {
                currentState_.pitch = pitchOpt.get().as(osgEarth::Units::DEGREES);
            }
            
            auto headingOpt = vp.heading();
            if (headingOpt.isSet()) {
                currentState_.heading = headingOpt.get().as(osgEarth::Units::DEGREES);
            }
            
            // 更新视角地理坐标 (窗口中心)
            auto focalPointOpt = vp.focalPoint();
            if (focalPointOpt.isSet()) {
                osgEarth::GeoPoint geoPoint = focalPointOpt.get();
                currentState_.viewLongitude = geoPoint.x();
                currentState_.viewLatitude = geoPoint.y();
                currentState_.viewAltitude = geoPoint.z();
                
                // 发出视角位置变化信号
                emit viewPositionChanged(currentState_.viewLongitude, currentState_.viewLatitude, currentState_.viewAltitude);
            }
            
            // 鼠标地理坐标已在updateMouseGeoPosition中更新
        }
    } catch (const std::exception& e) {
        qDebug() << "更新状态时发生异常:" << e.what();
    }
}

// 初始化MapNode
void MapStateManager::initializeMapNode()
{
    if (!viewer_) {
        qDebug() << "Viewer未初始化，无法初始化MapNode";
        return;
    }
    
    try {
        // 获取MapNode
        mapNode_ = osgEarth::MapNode::findMapNode(viewer_->getSceneData());
        if (mapNode_) {
            qDebug() << "MapNode初始化成功";
        } else {
            qDebug() << "无法找到MapNode";
        }
    } catch (const std::exception& e) {
        qDebug() << "初始化MapNode时发生异常:" << e.what();
    }
}

// 更新鼠标地理坐标 (简化实现)
void MapStateManager::updateMouseGeoPosition(QPoint mousePos)
{
    if (!viewer_ || !viewer_->getCamera() || !mapNode_) {
        return;
    }
    
    try {
        // 创建射线相交检测器
        osgUtil::LineSegmentIntersector::Intersections intersections;
        
        // 计算鼠标位置与地球表面的交点
        if (viewer_->computeIntersections(mousePos.x(), mousePos.y(), intersections)) {
            // 获取第一个交点
            const osgUtil::LineSegmentIntersector::Intersection& intersection = *intersections.begin();
            osg::Vec3 worldPos = intersection.getWorldIntersectPoint();
            
            // 将世界坐标转换为地理坐标
            osg::Vec3d geoVec;
            if (mapNode_->getMapSRS()->transformFromWorld(worldPos, geoVec)) {
                currentState_.mouseLongitude = geoVec.x();
                currentState_.mouseLatitude = geoVec.y();
                currentState_.mouseAltitude = geoVec.z();
                
                // 发出鼠标位置变化信号
                emit mousePositionChanged(currentState_.mouseLongitude, currentState_.mouseLatitude, currentState_.mouseAltitude);
                
                qDebug() << "鼠标地理坐标更新:" << currentState_.mouseLongitude << currentState_.mouseLatitude << currentState_.mouseAltitude;
            } else {
                qDebug() << "世界坐标转换为地理坐标失败";
                // 使用视角坐标作为默认值
                currentState_.mouseLongitude = currentState_.viewLongitude;
                currentState_.mouseLatitude = currentState_.viewLatitude;
                currentState_.mouseAltitude = currentState_.viewAltitude;
            }
        } else {
            // 如果没有交点，使用视角坐标作为默认值
            currentState_.mouseLongitude = currentState_.viewLongitude;
            currentState_.mouseLatitude = currentState_.viewLatitude;
            currentState_.mouseAltitude = currentState_.viewAltitude;
        }
    } catch (const std::exception& e) {
        qDebug() << "更新鼠标地理坐标时发生异常:" << e.what();
        // 使用视角坐标作为默认值
        currentState_.mouseLongitude = currentState_.viewLongitude;
        currentState_.mouseLatitude = currentState_.viewLatitude;
        currentState_.mouseAltitude = currentState_.viewAltitude;
    }
}

osgEarth::Util::EarthManipulator* MapStateManager::getEarthManipulator() const
{
    if (!viewer_ || !viewer_->getCameraManipulator()) {
        return nullptr;
    }
    
    return dynamic_cast<osgEarth::Util::EarthManipulator*>(viewer_->getCameraManipulator());
}