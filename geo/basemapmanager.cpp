/**
 * @file basemapmanager.cpp
 * @brief 底图管理器实现文件
 * 
 * 实现BaseMapManager类的所有功能，支持多图层叠加显示和配置管理
 */

#include "basemapmanager.h"
#include <osgEarth/ImageLayer>
#include <osgEarth/Registry>
#include <osgEarth/Config>
#include <QDebug>
#include <QFile>
#include <QJsonDocument>
#include <QDir>
#include <QStandardPaths>

// BaseMapSource JSON序列化实现
QJsonObject BaseMapSource::toJson() const
{
    QJsonObject json;
    json["name"] = name;
    json["driver"] = driver;
    json["url"] = url;
    json["profile"] = profile;
    json["cacheEnabled"] = cacheEnabled;
    json["format"] = format;
    json["visible"] = visible;
    json["opacity"] = opacity;
    return json;
}

BaseMapSource BaseMapSource::fromJson(const QJsonObject& json)
{
    BaseMapSource source;
    source.name = json["name"].toString();
    source.driver = json["driver"].toString();
    source.url = json["url"].toString();
    source.profile = json["profile"].toString();
    source.cacheEnabled = json["cacheEnabled"].toBool();
    source.format = json["format"].toString();
    source.visible = json["visible"].toBool(true);
    source.opacity = json["opacity"].toInt(100);
    return source;
}

BaseMapManager::BaseMapManager(osgEarth::Map* map, QObject *parent)
    : QObject(parent)
    , map_(map)
{
    if (!map_) {
        qDebug() << "BaseMapManager: Map对象为空";
        return;
    }
    
    // 设置默认配置文件路径（在应用程序目录）
    configFilePath_ = QDir::currentPath() + "/basemap_config.json";
    
    initializeBaseMapTemplates();
    qDebug() << "BaseMapManager初始化完成，可用模板数量:" << baseMapTemplates_.size();
    
    // 尝试加载保存的配置
    if (QFile::exists(configFilePath_)) {
        loadConfig(configFilePath_);
    }
}

void BaseMapManager::initializeBaseMapTemplates()
{
    // 添加高德卫星图模板
    BaseMapSource satellite("卫星", "xyz", 
        "https://webst01.is.autonavi.com/appmaptile?style=6&x={x}&y={y}&z={z}",
        "spherical-mercator", true, true, 100);
    baseMapTemplates_.append(satellite);
    
    // 添加高德路网图模板
    BaseMapSource road("路网", "xyz",
        "https://webst01.is.autonavi.com/appmaptile?style=8&x={x}&y={y}&z={z}",
        "spherical-mercator", true, true, 100);
    baseMapTemplates_.append(road);
    
    // 添加高德栅格渲染图模板
    BaseMapSource raster("栅格渲染", "xyz",
        "https://webrd04.is.autonavi.com/appmaptile?lang=zh_cn&size=1&scale=1&style=7&x={x}&y={y}&z={z}",
        "spherical-mercator", true, true, 100);
    baseMapTemplates_.append(raster);
}

bool BaseMapManager::addBaseMapLayer(const BaseMapSource& source)
{
    if (!map_) {
        qDebug() << "BaseMapManager: Map对象为空";
        return false;
    }
    
    if (source.name.isEmpty()) {
        qDebug() << "BaseMapManager: 底图名称不能为空";
        return false;
    }
    
    // 检查是否已存在同名底图
    if (hasBaseMap(source.name)) {
        qDebug() << "BaseMapManager: 底图已存在:" << source.name;
        return false;
    }
    
    // 创建ImageLayer
    osgEarth::ImageLayer* layer = createImageLayer(source);
    if (!layer) {
        qDebug() << "BaseMapManager: 创建底图图层失败:" << source.name;
        return false;
    }
    
    // 设置可见性和透明度
    if (layer) {
        layer->setVisible(source.visible);
        layer->setOpacity(qBound(0.0, source.opacity / 100.0, 1.0));
    }
    
    // 添加到Map（叠加显示）
    map_->addLayer(layer);
    
    // 保存到已加载列表
    loadedLayers_[source.name] = layer;
    loadedConfigs_[source.name] = source;
    
    // 添加到顺序列表（新图层默认在最上层，即列表第一行）
    layerOrder_.prepend(source.name);
    
    qDebug() << "BaseMapManager: 成功添加底图图层:" << source.name;
    emit baseMapAdded(source.name);
    
    return true;
}

bool BaseMapManager::removeBaseMapLayer(const QString& mapName)
{
    if (!map_ || !hasBaseMap(mapName)) {
        qDebug() << "BaseMapManager: 底图不存在:" << mapName;
        return false;
    }
    
    osgEarth::ImageLayer* layer = loadedLayers_[mapName].get();
    if (layer) {
        map_->removeLayer(layer);
    }
    
    loadedLayers_.remove(mapName);
    loadedConfigs_.remove(mapName);
    layerOrder_.removeAll(mapName);  // 从顺序列表中移除
    
    qDebug() << "BaseMapManager: 成功移除底图图层:" << mapName;
    emit baseMapRemoved(mapName);
    
    return true;
}

bool BaseMapManager::updateBaseMapLayer(const QString& oldName, const BaseMapSource& source)
{
    if (!map_ || !hasBaseMap(oldName)) {
        qDebug() << "BaseMapManager: 底图不存在:" << oldName;
        return false;
    }
    
    // 如果名称改变且新名称已存在，返回失败
    if (oldName != source.name && hasBaseMap(source.name)) {
        qDebug() << "BaseMapManager: 新名称已存在:" << source.name;
        return false;
    }
    
    // 先移除旧图层
    removeBaseMapLayer(oldName);
    
    // 添加新图层
    bool success = addBaseMapLayer(source);
    
    if (success) {
        emit baseMapUpdated(source.name);
    }
    
    return success;
}

bool BaseMapManager::setBaseMapVisible(const QString& mapName, bool visible)
{
    if (!hasBaseMap(mapName)) {
        return false;
    }
    
    osgEarth::ImageLayer* layer = loadedLayers_[mapName].get();
    if (layer) {
        layer->setVisible(visible);
        loadedConfigs_[mapName].visible = visible;
        emit baseMapVisibilityChanged(mapName, visible);
        return true;
    }
    
    return false;
}

bool BaseMapManager::setBaseMapOpacity(const QString& mapName, int opacity)
{
    if (!hasBaseMap(mapName)) {
        return false;
    }
    
    opacity = qBound(0, opacity, 100);
    osgEarth::ImageLayer* layer = loadedLayers_[mapName].get();
    if (layer) {
        layer->setOpacity(opacity / 100.0);
        loadedConfigs_[mapName].opacity = opacity;
        return true;
    }
    
    return false;
}

QList<QPair<QString, BaseMapSource>> BaseMapManager::getLoadedBaseMaps() const
{
    QList<QPair<QString, BaseMapSource>> result;
    // 按照layerOrder_的顺序返回
    for (const QString& name : layerOrder_) {
        if (loadedConfigs_.contains(name)) {
            result.append(qMakePair(name, loadedConfigs_[name]));
        }
    }
    return result;
}

QStringList BaseMapManager::getLayerOrder() const
{
    return layerOrder_;
}

bool BaseMapManager::moveLayerUp(const QString& mapName)
{
    if (!hasBaseMap(mapName)) {
        return false;
    }
    
    int index = layerOrder_.indexOf(mapName);
    if (index <= 0) {
        // 已经在列表第一行（最上层）或不存在
        return false;
    }
    
    // 在列表中向上移动（索引减小）→ 在图层中更上层
    layerOrder_.swap(index, index - 1);
    
    // 重新排序图层
    reorderLayers();
    
    qDebug() << "BaseMapManager: 图层上移:" << mapName;
    return true;
}

bool BaseMapManager::moveLayerDown(const QString& mapName)
{
    if (!hasBaseMap(mapName)) {
        return false;
    }
    
    int index = layerOrder_.indexOf(mapName);
    if (index < 0 || index >= layerOrder_.size() - 1) {
        // 已经在列表最后一行（最下层）或不存在
        return false;
    }
    
    // 在列表中向下移动（索引增大）→ 在图层中更下层
    layerOrder_.swap(index, index + 1);
    
    // 重新排序图层
    reorderLayers();
    
    qDebug() << "BaseMapManager: 图层下移:" << mapName;
    return true;
}

void BaseMapManager::reorderLayers()
{
    if (!map_) {
        return;
    }
    
    // 移除所有图层
    for (const QString& name : layerOrder_) {
        if (loadedLayers_.contains(name)) {
            osgEarth::ImageLayer* layer = loadedLayers_[name].get();
            if (layer) {
                map_->removeLayer(layer);
            }
        }
    }
    
    // 按照新顺序重新添加图层
    // layerOrder_[0] 是最上层（列表第一行），layerOrder_[size-1] 是最下层（列表最后一行）
    // osgEarth 中后添加的图层在上层，所以从后往前添加（先添加最下层，最后添加最上层）
    for (int i = layerOrder_.size() - 1; i >= 0; --i) {
        const QString& name = layerOrder_[i];
        if (loadedLayers_.contains(name)) {
            osgEarth::ImageLayer* layer = loadedLayers_[name].get();
            if (layer) {
                map_->addLayer(layer);
            }
        }
    }
}

QList<BaseMapSource> BaseMapManager::getAvailableBaseMapTemplates() const
{
    return baseMapTemplates_;
}

bool BaseMapManager::hasBaseMap(const QString& name) const
{
    return loadedLayers_.contains(name);
}

BaseMapSource BaseMapManager::getBaseMapConfig(const QString& name) const
{
    if (loadedConfigs_.contains(name)) {
        return loadedConfigs_[name];
    }
    return BaseMapSource();
}

bool BaseMapManager::saveConfig(const QString& filePath) const
{
    QJsonObject root;
    QJsonArray layersArray;
    
    // 按照layerOrder_的顺序保存图层
    for (const QString& name : layerOrder_) {
        if (loadedConfigs_.contains(name)) {
            layersArray.append(loadedConfigs_[name].toJson());
        }
    }
    
    root["layers"] = layersArray;
    
    // 保存图层顺序
    QJsonArray orderArray;
    for (const QString& name : layerOrder_) {
        orderArray.append(name);
    }
    root["layerOrder"] = orderArray;
    
    QJsonDocument doc(root);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "BaseMapManager: 无法打开配置文件写入:" << filePath;
        return false;
    }
    
    file.write(doc.toJson());
    file.close();
    
    qDebug() << "BaseMapManager: 配置已保存到:" << filePath;
    return true;
}

bool BaseMapManager::loadConfig(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "BaseMapManager: 无法打开配置文件读取:" << filePath;
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "BaseMapManager: 配置文件解析失败:" << error.errorString();
        return false;
    }
    
    QJsonObject root = doc.object();
    QJsonArray layersArray = root["layers"].toArray();
    
    // 清除现有图层
    QStringList namesToRemove = loadedLayers_.keys();
    for (const QString& name : namesToRemove) {
        removeBaseMapLayer(name);
    }
    
    layerOrder_.clear();  // 清空顺序列表
    
    // 加载配置的图层
    for (const QJsonValue& value : layersArray) {
        BaseMapSource source = BaseMapSource::fromJson(value.toObject());
        addBaseMapLayer(source);
    }
    
    // 如果配置中有保存的图层顺序，使用保存的顺序
    if (root.contains("layerOrder")) {
        QJsonArray orderArray = root["layerOrder"].toArray();
        QStringList savedOrder;
        for (const QJsonValue& value : orderArray) {
            savedOrder.append(value.toString());
        }
        
        // 验证保存的顺序是否有效（所有图层都存在）
        bool validOrder = true;
        for (const QString& name : savedOrder) {
            if (!hasBaseMap(name)) {
                validOrder = false;
                break;
            }
        }
        
        if (validOrder && savedOrder.size() == layerOrder_.size()) {
            layerOrder_ = savedOrder;
            reorderLayers();  // 按照保存的顺序重新排序
        }
    }
    
    configFilePath_ = filePath;
    qDebug() << "BaseMapManager: 配置已从文件加载:" << filePath;
    return true;
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
    
    // 对于网络瓦片服务（xyz驱动），设置网络超时和重试参数
    // 避免无网络时长时间等待导致界面卡死，但网络恢复后能自动重试
    if (source.driver == "xyz" || source.url.startsWith("http://") || source.url.startsWith("https://")) {
        // 设置HTTP请求超时（秒）
        layerConfig.set("timeout", "5");
        // 设置连接超时（秒）
        layerConfig.set("connect_timeout", "3");
        // 设置重试次数（网络恢复后会自动重试失败的瓦片）
        // 注意：这个重试次数是针对单次请求的，osgEarth会在渲染时自动重试失败的瓦片
        layerConfig.set("retries", "2");
        // 设置最大并发请求数（避免过多请求导致阻塞）
        layerConfig.set("max_connections", "20");
        // 启用自动重试失败的瓦片（osgEarth会在需要时自动重试）
        layerConfig.set("retry_delay", "2");  // 重试延迟（秒）
    }
    
    // 创建ImageLayerOptions
    osgEarth::ImageLayerOptions options(layerConfig);
    
    // 创建ImageLayer
    osgEarth::ImageLayer* layer = new osgEarth::ImageLayer(options);
    
    if (!layer) {
        qDebug() << "BaseMapManager: ImageLayer创建失败";
        return nullptr;
    }
    
    // 检查图层状态，但即使状态不是OK也继续（网络恢复后会自动重试）
    if (!layer->getStatus().isOK()) {
        qDebug() << "BaseMapManager: ImageLayer初始状态异常（可能是网络问题）:" 
                 << layer->getStatus().message().c_str();
        qDebug() << "BaseMapManager: 图层仍会被添加，网络恢复后会自动重试加载";
    }
    
    return layer;
}

osgEarth::ImageLayer* BaseMapManager::findLayerByName(const QString& name) const
{
    if (loadedLayers_.contains(name)) {
        return loadedLayers_[name].get();
    }
    return nullptr;
}
