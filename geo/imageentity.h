#ifndef IMAGEENTITY_H
#define IMAGEENTITY_H

#include "geoentity.h"
#include <QString>

/**
 * @brief 图片实体类
 * 
 * 基于图片的地理实体实现，支持从图片文件创建实体。
 * 实现了实体选择时的高亮显示功能。
 * 
 * 特性：
 * - 从图片文件加载和显示
 * - 选中时显示红色边框高亮
 * - 支持实体的位置、朝向、大小设置
 * - 多态支持，重写 setSelected() 方法
 * 
 * @note 图片路径从配置文件中获取
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

private:
    osg::ref_ptr<osg::Node> createNode() override;
    void addHighlightBorder(osg::PositionAttitudeTransform* pat);
    
    QString imagePath_;
    osg::ref_ptr<osg::Node> highlightNode_;
};

#endif // IMAGEENTITY_H
