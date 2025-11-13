/**
 * @file lineentity.cpp
 * @brief 直线实体实现文件
 */

#include "lineentity.h"
#include "geoutils.h"

#include <QtGlobal>
#include <QVariant>

#include <osg/LineWidth>
#include <osg/StateSet>

namespace {
QVariant buildEndpointObject(double lon, double lat, double alt)
{
    QVariantMap map;
    map.insert(QStringLiteral("longitude"), lon);
    map.insert(QStringLiteral("latitude"), lat);
    map.insert(QStringLiteral("altitude"), alt);
    return QVariant(map);
}
}

LineEntity::LineEntity(const QString& name,
                       double startLongitude, double startLatitude, double startAltitude,
                       double endLongitude, double endLatitude, double endAltitude,
                       const QString& uidOverride,
                       QObject* parent)
    : GeoEntity(name,
                QStringLiteral("line"),
                (startLongitude + endLongitude) * 0.5,
                (startLatitude + endLatitude) * 0.5,
                (startAltitude + endAltitude) * 0.5,
                uidOverride,
                parent)
    , startLongitude_(startLongitude)
    , startLatitude_(startLatitude)
    , startAltitude_(startAltitude)
    , endLongitude_(endLongitude)
    , endLatitude_(endLatitude)
    , endAltitude_(endAltitude)
{
    syncEndpointProperties();
    updateHighlightFromLength();

    QObject::connect(this, &LineEntity::propertyChanged,
                     this, &LineEntity::onPropertyChanged);
}

void LineEntity::setEndpoints(double startLongitude, double startLatitude, double startAltitude,
                              double endLongitude, double endLatitude, double endAltitude)
{
    startLongitude_ = startLongitude;
    startLatitude_ = startLatitude;
    startAltitude_ = startAltitude;
    endLongitude_ = endLongitude;
    endLatitude_ = endLatitude;
    endAltitude_ = endAltitude;

    double midLon = (startLongitude_ + endLongitude_) * 0.5;
    double midLat = (startLatitude_ + endLatitude_) * 0.5;
    double midAlt = (startAltitude_ + endAltitude_) * 0.5;

    GeoEntity::setPosition(midLon, midLat, midAlt);

    updateLineGeometry();
    updateHighlightFromLength();
    syncEndpointProperties();
}

void LineEntity::getEndpoints(double& startLongitude, double& startLatitude, double& startAltitude,
                              double& endLongitude, double& endLatitude, double& endAltitude) const
{
    startLongitude = startLongitude_;
    startLatitude = startLatitude_;
    startAltitude = startAltitude_;
    endLongitude = endLongitude_;
    endLatitude = endLatitude_;
    endAltitude = endAltitude_;
}

double LineEntity::lengthMeters() const
{
    return GeoUtils::calculateGeographicDistance(startLongitude_, startLatitude_,
                                                 endLongitude_, endLatitude_);
}

osg::ref_ptr<osg::Node> LineEntity::createNode()
{
    osg::ref_ptr<osg::PositionAttitudeTransform> pat = createPATNode();

    geometry_ = new osg::Geometry();
    vertices_ = new osg::Vec3Array();
    geometry_->setVertexArray(vertices_.get());
    geometry_->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, 2));

    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array();
    colors->push_back(osg::Vec4(0.2f, 0.9f, 0.3f, 1.0f));
    geometry_->setColorArray(colors.get(), osg::Array::BIND_OVERALL);

    osg::StateSet* stateSet = geometry_->getOrCreateStateSet();
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
    stateSet->setAttributeAndModes(new osg::LineWidth(4.0f), osg::StateAttribute::ON);
    stateSet->setRenderBinDetails(9998, "RenderBin");

    geode_ = new osg::Geode();
    geode_->addDrawable(geometry_.get());
    geode_->setCullingActive(false);

    pat->addChild(geode_.get());

    labelGeode_ = new osg::Geode();
    labelText_ = new osgText::Text();
    labelText_->setCharacterSize(250.0f);
    labelText_->setColor(osg::Vec4(1.0f, 1.0f, 0.2f, 1.0f));
    labelText_->setAlignment(osgText::Text::CENTER_BOTTOM);
    labelText_->setAxisAlignment(osgText::TextBase::SCREEN);
    labelText_->setText(resolveDisplayName().toStdString());
    labelText_->setPosition(osg::Vec3(0.0f, 0.0f, 0.0f));
    labelGeode_->addDrawable(labelText_.get());
    osg::StateSet* labelState = labelGeode_->getOrCreateStateSet();
    labelState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    labelState->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    labelState->setRenderBinDetails(9999, "RenderBin");
    labelGeode_->setCullingActive(false);
    pat->addChild(labelGeode_.get());

    updateLineGeometry();
    updateLabelText();

    return pat.get();
}

void LineEntity::onUpdated()
{
    updateLineGeometry();
}

void LineEntity::updateLineGeometry()
{
    if (!vertices_) {
        return;
    }

    osg::Vec3d worldStart = GeoUtils::geoToWorldCoordinates(startLongitude_, startLatitude_, startAltitude_);
    osg::Vec3d worldEnd = GeoUtils::geoToWorldCoordinates(endLongitude_, endLatitude_, endAltitude_);

    double midLon = (startLongitude_ + endLongitude_) * 0.5;
    double midLat = (startLatitude_ + endLatitude_) * 0.5;
    double midAlt = (startAltitude_ + endAltitude_) * 0.5;
    osg::Vec3d worldMid = GeoUtils::geoToWorldCoordinates(midLon, midLat, midAlt);

    if (vertices_->size() != 2) {
        vertices_->resize(2);
    }

    (*vertices_)[0] = worldStart - worldMid;
    (*vertices_)[1] = worldEnd - worldMid;
    vertices_->dirty();

    if (geometry_) {
        geometry_->dirtyDisplayList();
        geometry_->dirtyBound();
    }

    if (labelText_) {
        labelText_->setPosition(osg::Vec3(0.0f, 0.0f, 0.0f));
    }
}

void LineEntity::updateHighlightFromLength()
{
    const double len = lengthMeters();
    const double highlightSize = qBound(100.0, len * 0.2, 5000.0);
    setProperty(QStringLiteral("highlightSize"), highlightSize);
    setProperty(QStringLiteral("lineLengthMeters"), len);
}

void LineEntity::syncEndpointProperties()
{
    setProperty(QStringLiteral("lineStart"), buildEndpointObject(startLongitude_, startLatitude_, startAltitude_));
    setProperty(QStringLiteral("lineEnd"), buildEndpointObject(endLongitude_, endLatitude_, endAltitude_));
}

void LineEntity::updateLabelText()
{
    if (!labelText_) {
        return;
    }
    labelText_->setText(resolveDisplayName().toStdString());
}

void LineEntity::onPropertyChanged(const QString& key, const QVariant&)
{
    if (key == QStringLiteral("displayName")) {
        updateLabelText();
    }
}

QString LineEntity::resolveDisplayName() const
{
    QString displayName = getProperty(QStringLiteral("displayName")).toString();
    if (!displayName.isEmpty()) {
        return displayName;
    }
    return getName();
}
