/**
 * @file afsimscriptgenerator.cpp
 * @brief AFSIM脚本生成器实现文件
 *
 * 实现AfsimScriptGenerator类的所有功能
 */

#include "afsimscriptgenerator.h"
#include "../plan/planfilemanager.h"
#include "../util/databaseutils.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QSqlQuery>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QFileInfo>
#include <QSet>
#include <cmath>

AfsimScriptGenerator::AfsimScriptGenerator(GeoEntityManager* entityManager, PlanFileManager* planFileManager)
    : entityManager_(entityManager)
    , planFileManager_(planFileManager)
{
}

bool AfsimScriptGenerator::generateScript(const QString& filePath)
{
    if (!planFileManager_) {
        qDebug() << "方案文件管理器为空";
        return false;
    }

    QJsonObject planObj;
    if (!loadPlanData(planObj)) {
        qDebug() << "加载方案文件失败";
        return false;
    }

    QJsonArray entitiesArray = planObj["entities"].toArray();
    if (entitiesArray.isEmpty()) {
        qDebug() << "方案中没有实体";
        return false;
    }

    QJsonArray routesArray = planObj["routes"].toArray();

    scriptContent_.clear();
    QTextStream stream(&scriptContent_);

    QMap<QString, QString> platformTypes = collectPlatformTypes(entitiesArray);
    QMap<QString, QJsonObject> platformSamples;
    for (const QJsonValue& value : entitiesArray) {
        QJsonObject entityObj = value.toObject();
        QString modelName = entityObj["modelName"].toString();
        if (!modelName.isEmpty() && !platformSamples.contains(modelName)) {
            platformSamples.insert(modelName, entityObj);
        }
    }

    QMap<QString, QJsonObject> sensorComponents;
    QMap<QString, QPair<QString, QJsonObject>> signatureComponents;

    for (auto it = platformTypes.constBegin(); it != platformTypes.constEnd(); ++it) {
        const QString& platformName = it.key();
        const QString& wsfType = it.value();

        QJsonObject sampleEntity = platformSamples.value(platformName);
        if (sampleEntity.isEmpty()) {
            continue;
        }

        QString modelId = sampleEntity["modelId"].toString();
        QJsonArray components;
        if (!modelId.isEmpty()) {
            components = getModelComponentsFromDatabase(modelId);
        }
        if (components.isEmpty()) {
            QJsonObject modelAssembly = sampleEntity["modelAssembly"].toObject();
            components = modelAssembly["components"].toArray();
        }

        QJsonArray enrichedComponents;
        for (const QJsonValue& compValue : components) {
            QJsonObject mergedComp = mergeComponentConfig(compValue.toObject());
            enrichedComponents.append(mergedComp);

            QString type = mergedComp["type"].toString();
            QString name = mergedComp["name"].toString();
            QString wsf = mergedComp["wsf"].toString();

            if ((type == "传感器" || type == "雷达传感器" || type == "红外传感器") && !name.isEmpty()) {
                if (!sensorComponents.contains(name)) {
                    sensorComponents.insert(name, mergedComp);
                }
            }

            if (type == "目标特性" || type == "雷达特征" || type == "红外特征" || type == "光学特征") {
                QString signatureType;
                if (wsf.contains("radar") || type == "雷达特征") {
                    signatureType = "radar_signature";
                } else if (wsf.contains("infrared") || type == "红外特征") {
                    signatureType = "infrared_signature";
                } else if (wsf.contains("optical") || type == "光学特征") {
                    signatureType = "optical_signature";
                }

                if (!signatureType.isEmpty() && !name.isEmpty() && !signatureComponents.contains(name)) {
                    signatureComponents.insert(name, QPair<QString, QJsonObject>(signatureType, mergedComp));
                }
            }
        }

        stream << generatePlatformType(platformName, wsfType, enrichedComponents, sampleEntity) << "\n\n";
    }

    QMap<QString, QString> weapons = collectWeapons(entitiesArray);
    for (auto it = weapons.constBegin(); it != weapons.constEnd(); ++it) {
        const QString& weaponId = it.key();
        const QString& weaponName = it.value();

        QString platformTypeName = "MISSILE";
        if (DatabaseUtils::openDatabase()) {
            QSqlQuery query;
            query.prepare("SELECT mi.name FROM ModelInformation mi WHERE mi.id = ?");
            query.addBindValue(weaponId);
            if (query.exec() && query.next()) {
                platformTypeName = query.value(0).toString();
            }
        }

        stream << generateWeaponEffects(weaponName) << "\n\n";
        stream << generateWeapon(weaponName, platformTypeName) << "\n\n";
    }

    for (auto it = sensorComponents.constBegin(); it != sensorComponents.constEnd(); ++it) {
        const QString& sensorName = it.key();
        QJsonObject comp = it.value();
        QString wsf = comp["wsf"].toString();
        QJsonObject configInfo = comp["configInfo"].toObject();
        stream << generateSensor(sensorName, wsf, configInfo) << "\n\n";
    }

    for (auto it = signatureComponents.constBegin(); it != signatureComponents.constEnd(); ++it) {
        const QString& signatureName = it.key();
        QString signatureType = it.value().first;
        QJsonObject comp = it.value().second;
        QJsonObject configInfo = comp["configInfo"].toObject();
        stream << generateSignature(signatureName, signatureType, configInfo) << "\n\n";
    }

    QMap<QString, QJsonObject> routeLookup;
    for (const QJsonValue& routeValue : routesArray) {
        QJsonObject routeObj = routeValue.toObject();
        QString targetUid = routeObj["targetUid"].toString();
        if (targetUid.isEmpty()) {
            targetUid = routeObj["entityId"].toString();
        }
        if (!targetUid.isEmpty()) {
            routeLookup.insert(targetUid, routeObj);
        }
    }

    QSet<QString> emittedRoutes;
    for (auto it = routeLookup.constBegin(); it != routeLookup.constEnd(); ++it) {
        QJsonObject routeObj = it.value();
        QString routeName = routeObj["name"].toString();
        if (routeName.isEmpty()) {
            routeName = QString("route_%1").arg(it.key());
        }
        if (emittedRoutes.contains(routeName)) {
            continue;
        }
        emittedRoutes.insert(routeName);
        stream << generateRoute(routeName, routeObj) << "\n\n";
    }

    for (const QJsonValue& value : entitiesArray) {
        QJsonObject entityObj = value.toObject();
        QString entityUid = entityObj["uid"].toString();
        if (entityUid.isEmpty()) {
            entityUid = entityObj["id"].toString();
        }
        QString routeName;
        if (!entityUid.isEmpty() && routeLookup.contains(entityUid)) {
            routeName = routeLookup.value(entityUid)["name"].toString();
            if (routeName.isEmpty()) {
                routeName = QString("route_%1").arg(entityUid);
            }
        }
        stream << generatePlatform(entityObj, routeName) << "\n\n";
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件进行写入:" << filePath;
        return false;
    }

    QTextStream fileStream(&file);
    fileStream << scriptContent_;
    file.close();

    qDebug() << "AFSIM脚本已生成:" << filePath;
    return true;
}

QString AfsimScriptGenerator::getScriptContent() const
{
    return scriptContent_;
}

QString AfsimScriptGenerator::degreesToDMS(double degrees, bool isLatitude) const
{
    // 确定符号（AFSIM格式：纬度用s/n，经度用e/w，但示例中显示的是s和e）
    QString sign;
    if (isLatitude) {
        sign = (degrees >= 0) ? "n" : "s";
    } else {
        sign = (degrees >= 0) ? "e" : "w";
    }

    degrees = fabs(degrees);

    // 提取度、分、秒
    int d = static_cast<int>(degrees);
    double minutes = (degrees - d) * 60.0;
    int m = static_cast<int>(minutes);
    double seconds = (minutes - m) * 60.0;

    // 格式化为字符串
    return QString("%1:%2:%3.%4%5")
        .arg(d, 2, 10, QChar('0'))
        .arg(m, 2, 10, QChar('0'))
        .arg(static_cast<int>(seconds), 2, 10, QChar('0'))
        .arg(QString::number(seconds - static_cast<int>(seconds), 'f', 3).mid(2))
        .arg(sign);
}

QString AfsimScriptGenerator::generatePlatformType(const QString& platformName, const QString& wsfType,
                                                   const QJsonArray& components, const QJsonObject& entityObj) const
{
    QString result;
    QTextStream stream(&result);

    stream << "platform_type " << platformName << " " << wsfType << "\n";

    // 添加icon（从数据库获取）
    QString modelId = entityObj["modelId"].toString();
    if (!modelId.isEmpty()) {
        QString icon = getModelIconFromDatabase(modelId);
        if (!icon.isEmpty()) {
            stream << "   icon " << icon << "\n";
        }
    }

    // 添加weapon挂载（从实体属性获取）
    QJsonObject weaponMounts = entityObj["weaponMounts"].toObject();
    if (!weaponMounts.isEmpty()) {
        QJsonArray weaponsArray = weaponMounts["weapons"].toArray();
        for (const QJsonValue& weaponValue : weaponsArray) {
            QJsonObject weaponObj = weaponValue.toObject();
            QString weaponName = weaponObj["weaponName"].toString();
            int quantity = weaponObj["quantity"].toInt();
            if (quantity > 0 && !weaponName.isEmpty()) {
                QString alias = weaponName.toLower().replace(" ", "_");
                stream << "   weapon " << alias << " " << weaponName << "\n";
                stream << "      quantity " << quantity << "\n";
                stream << "   end_weapon\n";
            }
        }
    }

    // 提取mover信息（先处理mover）
    QString moverWSF = extractMoverWSF(components);
    QJsonObject moverConfig;
    QString moverComponentId;
    for (const QJsonValue& compValue : components) {
        QJsonObject comp = compValue.toObject();
        QString type = comp["type"].toString();
        if (type == "运动模型") {
            moverComponentId = comp["componentId"].toString();
            moverConfig = comp["configInfo"].toObject();
            break;
        }
    }

    // 如果mover配置为空，尝试从数据库获取
    if (moverConfig.isEmpty() && !moverComponentId.isEmpty()) {
        QJsonObject compInfo = getComponentInfoFromDatabase(moverComponentId);
        moverConfig = compInfo["configInfo"].toObject();
    }

    if (!moverWSF.isEmpty()) {
        stream << "   mover " << moverWSF << "\n";

        // 根据mover类型生成不同的配置
        if (moverWSF == "WSF_STRAIGHT_LINE_MOVER") {
            // 直线运动模型：使用配置中的参数
            if (moverConfig.contains("更新时间（秒）")) {
                double updateInterval = moverConfig["更新时间（秒）"].toDouble();
                stream << "      update_interval " << updateInterval << " s\n";
            } else {
                stream << "      update_interval 0.5 s\n";
            }
            stream << "      \n";

            // 飞行时间与速度表
            if (moverConfig.contains("飞行时间与速度表")) {
                QJsonObject tofSpeed = moverConfig["飞行时间与速度表"].toObject();
                stream << "      tof_and_speed\n";
                // 这里可以根据实际配置解析多个时间-速度对
                if (tofSpeed.contains("时间（秒）") && tofSpeed.contains("速度（节）")) {
                    double time = tofSpeed["时间（秒）"].toDouble();
                    double speed = tofSpeed["速度（节）"].toDouble();
                    stream << "          " << time << " s " << speed << " kts\n";
                } else {
                    // 默认值
                    stream << "          0.0 s 1700 kts\n";
                    stream << "         20.0 s 1400 kts\n";
                    stream << "         50.0 s 1000 kts\n";
                    stream << "         70.0 s 800 kts\n";
                }
                stream << "      end_tof_and_speed\n";
            } else {
                stream << "      tof_and_speed\n";
                stream << "          0.0 s 1700 kts\n";
                stream << "         20.0 s 1400 kts\n";
                stream << "         50.0 s 1000 kts\n";
                stream << "         70.0 s 800 kts\n";
                stream << "      end_tof_and_speed\n";
            }

            // 最大横向过载
            if (moverConfig.contains("最大横向过载（g）")) {
                double maxAccel = moverConfig["最大横向过载（g）"].toDouble();
                stream << "     maximum_lateral_acceleration  " << maxAccel << " g\n";
            } else {
                stream << "     maximum_lateral_acceleration  9 g\n";
            }
        }

        stream << "   end_mover\n";
    }

    // 提取processor信息（在mover之后）
    QList<QPair<QString, QString>> processors = extractProcessorWSF(components);
    for (const auto& proc : processors) {
        QString procName = proc.first;
        QString procWSF = proc.second;

        stream << "   processor " << procName << " " << procWSF << "\n";

        // 从组件配置中获取processor的配置信息
        QJsonObject procConfigObj;
        QString componentId;
        for (const QJsonValue& compValue : components) {
            QJsonObject comp = compValue.toObject();
            QString compWSF = comp["wsf"].toString();
            if (compWSF == procWSF) {
                componentId = comp["componentId"].toString();
                procConfigObj = comp["configInfo"].toObject();
                break;
            }
        }

        // 如果组件配置为空，尝试从数据库获取
        if (procConfigObj.isEmpty() && !componentId.isEmpty()) {
            QJsonObject compInfo = getComponentInfoFromDatabase(componentId);
            procConfigObj = compInfo["configInfo"].toObject();
        }

        // 根据processor类型和配置生成参数
        if (procWSF == "WSF_PERFECT_TRACKER") {
            if (procConfigObj.contains("update_interval")) {
                double interval = procConfigObj["update_interval"].toDouble();
                stream << "      update_interval " << interval << " s\n";
            } else {
                stream << "      update_interval 1 s\n";
            }
        } else if (procWSF == "WSF_AIR_TARGET_FUSE") {
            if (procConfigObj.contains("max_time_of_flight_to_detonate")) {
                double maxTime = procConfigObj["max_time_of_flight_to_detonate"].toDouble();
                stream << "      max_time_of_flight_to_detonate  " << maxTime << " s\n";
            } else {
                stream << "      max_time_of_flight_to_detonate  100 s\n";
            }
        } else if (procWSF == "WSF_TRACK_PROCESSOR") {
            if (procConfigObj.contains("purge_interval")) {
                double purgeInterval = procConfigObj["purge_interval"].toDouble();
                stream << "      purge_interval " << purgeInterval << " s\n";
            }
        }

        stream << "   end_processor\n";
    }

    // 添加sensor引用
    QList<QPair<QString, QString>> sensors = extractSensorWSF(components);
    for (const auto& sensor : sensors) {
        QString sensorName = sensor.first;
        stream << "   sensor " << sensorName << " " << sensor.second << "\n";
        stream << "   end_sensor\n";
    }

    // 添加signature引用
    QList<QPair<QString, QString>> signatures = extractSignatureWSF(components);
    for (const auto& sig : signatures) {
        QString sigType = sig.first;
        QString sigName = sig.second;
        stream << "   " << sigType << "  " << sigName << "\n";
    }

    stream << "end_platform_type";

    return result;
}

QString AfsimScriptGenerator::generateWeaponEffects(const QString& weaponName) const
{
    QString result;
    QTextStream stream(&result);

    stream << "weapon_effects graduated_effect WSF_GRADUATED_LETHALITY\n";
    stream << "   radius_and_pk  500 m 0.7\n";
    stream << "   radius_and_pk  1000 m 0.5\n";
    stream << "   radius_and_pk  2000 m 0.3\n";
    stream << "end_weapon_effects";

    return result;
}

QString AfsimScriptGenerator::generateWeapon(const QString& weaponName, const QString& platformTypeName) const
{
    QString result;
    QTextStream stream(&result);

    stream << "weapon " << weaponName << " WSF_EXPLICIT_WEAPON\n";
    stream << "   launched_platform_type  " << platformTypeName << "\n";
    stream << "   weapon_effects  graduated_effect\n";
    stream << "end_weapon";

    return result;
}

QString AfsimScriptGenerator::generatePlatform(const QJsonObject& entityObj, const QString& routeName) const
{
    QString result;
    QTextStream stream(&result);

    QString modelName = entityObj["modelName"].toString();
    if (modelName.isEmpty()) {
        return result;
    }

    QString entityUid = entityObj["uid"].toString();
    if (entityUid.isEmpty()) {
        entityUid = entityObj["id"].toString();
    }
    QString displayName = entityObj["name"].toString();
    if (displayName.isEmpty()) {
        displayName = entityUid;
    }

    QJsonObject positionObj = entityObj["position"].toObject();
    double latitude = positionObj["latitude"].toDouble();
    double longitude = positionObj["longitude"].toDouble();
    double altitude = positionObj["altitude"].toDouble();

    QString latStr = degreesToDMS(latitude, true);
    QString lonStr = degreesToDMS(longitude, false);

    QString side = entityObj["side"].toString();
    if (side.isEmpty()) {
        side = "blue";
    }

    QString altitudeStr = QString::number(altitude, 'g', 6);

    stream << "platform " << displayName << " " << modelName << "\n";
    stream << "  side " << side << "\n";
    stream << "  position " << latStr << " " << lonStr << " altitude " << altitudeStr << " m agl\n";

    if (!routeName.isEmpty()) {
        stream << "  use_route " << routeName << "\n";
    }

    stream << "end_platform";

    return result;
}

QString AfsimScriptGenerator::extractMoverWSF(const QJsonArray& components) const
{
    for (const auto& compValue : components) {
        QJsonObject comp = compValue.toObject();
        QString type = comp["type"].toString();
        QString wsf = comp["wsf"].toString();

        if (type == "运动模型" && !wsf.isEmpty()) {
            return wsf;
        }
    }
    return QString();
}

QList<QPair<QString, QString>> AfsimScriptGenerator::extractProcessorWSF(const QJsonArray& components) const
{
    QList<QPair<QString, QString>> result;

    for (const auto& compValue : components) {
        QJsonObject comp = compValue.toObject();
        QString type = comp["type"].toString();
        QString wsf = comp["wsf"].toString();
        QString name = comp["name"].toString();

        // 检查是否是processor类型（根据数据库，processor的类型是"处理器"）
        if (type == "处理器" && !wsf.isEmpty()) {
            // 根据WSF类型判断是tracker还是fuse
            if (wsf.contains("TRACKER") || wsf.contains("TRACK")) {
                result.append(QPair<QString, QString>("tracker", wsf));
            } else if (wsf.contains("FUSE")) {
                result.append(QPair<QString, QString>("fuse", wsf));
            } else {
                // 默认作为tracker处理
                result.append(QPair<QString, QString>("tracker", wsf));
            }
        }
    }

    return result;
}

QMap<QString, QString> AfsimScriptGenerator::collectWeapons(const QJsonArray& entitiesArray) const
{
    QMap<QString, QString> weapons;

    for (const QJsonValue& value : entitiesArray) {
        QJsonObject entityObj = value.toObject();
        QJsonObject weaponMounts = entityObj["weaponMounts"].toObject();
        if (weaponMounts.isEmpty()) {
            continue;
        }

        QJsonArray weaponsArray = weaponMounts["weapons"].toArray();
        for (const QJsonValue& weaponValue : weaponsArray) {
            QJsonObject weaponObj = weaponValue.toObject();
            QString weaponId = weaponObj["weaponId"].toString();
            QString weaponName = weaponObj["weaponName"].toString();

            if (!weaponId.isEmpty() && !weaponName.isEmpty()) {
                weapons[weaponId] = weaponName;
            }
        }
    }

    return weapons;
}

QMap<QString, QString> AfsimScriptGenerator::collectPlatformTypes(const QJsonArray& entitiesArray) const
{
    QMap<QString, QString> platformTypes;

    for (const QJsonValue& value : entitiesArray) {
        QJsonObject entityObj = value.toObject();
        QString modelName = entityObj["modelName"].toString();
        if (modelName.isEmpty()) {
            continue;
        }

        QString modelId = entityObj["modelId"].toString();
        QJsonArray components;
        if (!modelId.isEmpty()) {
            components = getModelComponentsFromDatabase(modelId);
        }
        if (components.isEmpty()) {
            QJsonObject modelAssembly = entityObj["modelAssembly"].toObject();
            components = modelAssembly["components"].toArray();
        }

        QString moverWSF = extractMoverWSF(components);
        QString platformWSF = determinePlatformWSF(moverWSF);
        platformTypes[modelName] = platformWSF;
    }

    return platformTypes;
}

bool AfsimScriptGenerator::loadPlanData(QJsonObject& planObj) const
{
    if (!planFileManager_) {
        return false;
    }

    QString planPath = planFileManager_->getCurrentPlanFile();
    if (planPath.isEmpty()) {
        return false;
    }

    QFile planFile(planPath);
    if (!planFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "无法打开方案文件:" << planPath << planFile.errorString();
        return false;
    }

    QByteArray data = planFile.readAll();
    planFile.close();

    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        qDebug() << "方案文件解析失败:" << parseError.errorString();
        return false;
    }

    planObj = doc.object();
    return true;
}

QString AfsimScriptGenerator::generateRoute(const QString& routeName, const QJsonObject& routeObj) const
{
    QString result;
    QTextStream stream(&result);

    QJsonArray waypoints = routeObj["waypoints"].toArray();
    if (waypoints.isEmpty()) {
        return result;
    }

    stream << "route " << routeName << "\n";
    for (int i = 0; i < waypoints.size(); ++i) {
        QJsonObject wp = waypoints.at(i).toObject();
        QString label = wp["label"].toString();
        if (label.isEmpty()) {
            label = QString("Waypoint-%1").arg(i);
        }

        double latitude = wp["latitude"].toDouble();
        double longitude = wp["longitude"].toDouble();
        double altitude = wp["altitude"].toDouble();

        QString latStr = degreesToDMS(latitude, true);
        QString lonStr = degreesToDMS(longitude, false);

        stream << "   label " << label << "\n";
        stream << "   position " << latStr << " " << lonStr << " altitude " << QString::number(altitude, 'g', 6) << " m";

        if (wp.contains("speed")) {
            double speed = wp["speed"].toDouble();
            stream << "  speed " << QString::number(speed, 'g', 6) << " m/s";
        }

        stream << "\n";
    }

    stream << "end_route";

    return result;
}

QString AfsimScriptGenerator::determinePlatformWSF(const QString& moverWSF) const
{
    if (moverWSF.contains("AIR")) {
        return "WSF_AIR_PLATFORM";
    }
    if (moverWSF.contains("GROUND")) {
        return "WSF_GROUND_PLATFORM";
    }
    if (moverWSF.contains("SEA")) {
        return "WSF_SEA_PLATFORM";
    }
    return "WSF_PLATFORM";
}

QJsonObject AfsimScriptGenerator::mergeComponentConfig(const QJsonObject& component) const
{
    QJsonObject result = component;
    QString componentId = result["componentId"].toString();

    bool needConfig = !result.contains("configInfo") || !result.value("configInfo").isObject();
    if (needConfig && !componentId.isEmpty()) {
        QJsonObject dbInfo = getComponentInfoFromDatabase(componentId);
        QJsonObject config = dbInfo["configInfo"].toObject();
        if (!config.isEmpty()) {
            result["configInfo"] = config;
        }
        if (result["wsf"].toString().isEmpty()) {
            result["wsf"] = dbInfo["wsf"].toString();
        }
        if (result["type"].toString().isEmpty()) {
            result["type"] = dbInfo["type"].toString();
        }
        if (result["name"].toString().isEmpty()) {
            result["name"] = dbInfo["name"].toString();
        }
    }

    return result;
}

QString AfsimScriptGenerator::getProcessorConfigFromDatabase(const QString& wsf) const
{
    if (!DatabaseUtils::openDatabase()) {
        return QString();
    }

    QSqlQuery query;
    query.prepare("SELECT afsimtype FROM ComponentType WHERE wsf = ?");
    query.addBindValue(wsf);

    if (query.exec() && query.next()) {
        QString afsimType = query.value(0).toString();
        if (!afsimType.isEmpty()) {
            // afsimtype字段可能包含AFSIM配置模板
            // 这里可以根据实际格式解析并返回
            return afsimType;
        }
    }

    return QString();
}

QJsonObject AfsimScriptGenerator::getComponentInfoFromDatabase(const QString& componentId) const
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT ci.componentid, ci.name, ci.type, ci.configinfo, "
                  "ct.wsf, ct.subtype, ct.afsimtype "
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

        // 解析AFSIM类型（如果有）
        QString afsimType = query.value(6).toString();
        if (!afsimType.isEmpty()) {
            result["afsimtype"] = afsimType;
        }
    }

    return result;
}

QJsonArray AfsimScriptGenerator::getModelComponentsFromDatabase(const QString& modelId) const
{
    QJsonArray result;

    if (!DatabaseUtils::openDatabase()) {
        return result;
    }

    // 首先获取模型的组件列表
    QSqlQuery modelQuery;
    modelQuery.prepare("SELECT componentlist FROM ModelInformation WHERE id = ?");
    modelQuery.addBindValue(modelId);

    QStringList componentIds;
    if (modelQuery.exec() && modelQuery.next()) {
        QString componentListStr = modelQuery.value(0).toString();
        componentIds = componentListStr.split(',', Qt::SkipEmptyParts);
    }

    // 为每个组件ID获取完整信息
    for (const QString& compId : componentIds) {
        QJsonObject compInfo = getComponentInfoFromDatabase(compId.trimmed());
        if (!compInfo.isEmpty()) {
            result.append(compInfo);
        }
    }

    return result;
}

QString AfsimScriptGenerator::generateSensor(const QString& sensorName, const QString& wsfType, const QJsonObject& configInfo) const
{
    QString result;
    QTextStream stream(&result);

    stream << "sensor " << sensorName << " " << wsfType << "\n";

    // 根据配置信息生成sensor参数
    if (wsfType == "WSF_RADAR_SENSOR") {
        // 雷达传感器参数
        if (configInfo.contains("1m²目标探测距离（海里）")) {
            double range = configInfo["1m²目标探测距离（海里）"].toDouble();
            stream << "   one_m2_detect_range " << range << " nm\n";
        }
        if (configInfo.contains("最大探测距离（海里）")) {
            double maxRange = configInfo["最大探测距离（海里）"].toDouble();
            stream << "   maximum_range " << maxRange << " nm\n";
        }
        if (configInfo.contains("天线高度(m)")) {
            double height = configInfo["天线高度(m)"].toDouble();
            stream << "   antenna_height " << height << " m\n";
        }
        if (configInfo.contains("帧时间（秒）")) {
            double frameTime = configInfo["帧时间（秒）"].toDouble();
            stream << "   frame_time " << frameTime << " s\n";
        }
        if (configInfo.contains("扫描模式")) {
            int scanMode = configInfo["扫描模式"].toInt();
            if (scanMode == 0) {
                stream << "   scan_mode azimuth_and_elevation\n";
            }
        }
        if (configInfo.contains("高程扫描限值（度）")) {
            QString elevLimits = configInfo["高程扫描限值（度）"].toString();
            QStringList limits = elevLimits.split(',');
            if (limits.size() >= 2) {
                stream << "   elevation_scan_limits " << limits[0].trimmed() << " deg " << limits[1].trimmed() << " deg\n";
            }
        }
        if (configInfo.contains("方位角扫描限值（度）")) {
            QString azimLimits = configInfo["方位角扫描限值（度）"].toString();
            QStringList limits = azimLimits.split(',');
            if (limits.size() >= 2) {
                stream << "   azimuth_scan_limits " << limits[0].trimmed() << " deg " << limits[1].trimmed() << " deg\n";
            }
        }

        // 发射器参数
        if (configInfo.contains("发射器")) {
            QJsonObject transmitter = configInfo["发射器"].toObject();
            stream << "   transmitter\n";
            if (transmitter.contains("发射功率（千瓦）")) {
                double power = transmitter["发射功率（千瓦）"].toDouble();
                stream << "      power " << power << " kw\n";
            }
            if (transmitter.contains("内部损耗（分贝）")) {
                double loss = transmitter["内部损耗（分贝）"].toDouble();
                stream << "      internal_loss " << loss << " db\n";
            }
            if (transmitter.contains("频率（兆赫）")) {
                double freq = transmitter["频率（兆赫）"].toDouble();
                stream << "      frequency " << freq << " mhz\n";
            }
            stream << "   end_transmitter\n";
        }

        // 接收器参数
        if (configInfo.contains("接收器")) {
            QJsonObject receiver = configInfo["接收器"].toObject();
            stream << "   receiver\n";
            if (receiver.contains("带宽（兆赫）")) {
                double bandwidth = receiver["带宽（兆赫）"].toDouble();
                stream << "      bandwidth " << bandwidth << " mhz\n";
            }
            if (receiver.contains("噪声功率（分贝瓦）")) {
                double noise = receiver["噪声功率（分贝瓦）"].toDouble();
                stream << "      noise_power " << noise << " dbw\n";
            }
            stream << "   end_receiver\n";
        }

        if (configInfo.contains("虚警概率")) {
            double pfa = configInfo["虚警概率"].toDouble();
            stream << "   probability_of_false_alarm " << pfa << "\n";
        }
        if (configInfo.contains("斯威林模型")) {
            int swerling = configInfo["斯威林模型"].toInt();
            stream << "   swerling_case " << swerling << "\n";
        }
        if (configInfo.contains("所需探测概率")) {
            double pd = configInfo["所需探测概率"].toDouble();
            stream << "   required_pd " << pd << "\n";
        }
        if (configInfo.contains("建立航迹所需探测次数")) {
            QString hits = configInfo["建立航迹所需探测次数"].toString();
            QStringList hitList = hits.split(',');
            if (hitList.size() >= 2) {
                stream << "   hits_to_establish_track " << hitList[0].trimmed() << " " << hitList[1].trimmed() << "\n";
            }
        }
        if (configInfo.contains("维持航迹所需探测次数")) {
            QString hits = configInfo["维持航迹所需探测次数"].toString();
            QStringList hitList = hits.split(',');
            if (hitList.size() >= 2) {
                stream << "   hits_to_maintain_track " << hitList[0].trimmed() << " " << hitList[1].trimmed() << "\n";
            }
        }
        if (configInfo.contains("航迹建立概率")) {
            double quality = configInfo["航迹建立概率"].toDouble();
            stream << "   track_quality " << quality << "\n";
        }

        // 报告选项
        if (configInfo.contains("报告位置") && configInfo["报告位置"].toBool()) {
            stream << "   reports_location\n";
        }
        if (configInfo.contains("报告方位") && configInfo["报告方位"].toBool()) {
            stream << "   reports_bearing\n";
        }
        if (configInfo.contains("报告距离") && configInfo["报告距离"].toBool()) {
            stream << "   reports_range\n";
        }
        if (configInfo.contains("报告敌我识别") && configInfo["报告敌我识别"].toBool()) {
            stream << "   reports_iff\n";
        }
        if (configInfo.contains("报告高程") && configInfo["报告高程"].toBool()) {
            stream << "   reports_elevation\n";
        }
    } else if (wsfType == "WSF_INFRARED_SENSOR") {
        // 红外传感器参数
        if (configInfo.contains("天线高度(m)")) {
            double height = configInfo["天线高度(m)"].toDouble();
            stream << "   antenna_height " << height << " m\n";
        }
        if (configInfo.contains("扫描模式")) {
            int scanMode = configInfo["扫描模式"].toInt();
            stream << "   scan_mode " << scanMode << "\n";
        }
        if (configInfo.contains("报告位置") && configInfo["报告位置"].toBool()) {
            stream << "   reports_location\n";
        }
        if (configInfo.contains("斯威林模型")) {
            int swerling = configInfo["斯威林模型"].toInt();
            stream << "   swerling_case " << swerling << "\n";
        }
    }

    stream << "end_sensor";

    return result;
}

QString AfsimScriptGenerator::generateSignature(const QString& signatureName, const QString& signatureType, const QJsonObject& configInfo) const
{
    QString result;
    QTextStream stream(&result);

    stream << signatureType << "  " << signatureName << "\n";

    // 根据signature类型生成参数
    if (signatureType == "radar_signature") {
        if (configInfo.contains("constant(㎡)")) {
            double constant = configInfo["constant(㎡)"].toDouble();
            stream << "      constant " << constant << " m^2\n";
        }
    } else if (signatureType == "infrared_signature") {
        if (configInfo.contains("constant(w/sr)")) {
            double constant = configInfo["constant(w/sr)"].toDouble();
            stream << "   constant " << constant << "  w/sr\n";
        }
    } else if (signatureType == "optical_signature") {
        if (configInfo.contains("constant(㎡)")) {
            double constant = configInfo["constant(㎡)"].toDouble();
            stream << "      constant " << constant << " m^2\n";
        }
    }

    stream << "end_" << signatureType;

    return result;
}

QList<QPair<QString, QString>> AfsimScriptGenerator::extractSensorWSF(const QJsonArray& components) const
{
    QList<QPair<QString, QString>> result;

    for (const auto& compValue : components) {
        QJsonObject comp = compValue.toObject();
        QString type = comp["type"].toString();
        QString wsf = comp["wsf"].toString();
        QString name = comp["name"].toString();

        if ((type == "传感器" || type == "雷达传感器" || type == "红外传感器") && !wsf.isEmpty() && !name.isEmpty()) {
            result.append(QPair<QString, QString>(name, wsf));
        }
    }

    return result;
}

QList<QPair<QString, QString>> AfsimScriptGenerator::extractSignatureWSF(const QJsonArray& components) const
{
    QList<QPair<QString, QString>> result;

    for (const auto& compValue : components) {
        QJsonObject comp = compValue.toObject();
        QString type = comp["type"].toString();
        QString wsf = comp["wsf"].toString();
        QString name = comp["name"].toString();

        if (type == "目标特性" || type == "雷达特征" || type == "红外特征" || type == "光学特征") {
            QString signatureType;
            if (wsf.contains("radar") || type == "雷达特征") {
                signatureType = "radar_signature";
            } else if (wsf.contains("infrared") || type == "红外特征") {
                signatureType = "infrared_signature";
            } else if (wsf.contains("optical") || type == "光学特征") {
                signatureType = "optical_signature";
            }

            if (!signatureType.isEmpty() && !name.isEmpty()) {
                result.append(QPair<QString, QString>(signatureType, name));
            }
        }
    }

    return result;
}

QString AfsimScriptGenerator::getModelIconFromDatabase(const QString& modelId) const
{
    if (!DatabaseUtils::openDatabase()) {
        return QString();
    }

    QSqlQuery query;
    query.prepare("SELECT icon FROM ModelInformation WHERE id = ?");
    query.addBindValue(modelId);

    if (query.exec() && query.next()) {
        QString iconPath = query.value(0).toString();
        if (!iconPath.isEmpty()) {
            // 提取文件名（不含路径）
            QFileInfo fileInfo(iconPath);
            QString fileName = fileInfo.baseName(); // 不含扩展名
            return fileName.toLower();
        }
    }

    return QString();
}

