/**
 * @file MapInfoOverlay.h
 * @brief 地图信息叠加层组件头文件
 * 
 * 定义MapInfoOverlay类，用于在地图上叠加显示各种信息
 */

#ifndef MAPINFOOVERLAY_H
#define MAPINFOOVERLAY_H

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPainter>
#include <QPaintEvent>

// 前向声明
class MapStateManager;
class PlanFileManager;

// 内部widget类声明
class CompassWidget;
class ScaleWidget;

/**
 * @brief 地图信息叠加层组件
 * 
 * 在地图上叠加显示各种信息，包括：
 * - 鼠标坐标（经纬度、高度）
 * - 相机参数（航向角、俯仰角、距离）
 * - 当前方案名称
 * - 导航控制（前进/后退）
 * - 指北针
 * - 比例尺
 */
class MapInfoOverlay : public QWidget
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父widget
     */
    explicit MapInfoOverlay(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~MapInfoOverlay();

    /**
     * @brief 设置地图状态管理器
     * @param mapStateManager 地图状态管理器指针
     */
    void setMapStateManager(MapStateManager* mapStateManager);
    
    /**
     * @brief 设置方案文件管理器
     * @param planFileManager 方案文件管理器指针
     */
    void setPlanFileManager(PlanFileManager* planFileManager);
    
    /**
     * @brief 更新鼠标坐标信息
     * @param longitude 经度
     * @param latitude 纬度
     * @param altitude 高度
     */
    void updateMouseCoordinates(double longitude, double latitude, double altitude);
    
    /**
     * @brief 更新相机参数
     * @param heading 航向角（度）
     * @param pitch 俯仰角（度）
     * @param range 距离（米）
     */
    void updateCameraParameters(double heading, double pitch, double range);
    
    /**
     * @brief 获取信息面板widget
     * @return 信息面板widget指针
     */
    QWidget* getInfoPanel() const { return infoPanel_; }
    
    /**
     * @brief 获取指北针widget
     * @return 指北针widget指针
     */
    QWidget* getCompassWidget() const { return compassWidget_; }
    
    /**
     * @brief 获取比例尺widget
     * @return 比例尺widget指针
     */
    QWidget* getScaleWidget() const { return scaleWidget_; }
    
    /**
     * @brief 更新指北针和比例尺widget的位置
     */
    void updateOverlayWidgetsPosition(int parentWidth, int parentHeight);

public slots:
    /**
     * @brief 更新所有信息（从MapStateManager获取）
     */
    void updateAllInfo();


protected:
    /**
     * @brief 鼠标事件处理（让信息面板外的区域透明传递鼠标事件）
     */
    bool event(QEvent* event) override;

private:
    // 信息标签
    QLabel* mouseCoordLabel_;        // 鼠标坐标
    QLabel* headingLabel_;           // 航向角
    QLabel* pitchLabel_;             // 俯仰角
    QLabel* rangeLabel_;             // 距离
    
    // 指北针相关
    bool showCompass_;               // 是否显示指北针
    double compassHeading_;          // 指北针航向角
    
    // 比例尺相关
    bool showScale_;                 // 是否显示比例尺
    double scaleRange_;              // 比例尺对应的距离（米）
    
    // 管理器引用
    MapStateManager* mapStateManager_;
    PlanFileManager* planFileManager_;
    
    // 信息面板（作为OsgMapWidget的直接子控件）
    QWidget* infoPanel_;                          // 信息面板容器
    
    // 指北针和比例尺widget（作为OsgMapWidget的直接子控件，不覆盖整个窗口）
    QWidget* compassWidget_;                      // 指北针widget（CompassWidget*，但使用QWidget*避免前向声明问题）
    QWidget* scaleWidget_;                        // 比例尺widget（ScaleWidget*，但使用QWidget*避免前向声明问题）
    
    /**
     * @brief 初始化UI
     */
    void setupUI();
    
    
};

#endif // MAPINFOOVERLAY_H

