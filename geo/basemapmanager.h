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
#include <QStringList>
#include <QList>
#include <QMap>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPair>
#include <osgEarth/Map>
#include <osg/ref_ptr>

// 前向声明
namespace osgEarth {
    class ImageLayer;
}

/**
 * @brief 底图数据源配置结构
 */
struct BaseMapSource {
    QString name;           // 底图名称（如"卫星"、"路网"等）
    QString driver;         // 驱动类型（"xyz"、"gdal"等）
    QString url;            // 数据源URL
    QString profile;        // 投影配置（如"spherical-mercator"）
    bool cacheEnabled;      // 是否启用缓存
    QString format;         // 格式（用于高程数据等，可选）
    bool visible;            // 是否可见（用于叠加显示控制）
    int opacity;             // 透明度（0-100）
    
    BaseMapSource() : cacheEnabled(false), visible(true), opacity(100) {}
    BaseMapSource(const QString& n, const QString& d, const QString& u, 
                  const QString& p = "spherical-mercator", bool cache = true, bool vis = true, int op = 100)
        : name(n), driver(d), url(u), profile(p), cacheEnabled(cache), visible(vis), opacity(op) {}
    
    // 转换为JSON对象
    QJsonObject toJson() const;
    // 从JSON对象加载
    static BaseMapSource fromJson(const QJsonObject& json);
};

/**
 * @brief 底图管理器
 * 
 * 负责管理底图数据源的配置，支持多个底图图层叠加显示
 * 支持用户新增、修改、删除底图配置，并支持配置的保存和加载
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
     * @brief 添加底图图层（叠加显示）
     * @param source 底图数据源配置
     * @return 成功返回true，失败返回false
     */
    bool addBaseMapLayer(const BaseMapSource& source);
    
    /**
     * @brief 移除底图图层
     * @param mapName 底图名称
     * @return 成功返回true，失败返回false
     */
    bool removeBaseMapLayer(const QString& mapName);
    
    /**
     * @brief 更新底图图层配置
     * @param oldName 原底图名称
     * @param source 新的底图配置
     * @return 成功返回true，失败返回false
     */
    bool updateBaseMapLayer(const QString& oldName, const BaseMapSource& source);
    
    /**
     * @brief 设置底图图层可见性
     * @param mapName 底图名称
     * @param visible 是否可见
     * @return 成功返回true，失败返回false
     */
    bool setBaseMapVisible(const QString& mapName, bool visible);
    
    /**
     * @brief 设置底图图层透明度
     * @param mapName 底图名称
     * @param opacity 透明度（0-100）
     * @return 成功返回true，失败返回false
     */
    bool setBaseMapOpacity(const QString& mapName, int opacity);
    
    /**
     * @brief 获取所有已加载的底图图层信息
     * @return 底图图层信息列表（名称和配置）
     */
    QList<QPair<QString, BaseMapSource>> getLoadedBaseMaps() const;
    
    /**
     * @brief 获取所有可用的底图配置模板（预定义）
     * @return 底图数据源列表
     */
    QList<BaseMapSource> getAvailableBaseMapTemplates() const;
    
    /**
     * @brief 检查底图名称是否已存在
     * @param name 底图名称
     * @return 存在返回true
     */
    bool hasBaseMap(const QString& name) const;
    
    /**
     * @brief 获取底图配置
     * @param name 底图名称
     * @return 底图配置，不存在返回空配置
     */
    BaseMapSource getBaseMapConfig(const QString& name) const;
    
    /**
     * @brief 保存配置到文件
     * @param filePath 配置文件路径
     * @return 成功返回true
     */
    bool saveConfig(const QString& filePath) const;
    
    /**
     * @brief 从文件加载配置
     * @param filePath 配置文件路径
     * @return 成功返回true
     */
    bool loadConfig(const QString& filePath);
    
    /**
     * @brief 将底图图层上移（在叠加顺序中向上移动，显示更上层）
     * @param mapName 底图名称
     * @return 成功返回true
     */
    bool moveLayerUp(const QString& mapName);
    
    /**
     * @brief 将底图图层下移（在叠加顺序中向下移动，显示更下层）
     * @param mapName 底图名称
     * @return 成功返回true
     */
    bool moveLayerDown(const QString& mapName);
    
    /**
     * @brief 获取底图图层的叠加顺序（从上到下，索引0是最上层）
     * @return 底图名称列表，列表第一行（索引0）是最上层
     */
    QStringList getLayerOrder() const;

signals:
    /**
     * @brief 底图图层添加完成信号
     * @param mapName 底图名称
     */
    void baseMapAdded(const QString& mapName);
    
    /**
     * @brief 底图图层移除完成信号
     * @param mapName 底图名称
     */
    void baseMapRemoved(const QString& mapName);
    
    /**
     * @brief 底图图层更新完成信号
     * @param mapName 底图名称
     */
    void baseMapUpdated(const QString& mapName);
    
    /**
     * @brief 底图图层可见性改变信号
     * @param mapName 底图名称
     * @param visible 是否可见
     */
    void baseMapVisibilityChanged(const QString& mapName, bool visible);

private:
    /**
     * @brief 初始化预定义的底图数据源模板
     */
    void initializeBaseMapTemplates();
    
    /**
     * @brief 创建ImageLayer
     * @param source 底图数据源配置
     * @return ImageLayer指针，失败返回nullptr
     */
    osgEarth::ImageLayer* createImageLayer(const BaseMapSource& source);
    
    /**
     * @brief 根据名称查找图层
     * @param name 底图名称
     * @return ImageLayer指针，不存在返回nullptr
     */
    osgEarth::ImageLayer* findLayerByName(const QString& name) const;

    /**
     * @brief 重新排序所有图层（根据layerOrder_的顺序）
     */
    void reorderLayers();
    
    osgEarth::Map* map_;                           // osgEarth Map对象
    QList<BaseMapSource> baseMapTemplates_;         // 预定义的底图配置模板
    QMap<QString, osg::ref_ptr<osgEarth::ImageLayer>> loadedLayers_;  // 已加载的底图图层（名称->图层）
    QMap<QString, BaseMapSource> loadedConfigs_;   // 已加载的底图配置（名称->配置）
    QStringList layerOrder_;                       // 图层顺序列表（从上到下，索引0是最上层，对应列表第一行）
    QString configFilePath_;                       // 配置文件路径
};

#endif // BASEMAPMANAGER_H

