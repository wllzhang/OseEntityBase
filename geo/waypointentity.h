#ifndef WAYPOINTENTITY_H
#define WAYPOINTENTITY_H

#include "geoentity.h"
#include <osg/Geode>
#include <osgText/Text>
#include <osgEarth/MapNode>
#include <osgEarthAnnotation/PlaceNode>

class WaypointEntity : public GeoEntity
{
    Q_OBJECT
public:
    WaypointEntity(const QString& id,
                   const QString& name,
                   double longitude,
                   double latitude,
                   double altitude,
                   QObject* parent = nullptr);

    void initialize() override;
    void update() override;
    void cleanup() override;

    // 设置序号标签内容（如 "1"、"2"）
    void setOrderLabel(const QString& text);
    // 设置 MapNode（优先用此绑定，避免运行时查找失败）
    void setMapNode(osgEarth::MapNode* mapNode) { mapNodeRef_ = mapNode; }

private:
    osg::ref_ptr<osg::Node> createNode() override;
    void updateLabel();

private:
    osg::ref_ptr<osg::Geode> pointGeode_;
    osg::ref_ptr<osg::Geode> labelGeode_;
    osg::ref_ptr<osgText::Text> labelText_;
    QString labelString_;
    osg::ref_ptr<osgEarth::Annotation::PlaceNode> placeNode_;
    osg::ref_ptr<osgEarth::MapNode> mapNodeRef_;
};

#endif // WAYPOINTENTITY_H


