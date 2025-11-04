#ifndef OSGMAPWIDGET_H
#define OSGMAPWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QResizeEvent>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Camera>
#include "OsgQt/GraphicsWindowQt.h"

// 前向声明
namespace osgEarth {
    class MapNode;
}

// 前向声明管理器类
class GeoEntityManager;
class MapStateManager;

/**
 * @brief OSG地图Widget组件
 * 
 * 用于在MainWidget中嵌入3D地球渲染
 */
class OsgMapWidget : public QWidget
{
    Q_OBJECT

public:
    explicit OsgMapWidget(QWidget *parent = nullptr);
    ~OsgMapWidget();

    // 获取管理器接口
    GeoEntityManager* getEntityManager() const { return entityManager_; }
    MapStateManager* getMapStateManager() const { return mapStateManager_; }
    osgViewer::Viewer* getViewer() const { return viewer_.get(); }
    osgEarth::MapNode* getMapNode() const { return mapNode_.get(); }
    
    // 设置2D/3D模式
    void setMode2D();
    void setMode3D();

signals:
    void mapLoaded();  // 地图加载完成信号

protected:
    void resizeEvent(QResizeEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    void initializeViewer();
    void setupCamera();
    void loadMap();
    void setupManipulator();

    osg::ref_ptr<osgViewer::Viewer> viewer_;
    osg::ref_ptr<osg::Group> root_;
    osg::ref_ptr<osgEarth::MapNode> mapNode_;
    osgQt::GraphicsWindowQt* gw_;
    QTimer* timer_;
    
    // 实体和地图状态管理器
    GeoEntityManager* entityManager_;
    MapStateManager* mapStateManager_;
};

#endif // OSGMAPWIDGET_H

