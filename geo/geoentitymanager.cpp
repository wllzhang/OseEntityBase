#include "geoentitymanager.h"
#include "imageentity.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>
#include <cmath>
#include <limits>

GeoEntityManager::GeoEntityManager(osg::Group* root, osgEarth::MapNode* mapNode, QObject *parent)
    : QObject(parent)
    , root_(root)
    , mapNode_(mapNode)
    , entityCounter_(0)
    , selectedEntity_(nullptr)
    , viewer_(nullptr)
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
            // 从配置中获取图片路径
            QString imagePath = getImagePathFromConfig(entityName);
            if (imagePath.isEmpty()) {
                qDebug() << "未找到图片路径:" << entityName;
                return nullptr;
            }
            
            // 创建图片实体
            entity = new ImageEntity(generateEntityId(entityType, entityName), 
                                   entityName, imagePath, longitude, latitude, altitude, this);
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
            entities_[entity->getId()] = entity;
            
            emit entityCreated(entity);
            qDebug() << "实体创建成功:" << entity->getId();
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

GeoEntity* GeoEntityManager::getEntity(const QString& entityId)
{
    return entities_.value(entityId, nullptr);
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

void GeoEntityManager::removeEntity(const QString& entityId)
{
    qDebug() << "移除实体:" << entityId;
    
    GeoEntity* entity = entities_.value(entityId);
    if (entity) {
        // 清理实体
        entity->cleanup();
        
        // 从场景中移除
        if (entity->getNode()) {
            entityGroup_->removeChild(entity->getNode());
        }
        
        // 从映射中移除
        entities_.remove(entityId);
        
        // 删除实体
        delete entity;
        
        emit entityRemoved(entityId);
        qDebug() << "实体移除成功:" << entityId;
    } else {
        qDebug() << "未找到要移除的实体:" << entityId;
    }
}

void GeoEntityManager::clearAllEntities()
{
    qDebug() << "清空所有实体";
    
    // 清理所有实体
    for (auto it = entities_.begin(); it != entities_.end(); ++it) {
        if (it.value()) {
            it.value()->cleanup();
            delete it.value();
        }
    }
    
    // 清空场景
    entityGroup_->removeChildren(0, entityGroup_->getNumChildren());
    
    // 清空映射
    entities_.clear();
    entityCounter_ = 0;
}

void GeoEntityManager::setEntityConfig(const QJsonObject& config)
{
    entityConfig_ = config;
    qDebug() << "设置实体配置完成";
}

QString GeoEntityManager::generateEntityId(const QString& entityType, const QString& entityName)
{
    entityCounter_++;
    return QString("%1_%2_%3").arg(entityType).arg(entityName).arg(entityCounter_);
}

QString GeoEntityManager::getImagePathFromConfig(const QString& entityName)
{
    if (!entityConfig_.contains("entities")) {
        qDebug() << "配置中没有entities数组";
        return QString();
    }
    
    QJsonArray entitiesArray = entityConfig_["entities"].toArray();
    QString imageDir = entityConfig_["image_directory"].toString();
    
    for (int i = 0; i < entitiesArray.size(); ++i) {
        QJsonObject entityObj = entitiesArray[i].toObject();
        QString name = entityObj["name"].toString();
        
        if (name == entityName) {
            QString filename = entityObj["filename"].toString();
            QString fullPath = QDir(imageDir).absoluteFilePath(filename);
            
            // 检查文件是否存在
            QFileInfo fileInfo(fullPath);
            if (fileInfo.exists()) {
                qDebug() << "找到匹配的图片路径:" << fullPath;
                return fullPath;
            } else {
                qDebug() << "图片文件不存在:" << fullPath;
            }
        }
    }
    
    qDebug() << "未找到实体对应的图片路径:" << entityName;
    return QString();
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
        }
    }
}

GeoEntity* GeoEntityManager::findEntityAtPosition(QPoint screenPos)
{
    if (!viewer_ || !viewer_->getCamera() || !mapNode_) {
        qDebug() << "Viewer、Camera或MapNode未初始化";
        return nullptr;
    }
    
    try {
        // 创建射线相交检测器
        osgUtil::LineSegmentIntersector::Intersections intersections;
        
        // 计算鼠标位置与地球表面的交点
        if (viewer_->computeIntersections(screenPos.x(), screenPos.y(), intersections)) {
            // 获取第一个交点
            const osgUtil::LineSegmentIntersector::Intersection& intersection = *intersections.begin();
            osg::Vec3 worldPos = intersection.getWorldIntersectPoint();
            
            // 将世界坐标转换为地理坐标
            osg::Vec3d geoVec;
            if (mapNode_->getMapSRS()->transformFromWorld(worldPos, geoVec)) {
                double mouseLongitude = geoVec.x();
                double mouseLatitude = geoVec.y();
                
                qDebug() << "鼠标地理坐标:" << mouseLongitude << mouseLatitude;
                
                // 遍历所有实体，找到距离最近的
                GeoEntity* closestEntity = nullptr;
                double minDistance = std::numeric_limits<double>::max();
                
                QStringList entityIds = getEntityIds();
                for (const QString& entityId : entityIds) {
                    GeoEntity* entity = getEntity(entityId);
                    if (entity) {
                        double entityLongitude, entityLatitude, entityAltitude;
                        entity->getPosition(entityLongitude, entityLatitude, entityAltitude);
                        
                        // 计算距离（简单的欧几里得距离）
                        double distance = sqrt(pow(mouseLongitude - entityLongitude, 2) + 
                                             pow(mouseLatitude - entityLatitude, 2));
                        
                        qDebug() << "实体" << entity->getName() << "距离:" << distance;
                        
                        if (distance < minDistance) {
                            minDistance = distance;
                            closestEntity = entity;
                        }
                    }
                }
                
                // 设置一个合理的距离阈值（例如0.01度，约1公里）
                double threshold = 0.1;
                if (closestEntity && minDistance < threshold) {
                    qDebug() << "找到最近实体:" << closestEntity->getName() << "距离:" << minDistance;
                    return closestEntity;
                } else {
                    qDebug() << "没有实体在阈值范围内，最近距离:" << minDistance;
                }
            } else {
                qDebug() << "世界坐标转换为地理坐标失败";
            }
        } else {
            qDebug() << "未找到与地球表面的交点";
        }
    } catch (const std::exception& e) {
        qDebug() << "查找实体时发生异常:" << e.what();
    }
    
    return nullptr;
}

void GeoEntityManager::setViewer(osgViewer::Viewer* viewer)
{
    viewer_ = viewer;
    qDebug() << "GeoEntityManager设置Viewer完成";
}