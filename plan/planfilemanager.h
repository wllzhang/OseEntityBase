/**
 * @file planfilemanager.h
 * @brief 方案文件管理器头文件
 * 
 * 定义PlanFileManager类，负责方案文件的创建、保存、加载和管理
 */

#ifndef PLANFILEMANAGER_H
#define PLANFILEMANAGER_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QList>
#include <QTimer>
#include <atomic>

// 前向声明
class GeoEntity;
class GeoEntityManager;
struct ModelInfo;  // 在ModelAssemblyDialog.h中定义

/**
 * @brief 方案文件管理器
 * 
 * 负责方案文件的创建、保存、加载和管理。
 * 方案文件采用JSON格式存储，包含实体列表、航点、航线等信息。
 */
class PlanFileManager : public QObject
{
    Q_OBJECT

public:
    explicit PlanFileManager(GeoEntityManager* entityManager, QObject *parent = nullptr);
    ~PlanFileManager();

    /**
     * @brief 创建新方案
     * @param name 方案名称
     * @param description 方案描述（可选）
     * @return 成功返回true
     */
    bool createPlan(const QString& name, const QString& description = "");

    /**
     * @brief 保存方案到文件
     * @param filePath 文件路径（如果为空则使用当前方案文件路径）
     * @return 成功返回true
     */
    bool savePlan(const QString& filePath = QString());

    /**
     * @brief 加载方案文件
     * @param filePath 方案文件路径
     * @return 成功返回true
     */
    bool loadPlan(const QString& filePath);
    void requestCancelLoad();

    /**
     * @brief 获取当前方案文件路径
     * @return 当前方案文件路径，如果没有则返回空字符串
     */
    QString getCurrentPlanFile() const;

    /**
     * @brief 设置当前方案文件路径
     * @param filePath 方案文件路径
     */
    void setCurrentPlanFile(const QString& filePath);

    /**
     * @brief 添加实体到方案
     * @param entity 实体指针
     */
    void addEntityToPlan(GeoEntity* entity);

    /**
     * @brief 从方案中移除实体
     * @param uid 实体UID
     */
    void removeEntityFromPlan(const QString& uid);

    /**
     * @brief 更新方案中的实体
     * @param entity 实体指针
     */
    void updateEntityInPlan(GeoEntity* entity);

    /**
     * @brief 检查是否有未保存的更改
     * @return 有未保存更改返回true
     */
    bool hasUnsavedChanges() const;

    /** @brief 手动标记方案已发生修改（发出planDataChanged信号） */
    void markPlanModified();

    /**
     * @brief 设置实体管理器
     * @param entityManager 实体管理器指针
     */
    void setEntityManager(GeoEntityManager* entityManager);
    
    /**
     * @brief 设置自动保存选项
     * @param enabled 是否启用自动保存
     * @param intervalMs 自动保存间隔（毫秒），默认2000ms（2秒）
     */
    void setAutoSaveEnabled(bool enabled, int intervalMs = 2000);

    /**
     * @brief 获取方案文件保存目录
     * @return 目录路径
     */
    static QString getPlansDirectory();
    
    /**
     * @brief 设置相机视角（保存方案时调用）
     * @param longitude 经度
     * @param latitude 纬度
     * @param altitude 高度
     * @param heading 航向角
     * @param pitch 俯仰角
     * @param range 距离
     */
    void setCameraViewpoint(double longitude, double latitude, double altitude,
                           double heading, double pitch, double range);
    
    /**
     * @brief 获取相机视角（加载方案时调用）
     * @param longitude 输出经度
     * @param latitude 输出纬度
     * @param altitude 输出高度
     * @param heading 输出航向角
     * @param pitch 输出俯仰角
     * @param range 输出距离
     * @return 如果存在保存的视角返回true，否则返回false
     */
    bool getCameraViewpoint(double& longitude, double& latitude, double& altitude,
                           double& heading, double& pitch, double& range) const;

signals:
    /**
     * @brief 当前方案文件改变时发出
     * @param filePath 新的方案文件路径
     */
    void planFileChanged(const QString& filePath);

    /**
     * @brief 方案保存完成时发出
     * @param filePath 保存的文件路径
     */
    void planSaved(const QString& filePath);

    /**
     * @brief 方案加载完成时发出
     * @param filePath 加载的文件路径
     */
    void planLoaded(const QString& filePath);

    /**
     * @brief 方案数据更改时发出（用于标记未保存状态）
     */
    void planDataChanged();

private:
    /**
     * @brief 序列化实体为JSON对象
     * @param entity 实体指针
     * @return JSON对象
     */
    QJsonObject entityToJson(GeoEntity* entity);

    /**
     * @brief 从JSON对象创建实体
     * @param json JSON对象
     * @return 创建的实体指针，失败返回nullptr
     */
    GeoEntity* jsonToEntity(const QJsonObject& json);

    /**
     * @brief 从数据库获取模型信息
     * @param modelId 模型ID
     * @return 模型信息
     */
    ModelInfo getModelInfoFromDatabase(const QString& modelId);

    /**
     * @brief 从数据库获取模型组装信息（JSON格式）
     * @param modelId 模型ID
     * @return JSON对象，包含location、icon、componentList
     */
    QJsonObject getModelAssemblyFromDatabase(const QString& modelId);

    /**
     * @brief 从数据库获取组件配置
     * @param componentId 组件ID
     * @return 组件配置JSON对象
     */
    QJsonObject getComponentConfigFromDatabase(const QString& componentId);
    
    /**
     * @brief 从数据库获取完整的组件信息（包括componentId, name, type, configInfo, templateInfo等）
     * @param componentId 组件ID
     * @return 完整的组件信息JSON对象
     */
    QJsonObject getComponentFullInfoFromDatabase(const QString& componentId);

 
    /**
     * @brief 生成方案文件名
     * @param name 方案名称
     * @return 文件名（不含路径）
     */
    QString generatePlanFileName(const QString& name);

signals:
    void loadProgress(int current, int total, const QString& message);
    void loadCancelled();

private:
    /**
     * @brief 比较两个JSON对象是否有差异
     * @param obj1 第一个对象
     * @param obj2 第二个对象
     * @return 有差异返回true
     */
    bool jsonObjectsDiffer(const QJsonObject& obj1, const QJsonObject& obj2);

    GeoEntityManager* entityManager_;  // 实体管理器
    QString currentPlanFile_;           // 当前方案文件路径
    QString planName_;                  // 方案名称
    QString planDescription_;           // 方案描述
    QDateTime createTime_;              // 创建时间
    int entityCounter_;                 // 实体计数器（用于生成唯一ID）
    bool hasUnsavedChanges_;           // 是否有未保存的更改
    
    // 自动保存相关
    QTimer* autoSaveTimer_;             // 自动保存定时器
    bool autoSaveEnabled_;              // 是否启用自动保存
    
    // 相机视角保存
    bool hasCameraViewpoint_;           // 是否有保存的相机视角
    double cameraLongitude_;
    double cameraLatitude_;
    double cameraAltitude_;
    double cameraHeading_;
    double cameraPitch_;
    double cameraRange_;

    std::atomic_bool cancelLoad_;
};

#endif // PLANFILEMANAGER_H

