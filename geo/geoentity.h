#ifndef GEOENTITY_H
#define GEOENTITY_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMap>
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
 * 所有地理实体的基类，提供实体管理的基础功能：
 * - 位置管理（经纬度和高度）
 * - 朝向管理（航向角）
 * - 状态管理（可见性和选中状态）
 * - 属性管理（动态属性设置）
 * - 节点管理（OSG渲染节点）
 * 
 * 子类必须实现生命周期与节点构建：
 * - initialize(), update(), cleanup(), createNode()
 */
class GeoEntity : public QObject
{
    Q_OBJECT

public:
    GeoEntity(const QString& id, const QString& name, const QString& type,
              double longitude, double latitude, double altitude, QObject* parent = nullptr);
    virtual ~GeoEntity() = default;

    // 基本信息
    QString getId() const { return entityId_; }
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
    
    // 生命周期（子类实现）
    /** @brief 初始化实体资源与节点 */
    virtual void initialize() = 0;
    /** @brief 根据业务状态更新实体 */
    virtual void update() = 0;
    /** @brief 释放资源与场景引用 */
    virtual void cleanup() = 0;

signals:
    void positionChanged(double longitude, double latitude, double altitude);
    void headingChanged(double heading);
    void visibilityChanged(bool visible);
    void selectionChanged(bool selected);
    void propertyChanged(const QString& key, const QVariant& value);

protected:
    QString entityId_;
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
};

#endif // GEOENTITY_H
