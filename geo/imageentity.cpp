#include "imageentity.h"
#include <osgDB/ReadFile>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/StateSet>
#include <osg/StateAttribute>
#include <osg/LineWidth>
#include <QDebug>
#include <QFileInfo>

ImageEntity::ImageEntity(const QString& id, const QString& name, const QString& imagePath,
                         double longitude, double latitude, double altitude, QObject* parent)
    : GeoEntity(id, name, "image", longitude, latitude, altitude, parent)
    , imagePath_(imagePath)
{
    // 设置默认属性
    setProperty("size", 3000.0);   
    setProperty("opacity", 1.0);
}

void ImageEntity::initialize()
{
    // 创建渲染节点
    node_ = createNode();
    
    // 设置初始状态
    setVisible(true);
    setSelected(false);
    
    qDebug() << "图片实体初始化完成:" << entityName_;
}

void ImageEntity::update()
{
    // 简单的更新逻辑
    updateNode();
}

void ImageEntity::cleanup()
{
    // 简单的清理逻辑
    if (node_) {
        node_ = nullptr;
    }
    if (highlightNode_) {
        highlightNode_ = nullptr;
    }
}

void ImageEntity::setSelected(bool selected)
{
    // 调用基类方法
    GeoEntity::setSelected(selected);
    
    if (selected) {
        // 选中时创建高亮边框
        if (!highlightNode_) {
            osg::PositionAttitudeTransform* pat = dynamic_cast<osg::PositionAttitudeTransform*>(node_.get());
            if (pat) {
                addHighlightBorder(pat);
            }
        }
        // 显示高亮边框
        if (highlightNode_) {
            highlightNode_->setNodeMask(0xffffffff);
            qDebug() << "实体高亮状态:" << entityName_ << "->选中";
        }
    } else {
        // 取消选中时隐藏高亮边框
        if (highlightNode_) {
            highlightNode_->setNodeMask(0x0);
            qDebug() << "实体高亮状态:" << entityName_ << "->未选中";
        }
    }
}

osg::ref_ptr<osg::Node> ImageEntity::createNode()
{
    try {
        // 检查图片文件是否存在
        QFileInfo fileInfo(imagePath_);
        if (!fileInfo.exists()) {
            qDebug() << "图片文件不存在:" << imagePath_;
            return nullptr;
        }
        
        // 加载图片
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(imagePath_.toStdString());
        if (!image.valid()) {
            qDebug() << "无法加载图片:" << imagePath_;
            return nullptr;
        }
        
        // 创建纹理
        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setImage(image.get());
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        
        // 使用PositionAttitudeTransform
        osg::ref_ptr<osg::PositionAttitudeTransform> pat = new osg::PositionAttitudeTransform();
        
        // 创建带纹理的几何体
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> geometry = new osg::Geometry;
        
        // 创建四边形顶点
        osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array;
        float size = getProperty("size").toFloat();
        vertices->push_back(osg::Vec3(-size/2, 0, -size/2));  // 左下
        vertices->push_back(osg::Vec3(size/2, 0, -size/2));   // 右下
        vertices->push_back(osg::Vec3(size/2, 0, size/2));    // 右上
        vertices->push_back(osg::Vec3(-size/2, 0, size/2));   // 左上
        geometry->setVertexArray(vertices);
        
        // 设置纹理坐标
        osg::ref_ptr<osg::Vec2Array> texCoords = new osg::Vec2Array;
        texCoords->push_back(osg::Vec2(0.0f, 0.0f));  // 左下
        texCoords->push_back(osg::Vec2(1.0f, 0.0f));  // 右下
        texCoords->push_back(osg::Vec2(1.0f, 1.0f));  // 右上
        texCoords->push_back(osg::Vec2(0.0f, 1.0f));  // 左上
        geometry->setTexCoordArray(0, texCoords);
        
        // 设置法线
        osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array;
        normals->push_back(osg::Vec3(0.0f, 0.0f, 1.0f)); // 朝上
        geometry->setNormalArray(normals);
        geometry->setNormalBinding(osg::Geometry::BIND_OVERALL);
        
        // 设置绘制方式
        geometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::QUADS, 0, 4));
        geode->addDrawable(geometry);
        
        // 设置纹理状态
        osg::ref_ptr<osg::StateSet> stateSet = geode->getOrCreateStateSet();
        texture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
        texture->setFilter(osg::Texture::MAG_FILTER, osg::Texture::LINEAR);
        stateSet->setTextureAttributeAndModes(0, texture.get(), osg::StateAttribute::ON);
        
        // 设置渲染状态
        stateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        stateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        stateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        
        // 将几何体添加到PAT
        pat->addChild(geode);
        
        // 高亮边框将在选中时动态创建
        
        // 设置PAT的位置
        osgEarth::GeoPoint geoPoint(osgEarth::SpatialReference::get("wgs84"), 
                                   longitude_, latitude_, altitude_, osgEarth::ALTMODE_ABSOLUTE);
        osg::Vec3d worldPos;
        geoPoint.toWorld(worldPos);
        pat->setPosition(worldPos);
        
        qDebug() << "图片实体创建成功:" << entityName_;
        qDebug() << "图片路径:" << imagePath_;
        qDebug() << "图片大小:" << image->s() << "x" << image->t();
        qDebug() << "几何体大小:" << size;
        qDebug() << "世界坐标:" << worldPos.x() << worldPos.y() << worldPos.z();
        
        return pat.get();
        
    } catch (...) {
        qDebug() << "图片实体创建失败";
        return nullptr;
    }
}

void ImageEntity::addHighlightBorder(osg::PositionAttitudeTransform* pat)
{
    if (!pat) return;
    
    try {
        // 创建高亮边框几何体
        osg::ref_ptr<osg::Geode> borderGeode = new osg::Geode;
        osg::ref_ptr<osg::Geometry> borderGeometry = new osg::Geometry;
        
        // 创建边框顶点（比原几何体稍大）
        osg::ref_ptr<osg::Vec3Array> borderVertices = new osg::Vec3Array;
        float size = getProperty("size").toFloat();
        float borderSize = size * 1.1f; // 边框比原几何体大10%
        
        // 创建边框线条
        borderVertices->push_back(osg::Vec3(-borderSize/2, 0, -borderSize/2));  // 左下
        borderVertices->push_back(osg::Vec3(borderSize/2, 0, -borderSize/2));    // 右下
        borderVertices->push_back(osg::Vec3(borderSize/2, 0, borderSize/2));     // 右上
        borderVertices->push_back(osg::Vec3(-borderSize/2, 0, borderSize/2));   // 左上
        borderVertices->push_back(osg::Vec3(-borderSize/2, 0, -borderSize/2));  // 闭合
        
        borderGeometry->setVertexArray(borderVertices);
        
        // 设置颜色为红色
        osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array;
        colors->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f)); // 红色
        borderGeometry->setColorArray(colors);
        borderGeometry->setColorBinding(osg::Geometry::BIND_OVERALL);
        
        // 设置绘制方式为线条
        borderGeometry->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::LINE_STRIP, 0, 5));
        borderGeode->addDrawable(borderGeometry);
        
        // 设置边框渲染状态
        osg::ref_ptr<osg::StateSet> borderStateSet = borderGeode->getOrCreateStateSet();
        borderStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
        borderStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
        borderStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
        borderStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
        
        // 设置线条宽度
        osg::ref_ptr<osg::LineWidth> lineWidth = new osg::LineWidth(3.0f);
        borderStateSet->setAttributeAndModes(lineWidth.get(), osg::StateAttribute::ON);
        
        // 将边框添加到PAT
        pat->addChild(borderGeode);
        
        // 保存高亮节点引用
        highlightNode_ = borderGeode;
        
        qDebug() << "高亮边框创建成功";
        
    } catch (...) {
        qDebug() << "高亮边框创建失败";
    }
}
