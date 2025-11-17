/**
 * @file geoutils.cpp
 * @brief 地理坐标转换工具实现文件
 * 
 * 实现GeoUtils类的所有功能
 */

#include "geoutils.h"
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QTemporaryFile>
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

/**
 * @brief 将屏幕坐标转换为地理坐标
 * 
 * 处理Qt窗口坐标系（Y=0在顶部）与OSG视口坐标系（Y=0在底部）的差异，
 * 通过射线相交检测获取地球表面的地理坐标。
 * 
 * @param viewer OSG Viewer指针
 * @param mapNode osgEarth MapNode指针
 * @param screenPos 屏幕坐标（相对于GLWidget的本地坐标）
 * @param longitude 输出经度
 * @param latitude 输出纬度
 * @param altitude 输出高度
 * @return 成功返回true，失败返回false
 */
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
            // qDebug() << "GeoUtils::screenToGeoCoordinates: 未找到与地球表面的交点";
            return false;
        }
    } catch (const std::exception& e) {
        qDebug() << "GeoUtils::screenToGeoCoordinates 异常:" << e.what();
        return false;
    }
}

/**
 * @brief 将地理坐标转换为世界坐标
 * 
 * 将经纬度坐标转换为OSG世界坐标系统（WGS84）。
 * 自动处理空间参考系统和高度模式。
 * 
 * @param longitude 经度（度）
 * @param latitude 纬度（度）
 * @param altitude 高度（米）
 * @param altMode 高度模式（默认：ALTMODE_ABSOLUTE）
 * @return 世界坐标向量，失败返回零向量
 */
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

/**
 * @brief 将Qt资源路径转换为可用的文件路径
 * 
 * 如果路径是Qt资源路径（以":/"开头），将其复制到临时文件并返回临时文件路径。
 * 如果路径是普通文件路径，直接返回原路径（如果文件存在）。
 * 
 * @param resourcePath Qt资源路径（如":/images/file.png"）或普通文件路径
 * @param errorMessage 输出错误信息（如果失败）
 * @return 成功返回可用文件路径，失败返回空字符串
 */
QString GeoUtils::convertResourcePathToFile(const QString& resourcePath, QString* errorMessage)
{
    // 如果不是Qt资源路径，直接返回原路径
    if (!resourcePath.startsWith(":/")) {
        // 检查文件是否存在
        QFileInfo fileInfo(resourcePath);
        if (!fileInfo.exists()) {
            QString error = QString("文件不存在: %1").arg(resourcePath);
            if (errorMessage) {
                *errorMessage = error;
            } else {
                qDebug() << "GeoUtils::convertResourcePathToFile:" << error;
            }
            return QString();
        }
        return resourcePath;
    }
    
    // Qt资源路径，需要复制到临时文件
    QFile resourceFile(resourcePath);
    if (!resourceFile.open(QIODevice::ReadOnly)) {
        QString error = QString("无法打开资源文件: %1").arg(resourcePath);
        if (errorMessage) {
            *errorMessage = error;
        } else {
            qDebug() << "GeoUtils::convertResourcePathToFile:" << error;
        }
        return QString();
    }
    
    // 从资源路径中提取文件扩展名
    QFileInfo fileInfo(resourcePath);
    QString suffix = fileInfo.suffix();
    if (suffix.isEmpty()) {
        // 如果无法从路径提取扩展名，尝试从文件名提取
        QString fileName = fileInfo.fileName();
        int lastDot = fileName.lastIndexOf('.');
        if (lastDot >= 0) {
            suffix = fileName.mid(lastDot + 1);
        }
    }
    
    // 创建带扩展名的临时文件模板
    QString tempTemplate = QDir::temp().absoluteFilePath("osgEarth.XXXXXX");
    if (!suffix.isEmpty()) {
        tempTemplate += "." + suffix;
    }
    
    QTemporaryFile tempFile(tempTemplate);
    // 设置不自动删除，确保文件在使用时不会被删除
    tempFile.setAutoRemove(false);
    
    if (!tempFile.open()) {
        QString error = QString("无法创建临时文件");
        if (errorMessage) {
            *errorMessage = error;
        } else {
            qDebug() << "GeoUtils::convertResourcePathToFile:" << error;
        }
        return QString();
    }
    
    // 写入资源文件内容
    QByteArray data = resourceFile.readAll();
    resourceFile.close();
    tempFile.write(data);
    tempFile.flush();  // 确保数据写入磁盘
    tempFile.close();
    
    QString tempFilePath = tempFile.fileName();
    qDebug() << "GeoUtils::convertResourcePathToFile: 资源文件已复制到临时文件:" << resourcePath << "->" << tempFilePath;
    
    return tempFilePath;
}

/**
 * @brief 加载JSON配置文件
 * 
 * 统一处理JSON文件的读取和解析，提供统一的错误处理。
 * 
 * @param filePath 文件路径
 * @param errorMessage 输出错误信息（如果失败）
 * @return 成功返回QJsonObject，失败返回空的QJsonObject（可通过isEmpty()检查）
 */
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

/**
 * @brief 计算两点间的欧几里得距离（2D，经纬度）
 * 
 * 用于简单的距离计算，适用于小范围区域。
 * 
 * @param lon1 点1经度
 * @param lat1 点1纬度
 * @param lon2 点2经度
 * @param lat2 点2纬度
 * @return 距离（度）
 */
double GeoUtils::calculateDistance2D(double lon1, double lat1, double lon2, double lat2)
{
    double dx = lon2 - lon1;
    double dy = lat2 - lat1;
    return sqrt(dx * dx + dy * dy);
}

/**
 * @brief 计算两点间的欧几里得距离（3D，包含高度）
 * 
 * @param lon1 点1经度
 * @param lat1 点1纬度
 * @param alt1 点1高度
 * @param lon2 点2经度
 * @param lat2 点2纬度
 * @param alt2 点2高度
 * @return 距离（度，高度单位为米）
 */
double GeoUtils::calculateDistance3D(double lon1, double lat1, double alt1,
                                      double lon2, double lat2, double alt2)
{
    double dx = lon2 - lon1;
    double dy = lat2 - lat1;
    double dz = alt2 - alt1;
    return sqrt(dx * dx + dy * dy + dz * dz);
}

/**
 * @brief 计算两点间的地理距离（大圆弧距离，Haversine公式）
 * 
 * 使用Haversine公式计算地球表面两点间的最短距离（大圆弧）。
 * 比简单的欧几里得距离更准确，适用于任意距离。
 * 
 * @param lon1 点1经度（度）
 * @param lat1 点1纬度（度）
 * @param lon2 点2经度（度）
 * @param lat2 点2纬度（度）
 * @return 距离（米）
 */
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

/**
 * @brief 获取Viewer的EarthManipulator
 * 
 * 统一获取并转换EarthManipulator，提供统一的空指针检查。
 * 
 * @param viewer OSG Viewer指针
 * @return EarthManipulator指针，失败返回nullptr
 */
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
