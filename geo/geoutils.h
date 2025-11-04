/**
 * @file geoutils.h
 * @brief 地理坐标转换工具头文件
 * 
 * 定义GeoUtils工具类，提供各种地理坐标转换和计算功能
 */

#ifndef GEOUTILS_H
#define GEOUTILS_H

#include <QPoint>
#include <QString>
#include <QJsonObject>
#include <osgViewer/Viewer>
#include <osgEarth/MapNode>
#include <osgEarthUtil/EarthManipulator>
#include <osg/Vec3d>

/**
 * @ingroup managers
 * @brief 地理坐标转换工具类
 * 
 * 提供屏幕坐标到地理坐标的转换功能，处理Qt和OSG之间的坐标系统差异。
 */
class GeoUtils
{
public:
    /**
     * @brief 将屏幕坐标转换为地理坐标
     * 
     * 将Qt窗口坐标（Y=0在顶部）转换为OSG视口坐标（Y=0在底部），
     * 然后通过射线相交检测获取地球表面的地理坐标。
     * 
     * @param viewer OSG Viewer指针
     * @param mapNode osgEarth MapNode指针
     * @param screenPos 屏幕坐标（相对于GLWidget的本地坐标）
     * @param longitude 输出经度
     * @param latitude 输出纬度
     * @param altitude 输出高度
     * @return 成功返回true，失败返回false
     */
    static bool screenToGeoCoordinates(
        osgViewer::Viewer* viewer,
        osgEarth::MapNode* mapNode,
        QPoint screenPos,
        double& longitude,
        double& latitude,
        double& altitude);
    
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
    static osg::Vec3d geoToWorldCoordinates(
        double longitude,
        double latitude,
        double altitude,
        osgEarth::AltitudeMode altMode = osgEarth::ALTMODE_ABSOLUTE);
    
    /**
     * @brief 将Qt资源路径转换为可用的文件路径
     * 
     * 如果路径是Qt资源路径（以":/"开头），将其复制到临时文件并返回临时文件路径。
     * 如果路径是普通文件路径，直接返回原路径。
     * 主要用于OSG等不支持Qt资源系统的库。
     * 
     * @param resourcePath Qt资源路径（如":/images/file.png"）或普通文件路径
     * @param errorMessage 输出错误信息（如果失败）
     * @return 成功返回可用文件路径，失败返回空字符串
     */
    static QString convertResourcePathToFile(const QString& resourcePath, QString* errorMessage = nullptr);
    
    /**
     * @brief 加载JSON配置文件
     * 
     * 统一处理JSON文件的读取和解析，提供统一的错误处理。
     * 
     * @param filePath 文件路径
     * @param errorMessage 输出错误信息（如果失败）
     * @return 成功返回QJsonObject，失败返回空的QJsonObject（可通过isEmpty()检查）
     */
    static QJsonObject loadJsonFile(const QString& filePath, QString* errorMessage = nullptr);
    
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
    static double calculateDistance2D(double lon1, double lat1, double lon2, double lat2);
    
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
    static double calculateDistance3D(double lon1, double lat1, double alt1,
                                      double lon2, double lat2, double alt2);
    
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
    static double calculateGeographicDistance(double lon1, double lat1, double lon2, double lat2);
    
    /**
     * @brief 获取Viewer的EarthManipulator
     * 
     * 统一获取并转换EarthManipulator，提供统一的空指针检查。
     * 
     * @param viewer OSG Viewer指针
     * @return EarthManipulator指针，失败返回nullptr
     */
    static osgEarth::Util::EarthManipulator* getEarthManipulator(osgViewer::Viewer* viewer);
};

#endif // GEOUTILS_H

