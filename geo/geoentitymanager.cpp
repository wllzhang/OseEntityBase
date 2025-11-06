/**
 * @file geoentitymanager.cpp
 * @brief 地理实体管理器实现文件
 * 
 * 实现GeoEntityManager类的所有功能
 */

#include "geoentitymanager.h"
#include "imageentity.h"
#include "geoutils.h"
#include "../util/databaseutils.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QTimer>
#include "mapstatemanager.h"
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>
#include <cmath>
#include <limits>
#include "waypointentity.h"
#include <osg/LineWidth>
#include <QSqlQuery>
#include <QSqlError>

GeoEntityManager::GeoEntityManager(osg::Group* root, osgEarth::MapNode* mapNode, QObject *parent)
    : QObject(parent)
    , root_(root)
    , mapNode_(mapNode)
    , entityCounter_(0)
    , selectedEntity_(nullptr)
    , viewer_(nullptr)
    , mapStateManager_(nullptr)
{
    // 创建实体组节点
    entityGroup_ = new osg::Group;
    entityGroup_->setName("EntityGroup");
    root_->addChild(entityGroup_);
    
    qDebug() << "GeoEntityManager初始化完成";
}

GeoEntity* GeoEntityManager::createEntity(const QString& entityType, const QString& entityName, 
                                         const QJsonObject& properties, double longitude, double latitude, double altitude)
{
    qDebug() << "=== 开始创建实体 ===";
    qDebug() << "实体类型:" << entityType;
    qDebug() << "实体名称:" << entityName;
    qDebug() << "位置:" << longitude << latitude << altitude;
    
    try {
        GeoEntity* entity = nullptr;
        
        // 根据实体类型创建不同的实体
        if (entityType == "aircraft" || entityType == "image") {
            // 从数据库查询图片路径（根据实体名称查询模型的icon字段）
            QString imagePath = getImagePathFromDatabase(entityName);
            if (imagePath.isEmpty()) {
                qDebug() << "未找到图片路径:" << entityName;
                return nullptr;
            }
            
            // 创建图片实体（不再需要generateEntityId，使用uid作为统一标识符）
            entity = new ImageEntity(entityName, imagePath, longitude, latitude, altitude, this);
        }
        
        if (!entity) {
            qDebug() << "未知的实体类型:" << entityType;
            return nullptr;
        }
        
        // 初始化实体
        entity->initialize();
        
        // 添加到场景
        if (entity->getNode()) {
            entityGroup_->addChild(entity->getNode());
            entities_[entity->getUid()] = entity;
            uidToEntity_.insert(entity->getUid(), entity);
            
            emit entityCreated(entity);
            qDebug() << "实体创建成功:" << entity->getUid();
            return entity;
        } else {
            qDebug() << "实体节点创建失败";
            delete entity;
            return nullptr;
        }
        
    } catch (const std::exception& e) {
        qDebug() << "createEntity异常:" << e.what();
        return nullptr;
    } catch (...) {
        qDebug() << "createEntity未知异常";
        return nullptr;
    }
}

/**
 * @brief 从拖拽数据添加实体
 * 
 * 解析拖拽数据格式（"aircraft:实体名称"），创建对应的实体。
 * 
 * @param dragData 拖拽数据字符串，格式为"aircraft:实体名称"
 * @param longitude 经度
 * @param latitude 纬度
 * @param altitude 高度
 * @return 成功返回true，失败返回false
 */
bool GeoEntityManager::addEntityFromDrag(const QString& dragData, double longitude, double latitude, double altitude)
{
    qDebug() << "=== 开始从拖拽数据添加实体 ===";
    qDebug() << "拖拽数据:" << dragData;
    
    try {
        // 解析拖拽数据格式: "aircraft:F-15战斗机"
        if (!dragData.startsWith("aircraft:")) {
            qDebug() << "无效的拖拽数据格式";
            return false;
        }
        
        QString entityName = dragData.mid(9); // 去掉"aircraft:"前缀
        qDebug() << "解析出的实体名称:" << entityName;
        
        // 创建实体
        GeoEntity* entity = createEntity("aircraft", entityName, QJsonObject(), longitude, latitude, altitude);
        return entity != nullptr;
        
    } catch (const std::exception& e) {
        qDebug() << "addEntityFromDrag异常:" << e.what();
        return false;
    } catch (...) {
        qDebug() << "addEntityFromDrag未知异常";
        return false;
    }
}

GeoEntity* GeoEntityManager::getEntity(const QString& uid)
{
    return entities_.value(uid, nullptr);
}

GeoEntity* GeoEntityManager::getEntityByUid(const QString& uid) const
{
    return uidToEntity_.value(uid, nullptr);
}

QStringList GeoEntityManager::getEntityIds() const
{
    return entities_.keys();
}

QStringList GeoEntityManager::getEntityIdsByType(const QString& entityType) const
{
    QStringList result;
    for (auto it = entities_.begin(); it != entities_.end(); ++it) {
        if (it.value() && it.value()->getType() == entityType) {
            result << it.key();
        }
    }
    return result;
}

QList<GeoEntity*> GeoEntityManager::getEntitiesByPlanFile(const QString& planFile) const
{
    QList<GeoEntity*> result;
    if (planFile.isEmpty()) {
        return result;
    }

    for (auto it = entities_.begin(); it != entities_.end(); ++it) {
        GeoEntity* entity = it.value();
        if (entity && entity->getProperty("planFile").toString() == planFile) {
            result.append(entity);
        }
    }

    return result;
}

void GeoEntityManager::removeEntity(const QString& uid)
{
    qDebug() << "标记实体待删除:" << uid;
    
    GeoEntity* entity = entities_.value(uid);
    if (!entity) {
        qDebug() << "未找到要移除的实体:" << uid;
        return;
    }
    
    // 立即清除选中引用（避免悬空指针，但不调用setSelected避免触发updateNode）
    if (selectedEntity_ == entity) {
        // 直接清除状态，不调用setSelected(false)避免在删除过程中修改节点
        selectedEntity_ = nullptr;
        emit entityDeselected();
        qDebug() << "清除选中实体引用";
    }
    
    // 立即从场景中移除节点（禁用节点，防止渲染时访问）
    // 这是安全的，因为removeChild不会访问节点内容，只是移除引用
    if (entity->getNode()) {
        // 先禁用节点，防止渲染时访问
        entity->getNode()->setNodeMask(0x0);
        // 从场景中移除（这个操作在渲染过程中也是相对安全的）
        entityGroup_->removeChild(entity->getNode());
        qDebug() << "从场景中移除实体节点";
    }
    
    // 从映射中移除（但不删除entity对象）
    entities_.remove(uid);
    if (entity) {
        uidToEntity_.remove(entity->getUid());
    }
    
    // 保存到待删除队列，延迟真正删除（避免在渲染过程中删除对象）
    pendingEntities_[uid] = entity;
    if (!pendingDeletions_.contains(uid)) {
        pendingDeletions_.enqueue(uid);
    }
    
    // 立即发出删除信号（UI可以立即更新）
    emit entityRemoved(uid);
    qDebug() << "实体已标记为待删除:" << uid << "将在下一帧渲染后真正删除";
}

void GeoEntityManager::clearAllEntities()
{
    qDebug() << "清空所有实体";
    
    // 清除选中实体引用
    if (selectedEntity_) {
        selectedEntity_ = nullptr;
        emit entityDeselected();
    }
    
    // 将所有实体添加到延迟删除队列
    QStringList entityUids = getEntityIds();
    for (const QString& uid : entityUids) {
        GeoEntity* entity = entities_.value(uid);
        if (entity) {
            // 禁用并移除节点
            if (entity->getNode()) {
                entity->getNode()->setNodeMask(0x0);
                entityGroup_->removeChild(entity->getNode());
            }
            
            // 从映射中移除，添加到待删除队列
            entities_.remove(uid);
            uidToEntity_.remove(uid);
            pendingEntities_[uid] = entity;
            if (!pendingDeletions_.contains(uid)) {
                pendingDeletions_.enqueue(uid);
            }
        }
    }
    
    entityCounter_ = 0;
    qDebug() << "所有实体已标记为待删除，将在下一帧渲染后真正删除";
}

QString GeoEntityManager::generateEntityId(const QString& entityType, const QString& entityName)
{
    entityCounter_++;
    return QString("%1_%2_%3").arg(entityType).arg(entityName).arg(entityCounter_);
}

QString GeoEntityManager::getImagePathFromDatabase(const QString& entityName)
{
    // 从数据库查询模型的icon字段（存储的是绝对路径）
    QSqlDatabase db = DatabaseUtils::getDatabase();
    
    // 确保数据库已打开
    if (!DatabaseUtils::isDatabaseOpen()) {
        if (!DatabaseUtils::openDatabase()) {
            qDebug() << "无法打开数据库:" << db.lastError().text();
            return QString();
        }
    }
    
    QSqlQuery query;
    query.prepare("SELECT icon FROM ModelInformation WHERE name = ?");
    query.addBindValue(entityName);
    
    if (query.exec() && query.next()) {
        QString iconPath = query.value(0).toString();
        if (!iconPath.isEmpty()) {
            // 验证文件是否存在
            QFileInfo fileInfo(iconPath);
            if (fileInfo.exists() && fileInfo.isFile()) {
                qDebug() << "从数据库找到图片路径:" << iconPath;
                return iconPath;
            } else {
                qDebug() << "数据库中的图片路径不存在:" << iconPath;
            }
        }
    } else {
        qDebug() << "数据库查询失败或未找到模型:" << entityName << query.lastError().text();
    }
    
    qDebug() << "未找到实体对应的图片路径:" << entityName;
    return QString();
}

QString GeoEntityManager::getImagePathFromConfig(const QString& entityName)
{
    // 此方法已废弃，转发到数据库查询方法
    return getImagePathFromDatabase(entityName);
}

void GeoEntityManager::onMousePress(QMouseEvent* event)
{
    qDebug() << "=== GeoEntityManager::onMousePress 被调用 ===";
    qDebug() << "鼠标位置:" << event->pos();
    qDebug() << "鼠标按钮:" << event->button();
    
    if (event->button() == Qt::LeftButton) {
        // 左键点击选择实体
        GeoEntity* entity = findEntityAtPosition(event->pos());
        qDebug() << "找到的实体:" << (entity ? entity->getName() : "无");
        
        if (entity) {
            // 如果点击的是已选中的实体，不做任何操作（避免重复设置）
            if (selectedEntity_ == entity && entity->isSelected()) {
                qDebug() << "实体已选中，无需重复设置:" << entity->getName();
                return;
            }
            
            // 取消之前的选择
            if (selectedEntity_ && selectedEntity_ != entity) {
                selectedEntity_->setSelected(false);
                emit entityDeselected();
            }
            
            // 选择新实体
            selectedEntity_ = entity;
            entity->setSelected(true);
            emit entitySelected(entity);
            
            qDebug() << "选择实体:" << entity->getName() << "ID:" << entity->getId();
        } else {
            // 点击空白处，取消选择
            if (selectedEntity_) {
                selectedEntity_->setSelected(false);
                selectedEntity_ = nullptr;
                emit entityDeselected();
                qDebug() << "取消实体选择";
            }
            // 通知空白处左键点击（用于点标绘放置）
            emit mapLeftClicked(event->pos());
        }
    } else if (event->button() == Qt::RightButton) {
        // 右键：先发结束标绘信号，再按需发实体菜单
        emit mapRightClicked(event->pos());
        GeoEntity* entity = findEntityAtPosition(event->pos());
        if (entity) {
            emit entityRightClicked(entity, event->pos());
            qDebug() << "右键点击实体:" << entity->getName();
        } else {
            qDebug() << "右键点击地图空白处";
        }
    }
}

void GeoEntityManager::onMouseDoubleClick(QMouseEvent* event)
{
    qDebug() << "=== GeoEntityManager::onMouseDoubleClick 被调用 ===";
    qDebug() << "鼠标位置:" << event->pos();
    
    if (event->button() == Qt::LeftButton) {
        GeoEntity* entity = findEntityAtPosition(event->pos());
        if (entity) {
            emit entityDoubleClicked(entity);
            qDebug() << "双击实体:" << entity->getName();
        }
    }
}

/**
 * @brief 查找指定位置的实体
 * 
 * 将屏幕坐标转换为地理坐标，然后遍历所有实体，找到距离最近的实体。
 * 使用动态阈值（基于相机距离）来判断是否选中实体。
 * 
 * @param screenPos 屏幕坐标（相对于GLWidget）
 * @return 找到的实体指针，未找到返回nullptr
 */
GeoEntity* GeoEntityManager::findEntityAtPosition(QPoint screenPos)
{
    double mouseLongitude, mouseLatitude, mouseAltitude;
    
    // 统一使用 MapStateManager 获取坐标信息（mapStateManager_ 必然存在）
    if (!mapStateManager_->getGeoCoordinatesFromScreen(screenPos, mouseLongitude, mouseLatitude, mouseAltitude)) {
        qDebug() << "findEntityAtPosition: 无法获取鼠标地理坐标";
        return nullptr;
    }
    
    try {
        qDebug() << "鼠标地理坐标:" << mouseLongitude << mouseLatitude << mouseAltitude;
        
        // 遍历所有实体，找到距离最近的可见实体
        GeoEntity* closestEntity = nullptr;
        double minDistance = std::numeric_limits<double>::max();
        
        QStringList entityUids = getEntityIds();
        for (const QString& uid : entityUids) {
            GeoEntity* entity = getEntity(uid);
            if (!entity) {
                continue;
            }
            
            // 跳过不可见的实体
            if (!entity->isVisible()) {
                continue;
            }
            
            // 跳过没有有效节点的实体
            if (!entity->getNode()) {
                continue;
            }
            
            double entityLongitude, entityLatitude, entityAltitude;
            entity->getPosition(entityLongitude, entityLatitude, entityAltitude);
            
            // 计算距离：使用地理距离（米），更精确
            double distanceMeters = GeoUtils::calculateGeographicDistance(mouseLongitude, mouseLatitude, 
                                                                          entityLongitude, entityLatitude);
            
            qDebug() << "实体" << entity->getName() << "距离:" << distanceMeters << "米";
            
            if (distanceMeters < minDistance) {
                minDistance = distanceMeters;
                closestEntity = entity;
            }
        }
        
        // 基于当前相机距离动态调整选中阈值（米）
        // 阈值应该根据相机距离动态调整：相机越远，阈值越大；相机越近，阈值越小
        double thresholdMeters = 100.0; // 默认阈值（100米）
        if (mapStateManager_) {
            double rangeMeters = mapStateManager_->getRange();
            // 基础阈值：50米（近距离时）
            const double baseThreshold = 50.0;      // 米，基础阈值
            // 根据相机距离动态调整：相机距离的1/100到1/1000作为阈值
            // 例如：相机距离1000米时，阈值约10-100米；相机距离10000米时，阈值约100-1000米
            double dynamicThreshold = baseThreshold + (rangeMeters * 0.05); // 相机距离的5%
            
            // 限制阈值范围：最小50米，最大2000米
            const double minThreshold = 50.0;       // 最小阈值（50米）
            const double maxThreshold = 2000.0;     // 最大阈值（2公里）
            if (dynamicThreshold < minThreshold) dynamicThreshold = minThreshold;
            if (dynamicThreshold > maxThreshold) dynamicThreshold = maxThreshold;
            thresholdMeters = dynamicThreshold;
        }
        
        if (closestEntity && minDistance < thresholdMeters) {
            qDebug() << "找到最近实体:" << closestEntity->getName() << "距离:" << minDistance << "米" << "阈值:" << thresholdMeters << "米";
            return closestEntity;
        } else {
            qDebug() << "没有实体在阈值范围内，最近距离:" << minDistance << "米" << "阈值:" << thresholdMeters << "米";
        }
        
        return nullptr;
    } catch (const std::exception& e) {
        qDebug() << "findEntityAtPosition异常:" << e.what();
        return nullptr;
    }
}

// ===== 航点/航线实现 =====

QString GeoEntityManager::createWaypointGroup(const QString& name)
{
    QString gid = QString("wpgroup_%1").arg(++entityCounter_);
    WaypointGroupInfo info; info.groupId = gid; info.name = name; info.routeNode = nullptr;
    waypointGroups_.insert(gid, info);
    return gid;
}

WaypointEntity* GeoEntityManager::addWaypointToGroup(const QString& groupId, double lon, double lat, double alt)
{
    auto it = waypointGroups_.find(groupId);
    if (it == waypointGroups_.end()) return nullptr;

    WaypointEntity* wp = new WaypointEntity(QString("WP-%1").arg(it->waypoints.size()+1),
                                            lon, lat, alt, this);
    // 优先绑定 MapNode，确保 PlaceNode 立即可见
    wp->setMapNode(mapNode_.get());
    wp->initialize();
    // 标记序号
    wp->setOrderLabel(QString::number(it->waypoints.size()+1));

    if (wp->getNode()) {
        entityGroup_->addChild(wp->getNode());
    }
    it->waypoints.push_back(wp);

    // 也注册到通用实体表（可选）
    entities_.insert(wp->getId(), wp);
    emit entityCreated(wp);

    return wp;
}

bool GeoEntityManager::removeWaypointFromGroup(const QString& groupId, int index)
{
    auto it = waypointGroups_.find(groupId);
    if (it == waypointGroups_.end()) return false;
    if (index < 0 || index >= it->waypoints.size()) return false;
    WaypointEntity* wp = it->waypoints.at(index);
    if (wp) {
        if (wp->getNode()) entityGroup_->removeChild(wp->getNode());
        wp->cleanup();
        entities_.remove(wp->getId());
        delete wp;
    }
    it->waypoints.remove(index);
    // 重新编号
    for (int i=0;i<it->waypoints.size();++i) it->waypoints[i]->setOrderLabel(QString::number(i+1));
    return true;
}

/**
 * @brief 生成线性航线节点
 * 
 * 根据航点列表生成直线连接的航线几何体。
 * 
 * @param wps 航点实体向量
 * @return 航线几何节点（Geode），失败返回nullptr
 */
osg::ref_ptr<osg::Geode> GeoEntityManager::buildLinearRoute(const QVector<WaypointEntity*>& wps)
{
    if (wps.size() < 2) return nullptr;
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array();
    const double routeAltMeters = 4000.0; // 固定航线高度（米）

    for (auto* wp : wps) {
        double lon, lat, alt; wp->getPosition(lon, lat, alt);
        // 使用工具函数进行地理坐标到世界坐标的转换
        osg::Vec3d world = GeoUtils::geoToWorldCoordinates(lon, lat, routeAltMeters);
        verts->push_back(world);
    }
    geom->setVertexArray(verts.get());
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, verts->size()));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(0.2f, 0.8f, 1.0f, 1.0f));
    geom->setColorArray(colors.get(), osg::Array::BIND_OVERALL);

    geode->addDrawable(geom.get());
    // 提高可见性：加粗线宽、关闭光照、可选关闭深度测试
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(4.0f);
    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    // 提前到最前层渲染，避免被其它对象覆盖
    ss->setRenderBinDetails(9999, "RenderBin");
    return geode.get();
}

/**
 * @brief 生成贝塞尔航线节点
 * 
 * 根据航点列表生成平滑的贝塞尔曲线航线几何体。
 * 使用二次贝塞尔插值算法，在相邻航点间插入中间点实现平滑。
 * 
 * @param wps 航点实体向量
 * @return 航线几何节点（Geode），失败返回nullptr
 */
osg::ref_ptr<osg::Geode> GeoEntityManager::buildBezierRoute(const QVector<WaypointEntity*>& wps)
{
    // 简化：将每对相邻点间插入若干中间点进行平滑
    if (wps.size() < 2) return buildLinearRoute(wps);
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();
    osg::ref_ptr<osg::Vec3Array> verts = new osg::Vec3Array();
    const double routeAltMeters = 4000.0; // 固定航线高度（米）

    // 使用工具函数进行地理坐标到世界坐标的转换
    auto toWorld = [routeAltMeters](double lon, double lat, double /*alt*/) {
        return GeoUtils::geoToWorldCoordinates(lon, lat, routeAltMeters);
    };

    for (int i=0;i<wps.size()-1;++i){
        double lon1,lat1,alt1; wps[i]->getPosition(lon1,lat1,alt1);
        double lon2,lat2,alt2; wps[i+1]->getPosition(lon2,lat2,alt2);
        // 控制点：使用中点作为近似控制
        double cx = (lon1+lon2)/2.0; double cy=(lat1+lat2)/2.0; double cz=routeAltMeters;
        const int steps = 16;
        for (int t=0;t<=steps;++t){
            double u = double(t)/steps;
            // 二次贝塞尔插值
            double lon = (1-u)*(1-u)*lon1 + 2*u*(1-u)*cx + u*u*lon2;
            double lat = (1-u)*(1-u)*lat1 + 2*u*(1-u)*cy + u*u*lat2;
            double alt = routeAltMeters;
            verts->push_back(toWorld(lon,lat,alt));
        }
    }

    geom->setVertexArray(verts.get());
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, verts->size()));
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(1.0f, 0.6f, 0.2f, 1.0f));
    geom->setColorArray(colors.get(), osg::Array::BIND_OVERALL);
    geode->addDrawable(geom.get());
    osg::ref_ptr<osg::LineWidth> lw = new osg::LineWidth(4.0f);
    osg::StateSet* ss = geode->getOrCreateStateSet();
    ss->setAttributeAndModes(lw.get(), osg::StateAttribute::ON);
    ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    ss->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    ss->setRenderBinDetails(9999, "RenderBin");
    return geode.get();
}

bool GeoEntityManager::generateRouteForGroup(const QString& groupId, const QString& model)
{
    qDebug() << "[Route] 请求生成路线 groupId=" << groupId << ", model=" << model;
    auto it = waypointGroups_.find(groupId);
    if (it == waypointGroups_.end()) return false;
    qDebug() << "[Route] 航点数量=" << it->waypoints.size();
    if (it->routeNode.valid()) {
        entityGroup_->removeChild(it->routeNode.get());
        it->routeNode = nullptr;
    }
    osg::ref_ptr<osg::Geode> route = (model == "bezier") ? buildBezierRoute(it->waypoints)
                                                           : buildLinearRoute(it->waypoints);
    if (!route) return false;
    it->routeNode = route;
    entityGroup_->addChild(route.get());
    qDebug() << "[Route] 路线已生成并添加到场景";
    return true;
}

bool GeoEntityManager::bindRouteToEntity(const QString& groupId, const QString& targetEntityUid)
{
    if (!entities_.contains(targetEntityUid)) return false;
    if (!waypointGroups_.contains(groupId)) return false;
    routeBinding_[groupId] = targetEntityUid;
    return true;
}

QString GeoEntityManager::getRouteGroupIdForEntity(const QString& entityUid) const
{
    for (auto it = routeBinding_.begin(); it != routeBinding_.end(); ++it) {
        if (it.value() == entityUid) {
            return it.key();
        }
    }
    return QString();
}

QList<GeoEntityManager::WaypointGroupInfo> GeoEntityManager::getAllWaypointGroups() const
{
    QList<WaypointGroupInfo> result;
    for (auto it = waypointGroups_.begin(); it != waypointGroups_.end(); ++it) {
        result.append(it.value());
    }
    return result;
}

GeoEntityManager::WaypointGroupInfo GeoEntityManager::getWaypointGroup(const QString& groupId) const
{
    auto it = waypointGroups_.find(groupId);
    if (it != waypointGroups_.end()) {
        return it.value();
    }
    return WaypointGroupInfo();
}

WaypointEntity* GeoEntityManager::addStandaloneWaypoint(double lon, double lat, double alt, const QString& labelText)
{
    WaypointEntity* wp = new WaypointEntity(QString("WP-%1").arg(entityCounter_),
                                            lon, lat, alt, this);
    // 优先绑定 MapNode，确保 PlaceNode 立即可见
    wp->setMapNode(mapNode_.get());
    wp->initialize();
    wp->setOrderLabel(labelText);
    if (wp->getNode()) {
        entityGroup_->addChild(wp->getNode());
    }
    entities_.insert(wp->getId(), wp);
    emit entityCreated(wp);
    return wp;
}

void GeoEntityManager::setViewer(osgViewer::Viewer* viewer)
{
    viewer_ = viewer;
    qDebug() << "GeoEntityManager设置Viewer完成";
}

void GeoEntityManager::setMapStateManager(MapStateManager* mapStateManager)
{
    mapStateManager_ = mapStateManager;
}

/**
 * @brief 处理延迟删除队列
 * 
 * 在渲染完成后调用，安全地删除已标记为待删除的实体。
 * 使用延迟删除机制避免在渲染过程中删除OSG节点导致崩溃。
 * 
 * @note 应该在每一帧渲染完成后（frame()）调用此方法
 */
void GeoEntityManager::processPendingDeletions()
{
    // qDebug() << "处理延迟删除队列，共有" << pendingDeletions_.size() << "个实体待删除";
    
    while (!pendingDeletions_.isEmpty()) {
        QString uid = pendingDeletions_.dequeue();
        
        // 从待删除映射中获取实体
        GeoEntity* entity = pendingEntities_.take(uid);
        if (!entity) {
            qDebug() << "警告：待删除实体不存在:" << uid;
            continue;
        }
        
        qDebug() << "开始真正删除实体:" << uid;
        
        // 现在可以安全地清理和删除实体了
        // 此时节点已经从场景中移除，且不在渲染过程中
        entity->cleanup();
        
        // 删除实体对象
        delete entity;
        entity = nullptr;
        
        qDebug() << "实体完全删除完成:" << uid;
    }
    
    // qDebug() << "所有延迟删除完成";
}