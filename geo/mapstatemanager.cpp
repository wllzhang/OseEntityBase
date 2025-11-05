/**
 * @file mapstatemanager.cpp
 * @brief 地图状态管理器实现文件
 * 
 * 实现MapStateManager类的所有功能
 */

#include "mapstatemanager.h"
#include "geoutils.h"
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
    // qDebug() << "MapStateManager::onMousePress 被调用" << event->pos();
    // 更新鼠标地理坐标
    updateMouseGeoPosition(event->pos());
    // 更新状态
    updateState();
}

void MapStateManager::onMouseMove(QMouseEvent* event)
{
    // qDebug() << "MapStateManager::onMouseMove 被调用" << event->pos();
    // 更新鼠标地理坐标
    updateMouseGeoPosition(event->pos());
    // 更新状态
    updateState();
    
    // 发出状态变化信号
    emit stateChanged(currentState_);
}

void MapStateManager::onMouseRelease(QMouseEvent* event)
{
    // qDebug() << "MapStateManager::onMouseRelease 被调用" << event->pos();
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
    
    // qDebug() << "滚轮事件，当前距离:" << currentState_.range;
}

/**
 * @brief 更新所有状态信息
 * 
 * 从EarthManipulator获取当前的相机参数（俯仰角、航向角、距离）和视角位置，
 * 更新到currentState_中，并发出相应的信号。
 * 
 * @note 鼠标地理坐标已在updateMouseGeoPosition中更新
 */
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

/**
 * @brief 初始化MapNode
 * 
 * 从Viewer的场景图中查找MapNode，用于后续的地理坐标转换。
 * 
 * @note 如果MapNode未找到，某些功能（如鼠标坐标转换）可能无法使用
 */
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

/**
 * @brief 更新鼠标地理坐标
 * 
 * 将屏幕坐标转换为地理坐标，更新到currentState_中，
 * 并发出mousePositionChanged信号。
 * 
 * @param mousePos 鼠标屏幕坐标
 */
void MapStateManager::updateMouseGeoPosition(QPoint mousePos)
{
    // 使用工具函数进行坐标转换
    if (GeoUtils::screenToGeoCoordinates(viewer_, mapNode_, mousePos, 
                                         currentState_.mouseLongitude, 
                                         currentState_.mouseLatitude, 
                                         currentState_.mouseAltitude)) {
        // 发出鼠标位置变化信号
        emit mousePositionChanged(currentState_.mouseLongitude, 
                                  currentState_.mouseLatitude, 
                                  currentState_.mouseAltitude);
    } else {
        // 如果转换失败，使用视角坐标作为默认值
        currentState_.mouseLongitude = currentState_.viewLongitude;
        currentState_.mouseLatitude = currentState_.viewLatitude;
        currentState_.mouseAltitude = currentState_.viewAltitude;
    }
}

osgEarth::Util::EarthManipulator* MapStateManager::getEarthManipulator() const
{
    // 使用工具函数统一获取EarthManipulator
    return GeoUtils::getEarthManipulator(viewer_);
}