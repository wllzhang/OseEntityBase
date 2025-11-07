/**
 * @file OsgMapWidget.h
 * @brief OSG地图Widget组件头文件
 * 
 * 定义OsgMapWidget类，用于在Qt应用程序中嵌入基于osgEarth的3D地球渲染
 */

#ifndef OSGMAPWIDGET_H
#define OSGMAPWIDGET_H

#include <QWidget>
#include <QTimer>
#include <QResizeEvent>
#include <QShowEvent>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QPoint>
#include <QVBoxLayout>
#include <osgViewer/Viewer>
#include <osg/Group>
#include <osg/Camera>
#include "OsgQt/GraphicsWindowQt.h"

// 前向声明
class MapInfoOverlay;

// 前向声明
namespace osgEarth {
    class MapNode;
}

// 前向声明管理器类
class GeoEntityManager;
class MapStateManager;
class NavigationHistory;

/**
 * @brief OSG地图Widget组件
 * 
 * 用于在MainWidget中嵌入3D地球渲染。提供完整的地图显示、实体管理、
 * 拖拽部署和2D/3D视图切换功能。
 * 
 * 主要功能：
 * - 初始化OSG Viewer和osgEarth地图
 * - 管理GeoEntityManager和MapStateManager
 * - 支持从模型部署对话框拖拽实体到地图
 * - 支持2D/3D视图切换
 * - 处理窗口大小变化和显示事件
 * 
 * @note 使用单线程渲染模式（SingleThreaded）以避免多线程问题
 */
class OsgMapWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父widget
     */
    explicit OsgMapWidget(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~OsgMapWidget();

    /**
     * @brief 获取实体管理器
     * @return 实体管理器指针
     */
    GeoEntityManager* getEntityManager() const { return entityManager_; }
    
    /**
     * @brief 获取地图状态管理器
     * @return 地图状态管理器指针
     */
    MapStateManager* getMapStateManager() const { return mapStateManager_; }
    
    /**
     * @brief 获取OSG Viewer
     * @return Viewer指针
     */
    osgViewer::Viewer* getViewer() const { return viewer_.get(); }
    
    /**
     * @brief 获取osgEarth MapNode
     * @return MapNode指针
     */
    osgEarth::MapNode* getMapNode() const { return mapNode_.get(); }
    
    /**
     * @brief 设置2D视图模式
     * 
     * 切换到2D视图（俯视角度，pitch=-90°）
     */
    void setMode2D();
    
    /**
     * @brief 设置3D视图模式
     * 
     * 切换到3D视图（斜视角度，pitch=-76.466°）
     */
    void setMode3D();
    
    /**
     * @brief 设置方案文件管理器
     * 
     * 用于拖拽部署实体时添加到方案文件
     * @param planFileManager 方案文件管理器指针
     */
    void setPlanFileManager(class PlanFileManager* planFileManager);
    
    /**
     * @brief 获取信息叠加层组件
     * @return 信息叠加层组件指针
     */
    MapInfoOverlay* getMapInfoOverlay() const { return mapInfoOverlay_; }
    
    /**
     * @brief 获取导航历史管理器
     * @return 导航历史管理器指针
     */
    NavigationHistory* getNavigationHistory() const { return navigationHistory_; }

    /**
     * @brief 合成鼠标释放事件
     *
     * 某些情况下（如弹出上下文菜单、模态对话框）会吞掉原始的鼠标释放事件，
     * 从而导致 EarthManipulator 仍认为鼠标按键处于按下状态，这会造成视角被锁定。
     * 调用该方法可以向底层 GLWidget 注入一次鼠标释放事件以恢复正常状态。
     *
     * @param button 需要合成释放的鼠标按键
     */
    void synthesizeMouseRelease(Qt::MouseButton button);

signals:
    /**
     * @brief 地图加载完成信号
     * 
     * 当地图文件加载完成并初始化后发出
     */
    void mapLoaded();

protected:
    /**
     * @brief 窗口大小变化事件处理
     * @param event 大小变化事件
     */
    void resizeEvent(QResizeEvent* event) override;
    
    /**
     * @brief 窗口显示事件处理
     * 
     * 窗口显示时启动渲染定时器
     * @param event 显示事件
     */
    void showEvent(QShowEvent* event) override;
    
    /**
     * @brief 拖拽进入事件处理
     * 
     * 接受来自模型部署对话框的拖拽操作
     * @param event 拖拽进入事件
     */
    void dragEnterEvent(QDragEnterEvent* event) override;
    
    /**
     * @brief 拖拽释放事件处理
     * 
     * 在释放位置创建实体并添加到方案文件
     * @param event 拖拽释放事件
     */
    void dropEvent(QDropEvent* event) override;

private:
    /**
     * @brief 初始化Viewer
     * 
     * 延迟加载地图和设置操作器
     */
    void initializeViewer();
    
    /**
     * @brief 设置相机参数
     * 
     * 配置相机的视口和投影矩阵
     */
    void setupCamera();
    
    /**
     * @brief 加载osgEarth地图
     * 
     * 从earth/my.earth文件加载地图配置
     */
    void loadMap();
    
    /**
     * @brief 设置地球操作器
     * 
     * 创建EarthManipulator并关联到Viewer
     */
    void setupManipulator();

    osg::ref_ptr<osgViewer::Viewer> viewer_;      // OSG Viewer
    osg::ref_ptr<osg::Group> root_;                // 场景根节点
    osg::ref_ptr<osgEarth::MapNode> mapNode_;     // osgEarth地图节点
    osgQt::GraphicsWindowQt* gw_;                 // Qt图形窗口适配器
    QTimer* timer_;                                // 渲染定时器
    
    // 实体和地图状态管理器
    GeoEntityManager* entityManager_;              // 实体管理器
    MapStateManager* mapStateManager_;             // 地图状态管理器
    
    // 方案文件管理器（用于拖拽时添加到方案）
    class PlanFileManager* planFileManager_;
    
    // 信息叠加层组件
    MapInfoOverlay* mapInfoOverlay_;               // 地图信息叠加层
    
    // 导航历史管理器
    NavigationHistory* navigationHistory_;         // 导航历史管理器
};

#endif // OSGMAPWIDGET_H

