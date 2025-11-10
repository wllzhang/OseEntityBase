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
#include <QElapsedTimer>
#include "mapstatemanager.h"
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>
#include <cmath>
#include <limits>
#include "waypointentity.h"
#include <osg/LineWidth>
#include <QSqlQuery>
#include <QSqlError>
#include <QMenu>
#include <algorithm>

namespace {
bool isFinite(double value) {
    return std::isfinite(value);
}

QVector<osg::Vec3d> generateBezierCurve(const QVector<osg::Vec3d>& controlPoints, int steps)
{
    QVector<osg::Vec3d> result;
    if (controlPoints.isEmpty()) {
        return result;
    }

    steps = qMax(1, steps);

    for (int i = 0; i <= steps; ++i) {
        double t = static_cast<double>(i) / steps;
        QVector<osg::Vec3d> temp = controlPoints;
        int n = temp.size();
        while (n > 1) {
            for (int k = 0; k < n - 1; ++k) {
                temp[k] = temp[k] * (1.0 - t) + temp[k + 1] * t;
            }
            --n;
        }
        result.append(temp[0]);
    }

    return result;
}
}

GeoEntityManager::GeoEntityManager(osg::Group* root, osgEarth::MapNode* mapNode, QObject *parent)
    : QObject(parent)
    , root_(root)
    , mapNode_(mapNode)
    , entityCounter_(0)
    , selectedEntity_(nullptr)
    , hoveredEntity_(nullptr)
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
                                         const QJsonObject& properties, double longitude, double latitude, double altitude,
                                         const QString& uidOverride)
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
            entity = new ImageEntity(entityName, imagePath, longitude, latitude, altitude, uidOverride, this);
        } else if (entityType == "waypoint") {
            WaypointEntity* waypointEntity = new WaypointEntity(entityName,
                                                                longitude,
                                                                latitude,
                                                                altitude,
                                                                uidOverride,
                                                                this);
            waypointEntity->setMapNode(mapNode_.get());
            entity = waypointEntity;
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

QList<GeoEntity*> GeoEntityManager::getAllEntities() const
{
    return entities_.values();
}

GeoEntity* GeoEntityManager::getSelectedEntity() const
{
    return selectedEntity_;
}

void GeoEntityManager::setSelectedEntity(GeoEntity* entity, bool emitSignal)
{
    if (selectedEntity_ == entity) {
        return;
    }

    if (selectedEntity_) {
        selectedEntity_->setSelected(false);
        if (emitSignal) {
            emit entityDeselected();
        }
    }

    selectedEntity_ = entity;
    if (hoveredEntity_ == selectedEntity_) {
        if (hoveredEntity_) {
            hoveredEntity_->setHovered(false);
        }
        hoveredEntity_ = nullptr;
    }

    if (selectedEntity_) {
        selectedEntity_->setSelected(true);
        if (emitSignal) {
            emit entitySelected(selectedEntity_);
        }
    }
}

bool GeoEntityManager::setEntityVisible(const QString& uid, bool visible)
{
    GeoEntity* entity = getEntity(uid);
    if (!entity) {
        return false;
    }

    entity->setVisible(visible);
    if (!visible) {
        if (selectedEntity_ == entity) {
            setSelectedEntity(nullptr);
        }
        if (hoveredEntity_ == entity) {
            hoveredEntity_->setHovered(false);
            hoveredEntity_ = nullptr;
        }
    }
    return true;
}

bool GeoEntityManager::isEntityVisible(const QString& uid) const
{
    GeoEntity* entity = uidToEntity_.value(uid, nullptr);
    return entity ? entity->isVisible() : false;
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

void GeoEntityManager::removeEntity(const QString& uid)
{
    qDebug() << "标记实体待删除:" << uid;
    
    GeoEntity* entity = entities_.value(uid);
    if (!entity) {
        qDebug() << "未找到要移除的实体:" << uid;
        return;
    }
    
    // 清除选中/悬停引用
    if (selectedEntity_ == entity) {
        setSelectedEntity(nullptr);
        qDebug() << "清除选中实体引用";
    }
    if (hoveredEntity_ == entity) {
        hoveredEntity_->setHovered(false);
        hoveredEntity_ = nullptr;
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
        setSelectedEntity(nullptr);
    }
    if (hoveredEntity_) {
        hoveredEntity_->setHovered(false);
        hoveredEntity_ = nullptr;
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
    for (auto it = waypointGroups_.begin(); it != waypointGroups_.end(); ++it) {
        if (it->routeNode.valid()) {
            entityGroup_->removeChild(it->routeNode.get());
        }
    }
    waypointGroups_.clear();
    routeBinding_.clear();
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

double GeoEntityManager::computeSelectionThreshold() const
{
    double thresholdMeters = 100.0;
    if (mapStateManager_) {
        double rangeMeters = mapStateManager_->getRange();
        const double baseThreshold = 50.0;
        double dynamicThreshold = baseThreshold + (rangeMeters * 0.05);
        const double minThreshold = 50.0;
        const double maxThreshold = 2000.0;
        if (dynamicThreshold < minThreshold) {
            dynamicThreshold = minThreshold;
        } else if (dynamicThreshold > maxThreshold) {
            dynamicThreshold = maxThreshold;
        }
        thresholdMeters = dynamicThreshold;
    }
    return thresholdMeters;
}

bool GeoEntityManager::collectPickCandidates(QPoint screenPos, QVector<PickCandidate>& outCandidates, double& thresholdMeters, bool verbose)
{
    outCandidates.clear();
    thresholdMeters = computeSelectionThreshold();

    if (!mapStateManager_) {
        if (verbose) {
            qDebug() << "collectPickCandidates: mapStateManager_ 未初始化";
        }
        return false;
    }

    double mouseLongitude = 0.0;
    double mouseLatitude = 0.0;
    double mouseAltitude = 0.0;

    if (!mapStateManager_->getGeoCoordinatesFromScreen(screenPos, mouseLongitude, mouseLatitude, mouseAltitude)) {
        if (verbose) {
            qDebug() << "collectPickCandidates: 无法获取鼠标地理坐标";
        }
        return false;
    }

    if (verbose) {
        qDebug() << "鼠标地理坐标:" << mouseLongitude << mouseLatitude << mouseAltitude;
    }

    double minDistance = std::numeric_limits<double>::max();

    QStringList entityUids = getEntityIds();
    for (const QString& uid : entityUids) {
        GeoEntity* entity = getEntity(uid);
        if (!entity) {
            continue;
        }
        if (!entity->isVisible() || !entity->getNode()) {
            continue;
        }

        double entityLongitude = 0.0;
        double entityLatitude = 0.0;
        double entityAltitude = 0.0;
        entity->getPosition(entityLongitude, entityLatitude, entityAltitude);

        double distanceMeters = GeoUtils::calculateGeographicDistance(mouseLongitude, mouseLatitude,
                                                                       entityLongitude, entityLatitude);

        if (verbose) {
            qDebug() << "实体" << entity->getName() << "距离:" << distanceMeters << "米";
        }

        if (distanceMeters < minDistance) {
            minDistance = distanceMeters;
        }

        if (distanceMeters <= thresholdMeters) {
            PickCandidate candidate;
            candidate.entity = entity;
            candidate.distanceMeters = distanceMeters;
            outCandidates.append(candidate);
        }
    }

    std::sort(outCandidates.begin(), outCandidates.end(), [](const PickCandidate& lhs, const PickCandidate& rhs) {
        return lhs.distanceMeters < rhs.distanceMeters;
    });

    if (verbose) {
        if (!outCandidates.isEmpty()) {
            const auto& first = outCandidates.first();
            qDebug() << "找到最近实体:" << first.entity->getName()
                     << "距离:" << first.distanceMeters << "米"
                     << "阈值:" << thresholdMeters << "米";
        } else if (minDistance != std::numeric_limits<double>::max()) {
            qDebug() << "没有实体在阈值范围内，最近距离:" << minDistance
                     << "米 阈值:" << thresholdMeters << "米";
        }
    }

    return true;
}

void GeoEntityManager::onMousePress(QMouseEvent* event)
{
    qDebug() << "=== GeoEntityManager::onMousePress 被调用 ===";
    qDebug() << "鼠标位置:" << event->pos();
    qDebug() << "鼠标按钮:" << event->button();
    
    if (event->button() == Qt::LeftButton) {
        QVector<PickCandidate> candidates;
        double thresholdMeters = 0.0;
        if (!collectPickCandidates(event->pos(), candidates, thresholdMeters, false)) {
            emit mapLeftClicked(event->pos());
            return;
        }

        if (candidates.isEmpty()) {
            if (selectedEntity_) {
                setSelectedEntity(nullptr);
                qDebug() << "取消实体选择";
            }
            emit mapLeftClicked(event->pos());
            return;
        }

        if (candidates.size() == 1) {
            GeoEntity* entity = candidates.first().entity;
            if (entity && selectedEntity_ != entity) {
                setSelectedEntity(entity);
                qDebug() << "选择实体:" << entity->getName() << "UID:" << entity->getUid();
            }
            return;
        }

        QMenu menu;
        QHash<QAction*, GeoEntity*> actionMap;
        for (const auto& candidate : candidates) {
            if (!candidate.entity) {
                continue;
            }
            QString label = QString("%1 (%2 米)").arg(candidate.entity->getName())
                                .arg(candidate.distanceMeters, 0, 'f', 0);
            QAction* action = menu.addAction(label);
            actionMap.insert(action, candidate.entity);
        }

        QAction* chosen = menu.exec(event->globalPos());
        if (chosen) {
            GeoEntity* selected = actionMap.value(chosen, nullptr);
            if (selected && selectedEntity_ != selected) {
                setSelectedEntity(selected);
                qDebug() << "选择实体:" << selected->getName() << "UID:" << selected->getUid();
            }
        }
    } else if (event->button() == Qt::RightButton) {
        // 右键：先发结束标绘信号，再按需发实体菜单
        emit mapRightClicked(event->pos());
        GeoEntity* entity = selectedEntity_;
        if (!entity) {
            entity = findEntityAtPosition(event->pos());
        }
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
 
void GeoEntityManager::onMouseMove(QMouseEvent* event)
{
    //每次移动鼠标都计算最近的实体
    // if (!mapStateManager_) {
    //         return;
    //     }

    // GeoEntity* entity = findEntityAtPosition(event->pos(), false);

    // if (entity == selectedEntity_) {
    //     if (hoveredEntity_ && hoveredEntity_ != selectedEntity_) {
    //         hoveredEntity_->setHovered(false);
    //     }
    //     hoveredEntity_ = nullptr;
    //     return;
    // }

    // if (entity) {
    //     if (hoveredEntity_ != entity) {
    //         if (hoveredEntity_) {
    //             hoveredEntity_->setHovered(false);
    //         }
    //         if (entity != selectedEntity_) {
    //             entity->setHovered(true);
    //             hoveredEntity_ = entity;
    //         } else {
    //             hoveredEntity_ = nullptr;
    //         }
    //     }
    // } else {
    //     if (hoveredEntity_) {
    //         hoveredEntity_->setHovered(false);
    //     }
    //     hoveredEntity_ = nullptr;
    // }


    
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
GeoEntity* GeoEntityManager::findEntityAtPosition(QPoint screenPos, bool verbose)
{
    QVector<PickCandidate> candidates;
    double thresholdMeters = 0.0;
    if (!collectPickCandidates(screenPos, candidates, thresholdMeters, verbose)) {
        return nullptr;
    }
    if (candidates.isEmpty()) {
        return nullptr;
    }
    return candidates.first().entity;
}

// ===== 航点/航线实现 =====

QString GeoEntityManager::createWaypointGroup(const QString& name)
{
    QString gid = QString("wpgroup_%1").arg(++entityCounter_);
    WaypointGroupInfo info; info.groupId = gid; info.name = name; info.routeNode = nullptr; info.routeModel = QStringLiteral("linear");
    waypointGroups_.insert(gid, info);
    return gid;
}

WaypointEntity* GeoEntityManager::addWaypointToGroup(const QString& groupId, double lon, double lat, double alt,
                                                    const QString& uidOverride, const QString& label)
{
    auto it = waypointGroups_.find(groupId);
    if (it == waypointGroups_.end()) return nullptr;

    QString name = label.isEmpty() ? QString("WP-%1").arg(it->waypoints.size()+1) : label;
    WaypointEntity* wp = new WaypointEntity(name,
                                            lon, lat, alt, uidOverride, this);
    // 优先绑定 MapNode，确保 PlaceNode 立即可见
    wp->setMapNode(mapNode_.get());
    wp->initialize();
    // 标记序号
    if (label.isEmpty()) {
        wp->setOrderLabel(QString::number(it->waypoints.size()+1));
    } else {
        wp->setOrderLabel(label);
    }

    if (wp->getNode()) {
        entityGroup_->addChild(wp->getNode());
    }
    it->waypoints.push_back(wp);

    wp->setProperty("waypointGroupId", groupId);
    wp->setProperty("waypointOrder", it->waypoints.size());

    // 也注册到通用实体表（可选）
    entities_.insert(wp->getUid(), wp);
    uidToEntity_.insert(wp->getUid(), wp);
    emit entityCreated(wp);

    return wp;
}

bool GeoEntityManager::attachWaypointToGroup(const QString& groupId, WaypointEntity* waypoint)
{
    if (!waypoint || !waypointGroups_.contains(groupId)) {
        return false;
    }

    QString currentGroup;
    int index = -1;
    if (findWaypointLocation(waypoint, currentGroup, index)) {
        if (currentGroup == groupId) {
            return true;
        }
        auto currentIt = waypointGroups_.find(currentGroup);
        if (currentIt != waypointGroups_.end() && index >= 0 && index < currentIt->waypoints.size()) {
            currentIt->waypoints.removeAt(index);
            for (int i = 0; i < currentIt->waypoints.size(); ++i) {
                currentIt->waypoints[i]->setOrderLabel(QString::number(i + 1));
                currentIt->waypoints[i]->setProperty("waypointOrder", i + 1);
            }
        }
    }

    auto& info = waypointGroups_[groupId];
    if (!info.waypoints.contains(waypoint)) {
        info.waypoints.push_back(waypoint);
    }

    waypoint->setMapNode(mapNode_.get());
    if (!waypoint->getNode()) {
        waypoint->initialize();
    }
    if (waypoint->getNode() && !entityGroup_->containsNode(waypoint->getNode())) {
        entityGroup_->addChild(waypoint->getNode());
    }

    waypoint->setOrderLabel(QString::number(info.waypoints.size()));
    waypoint->setProperty("waypointGroupId", groupId);
    waypoint->setProperty("waypointOrder", info.waypoints.size());

    return true;
}

bool GeoEntityManager::removeWaypointFromGroup(const QString& groupId, int index)
{
    auto it = waypointGroups_.find(groupId);
    if (it == waypointGroups_.end()) return false;
    if (index < 0 || index >= it->waypoints.size()) return false;
    return removeWaypointEntity(it->waypoints.at(index));
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

    for (auto* wp : wps) {
        double lon, lat, alt; wp->getPosition(lon, lat, alt);
        osg::Vec3d world = GeoUtils::geoToWorldCoordinates(lon, lat, alt);
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

    for (int i=0;i<wps.size()-1;++i){
        double lon1,lat1,alt1; wps[i]->getPosition(lon1,lat1,alt1);
        double lon2,lat2,alt2; wps[i+1]->getPosition(lon2,lat2,alt2);
        // 控制点：使用中点作为近似控制
        double cx = (lon1+lon2)/2.0;
        double cy = (lat1+lat2)/2.0;
        double cz = (alt1+alt2)/2.0;
        QVector<osg::Vec3d> controlPoints;
        controlPoints.append(GeoUtils::geoToWorldCoordinates(lon1, lat1, alt1));
        controlPoints.append(GeoUtils::geoToWorldCoordinates(cx, cy, cz));
        controlPoints.append(GeoUtils::geoToWorldCoordinates(lon2, lat2, alt2));

        QVector<osg::Vec3d> curve = generateBezierCurve(controlPoints, 16);
        if (!curve.isEmpty()) {
            if (!verts->empty()) {
                curve.removeFirst();
            }
            for (const auto& pt : curve) {
                verts->push_back(pt);
            }
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
    it->routeModel = model;
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

WaypointEntity* GeoEntityManager::addStandaloneWaypoint(double lon, double lat, double alt, const QString& labelText,
                                                        const QString& uidOverride)
{
    QString name = labelText.isEmpty() ? QString("WP-%1").arg(entityCounter_) : labelText;
    WaypointEntity* wp = new WaypointEntity(name,
                                            lon, lat, alt, uidOverride, this);
    // 优先绑定 MapNode，确保 PlaceNode 立即可见
    wp->setMapNode(mapNode_.get());
    wp->initialize();
    if (!labelText.isEmpty()) {
        wp->setOrderLabel(labelText);
    }
    if (wp->getNode()) {
        entityGroup_->addChild(wp->getNode());
    }
    entities_.insert(wp->getUid(), wp);
    uidToEntity_.insert(wp->getUid(), wp);
    emit entityCreated(wp);
    return wp;
}

bool GeoEntityManager::findWaypointLocation(WaypointEntity* waypoint, QString& groupIdOut, int& indexOut) const
{
    if (!waypoint) {
        return false;
    }

    for (auto it = waypointGroups_.constBegin(); it != waypointGroups_.constEnd(); ++it) {
        const QVector<WaypointEntity*>& wps = it.value().waypoints;
        for (int i = 0; i < wps.size(); ++i) {
            if (wps[i] == waypoint) {
                groupIdOut = it.key();
                indexOut = i;
                return true;
            }
        }
    }

    return false;
}

bool GeoEntityManager::removeWaypointEntity(WaypointEntity* waypoint)
{
    if (!waypoint) {
        return false;
    }

    QString groupId;
    int index = -1;
    if (!findWaypointLocation(waypoint, groupId, index)) {
        qDebug() << "removeWaypointEntity: 未找到航点所属分组";
        return false;
    }

    auto it = waypointGroups_.find(groupId);
    if (it == waypointGroups_.end() || index < 0 || index >= it->waypoints.size()) {
        return false;
    }

    if (selectedEntity_ == waypoint) {
        setSelectedEntity(nullptr);
    }
    if (hoveredEntity_ == waypoint) {
        hoveredEntity_ = nullptr;
    }

    if (it->routeNode.valid()) {
        entityGroup_->removeChild(it->routeNode.get());
        it->routeNode = nullptr;
    }

    if (waypoint->getNode()) {
        waypoint->getNode()->setNodeMask(0x0);
        entityGroup_->removeChild(waypoint->getNode());
    }
    const QString wpUid = waypoint->getUid();
    entities_.remove(wpUid);
    uidToEntity_.remove(wpUid);

    it->waypoints.removeAt(index);

    for (int i = 0; i < it->waypoints.size(); ++i) {
        it->waypoints[i]->setOrderLabel(QString::number(i + 1));
        it->waypoints[i]->setProperty("waypointOrder", i + 1);
    }

    waypoint->setProperty("waypointGroupId", QString());
    waypoint->setProperty("waypointOrder", QVariant());

    pendingEntities_[wpUid] = waypoint;
    if (!pendingDeletions_.contains(wpUid)) {
        pendingDeletions_.enqueue(wpUid);
    }

    if (it->waypoints.size() >= 2) {
        const QString model = it->routeModel.isEmpty() ? QStringLiteral("linear") : it->routeModel;
        generateRouteForGroup(groupId, model);
    } else {
        if (it->routeNode.valid()) {
            entityGroup_->removeChild(it->routeNode.get());
            it->routeNode = nullptr;
        }
    }

    return true;
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