#include "geoentity.h"
#include <QDebug>
#include <QColor>

GeoEntity::GeoEntity(const QString& id, const QString& name, const QString& type,
                     double longitude, double latitude, double altitude, QObject* parent)
    : QObject(parent)
    , entityId_(id)
    , entityName_(name)
    , entityType_(type)
    , longitude_(longitude)
    , latitude_(latitude)
    , altitude_(altitude)
    , heading_(0.0)
    , visible_(true)
    , selected_(false)
{
    // 设置默认属性
    setProperty("size", 100.0);
    setProperty("color", QColor(255, 255, 255));
    setProperty("opacity", 1.0);
}

void GeoEntity::setPosition(double longitude, double latitude, double altitude)
{
    longitude_ = longitude;
    latitude_ = latitude;
    altitude_ = altitude;
    
    // 自我更新渲染节点
    updateNode();
    
    // 发出信号
    emit positionChanged(longitude, latitude, altitude);
}

void GeoEntity::getPosition(double& longitude, double& latitude, double& altitude) const
{
    longitude = longitude_;
    latitude = latitude_;
    altitude = altitude_;
}

void GeoEntity::setHeading(double headingDegrees)
{
    heading_ = headingDegrees;
    
    // 自我更新渲染节点
    updateNode();
    
    // 发出信号
    emit headingChanged(heading_);
}

void GeoEntity::setVisible(bool visible)
{
    visible_ = visible;
    
    // 自我更新渲染节点
    if (node_) {
        node_->setNodeMask(visible ? 0xffffffff : 0x0);
    }
    
    // 发出信号
    emit visibilityChanged(visible);
}

void GeoEntity::setSelected(bool selected)
{
    selected_ = selected;
    
    // 自我更新渲染节点（比如改变颜色或边框）
    updateNode();
    
    // 发出信号
    emit selectionChanged(selected);
}

void GeoEntity::setProperty(const QString& key, const QVariant& value)
{
    properties_[key] = value;
    
    // 自我更新渲染节点
    updateNode();
    
    // 发出信号
    emit propertyChanged(key, value);
}

QVariant GeoEntity::getProperty(const QString& key) const
{
    return properties_.value(key, QVariant());
}

void GeoEntity::updateNode()
{
    if (!node_) return;
    
    // 更新PositionAttitudeTransform的位置
    osg::PositionAttitudeTransform* pat = dynamic_cast<osg::PositionAttitudeTransform*>(node_.get());
    if (pat) {
        // 更新位置
        osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"), 
                                   longitude_, latitude_, altitude_, osgEarth::ALTMODE_ABSOLUTE);
        osg::Vec3d worldPos;
        geoPoint.toWorld(worldPos);
        pat->setPosition(worldPos);
        
        // 更新旋转
        double angleRad = heading_ * M_PI / 180.0;
        osg::Quat rotation(angleRad, osg::Vec3d(0.0, 0.0, 1.0));
        pat->setAttitude(rotation);
        
     
    }
}
