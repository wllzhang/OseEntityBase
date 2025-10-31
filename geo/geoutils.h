#ifndef GEOUTILS_H
#define GEOUTILS_H

#include <QPoint>
#include <osgViewer/Viewer>
#include <osgEarth/MapNode>

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
};

#endif // GEOUTILS_H

