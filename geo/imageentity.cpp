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
#include <QDebug>

ImageEntity::ImageEntity(const QString& name, const QString& imagePath,
                         double longitude, double latitude, double altitude,
                         const QString& uidOverride, QObject* parent)
    : GeoEntity(name, "image", longitude, latitude, altitude, uidOverride, parent)
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
 * @brief 创建图片实体的渲染节点
 * 
 * 创建流程：
 * 1. 检查并加载图片文件
 * 2. 创建纹理对象并绑定图片
 * 3. 创建带纹理的四边形几何体（根据 size 属性确定大小）
 * 4. 设置纹理坐标、法线和渲染状态
 * 
 * @return 返回包含图片几何体的节点，失败返回 nullptr
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
        
        qDebug() << "图片实体创建成功:" << entityName_;
        qDebug() << "图片路径:" << imagePath_;
        qDebug() << "图片大小:" << image->s() << "x" << image->t();
        qDebug() << "几何体大小:" << size;
        
        return geode.get();
        
    } catch (...) {
        qDebug() << "图片实体创建失败";
        return nullptr;
    }
}
