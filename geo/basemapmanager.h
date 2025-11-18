/**
 * @file basemapmanager.h
 * @brief 底图管理器头文件
 * 
 * 定义BaseMapManager类，用于管理底图数据源的配置和切换
 */

#ifndef BASEMAPMANAGER_H
#define BASEMAPMANAGER_H

#include <QObject>
#include <QString>
#include <QList>
#include <osgEarth/Map>

// 前向声明
namespace osgEarth {
    class ImageLayer;
}

/**
 * @brief 底图数据源配置结构
 */
struct BaseMapSource {
    QString name;           // 底图名称（如"无底图"、"卫星"、"路网"等）
    QString driver;         // 驱动类型（"xyz"、"gdal"等）
    QString url;            // 数据源URL
    QString profile;        // 投影配置（如"spherical-mercator"）
    bool cacheEnabled;      // 是否启用缓存
    QString format;         // 格式（用于高程数据等，可选）
    
    BaseMapSource() : cacheEnabled(false) {}
    BaseMapSource(const QString& n, const QString& d, const QString& u, 
                  const QString& p = "spherical-mercator", bool cache = true)
        : name(n), driver(d), url(u), profile(p), cacheEnabled(cache) {}
};

/**
 * @brief 底图管理器
 * 
 * 负责管理底图数据源的配置和切换，支持多种底图数据源类型
 */
class BaseMapManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param map osgEarth Map对象
     * @param parent Qt父对象
     */
    explicit BaseMapManager(osgEarth::Map* map, QObject *parent = nullptr);
    
    /**
     * @brief 设置默认底图（无底图，只有地球）
     * 
     * 对应 my.earth 文件的默认状态（所有image层都被注释）
     */
    void setDefaultBaseMap();
    
    /**
     * @brief 切换到底图
     * @param mapName 底图名称
     * @return 成功返回true，失败返回false
     */
    bool switchToBaseMap(const QString& mapName);
    
    /**
     * @brief 获取所有可用的底图列表
     * @return 底图数据源列表
     */
    QList<BaseMapSource> getAvailableBaseMaps() const;
    
    /**
     * @brief 获取当前底图名称
     * @return 当前底图名称，如果没有底图返回"无底图"
     */
    QString getCurrentBaseMapName() const;
    
    /**
     * @brief 添加自定义底图数据源
     * @param source 底图数据源配置
     */
    void addBaseMapSource(const BaseMapSource& source);

signals:
    /**
     * @brief 底图切换完成信号
     * @param mapName 切换后的底图名称
     */
    void baseMapSwitched(const QString& mapName);

private:
    /**
     * @brief 初始化预定义的底图数据源
     */
    void initializeBaseMapSources();
    
    /**
     * @brief 创建ImageLayer
     * @param source 底图数据源配置
     * @return ImageLayer指针，失败返回nullptr
     */
    osgEarth::ImageLayer* createImageLayer(const BaseMapSource& source);
    
    /**
     * @brief 移除当前底图图层
     */
    void removeCurrentBaseMapLayer();

    osgEarth::Map* map_;                           // osgEarth Map对象
    QList<BaseMapSource> baseMapSources_;          // 所有可用的底图数据源
    QString currentBaseMapName_;                   // 当前底图名称
    osg::ref_ptr<osgEarth::ImageLayer> currentImageLayer_;  // 当前底图图层
};

#endif // BASEMAPMANAGER_H

