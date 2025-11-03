#ifndef IMAGEENTITY_H
#define IMAGEENTITY_H

#include "geoentity.h"
#include <QString>

/**
 * @ingroup geo_entities
 * @brief 图片实体类
 * 
 * 继承自GeoEntity，基于图片的地理实体实现，支持从图片文件创建实体。
 * 实体被选中时支持高亮边框显示。
 * 
 * **继承关系：**
 * ```
 * QObject
 *   └─ GeoEntity
 *       └─ ImageEntity
 * ```
 * 
 * 特性：
 * - 从图片文件加载和显示
 * - 选中时显示红色边框高亮
 * - 支持实体的位置、朝向、大小设置
 * - 多态支持，重写 setSelected() 方法
 * 
 * @note 图片路径通常从配置（如 JSON）中获取
 */
class ImageEntity : public GeoEntity
{
    Q_OBJECT

public:
    ImageEntity(const QString& id, const QString& name, const QString& imagePath,
                double longitude, double latitude, double altitude, QObject* parent = nullptr);
    
    // 实现基类纯虚函数
    void initialize() override;
    void update() override;
    void cleanup() override;
    
    // 重写选择状态设置
    void setSelected(bool selected) override;

protected:
    // 生命周期回调重写
    /** 
     * @brief 清理前的回调
     * 
     * 在基类 cleanup() 清除节点引用前执行，
     * 用于从 PAT 节点中移除高亮边框节点，避免场景图中残留引用。
     */
    void onBeforeCleanup() override;
    
    /** 
     * @brief 清理后的回调
     * 
     * 在基类 cleanup() 清除节点引用后执行，
     * 用于记录清理完成的日志或其他后续工作。
     */
    void onAfterCleanup() override;

private:
    osg::ref_ptr<osg::Node> createNode() override;
    void addHighlightBorder(osg::PositionAttitudeTransform* pat);
    
    QString imagePath_;
    osg::ref_ptr<osg::Node> highlightNode_;
};

#endif // IMAGEENTITY_H
