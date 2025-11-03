#include "waypointentity.h"
#include <osg/ShapeDrawable>
#include <osg/PositionAttitudeTransform>
#include <osg/Geometry>
#include <osg/Point>
#include <osgEarth/GeoData>
#include <osgEarth/SpatialReference>
#include <osgEarthSymbology/Style>
#include <osgEarthSymbology/PointSymbol>
#include <osgEarthSymbology/TextSymbol>
#include <osgEarthAnnotation/PlaceNode>
#include <osgEarthAnnotation/CircleNode>

WaypointEntity::WaypointEntity(const QString& id,
                               const QString& name,
                               double longitude,
                               double latitude,
                               double altitude,
                               QObject* parent)
    : GeoEntity(id, name, "waypoint", longitude, latitude, altitude, parent)
{

    setProperty("size", 8000.0);
    labelString_ = "";
}

void WaypointEntity::initialize()
{
    // 调用基类默认实现（创建节点、设置可见性等）
    GeoEntity::initialize();
}

void WaypointEntity::onUpdated()
{
    // 更新标签（特定逻辑）
    updateLabel();
}

void WaypointEntity::update()
{
    // 使用基类默认实现（调用updateNode），然后通过onUpdated更新标签
    GeoEntity::update();
}

void WaypointEntity::onBeforeCleanup()
{
    // 清理特定的节点引用
    pointGeode_ = nullptr;
    labelText_ = nullptr;
    labelGeode_ = nullptr;
}

void WaypointEntity::cleanup()
{
    // 调用基类清理（会调用onBeforeCleanup）
    GeoEntity::cleanup();
}

void WaypointEntity::setOrderLabel(const QString& text)
{
    labelString_ = text;
    updateLabel();
}

osg::ref_ptr<osg::Node> WaypointEntity::createNode()
{
    // 仅支持显式绑定 MapNode 的直接创建
    if (!mapNodeRef_.valid()) {
        return nullptr;
    }

    using namespace osgEarth::Symbology;
    using namespace osgEarth::Annotation;

    osgEarth::GeoPoint gp(osgEarth::SpatialReference::get("wgs84"),
                          longitude_, latitude_, 5000.0,
                          osgEarth::ALTMODE_ABSOLUTE);

    // 使用 CircleNode 作为“点”（真实地理半径），固定高度，避免地形遮挡
    Style circleStyle;
    circleStyle.getOrCreate<PolygonSymbol>()->fill()->color() = Color(Color::Red, 1.0);
    circleStyle.getOrCreate<AltitudeSymbol>()->clamping() = AltitudeSymbol::CLAMP_NONE;
    circleStyle.getOrCreate<RenderSymbol>()->depthTest() = false; // 始终可见

    osg::ref_ptr<CircleNode> circle = new CircleNode();
    circle->set(
        gp,
        Distance(200.0, Units::METERS), // 半径按需调整
        circleStyle,
        Angle(0.0, Units::DEGREES),
        Angle(360.0, Units::DEGREES),
        true);

    // 构建标签（仅文本）
    Style labelStyle;
    TextSymbol* ts = labelStyle.getOrCreate<TextSymbol>();
    ts->encoding() = TextSymbol::ENCODING_UTF8;
    ts->font() = "simsun.ttc";
    ts->size() = 16.0f;
    ts->fill()->color() = Color::White;
    ts->halo()->color() = Color(0,0,0,0.6);
    placeNode_ = new PlaceNode(gp, labelString_.toStdString(), labelStyle);
    placeNode_->setMapNode(mapNodeRef_.get());

    osg::ref_ptr<osg::Group> group = new osg::Group();
    group->addChild(circle.get());
    group->addChild(placeNode_.get());
    return group.get();
}

void WaypointEntity::updateLabel()
{
    if (placeNode_.valid()) {
        placeNode_->setText(labelString_.toStdString());
    }
}


