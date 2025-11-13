#ifndef LINEENTITY_H
#define LINEENTITY_H

#include "geoentity.h"

#include <osg/Geode>
#include <osgText/Text>

/**
 * @file lineentity.h
 * @brief 直线实体头文件
 *
 * 定义LineEntity类，用于在地图上绘制两个地理位置之间的直线。
 */
class LineEntity : public GeoEntity
{
    Q_OBJECT

public:
    LineEntity(const QString& name,
               double startLongitude, double startLatitude, double startAltitude,
               double endLongitude, double endLatitude, double endAltitude,
               const QString& uidOverride = QString(),
               QObject* parent = nullptr);

    void setEndpoints(double startLongitude, double startLatitude, double startAltitude,
                      double endLongitude, double endLatitude, double endAltitude);
    void getEndpoints(double& startLongitude, double& startLatitude, double& startAltitude,
                      double& endLongitude, double& endLatitude, double& endAltitude) const;

    double lengthMeters() const;

protected:
    osg::ref_ptr<osg::Node> createNode() override;
    void onUpdated() override;

private:
    void updateLineGeometry();
    void updateHighlightFromLength();
    void syncEndpointProperties();
    void updateLabelText();
    void onPropertyChanged(const QString& key, const QVariant& value);
    QString resolveDisplayName() const;

    double startLongitude_;
    double startLatitude_;
    double startAltitude_;
    double endLongitude_;
    double endLatitude_;
    double endAltitude_;

    osg::ref_ptr<osg::Geometry> geometry_;
    osg::ref_ptr<osg::Vec3Array> vertices_;
    osg::ref_ptr<osg::Geode> geode_;
    osg::ref_ptr<osg::Geode> labelGeode_;
    osg::ref_ptr<osgText::Text> labelText_;
};


#endif // LINEENTITY_H
