/**
 * @file afsimscriptgenerator.h
 * @brief AFSIM脚本生成器头文件
 *
 * 定义AfsimScriptGenerator类，用于根据地图上的实体信息生成AFSIM脚本
 */

#ifndef AFSIMSCRIPTGENERATOR_H
#define AFSIMSCRIPTGENERATOR_H

#include <QString>
#include <QList>
#include <QMap>
#include <QJsonArray>
#include <QJsonObject>

// 前向声明
class GeoEntityManager;
class PlanFileManager;

/**
 * @brief AFSIM脚本生成器
 *
 * 根据地图上当前部署的实体信息、挂载信息和路线信息，生成AFSIM脚本代码。
 *
 * 生成的脚本包括：
 * - platform_type定义（平台类型）
 * - weapon_effects定义（武器效果）
 * - weapon定义（武器）
 * - platform定义（平台实例，包含位置和路线）
 */
class AfsimScriptGenerator
{
public:
    /**
     * @brief 构造函数
     * @param entityManager 实体管理器指针
     * @param planFileManager 方案文件管理器指针
     */
    explicit AfsimScriptGenerator(GeoEntityManager* entityManager, PlanFileManager* planFileManager);

    /**
     * @brief 生成AFSIM脚本
     * @param filePath 保存脚本的文件路径
     * @return 成功返回true，失败返回false
     */
    bool generateScript(const QString& filePath);

    /**
     * @brief 获取生成的脚本内容（用于预览）
     * @return 脚本内容字符串
     */
    QString getScriptContent() const;

private:
    /**
     * @brief 将十进制度数转换为度分秒格式
     * @param degrees 十进制度数
     * @param isLatitude 是否为纬度（true为纬度，false为经度）
     * @return 度分秒格式字符串，如"39:57:07.522s"或"17:55:59.764e"
     */
    QString degreesToDMS(double degrees, bool isLatitude) const;

    /**
     * @brief 生成platform_type定义
     * @param platformName 平台类型名称
     * @param wsfType WSF类型
     * @param components 组件列表
     * @param entityObj 实体信息（用于获取weapon挂载等）
     * @return platform_type定义字符串
     */
    QString generatePlatformType(const QString& platformName, const QString& wsfType,
                                 const QJsonArray& components, const QJsonObject& entityObj) const;

    /**
     * @brief 生成weapon_effects定义
     * @param weaponName 武器名称
     * @return weapon_effects定义字符串
     */
    QString generateWeaponEffects(const QString& weaponName) const;

    /**
     * @brief 生成weapon定义
     * @param weaponName 武器名称
     * @param platformTypeName 平台类型名称
     * @return weapon定义字符串
     */
    QString generateWeapon(const QString& weaponName, const QString& platformTypeName) const;

    /**
     * @brief 生成platform实例定义
     * @param entityObj 实体信息
     * @param routeName 路线名称（如果有）
     * @return platform定义字符串
     */
    QString generatePlatform(const QJsonObject& entityObj, const QString& routeName = QString()) const;

    /**
     * @brief 从组件列表中提取mover的WSF类型
     * @param components 组件列表
     * @return mover的WSF类型，如果未找到返回空字符串
     */
    QString extractMoverWSF(const QJsonArray& components) const;

    /**
     * @brief 从组件列表中提取processor的WSF类型
     * @param components 组件列表
     * @return processor的WSF类型列表
     */
    QList<QPair<QString, QString>> extractProcessorWSF(const QJsonArray& components) const;

    /**
     * @brief 获取实体的路线名称
     * @param entity 实体指针
     * @return 路线名称，如果没有路线返回空字符串
     */
    /**
     * @brief 加载当前方案文件数据
     * @param planObj 输出的方案JSON对象
     * @return 成功返回true
     */
    bool loadPlanData(QJsonObject& planObj) const;

    /**
     * @brief 收集所有使用的武器信息
     * @param entitiesArray 实体数组
     * @return 武器ID到武器名称的映射
     */
    QMap<QString, QString> collectWeapons(const QJsonArray& entitiesArray) const;

    /**
     * @brief 收集所有平台类型信息
     * @param entitiesArray 实体数组
     * @return 平台类型名称到WSF类型的映射
     */
    QMap<QString, QString> collectPlatformTypes(const QJsonArray& entitiesArray) const;

    /**
     * @brief 从数据库获取processor的配置信息
     * @param wsf WSF类型
     * @return 配置字符串，如果未找到返回空字符串
     */
    QString getProcessorConfigFromDatabase(const QString& wsf) const;

    /**
     * @brief 从数据库获取组件的完整信息
     * @param componentId 组件ID
     * @return 组件信息JSON对象，包含configInfo等
     */
    QJsonObject getComponentInfoFromDatabase(const QString& componentId) const;

    /**
     * @brief 从数据库获取模型的所有组件信息
     * @param modelId 模型ID
     * @return 组件信息数组
     */
    QJsonArray getModelComponentsFromDatabase(const QString& modelId) const;

    /**
     * @brief 生成sensor定义
     * @param sensorName 传感器名称
     * @param wsfType WSF类型
     * @param configInfo 配置信息
     * @return sensor定义字符串
     */
    QString generateSensor(const QString& sensorName, const QString& wsfType, const QJsonObject& configInfo) const;

    /**
     * @brief 生成route定义
     * @param routeName 路线名称
     * @param routeObj 路线JSON对象
     * @return route定义字符串
     */
    QString generateRoute(const QString& routeName, const QJsonObject& routeObj) const;
    /**
     * @brief 根据mover类型推断平台WSF
     * @param moverWSF mover的WSF类型
     * @return 平台WSF类型
     */
    QString determinePlatformWSF(const QString& moverWSF) const;

    /**
     * @brief 合并组件配置，确保configInfo存在
     * @param component 组件对象
     * @return 带有configInfo的组件对象
     */
    QJsonObject mergeComponentConfig(const QJsonObject& component) const;

    /**
     * @brief 生成signature定义
     * @param signatureName 特征名称
     * @param signatureType 特征类型（radar/infrared/optical）
     * @param configInfo 配置信息
     * @return signature定义字符串
     */
    QString generateSignature(const QString& signatureName, const QString& signatureType, const QJsonObject& configInfo) const;

    /**
     * @brief 从组件列表中提取sensor的WSF类型
     * @param components 组件列表
     * @return sensor的WSF类型列表
     */
    QList<QPair<QString, QString>> extractSensorWSF(const QJsonArray& components) const;

    /**
     * @brief 从组件列表中提取signature的WSF类型
     * @param components 组件列表
     * @return signature的WSF类型列表
     */
    QList<QPair<QString, QString>> extractSignatureWSF(const QJsonArray& components) const;

    /**
     * @brief 从数据库获取模型的icon路径
     * @param modelId 模型ID
     * @return icon文件名（不含路径）
     */
    QString getModelIconFromDatabase(const QString& modelId) const;

    GeoEntityManager* entityManager_;
    PlanFileManager* planFileManager_;
    QString scriptContent_;
};

#endif // AFSIMSCRIPTGENERATOR_H

