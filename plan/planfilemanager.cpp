/**
 * @file planfilemanager.cpp
 * @brief 方案文件管理器实现文件
 * 
 * 实现PlanFileManager类的所有功能
 */

#include "planfilemanager.h"
#include "../geo/geoentitymanager.h"
#include "../geo/geoentity.h"
#include "../geo/waypointentity.h"
#include "../util/databaseutils.h"
#include "../ui/ModelAssemblyDialog.h"  // 包含ModelInfo定义
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QDateTime>
#include <QTimer>
#include <QtMath>
#include <QTextStream>

namespace {

QJsonArray sanitizeComponentArray(const QJsonArray& array)
{
    QJsonArray cleanArray;
    for (const QJsonValue& value : array) {
        QJsonObject component = value.toObject();
        component.remove("templateInfo");
        cleanArray.append(component);
    }
    return cleanArray;
}

QJsonObject sanitizeModelAssembly(const QJsonObject& assembly)
{
    QJsonObject result = assembly;
    if (result.contains("components") && result["components"].isArray()) {
        result["components"] = sanitizeComponentArray(result["components"].toArray());
    }
    return result;
}

}

PlanFileManager::PlanFileManager(GeoEntityManager* entityManager, QObject *parent)
    : QObject(parent)
    , entityManager_(entityManager)
    , entityCounter_(0)
    , hasUnsavedChanges_(false)
    , autoSaveEnabled_(false)
    , cancelLoad_(false)
{
    if (!entityManager_) {
        qDebug() << "警告: PlanFileManager的entityManager为空，将在后续设置";
    }
    
    // 初始化自动保存定时器
    autoSaveTimer_ = new QTimer(this);
    autoSaveTimer_->setSingleShot(true);  // 单次触发
    connect(autoSaveTimer_, &QTimer::timeout, this, [this]() {
        if (hasUnsavedChanges_ && !currentPlanFile_.isEmpty()) {
            qDebug() << "自动保存方案文件:" << currentPlanFile_;
            savePlan();
        }
    });
    
    // 连接planDataChanged信号到自动保存定时器
    connect(this, &PlanFileManager::planDataChanged, this, [this]() {
        if (autoSaveEnabled_ && !currentPlanFile_.isEmpty()) {
            // 重启定时器（防抖处理）
            autoSaveTimer_->stop();
            autoSaveTimer_->start();
        }
    });
}

PlanFileManager::~PlanFileManager()
{
}

void PlanFileManager::requestCancelLoad()
{
    cancelLoad_.store(true);
}

void PlanFileManager::setEntityManager(GeoEntityManager* entityManager)
{
    entityManager_ = entityManager;
    if (entityManager_) {
        qDebug() << "PlanFileManager: EntityManager已设置";
    }
}


QString PlanFileManager::getPlansDirectory()
{
    QString plansDir = QCoreApplication::applicationDirPath() + "/plans/";
    QDir dir;
    if (!dir.exists(plansDir)) {
        dir.mkpath(plansDir);
        qDebug() << "创建方案目录:" << plansDir;
    }
    return plansDir;
}

bool PlanFileManager::createPlan(const QString& name, const QString& description)
{
    if (name.isEmpty()) {
        qDebug() << "方案名称不能为空";
        return false;
    }

    planName_ = name;
    planDescription_ = description;
    createTime_ = QDateTime::currentDateTime();
    entityCounter_ = 0;
    hasUnsavedChanges_ = false;

    // 生成文件路径
    QString fileName = generatePlanFileName(name);
    QString filePath = getPlansDirectory() + fileName;

    // 创建空的方案文件
    QJsonObject planObject;
    planObject["version"] = "1.0";

    QJsonObject metadata;
    metadata["name"] = planName_;
    metadata["description"] = planDescription_;
    metadata["createTime"] = createTime_.toString(Qt::ISODate);
    metadata["updateTime"] = createTime_.toString(Qt::ISODate);
    metadata["coordinateSystem"] = "WGS84";
    planObject["metadata"] = metadata;

    planObject["entities"] = QJsonArray();
    planObject["waypoints"] = QJsonArray();
    planObject["routes"] = QJsonArray();

    QJsonDocument doc(planObject);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法创建方案文件:" << filePath << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    currentPlanFile_ = filePath;
    emit planFileChanged(currentPlanFile_);
    qDebug() << "方案创建成功:" << currentPlanFile_;

    return true;
}

bool PlanFileManager::savePlan(const QString& filePath)
{
    QString savePath = filePath.isEmpty() ? currentPlanFile_ : filePath;
    
    if (savePath.isEmpty()) {
        qDebug() << "没有指定方案文件路径";
        return false;
    }

    if (!entityManager_) {
        qDebug() << "EntityManager为空，无法保存方案";
        return false;
    }

    QJsonObject planObject;
    planObject["version"] = "1.0";

    // 元数据
    QJsonObject metadata;
    metadata["name"] = planName_;
    metadata["description"] = planDescription_;
    metadata["createTime"] = createTime_.toString(Qt::ISODate);
    metadata["updateTime"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metadata["coordinateSystem"] = "WGS84";
    planObject["metadata"] = metadata;

    // 实体列表
    QJsonArray entitiesArray;
    QList<GeoEntity*> entities = entityManager_->getAllEntities();
    for (GeoEntity* entity : entities) {
        QJsonObject obj = entityToJson(entity);
        obj["uid"] = entity->getUid(); // 写入实体实例UID
        entitiesArray.append(obj);
    }
    planObject["entities"] = entitiesArray;

    // 保存航线信息（关联到实体的航线）
    QJsonArray routesArray;
    QList<GeoEntityManager::WaypointGroupInfo> allGroups = entityManager_->getAllWaypointGroups();
    for (const auto& groupInfo : allGroups) {
        // 通过routeBinding查找关联的实体UID（反向查找）
        QString entityUid;
        for (auto it = groupInfo.waypoints.begin(); it != groupInfo.waypoints.end(); ++it) {
            // 通过groupInfo无法直接获取entityId，需要通过entityManager的routeBinding查找
            // 这里使用另一种方式：遍历所有实体，检查其routeGroupId属性
            break;
        }
        
        // 更好的方式：遍历当前方案的所有实体，检查其routeGroupId属性
        for (GeoEntity* entity : entities) {
            QString routeGroupId = entity->getProperty("routeGroupId").toString();
            if (routeGroupId == groupInfo.groupId) {
                entityUid = entity->getUid();
                break;
            }
        }
        
        if (entityUid.isEmpty()) {
            continue;  // 跳过未关联到实体的航线
        }
        
        // 构建路线JSON对象（使用实体实例UID作为唯一主键）
        QJsonObject routeObj;
        routeObj["groupId"] = groupInfo.groupId;
        routeObj["name"] = groupInfo.name;
        routeObj["targetUid"] = entityUid;

        // 按顺序保存航点UID列表
        QJsonArray waypointUidArray;
        for (WaypointEntity* wp : groupInfo.waypoints) {
            if (!wp) {
                continue;
            }
            waypointUidArray.append(wp->getUid());
        }
        routeObj["waypointUids"] = waypointUidArray;

        routesArray.append(routeObj);
    }
    planObject["routes"] = routesArray;
    
    // 独立航点（不属于任何航线组的航点）
    planObject["waypoints"] = QJsonArray();
    
    // 相机视角
    if (hasCameraViewpoint_) {
        QJsonObject camera;
        camera["longitude"] = cameraLongitude_;
        camera["latitude"] = cameraLatitude_;
        camera["altitude"] = cameraAltitude_;
        camera["heading"] = cameraHeading_;
        camera["pitch"] = cameraPitch_;
        camera["range"] = cameraRange_;
        planObject["camera"] = camera;
    } else {
        planObject["camera"] = QJsonObject();  // 空对象
    }

    // 写入文件
    QJsonDocument doc(planObject);
    QFile file(savePath);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "无法保存方案文件:" << savePath << file.errorString();
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();

    hasUnsavedChanges_ = false;
    emit planSaved(savePath);
    qDebug() << "方案保存成功:" << savePath;

    return true;
}

bool PlanFileManager::loadPlan(const QString& filePath)
{
    cancelLoad_.store(false);

    if (filePath.isEmpty()) {
        qDebug() << "方案文件路径为空";
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开方案文件:" << filePath << file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "方案文件JSON解析错误:" << parseError.errorString();
        return false;
    }

    QJsonObject planObject = doc.object();

    // 检查版本
    QString version = planObject["version"].toString();
    if (version != "1.0") {
        qDebug() << "不支持的方案文件版本:" << version;
        // 可以添加版本转换逻辑
    }

    // 读取元数据
    QJsonObject metadata = planObject["metadata"].toObject();
    planName_ = metadata["name"].toString();
    planDescription_ = metadata["description"].toString();
    QString createTimeStr = metadata["createTime"].toString();
    createTime_ = QDateTime::fromString(createTimeStr, Qt::ISODate);

    if (!entityManager_) {
        qDebug() << "EntityManager为空，无法加载方案";
        return false;
    }

    QJsonArray entitiesArray = planObject["entities"].toArray();
    QJsonArray routesArray = planObject["routes"].toArray();

    int totalSteps = qMax(1, entitiesArray.size() + routesArray.size() + 1);
    int currentStep = 0;

    emit loadProgress(currentStep, totalSteps, QString::fromUtf8(u8"正在清理当前场景..."));

    entityManager_->clearAllEntities();
    entityManager_->processPendingDeletions();

    // 加载实体
    entityCounter_ = 0;
    for (int i = 0; i < entitiesArray.size(); ++i) {
        if (cancelLoad_.load()) {
            emit loadCancelled();
            entityManager_->clearAllEntities();
            entityManager_->processPendingDeletions();
            return false;
        }
        const auto& entityValue = entitiesArray.at(i);
        QJsonObject entityObj = entityValue.toObject();
        QString entityName = entityObj["name"].toString();
        if (entityName.isEmpty()) {
            entityName = entityObj["modelName"].toString();
        }
        emit loadProgress(currentStep, totalSteps,
                          QString::fromUtf8(u8"加载实体 %1/%2：%3")
                              .arg(i + 1)
                              .arg(entitiesArray.size())
                              .arg(entityName));
        GeoEntity* entity = jsonToEntity(entityObj);
        if (entity) {
            entity->setProperty("modelId", entityObj["modelId"].toString());

            // 保存模型组装和组件配置覆盖到entity属性中
            if (entityObj.contains("modelAssembly")) {
                QJsonObject modelAssembly = sanitizeModelAssembly(entityObj["modelAssembly"].toObject());
                entity->setProperty("modelAssembly", modelAssembly);
            }
            if (entityObj.contains("componentConfigs")) {
                QJsonObject componentConfigs = entityObj["componentConfigs"].toObject();
                entity->setProperty("componentConfigs", componentConfigs);
            }
        }
        ++currentStep;
        emit loadProgress(currentStep, totalSteps,
                          QString::fromUtf8(u8"实体加载进度 %1/%2")
                              .arg(i + 1)
                              .arg(entitiesArray.size()));
    }

    // 加载航线信息
    for (int i = 0; i < routesArray.size(); ++i) {
        if (cancelLoad_.load()) {
            emit loadCancelled();
            entityManager_->clearAllEntities();
            entityManager_->processPendingDeletions();
            return false;
        }
        const auto& routeValue = routesArray.at(i);
        QJsonObject routeObj = routeValue.toObject();
        QString groupId = routeObj["groupId"].toString();
        QString targetUid = routeObj["targetUid"].toString();

        emit loadProgress(currentStep, totalSteps,
                          QString::fromUtf8(u8"加载航线 %1/%2：%3")
                              .arg(i + 1)
                              .arg(routesArray.size())
                              .arg(groupId));

        // 查找对应的实体（通过稳定UID）
        GeoEntity* entity = entityManager_->getEntityByUid(targetUid);
        if (!entity) {
            qDebug() << "加载航线失败：找不到实体UID" << targetUid;
            continue;
        }

        // 创建航点组
        QString newGroupId = entityManager_->createWaypointGroup(routeObj["name"].toString());

        QJsonArray waypointUidArray = routeObj["waypointUids"].toArray();
        if (!waypointUidArray.isEmpty()) {
            for (const QJsonValue& value : waypointUidArray) {
                QString waypointUid = value.toString();
                GeoEntity* waypointEntity = entityManager_->getEntityByUid(waypointUid);
                WaypointEntity* waypoint = qobject_cast<WaypointEntity*>(waypointEntity);
                if (!waypoint) {
                    qDebug() << "加载航线警告：waypointUid 未找到对应航点" << waypointUid;
                    continue;
                }
                entityManager_->attachWaypointToGroup(newGroupId, waypoint);
            }
        } else {
            // 兼容旧格式：直接使用坐标重建航点
            QJsonArray waypointsArray = routeObj["waypoints"].toArray();
            for (const auto& wpValue : waypointsArray) {
                QJsonObject wpObj = wpValue.toObject();
                double lon = wpObj["longitude"].toDouble();
                double lat = wpObj["latitude"].toDouble();
                double alt = wpObj["altitude"].toDouble();
                entityManager_->addWaypointToGroup(newGroupId, lon, lat, alt);
            }
        }

        // 绑定航线到实体（使用uid作为统一标识符）
        entityManager_->bindRouteToEntity(newGroupId, entity->getUid());

        // 保存航线组ID到实体属性
        entity->setProperty("routeGroupId", newGroupId);

        QString routeType = entity->getProperty("routeType").toString();
        if (routeType.isEmpty() && routeObj.contains("routeType")) {
            routeType = routeObj["routeType"].toString();
        }
        if (routeType.isEmpty()) {
            routeType = QStringLiteral("linear");
        }
        entity->setProperty("routeType", routeType);

        // 如果航点数量>=2，生成路线
        auto groupInfo = entityManager_->getWaypointGroup(newGroupId);
        if (groupInfo.waypoints.size() >= 2) {
            entityManager_->generateRouteForGroup(newGroupId, routeType);
            groupInfo = entityManager_->getWaypointGroup(newGroupId);
        }

        qDebug() << "加载航线成功:" << groupId << "->" << newGroupId << "实体UID:" << targetUid << "航点数:" << groupInfo.waypoints.size();

        ++currentStep;
        emit loadProgress(currentStep, totalSteps,
                          QString::fromUtf8(u8"航线加载进度 %1/%2")
                              .arg(i + 1)
                              .arg(routesArray.size()));
    }
    
    // 加载相机视角
    emit loadProgress(currentStep, totalSteps, QString::fromUtf8(u8"恢复相机视角..."));
    if (planObject.contains("camera")) {
        if (cancelLoad_.load()) {
            emit loadCancelled();
            entityManager_->clearAllEntities();
            entityManager_->processPendingDeletions();
            return false;
        }
        QJsonObject camera = planObject["camera"].toObject();
        if (!camera.isEmpty()) {
            double lon = camera["longitude"].toDouble();
            double lat = camera["latitude"].toDouble();
            double alt = camera["altitude"].toDouble();
            double heading = camera["heading"].toDouble();
            double pitch = camera["pitch"].toDouble();
            double range = camera["range"].toDouble();
            
            // 验证数据的有效性
            bool isValid = true;
            // 检查是否为NaN或无穷大
            if (qIsNaN(lon) || qIsInf(lon) || qIsNaN(lat) || qIsInf(lat) || 
                qIsNaN(alt) || qIsInf(alt) || qIsNaN(heading) || qIsInf(heading) ||
                qIsNaN(pitch) || qIsInf(pitch) || qIsNaN(range) || qIsInf(range)) {
                isValid = false;
                qDebug() << "相机视角数据包含NaN或Inf，忽略";
            }
            // 检查经纬度范围
            if (isValid && (lon < -180.0 || lon > 180.0 || lat < -90.0 || lat > 90.0)) {
                isValid = false;
                qDebug() << "相机视角经纬度超出有效范围，忽略";
            }
            // 检查距离和高度是否合理（避免异常大的值）
            if (isValid && (range < 0.0 || range > 1e8 || alt < -10000.0 || alt > 1e7)) {
                isValid = false;
                qDebug() << "相机视角距离或高度超出合理范围，忽略";
            }
            
            if (isValid) {
                hasCameraViewpoint_ = true;
                cameraLongitude_ = lon;
                cameraLatitude_ = lat;
                cameraAltitude_ = alt;
                cameraHeading_ = heading;
                cameraPitch_ = pitch;
                cameraRange_ = range;
                qDebug() << "加载相机视角:" << cameraLongitude_ << cameraLatitude_ << cameraRange_;
            } else {
                hasCameraViewpoint_ = false;
                qDebug() << "相机视角数据无效，已忽略";
            }
        } else {
            hasCameraViewpoint_ = false;
        }
    } else {
        hasCameraViewpoint_ = false;
    }

    ++currentStep;

    currentPlanFile_ = filePath;
    hasUnsavedChanges_ = false;
    emit planLoaded(filePath);
    qDebug() << "方案加载成功:" << filePath << "实体数量:" << entitiesArray.size();

    emit loadProgress(totalSteps, totalSteps, QString::fromUtf8(u8"方案加载完成"));

    return true;
}

QString PlanFileManager::getCurrentPlanFile() const
{
    return currentPlanFile_;
}

void PlanFileManager::setCurrentPlanFile(const QString& filePath)
{
    if (currentPlanFile_ != filePath) {
        currentPlanFile_ = filePath;
        emit planFileChanged(filePath);
    }
}

void PlanFileManager::addEntityToPlan(GeoEntity* entity)
{
    if (!entity) {
        return;
    }

    if (currentPlanFile_.isEmpty()) {
        qDebug() << "当前没有打开的方案文件";
        return;
    }

    hasUnsavedChanges_ = true;
    emit planDataChanged();
    qDebug() << "实体已添加到方案:" << entity->getUid();
}

void PlanFileManager::removeEntityFromPlan(const QString& uid)
{
    if (currentPlanFile_.isEmpty()) {
        return;
    }

    Q_UNUSED(uid);
    hasUnsavedChanges_ = true;
    emit planDataChanged();
    qDebug() << "实体已从方案中移除:" << uid;
}

void PlanFileManager::updateEntityInPlan(GeoEntity* entity)
{
    if (!entity) {
        return;
    }

    if (currentPlanFile_.isEmpty()) {
        return;
    }

    hasUnsavedChanges_ = true;
    emit planDataChanged();
    qDebug() << "方案中的实体已更新:" << entity->getUid();
}

void PlanFileManager::markPlanModified()
{
    if (currentPlanFile_.isEmpty()) {
        return;
    }
    hasUnsavedChanges_ = true;
    emit planDataChanged();
}

bool PlanFileManager::hasUnsavedChanges() const
{
    return hasUnsavedChanges_;
}

/**
 * @brief 序列化实体为JSON对象
 * 
 * 将实体的所有信息（规划属性、模型组装、组件配置）序列化为JSON格式。
 * 组件信息采用深层复制，确保方案文件完全独立于数据库。
 * 
 * @param entity 实体指针
 * @return JSON对象，包含实体的完整信息
 */
QJsonObject PlanFileManager::entityToJson(GeoEntity* entity)
{
    if (!entity) {
        return QJsonObject();
    }

    QJsonObject entityObj;

    // 基本信息（统一使用uid作为标识符）
    entityObj["uid"] = entity->getUid();
    QString displayName = entity->getProperty("displayName").toString();
    entityObj["name"] = displayName.isEmpty() ? entity->getName() : displayName;
    entityObj["type"] = entity->getType();
    entityObj["modelId"] = entity->getProperty("modelId").toString();
    entityObj["modelName"] = entity->getName();  // 模型名称

    // 规划属性：位置
    double longitude, latitude, altitude;
    entity->getPosition(longitude, latitude, altitude);
    QJsonObject position;
    position["longitude"] = longitude;
    position["latitude"] = latitude;
    position["altitude"] = altitude;
    entityObj["position"] = position;

    // 规划属性：姿态和可见性
    entityObj["heading"] = entity->getHeading();
    entityObj["visible"] = entity->isVisible();

    QVariant routeTypeValue = entity->getProperty("routeType");
    if (routeTypeValue.isValid() && !routeTypeValue.toString().isEmpty()) {
        entityObj["routeType"] = routeTypeValue.toString();
    }

    // 模型组装属性：保存完整的组件信息（深层复制）
    QJsonObject entityModelAssembly = sanitizeModelAssembly(entity->getProperty("modelAssembly").toJsonObject());
    
    // 如果实体属性中包含完整的组件信息数组，直接保存
    if (entityModelAssembly.contains("components") && entityModelAssembly["components"].isArray()) {
        QJsonObject modelAssembly;
        
        modelAssembly["components"] = sanitizeComponentArray(entityModelAssembly["components"].toArray());
        
        // 保存location和icon（如果存在）
        if (entityModelAssembly.contains("location")) {
            modelAssembly["location"] = entityModelAssembly["location"];
        }
        if (entityModelAssembly.contains("icon")) {
            modelAssembly["icon"] = entityModelAssembly["icon"];
        }
        
        entityObj["modelAssembly"] = modelAssembly;
    } else {
        // 兼容旧格式：如果没有components数组，尝试从数据库加载并保存完整信息
        QString modelId = entity->getProperty("modelId").toString();
        QJsonObject dbModelAssembly = getModelAssemblyFromDatabase(modelId);
        
        QJsonObject modelAssembly;
        
        // 合并location和icon
        QString location = entityModelAssembly.contains("location") ? 
                          entityModelAssembly["location"].toString() : 
                          dbModelAssembly["location"].toString();
        QString icon = entityModelAssembly.contains("icon") ? 
                      entityModelAssembly["icon"].toString() : 
                      dbModelAssembly["icon"].toString();
        
        if (location != dbModelAssembly["location"].toString()) {
            modelAssembly["location"] = location;
        }
        if (icon != dbModelAssembly["icon"].toString()) {
            modelAssembly["icon"] = icon;
        }
        
        // 从数据库获取完整的组件信息
        QStringList componentIds;
        QJsonArray componentList = entityModelAssembly.contains("componentList") ? 
                                   entityModelAssembly["componentList"].toArray() : 
                                   dbModelAssembly["componentList"].toArray();
        for (const auto& compId : componentList) {
            componentIds << compId.toString();
        }
        
        QJsonArray componentsArray;
        for (const QString& componentId : componentIds) {
            QJsonObject componentInfo = getComponentFullInfoFromDatabase(componentId);
            componentsArray.append(componentInfo);
        }
        modelAssembly["components"] = componentsArray;
        
        if (!modelAssembly.isEmpty()) {
            entityObj["modelAssembly"] = modelAssembly;
        }
    }

    // 组件配置：保存完整的配置信息（不再比较差异）
    QJsonObject entityComponentConfigs = entity->getProperty("componentConfigs").toJsonObject();
    if (!entityComponentConfigs.isEmpty()) {
        entityObj["componentConfigs"] = entityComponentConfigs;
    }

    // 武器挂载信息：保存武器挂载配置
    QJsonObject weaponMounts = entity->getProperty("weaponMounts").toJsonObject();
    if (!weaponMounts.isEmpty()) {
        entityObj["weaponMounts"] = weaponMounts;
    }

    return entityObj;
}

/**
 * @brief 从JSON对象创建实体
 * 
 * 反序列化JSON对象，创建实体并设置所有属性（规划属性、模型组装、组件配置）。
 * 优先使用方案文件中的完整信息，而不是数据库默认值。
 * 
 * @param json JSON对象，包含实体的完整信息
 * @return 创建的实体指针，失败返回nullptr
 */
GeoEntity* PlanFileManager::jsonToEntity(const QJsonObject& json)
{
    if (!entityManager_) {
        qDebug() << "EntityManager为空，无法创建实体";
        return nullptr;
    }

    QString modelId = json["modelId"].toString();
    QString modelName = json["modelName"].toString();

    // 获取位置信息
    QJsonObject position = json["position"].toObject();
    double longitude = position["longitude"].toDouble();
    double latitude = position["latitude"].toDouble();
    double altitude = position["altitude"].toDouble();

    // 创建实体
    QString uidOverride = json["uid"].toString();
    GeoEntity* entity = entityManager_->createEntity(
        json["type"].toString(),
        modelName,
        QJsonObject(),  // properties会在后面设置
        longitude,
        latitude,
        altitude,
        uidOverride
    );

    if (entity) {
        // 设置规划属性
        if (json.contains("heading")) {
            entity->setHeading(json["heading"].toDouble());
        }
        if (json.contains("visible")) {
            entity->setVisible(json["visible"].toBool());
        }
        if (json.contains("name")) {
            entity->setProperty("displayName", json["name"].toString());
        }
        if (json.contains("routeType")) {
            entity->setProperty("routeType", json["routeType"].toString());
        }

        // 设置模型ID
        entity->setProperty("modelId", modelId);
        
        // 从JSON文件加载完整的模型组装信息（不再依赖数据库）
        QJsonObject modelAssemblyObj;
        if (json.contains("modelAssembly")) {
            QJsonObject jsonModelAssembly = json["modelAssembly"].toObject();
            
            // 直接复制完整的组件信息数组
            if (jsonModelAssembly.contains("components")) {
                modelAssemblyObj["components"] = jsonModelAssembly["components"];
            }
            
            // 复制location和icon
            if (jsonModelAssembly.contains("location")) {
                modelAssemblyObj["location"] = jsonModelAssembly["location"];
            }
            if (jsonModelAssembly.contains("icon")) {
                modelAssemblyObj["icon"] = jsonModelAssembly["icon"];
            }
        } else {
            // 兼容旧格式：如果没有modelAssembly，从数据库加载（仅用于兼容）
            ModelInfo modelInfo = getModelInfoFromDatabase(modelId);
            modelAssemblyObj["location"] = modelInfo.location;
            modelAssemblyObj["icon"] = modelInfo.icon;
            QJsonArray compListArray;
            for (const QString& compId : modelInfo.componentList) {
                compListArray.append(compId);
            }
            modelAssemblyObj["componentList"] = compListArray;
        }
        
        entity->setProperty("modelAssembly", modelAssemblyObj);

        // 从JSON文件加载完整的组件配置（不再依赖数据库）
        if (json.contains("componentConfigs")) {
            entity->setProperty("componentConfigs", json["componentConfigs"].toObject());
        }
        // 从JSON文件加载武器挂载信息
        if (json.contains("weaponMounts")) {
            entity->setProperty("weaponMounts", json["weaponMounts"].toObject());
        }
    }

    return entity;
}

/**
 * @brief 从数据库获取模型信息
 * 
 * 查询ModelInformation表，获取模型的完整信息。
 * 
 * @param modelId 模型ID
 * @return ModelInfo结构体
 */
ModelInfo PlanFileManager::getModelInfoFromDatabase(const QString& modelId)
{
    ModelInfo info;
    info.id = modelId;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return info;
    }

    QSqlQuery query;
    query.prepare("SELECT mi.id, mi.name, mi.location, mi.icon, mi.componentlist, mt.type "
                  "FROM ModelInformation mi "
                  "JOIN ModelType mt ON mi.modeltypeid = mt.id "
                  "WHERE mi.id = ?");
    query.addBindValue(modelId);

    if (query.exec() && query.next()) {
        info.id = query.value(0).toString();
        info.name = query.value(1).toString();
        info.location = query.value(2).toString();
        info.icon = query.value(3).toString();
        QString componentListStr = query.value(4).toString();
        info.componentList = componentListStr.split(',', Qt::SkipEmptyParts);
        info.type = query.value(5).toString();
    } else {
        qDebug() << "未找到模型信息:" << modelId << query.lastError().text();
    }

    return info;
}

/**
 * @brief 从数据库获取模型组装信息（JSON格式）
 * 
 * 查询ModelInformation表，获取模型的部署位置、图标和组件列表。
 * 
 * @param modelId 模型ID
 * @return JSON对象，包含location、icon、componentList
 */
QJsonObject PlanFileManager::getModelAssemblyFromDatabase(const QString& modelId)
{
    QJsonObject result;
    ModelInfo info = getModelInfoFromDatabase(modelId);
    
    result["location"] = info.location;
    result["icon"] = info.icon;
    
    QJsonArray compListArray;
    for (const QString& compId : info.componentList) {
        compListArray.append(compId);
    }
    result["componentList"] = compListArray;
    
    return result;
}

/**
 * @brief 从数据库获取完整的组件信息
 * 
 * 查询ComponentInformation和ComponentType表，获取组件的完整信息，
 * 包括componentId、name、type、configInfo、templateInfo等。
 * 
 * @param componentId 组件ID
 * @return 完整的组件信息JSON对象
 */
QJsonObject PlanFileManager::getComponentFullInfoFromDatabase(const QString& componentId)
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT ci.componentid, ci.name, ci.type, ci.configinfo, "
                  "ct.wsf, ct.subtype, ct.template "
                  "FROM ComponentInformation ci "
                  "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
                  "WHERE ci.componentid = ?");
    query.addBindValue(componentId);

    if (query.exec() && query.next()) {
        result["componentId"] = query.value(0).toString();
        result["name"] = query.value(1).toString();
        result["type"] = query.value(2).toString();
        result["wsf"] = query.value(4).toString();
        result["subtype"] = query.value(5).toString();
        
        // 解析配置信息
        QString configStr = query.value(3).toString();
        if (!configStr.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument configDoc = QJsonDocument::fromJson(configStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                result["configInfo"] = configDoc.object();
            }
        }
        
        // 模板信息不再写入方案文件，直接忽略
    }

    return result;
}

/**
 * @brief 从数据库获取组件配置
 * 
 * 查询ComponentInformation表，获取组件的配置信息（configInfo字段）。
 * 
 * @param componentId 组件ID
 * @return 组件配置JSON对象
 */
QJsonObject PlanFileManager::getComponentConfigFromDatabase(const QString& componentId)
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT configinfo FROM ComponentInformation WHERE componentid = ?");
    query.addBindValue(componentId);

    if (query.exec() && query.next()) {
        QString configStr = query.value(0).toString();
        if (!configStr.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(configStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                result = doc.object();
            }
        }
    }

    return result;
}

/**
 * @brief 生成方案文件名
 * 
 * 根据方案名称和时间戳生成安全的文件名。
 * 移除文件名中的非法字符，添加时间戳避免重名。
 * 
 * @param name 方案名称
 * @return 文件名（不含路径）
 */
QString PlanFileManager::generatePlanFileName(const QString& name)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
    QString safeName = name;
    // 移除文件名中的非法字符
    safeName.replace(QRegExp("[\\\\/:*?\"<>|]"), "_");
    return QString("%1_%2.plan.json").arg(safeName).arg(timestamp);
}

/**
 * @brief 比较两个JSON对象是否有差异
 * 
 * 简单比较两个JSON对象是否相等。
 * 
 * @param obj1 第一个对象
 * @param obj2 第二个对象
 * @return 有差异返回true，相同返回false
 */
bool PlanFileManager::jsonObjectsDiffer(const QJsonObject& obj1, const QJsonObject& obj2)
{
    return obj1 != obj2;
}

/**
 * @brief 设置自动保存选项
 * 
 * 启用或禁用自动保存功能，并设置保存间隔。
 * 当启用时，planDataChanged信号会触发定时器，定时器到期后自动保存。
 * 
 * @param enabled 是否启用自动保存
 * @param intervalMs 自动保存间隔（毫秒），默认2000ms（2秒）
 */
void PlanFileManager::setAutoSaveEnabled(bool enabled, int intervalMs)
{
    autoSaveEnabled_ = enabled;
    autoSaveTimer_->setInterval(intervalMs);
    
    if (enabled) {
        qDebug() << "自动保存已启用，间隔:" << intervalMs << "ms";
    } else {
        autoSaveTimer_->stop();
        qDebug() << "自动保存已禁用";
    }
}

/**
 * @brief 设置相机视角（保存方案时调用）
 * 
 * 保存当前相机的视角参数，包括位置、朝向和距离。
 * 这些参数会在保存方案时序列化到JSON文件中。
 * 
 * @param longitude 经度
 * @param latitude 纬度
 * @param altitude 高度
 * @param heading 航向角
 * @param pitch 俯仰角
 * @param range 距离
 */
void PlanFileManager::setCameraViewpoint(double longitude, double latitude, double altitude,
                                        double heading, double pitch, double range)
{
    // 验证数据的有效性
    bool isValid = true;
    // 检查是否为NaN或无穷大
    if (qIsNaN(longitude) || qIsInf(longitude) || qIsNaN(latitude) || qIsInf(latitude) || 
        qIsNaN(altitude) || qIsInf(altitude) || qIsNaN(heading) || qIsInf(heading) ||
        qIsNaN(pitch) || qIsInf(pitch) || qIsNaN(range) || qIsInf(range)) {
        isValid = false;
        qDebug() << "setCameraViewpoint: 数据包含NaN或Inf，忽略设置";
    }
    // 检查经纬度范围
    if (isValid && (longitude < -180.0 || longitude > 180.0 || latitude < -90.0 || latitude > 90.0)) {
        isValid = false;
        qDebug() << "setCameraViewpoint: 经纬度超出有效范围，忽略设置";
    }
    // 检查距离和高度是否合理（避免异常大的值）
    if (isValid && (range < 0.0 || range > 1e8 || altitude < -10000.0 || altitude > 1e7)) {
        isValid = false;
        qDebug() << "setCameraViewpoint: 距离或高度超出合理范围，忽略设置";
    }
    
    if (isValid) {
        hasCameraViewpoint_ = true;
        cameraLongitude_ = longitude;
        cameraLatitude_ = latitude;
        cameraAltitude_ = altitude;
        cameraHeading_ = heading;
        cameraPitch_ = pitch;
        cameraRange_ = range;
    } else {
        hasCameraViewpoint_ = false;
    }
}

/**
 * @brief 获取相机视角（加载方案时调用）
 * 
 * 获取保存的相机视角参数。如果方案文件中没有保存视角，返回false。
 * 
 * @param longitude 输出经度
 * @param latitude 输出纬度
 * @param altitude 输出高度
 * @param heading 输出航向角
 * @param pitch 输出俯仰角
 * @param range 输出距离
 * @return 如果存在保存的视角返回true，否则返回false
 */
bool PlanFileManager::getCameraViewpoint(double& longitude, double& latitude, double& altitude,
                                        double& heading, double& pitch, double& range) const
{
    if (!hasCameraViewpoint_) {
        return false;
    }
    
    longitude = cameraLongitude_;
    latitude = cameraLatitude_;
    altitude = cameraAltitude_;
    heading = cameraHeading_;
    pitch = cameraPitch_;
    range = cameraRange_;
    
    return true;
}

