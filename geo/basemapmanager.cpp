/**
 * @file basemapmanager.cpp
 * @brief 底图管理器实现文件
 * 
 * 实现BaseMapManager类的所有功能
 */

#include "basemapmanager.h"
#include <osgEarth/ImageLayer>
#include <osgEarth/Registry>
#include <osgEarth/Config>
#include <QDebug>

BaseMapManager::BaseMapManager(osgEarth::Map* map, QObject *parent)
    : QObject(parent)
    , map_(map)
    , currentBaseMapName_("无底图")
{
    if (!map_) {
        qDebug() << "BaseMapManager: Map对象为空";
        return;
    }
    
    initializeBaseMapSources();
    qDebug() << "BaseMapManager初始化完成，可用底图数量:" << baseMapSources_.size();
}

void BaseMapManager::initializeBaseMapSources()
{
    // 添加"无底图"选项（默认）
    BaseMapSource none("无底图", "", "", "", false);
    baseMapSources_.append(none);
    
    // 添加高德卫星图
    BaseMapSource satellite("卫星", "xyz", 
        "https://webst01.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}",
        "spherical-mercator", true);
    baseMapSources_.append(satellite);
    
    // 添加高德路网图
    BaseMapSource road("路网", "xyz",
        "https://webst01.is.autonavi.com/appmaptile?style=8&x={x}&y={y}&z={z}",
        "spherical-mercator", true);
    baseMapSources_.append(road);
    
    // 添加高德栅格渲染图
    BaseMapSource raster("栅格渲染", "xyz",
        "https://webrd04.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=7&x={x}&y={y}&z={z}",
        "spherical-mercator", true);
    baseMapSources_.append(raster);
    
    // 可以添加更多底图数据源...
    // 例如：OpenStreetMap、Google Maps等
}

void BaseMapManager::setDefaultBaseMap()
{
    // 默认就是无底图，只设置当前名称
    currentBaseMapName_ = "无底图";
    currentImageLayer_ = nullptr;
    qDebug() << "BaseMapManager: 设置默认底图（无底图）";
}

bool BaseMapManager::switchToBaseMap(const QString& mapName)
{
    if (!map_) {
        qDebug() << "BaseMapManager: Map对象为空，无法切换底图";
        return false;
    }
    
    // 查找对应的底图数据源
    BaseMapSource* source = nullptr;
    for (auto& s : baseMapSources_) {
        if (s.name == mapName) {
            source = &s;
            break;
        }
    }
    
    if (!source) {
        qDebug() << "BaseMapManager: 未找到底图数据源:" << mapName;
        return false;
    }
    
    // 如果是"无底图"，移除当前图层
    if (mapName == "无底图") {
        removeCurrentBaseMapLayer();
        currentBaseMapName_ = "无底图";
        currentImageLayer_ = nullptr;
        emit baseMapSwitched("无底图");
        qDebug() << "BaseMapManager: 切换到无底图";
        return true;
    }
    
    // 移除当前底图图层
    removeCurrentBaseMapLayer();
    
    // 创建新的底图图层
    osgEarth::ImageLayer* imageLayer = createImageLayer(*source);
    if (!imageLayer) {
        qDebug() << "BaseMapManager: 创建底图图层失败:" << mapName;
        return false;
    }
    
    // 添加到Map（使用addLayer方法）
    map_->addLayer(imageLayer);
    currentImageLayer_ = imageLayer;
    currentBaseMapName_ = mapName;
    
    qDebug() << "BaseMapManager: 成功切换到底图:" << mapName;
    emit baseMapSwitched(mapName);
    
    return true;
}

QList<BaseMapSource> BaseMapManager::getAvailableBaseMaps() const
{
    return baseMapSources_;
}

QString BaseMapManager::getCurrentBaseMapName() const
{
    return currentBaseMapName_;
}

void BaseMapManager::addBaseMapSource(const BaseMapSource& source)
{
    // 检查是否已存在同名底图
    for (const auto& s : baseMapSources_) {
        if (s.name == source.name) {
            qDebug() << "BaseMapManager: 底图数据源已存在:" << source.name;
            return;
        }
    }
    
    baseMapSources_.append(source);
    qDebug() << "BaseMapManager: 添加底图数据源:" << source.name;
}

osgEarth::ImageLayer* BaseMapManager::createImageLayer(const BaseMapSource& source)
{
    if (source.driver.isEmpty() || source.url.isEmpty()) {
        qDebug() << "BaseMapManager: 底图数据源配置不完整";
        return nullptr;
    }
    
    // 使用Config对象创建ImageLayer配置（类似.earth文件的格式）
    osgEarth::Config layerConfig;
    layerConfig.set("name", source.name.toStdString());
    layerConfig.set("driver", source.driver.toStdString());
    layerConfig.set("url", source.url.toStdString());
    
    // 设置投影配置
    if (!source.profile.isEmpty()) {
        layerConfig.set("profile", source.profile.toStdString());
    }
    
    // 设置缓存
    if (source.cacheEnabled) {
        layerConfig.set("cache_enabled", "true");
    }
    
    // 创建ImageLayerOptions
    osgEarth::ImageLayerOptions options(layerConfig);
    
    // 创建ImageLayer
    osgEarth::ImageLayer* layer = new osgEarth::ImageLayer(options);
    
    if (!layer || !layer->getStatus().isOK()) {
        qDebug() << "BaseMapManager: ImageLayer创建失败或状态异常";
        if (layer) {
            qDebug() << "BaseMapManager: 错误信息:" << layer->getStatus().message().c_str();
            layer->unref();
        }
        return nullptr;
    }
    
    return layer;
}

void BaseMapManager::removeCurrentBaseMapLayer()
{
    if (currentImageLayer_.valid() && map_) {
        // 使用removeLayer方法移除图层
        map_->removeLayer(currentImageLayer_.get());
        currentImageLayer_ = nullptr;
        qDebug() << "BaseMapManager: 移除当前底图图层";
    }
}

