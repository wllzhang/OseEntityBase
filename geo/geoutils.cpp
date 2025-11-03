#include "geoutils.h"
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QJsonParseError>
#include <osg/Viewport>
#include <osgUtil/LineSegmentIntersector>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

bool GeoUtils::screenToGeoCoordinates(
    osgViewer::Viewer* viewer,
    osgEarth::MapNode* mapNode,
    QPoint screenPos,
    double& longitude,
    double& latitude,
    double& altitude)
{
    // 参数检查
    if (!viewer || !viewer->getCamera() || !mapNode) {
        qDebug() << "GeoUtils::screenToGeoCoordinates: Viewer、Camera或MapNode未初始化";
        return false;
    }
    
    try {
        // 获取相机视口，用于Y轴翻转
        osg::Camera* camera = viewer->getCamera();
        osg::Viewport* viewport = camera ? camera->getViewport() : nullptr;
        if (!viewport) {
            qDebug() << "GeoUtils::screenToGeoCoordinates: 无法获取视口";
            return false;
        }
        
        // Y轴翻转：Qt窗口坐标Y=0在顶部向下增加，OSG视口坐标Y=0在底部向上增加
        // 需要进行翻转：flippedY = viewportHeight - screenPos.y() - 1
        int viewportHeight = static_cast<int>(viewport->height());
        int flippedY = viewportHeight - screenPos.y() - 1;
        
        // 创建射线相交检测器
        osgUtil::LineSegmentIntersector::Intersections intersections;
        
        // 计算鼠标位置与地球表面的交点
        // computeIntersections 期望的是相对于 OSG 视口（GLWidget）的坐标，Y轴已经翻转
        if (viewer->computeIntersections(screenPos.x(), flippedY, intersections)) {
            // 获取第一个交点
            const osgUtil::LineSegmentIntersector::Intersection& intersection = *intersections.begin();
            osg::Vec3 worldPos = intersection.getWorldIntersectPoint();
            
            // 将世界坐标转换为地理坐标
            osg::Vec3d geoVec;
            if (mapNode->getMapSRS()->transformFromWorld(worldPos, geoVec)) {
                longitude = geoVec.x();
                latitude = geoVec.y();
                altitude = geoVec.z();
                
                // qDebug() << "GeoUtils::screenToGeoCoordinates 转换成功:"
                //          << screenPos << "-> 经度:" << longitude << "纬度:" << latitude << "高度:" << altitude;
                return true;
            } else {
                qDebug() << "GeoUtils::screenToGeoCoordinates: 世界坐标转换为地理坐标失败";
                return false;
            }
        } else {
            qDebug() << "GeoUtils::screenToGeoCoordinates: 未找到与地球表面的交点";
            return false;
        }
    } catch (const std::exception& e) {
        qDebug() << "GeoUtils::screenToGeoCoordinates 异常:" << e.what();
        return false;
    }
}

osg::Vec3d GeoUtils::geoToWorldCoordinates(
    double longitude,
    double latitude,
    double altitude,
    osgEarth::AltitudeMode altMode)
{
    try {
        // 创建地理坐标点（WGS84坐标系）
        osgEarth::GeoPoint geoPoint(
            osgEarth::SpatialReference::get("wgs84"),
            longitude,
            latitude,
            altitude,
            altMode);
        
        // 转换为世界坐标
        osg::Vec3d worldPos;
        geoPoint.toWorld(worldPos);
        
        return worldPos;
    } catch (const std::exception& e) {
        qDebug() << "GeoUtils::geoToWorldCoordinates 异常:" << e.what();
        return osg::Vec3d(0.0, 0.0, 0.0);
    }
}

QJsonObject GeoUtils::loadJsonFile(const QString& filePath, QString* errorMessage)
{
    QJsonObject result;
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QString error = QString("无法打开文件: %1").arg(filePath);
        if (errorMessage) {
            *errorMessage = error;
        } else {
            qDebug() << "GeoUtils::loadJsonFile:" << error;
        }
        return result; // 返回空的QJsonObject
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("JSON解析错误: %1").arg(parseError.errorString());
        if (errorMessage) {
            *errorMessage = error;
        } else {
            qDebug() << "GeoUtils::loadJsonFile:" << error;
        }
        return result; // 返回空的QJsonObject
    }
    
    result = doc.object();
    return result;
}

double GeoUtils::calculateDistance2D(double lon1, double lat1, double lon2, double lat2)
{
    double dx = lon2 - lon1;
    double dy = lat2 - lat1;
    return sqrt(dx * dx + dy * dy);
}

double GeoUtils::calculateDistance3D(double lon1, double lat1, double alt1,
                                      double lon2, double lat2, double alt2)
{
    double dx = lon2 - lon1;
    double dy = lat2 - lat1;
    double dz = alt2 - alt1;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

double GeoUtils::calculateGeographicDistance(double lon1, double lat1, double lon2, double lat2)
{
    // Haversine公式计算大圆弧距离
    const double R = 6378137.0; // 地球半径（米）
    
    double dLat = (lat2 - lat1) * M_PI / 180.0;
    double dLon = (lon2 - lon1) * M_PI / 180.0;
    
    double a = sin(dLat / 2.0) * sin(dLat / 2.0) +
               cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
               sin(dLon / 2.0) * sin(dLon / 2.0);
    
    double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
    
    return R * c; // 返回米
}

osgEarth::Util::EarthManipulator* GeoUtils::getEarthManipulator(osgViewer::Viewer* viewer)
{
    if (!viewer) {
        qDebug() << "GeoUtils::getEarthManipulator: Viewer为空";
        return nullptr;
    }
    
    osgGA::CameraManipulator* manipulator = viewer->getCameraManipulator();
    if (!manipulator) {
        qDebug() << "GeoUtils::getEarthManipulator: CameraManipulator为空";
        return nullptr;
    }
    
    osgEarth::Util::EarthManipulator* em = 
        dynamic_cast<osgEarth::Util::EarthManipulator*>(manipulator);
    
    if (!em) {
        qDebug() << "GeoUtils::getEarthManipulator: CameraManipulator不是EarthManipulator类型";
    }
    
    return em;
}
