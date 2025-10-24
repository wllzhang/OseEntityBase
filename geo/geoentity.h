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
    
    // 位置管理 - 自我管理
    void setPosition(double longitude, double latitude, double altitude);
    void getPosition(double& longitude, double& latitude, double& altitude) const;
    
    // 朝向管理 - 自我管理  
    void setHeading(double headingDegrees);
    double getHeading() const { return heading_; }
    
    // 状态管理 - 自我管理
    void setVisible(bool visible);
    bool isVisible() const { return visible_; }
    
    void setSelected(bool selected);
    bool isSelected() const { return selected_; }
    
    // 渲染节点管理 - 自我管理
    osg::ref_ptr<osg::Node> getNode() const { return node_; }
    void updateNode(); // 更新渲染节点
    
    // 属性管理 - 自我管理
    void setProperty(const QString& key, const QVariant& value);
    QVariant getProperty(const QString& key) const;
    QMap<QString, QVariant> getAllProperties() const { return properties_; }
    
    // 生命周期管理 - 自我管理
    virtual void initialize() = 0;  // 子类实现初始化
    virtual void update() = 0;      // 子类实现更新逻辑
    virtual void cleanup() = 0;     // 子类实现清理

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
