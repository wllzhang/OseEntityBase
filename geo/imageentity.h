/**
 * @file imageentity.h
 * @brief 图片实体头文件
 * 
 * 定义ImageEntity类，基于图片的地理实体实现
 */

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
 * - 支持实体的位置、朝向、大小设置
 * 
 * @note 图片路径通常从配置（如 JSON）中获取
 */
class ImageEntity : public GeoEntity
{
    Q_OBJECT

public:
    ImageEntity(const QString& name, const QString& imagePath,
                double longitude, double latitude, double altitude,
                const QString& uidOverride = QString(), QObject* parent = nullptr);
    
    // 实现基类纯虚函数
    void initialize() override;

protected:
    osg::ref_ptr<osg::Node> createNode() override;
    
    QString imagePath_;
};

#endif // IMAGEENTITY_H
