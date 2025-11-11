/**
 * @file waypointentity.cpp
 * @brief 航点实体实现文件
 * 
 * 实现WaypointEntity类的所有功能
 */

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

WaypointEntity::WaypointEntity(const QString& name,
                               double longitude,
                               double latitude,
                               double altitude,
                               const QString& uidOverride,
                               QObject* parent)
    : GeoEntity(name, "waypoint", longitude, latitude, altitude, uidOverride, parent)
{

    setProperty("size", 8000.0);
    labelString_ = "";

    connect(this, &WaypointEntity::positionChanged,
            this, &WaypointEntity::handlePositionChanged);
}

/**
 * @brief 初始化航点实体
 * 
 * 使用基类默认实现，创建节点并设置初始状态。
 * 航点的特定初始化逻辑（如创建 CircleNode 和 PlaceNode）在 createNode() 中完成。
 */
void WaypointEntity::initialize()
{
    // 调用基类默认实现（创建节点、设置可见性等）
    GeoEntity::initialize();
}

/**
 * @brief 更新完成后的回调：更新标签文本
 * 
 * 当实体位置或属性变化时，基类会调用此方法。
 * 这里用于更新航点的序号标签，使其与当前 labelString_ 同步。
 */
void WaypointEntity::onUpdated()
{
    // 更新标签（特定逻辑）
    updateAnnotationPosition();
    updateLabel();
}

/**
 * @brief 更新航点实体
 * 
 * 使用基类默认实现更新节点变换，然后通过 onUpdated() 回调更新标签。
 */
void WaypointEntity::update()
{
    // 使用基类默认实现（调用updateNode），然后通过onUpdated更新标签
    GeoEntity::update();
}

/**
 * @brief 清理前的回调：清除航点特定资源
 * 
 * 在基类 cleanup() 清除节点引用前执行，
 * 用于清理航点特有的节点引用，避免内存泄漏。
 */
void WaypointEntity::onBeforeCleanup()
{
    // 清理特定的节点引用
//    pointGeode_ = nullptr;
//    labelText_ = nullptr;
//    labelGeode_ = nullptr;
    circleNode_ = nullptr;
    placeNode_ = nullptr;
    annotationGroup_ = nullptr;
}

/**
 * @brief 清理航点实体
 * 
 * 调用基类清理方法，基类会先调用 onBeforeCleanup() 清理特定资源，
 * 然后清除节点引用。
 */
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

/**
 * @brief 创建航点实体的渲染节点
 * 
 * 创建流程：
 * 1. 检查 MapNode 是否已绑定（必须）
 * 2. 创建 CircleNode 作为航点标记（红色圆形，固定高度避免地形遮挡）
 * 3. 创建 PlaceNode 作为序号标签（使用中文字体）
 * 4. 将两者组合到 Group 节点中
 * 
 * @return 返回包含 CircleNode 和 PlaceNode 的 Group 节点，失败返回 nullptr
 * 
 * @note 航点使用 CircleNode 和 PlaceNode（osgEarth 注解节点），
 *       这些节点需要 MapNode 引用，因此必须通过 setMapNode() 预先绑定。
 *       航点的位置通过 GeoPoint 设置，不使用 PAT 节点。
 */
osg::ref_ptr<osg::Node> WaypointEntity::createNode()
{
    // 仅支持显式绑定 MapNode 的直接创建
    if (!mapNodeRef_.valid()) {
        return nullptr;
    }

    using namespace osgEarth::Symbology;
    using namespace osgEarth::Annotation;

    osgEarth::GeoPoint gp(osgEarth::SpatialReference::get("wgs84"),
                          longitude_, latitude_, altitude_,  // 使用实体的高度属性（默认通过 MapStateConstants::DEFAULT_ALTITUDE_METERS 设置）
                          osgEarth::ALTMODE_ABSOLUTE);

    // 使用 CircleNode 作为“点”（真实地理半径），固定高度，避免地形遮挡
    Style circleStyle;
    circleStyle.getOrCreate<PolygonSymbol>()->fill()->color() = Color(Color::Red, 1.0);
    circleStyle.getOrCreate<AltitudeSymbol>()->clamping() = AltitudeSymbol::CLAMP_NONE;
    circleStyle.getOrCreate<RenderSymbol>()->depthTest() = false; // 始终可见

//    osg::ref_ptr<CircleNode> circle = new CircleNode();
    circleNode_ = new CircleNode();
    circleNode_->set(
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
    ts->size() = 22.0f;
    ts->fill()->color() = Color(Color::Red, 1.0);
//    ts->fill()->color() = Color::White;
    ts->halo()->color() = Color(0,0,0,0.6);
    placeNode_ = new PlaceNode(gp, labelString_.toStdString(), labelStyle);
    placeNode_->setMapNode(mapNodeRef_.get());

//    osg::ref_ptr<osg::Group> group = new osg::Group();
//    group->addChild(circle.get());
//    group->addChild(placeNode_.get());
//    return group.get();
    annotationGroup_ = new osg::Group();
    annotationGroup_->addChild(circleNode_.get());
    annotationGroup_->addChild(placeNode_.get());
    return annotationGroup_.get();
}

/**
 * @brief 更新航点标签文本
 * 
 * 当 labelString_ 变化时（如通过 setOrderLabel() 设置），
 * 更新 PlaceNode 的文本内容，使其显示新的序号。
 * 
 * @note 此方法由 onUpdated() 回调调用，确保标签与实体状态同步。
 */
void WaypointEntity::updateLabel()
{
    if (placeNode_.valid()) {
        placeNode_->setText(labelString_.toStdString());
    }
}

void WaypointEntity::updateAnnotationPosition()
{
    if (!mapNodeRef_.valid()) {
        return;
    }

    osgEarth::GeoPoint gp(osgEarth::SpatialReference::get("wgs84"),
                          longitude_, latitude_, altitude_,
                          osgEarth::ALTMODE_ABSOLUTE);

    if (circleNode_.valid()) {
        circleNode_->setPosition(gp);
    }
    if (placeNode_.valid()) {
        placeNode_->setPosition(gp);
    }
}

void WaypointEntity::handlePositionChanged(double longitude, double latitude, double altitude)
{
    Q_UNUSED(longitude);
    Q_UNUSED(latitude);
    Q_UNUSED(altitude);
    updateAnnotationPosition();
    updateLabel();
}
