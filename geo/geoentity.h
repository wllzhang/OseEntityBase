#ifndef GEOENTITY_H
#define GEOENTITY_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
#include <QUuid>
#include <QColor>
#include <osg/Node>
#include <osg/PositionAttitudeTransform>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @file geoentity.h
 * @brief 通用地理实体抽象接口与基础实现。
 *
 * @defgroup geo_entities Geo Entities
 * 地理实体模块：包含通用实体抽象与具体实体实现。
 *  - 基类：GeoEntity
 *  - 示例实现：ImageEntity、WaypointEntity
 */

/**
 * @ingroup geo_entities
 * @brief 通用地理实体基类
 * 
 * 继承自QObject，所有地理实体的基类，提供实体管理的基础功能：
 * - 位置管理（经纬度和高度）
 * - 朝向管理（航向角）
 * - 状态管理（可见性和选中状态）
 * - 属性管理（动态属性设置）
 * - 节点管理（OSG渲染节点）
 * 
 * **继承关系：**
 * ```
 * QObject
 *   └─ GeoEntity
 *       ├─ ImageEntity
 *       └─ WaypointEntity
 * ```
 * 
 * 子类必须实现生命周期与节点构建：
 * - initialize(), update(), cleanup(), createNode()
 */
class GeoEntity : public QObject
{
    Q_OBJECT

public:
    GeoEntity(const QString& name, const QString& type,
              double longitude, double latitude, double altitude,
              const QString& uidOverride = QString(), QObject* parent = nullptr);
    virtual ~GeoEntity() = default;

    // 基本信息
    /** @brief 获取实体实例的稳定唯一UID（统一标识符，替代原entityId） */
    const QString& getId() const { return uid_; }
    const QString& getUid() const { return uid_; }  // 别名，保持兼容
    QString getName() const { return entityName_; }
    QString getType() const { return entityType_; }
    
    /**
     * @brief 设置实体的地理位置
     * @param longitude 经度（度）
     * @param latitude  纬度（度）
     * @param altitude  高度（米）
     */
    void setPosition(double longitude, double latitude, double altitude);
    /**
     * @brief 获取实体的地理位置
     * @param longitude 输出经度
     * @param latitude  输出纬度
     * @param altitude  输出高度
     */
    void getPosition(double& longitude, double& latitude, double& altitude) const;
    
    /**
     * @brief 设置航向角
     * @param headingDegrees 航向角（度），0表示正北，顺时针为正
     */
    void setHeading(double headingDegrees);
    double getHeading() const { return heading_; }
    
    /** @brief 设置可见性 */
    void setVisible(bool visible);
    bool isVisible() const { return visible_; }
    
    /** @brief 设置选中状态（子类可重写用于高亮等效果） */
    virtual void setSelected(bool selected);
    bool isSelected() const { return selected_; }
    
    /** @brief 获取渲染节点 */
    osg::ref_ptr<osg::Node> getNode() const { return node_; }
    /** @brief 根据当前状态刷新渲染节点 */
    void updateNode();
    
    /** @brief 设置自定义属性（可用于外挂业务数据） */
    void setProperty(const QString& key, const QVariant& value);
    /** @brief 读取自定义属性 */
    QVariant getProperty(const QString& key) const;
    QMap<QString, QVariant> getAllProperties() const { return properties_; }
    
    // 生命周期（基类提供默认实现，子类可重写扩展）
    /** 
     * @brief 初始化实体资源与节点
     * 
     * 默认实现：创建节点、设置可见性、设置初始变换。
     * 子类可重写扩展特定初始化逻辑，或重写 onInitialized() 回调。
     */
    virtual void initialize();
    /** 
     * @brief 根据业务状态更新实体
     * 
     * 默认实现：更新节点变换。
     * 子类可重写扩展特定更新逻辑，或重写 onUpdated() 回调。
     */
    virtual void update();
    /** 
     * @brief 释放资源与场景引用
     * 
     * 默认实现：调用 onBeforeCleanup()，清除节点引用，调用 onAfterCleanup()。
     * 子类可重写扩展特定清理逻辑，或重写回调方法。
     */
    virtual void cleanup();

signals:
    void positionChanged(double longitude, double latitude, double altitude);
    void headingChanged(double heading);
    void visibilityChanged(bool visible);
    void selectionChanged(bool selected);
    void propertyChanged(const QString& key, const QVariant& value);

protected:
    // 稳定唯一实例UID（统一标识符，用于所有实体查找和管理）
    QString uid_;
    QString entityName_;
    QString entityType_;
    
    double longitude_, latitude_, altitude_;
    double heading_;
    bool visible_;
    bool selected_;
    
    QMap<QString, QVariant> properties_;
    osg::ref_ptr<osg::Node> node_;
    
    // 子类需要实现的纯虚函数
    virtual osg::ref_ptr<osg::Node> createNode() = 0;
    
    // 生命周期回调（子类可重写扩展）
    /** @brief 节点创建完成后的回调，子类可重写添加特定初始化逻辑 */
    virtual void onInitialized() {}
    /** @brief 更新完成后的回调，子类可重写添加特定更新逻辑 */
    virtual void onUpdated() {}
    /** @brief 清理前的回调，子类可重写清理特定资源（如高亮节点） */
    virtual void onBeforeCleanup() {}
    /** @brief 清理后的回调，子类可重写做额外清理 */
    virtual void onAfterCleanup() {}
    
    // 节点变换辅助方法
    /** 
     * @brief 设置节点的位置和旋转变换
     * 
     * 如果节点是PositionAttitudeTransform，自动设置位置和旋转。
     * 通常在createNode()后调用，或用于更新节点变换。
     * 
     * @param node 要设置的节点（如果是PAT则设置，否则忽略）
     */
    void setupNodeTransform(osg::Node* node);
    
    /** 
     * @brief 创建并初始化PositionAttitudeTransform节点
     * 
     * 创建PAT节点并设置初始位置和旋转，子类可直接使用。
     * 
     * @return 已设置好变换的PAT节点
     */
    osg::ref_ptr<osg::PositionAttitudeTransform> createPATNode();
};

#endif // GEOENTITY_H
