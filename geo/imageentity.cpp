/**
 * @file imageentity.cpp
 * @brief 图片实体实现文件
 * 
 * 实现ImageEntity类的所有功能
 */

#include "imageentity.h"
#include "geoutils.h"
#include <osgDB/ReadFile>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Texture2D>
#include <osg/Image>
#include <osg/StateSet>
#include <osg/StateAttribute>
#include <osg/LineWidth>
#include <QDebug>

ImageEntity::ImageEntity(const QString& name, const QString& imagePath,
                         double longitude, double latitude, double altitude, QObject* parent)
    : GeoEntity(name, "image", longitude, latitude, altitude, parent)
    , imagePath_(imagePath)
{
    // 设置默认属性
    setProperty("size", 3000.0);   
    setProperty("opacity", 1.0);
}

/**
 * @brief 初始化图片实体
 * 
 * 使用基类默认实现，创建节点并设置初始状态。
 * 图片的特定初始化逻辑（如加载图片、创建纹理、创建几何体）在 createNode() 中完成。
 * 
 * @note 初始化完成后会记录日志，便于调试。
 */
void ImageEntity::initialize()
{
    // 调用基类默认实现（创建节点、设置可见性等）
    GeoEntity::initialize();
    
    qDebug() << "图片实体初始化完成:" << entityName_;
}

/**
 * @brief 更新图片实体
 * 
 * 使用基类默认实现更新节点变换（位置和旋转）。
 * 图片实体没有需要额外更新的内容，所以直接使用基类实现。
 */
void ImageEntity::update()
{
    // 使用基类默认实现（调用updateNode）
    GeoEntity::update();
}

/**
 * @brief 清理前的回调：移除高亮节点
 * 
 * 在基类 cleanup() 清除节点引用前执行。
 * 高亮边框节点是动态添加到 PAT 节点的子节点，
 * 必须在清理前从场景图中移除，避免残留引用。
 */
void ImageEntity::onBeforeCleanup()
{
    qDebug() << "清理图片实体:" << entityName_;
    
    // 清理前：从PAT中移除高亮节点
    if (node_) {
        osg::PositionAttitudeTransform* pat = dynamic_cast<osg::PositionAttitudeTransform*>(node_.get());
        if (pat && highlightNode_) {
            // 从PAT中移除高亮节点（这才是正确的清理方式）
            pat->removeChild(highlightNode_);
            qDebug() << "从PAT中移除高亮节点";
        }
    }
    
    // 清除高亮节点引用
    highlightNode_ = nullptr;
}

/**
 * @brief 清理后的回调：记录清理完成
 * 
 * 在基类 cleanup() 清除节点引用后执行，
 * 用于记录清理完成的日志。
 */
void ImageEntity::onAfterCleanup()
{
    qDebug() << "图片实体清理完成";
}

/**
 * @brief 清理图片实体
 * 
 * 调用基类清理方法，基类会：
 * 1. 先调用 onBeforeCleanup() 移除高亮节点
 * 2. 清除节点引用
 * 3. 调用 onAfterCleanup() 记录日志
 */
void ImageEntity::cleanup()
{
    // 调用基类清理（会调用onBeforeCleanup和onAfterCleanup）
    GeoEntity::cleanup();
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

/**
 * @brief 创建图片实体的渲染节点
 * 
 * 创建流程：
 * 1. 检查并加载图片文件
 * 2. 创建纹理对象并绑定图片
 * 3. 使用基类 createPATNode() 创建已初始化的 PAT 节点
 * 4. 创建带纹理的四边形几何体（根据 size 属性确定大小）
 * 5. 设置纹理坐标、法线和渲染状态
 * 6. 将几何体添加到 PAT 节点
 * 
 * @return 返回包含图片几何体的 PAT 节点，失败返回 nullptr
 * 
 * @note PAT 节点的位置和旋转已由 createPATNode() 根据实体地理位置和航向角设置。
 *       高亮边框节点将在选中时动态添加，不在 createNode() 中创建。
 */
osg::ref_ptr<osg::Node> ImageEntity::createNode()
{
    try {
        // 使用工具函数将Qt资源路径转换为文件路径
        QString errorMsg;
        QString filePath = GeoUtils::convertResourcePathToFile(imagePath_, &errorMsg);
        if (filePath.isEmpty()) {
            qDebug() << "无法转换图片资源路径:" << errorMsg;
            return nullptr;
        }
        
        // 加载图片
        osg::ref_ptr<osg::Image> image = osgDB::readImageFile(filePath.toStdString());
        if (!image.valid()) {
            qDebug() << "无法加载图片:" << imagePath_;
            return nullptr;
        }
        
        // 创建纹理并绑定图片
        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setImage(image.get());
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
        
        // 使用基类辅助方法创建PAT节点（已设置初始位置和旋转）
        osg::ref_ptr<osg::PositionAttitudeTransform> pat = createPATNode();
        
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
        // 注意：PAT的位置和旋转已在createPATNode()中设置
        
        qDebug() << "图片实体创建成功:" << entityName_;
        qDebug() << "图片路径:" << imagePath_;
        qDebug() << "图片大小:" << image->s() << "x" << image->t();
        qDebug() << "几何体大小:" << size;
        osg::Vec3d worldPos = pat->getPosition();
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
