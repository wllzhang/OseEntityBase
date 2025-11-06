/**
 * @file waypointentity.h
 * @brief 航点实体头文件
 * 
 * 定义WaypointEntity类，用于路线规划与点标绘的基础实体
 */

#ifndef WAYPOINTENTITY_H
#define WAYPOINTENTITY_H

#include "geoentity.h"
#include <osg/Geode>
#include <osgText/Text>
#include <osgEarth/MapNode>
#include <osgEarthAnnotation/PlaceNode>

/**
 * @ingroup geo_entities
 * @brief 航点实体
 * 
 * 继承自GeoEntity，用于路线规划与点标绘的基础实体，支持显示序号标签，
 * 并可结合航线生成功能（线性/贝塞尔）使用。
 * 
 * **继承关系：**
 * ```
 * QObject
 *   └─ GeoEntity
 *       └─ WaypointEntity
 * ```
 */
class WaypointEntity : public GeoEntity
{
    Q_OBJECT
public:
    WaypointEntity(const QString& name,
                   double longitude,
                   double latitude,
                   double altitude,
                   QObject* parent = nullptr);

    void initialize() override;
    void update() override;
    void cleanup() override;

    // 设置序号标签内容（如 "1"、"2"）
    /** @brief 设置序号标签内容（如 "1"、"2"） */
    void setOrderLabel(const QString& text);
    // 设置 MapNode（优先用此绑定，避免运行时查找失败）
    /** @brief 绑定 MapNode（优先使用，避免运行时查找失败） */
    void setMapNode(osgEarth::MapNode* mapNode) { mapNodeRef_ = mapNode; }

protected:
    // 生命周期回调重写
    /** 
     * @brief 更新完成后的回调
     * 
     * 在基类 update() 调用 updateNode() 后执行，
     * 用于更新航点标签文本，使其与位置变化同步。
     */
    void onUpdated() override;
    
    /** 
     * @brief 清理前的回调
     * 
     * 在基类 cleanup() 清除节点引用前执行，
     * 用于清理航点特定的节点引用（如 pointGeode_、labelText_ 等）。
     */
    void onBeforeCleanup() override;

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


