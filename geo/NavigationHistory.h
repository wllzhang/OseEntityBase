/**
 * @file NavigationHistory.h
 * @brief 导航历史管理类头文件
 * 
 * 定义NavigationHistory类，用于管理相机视角的导航历史（前进/后退）
 */

#ifndef NAVIGATIONHISTORY_H
#define NAVIGATIONHISTORY_H

#include <QObject>
#include <QStack>
#include <QString>
#include <QList>
#include <osgEarth/Viewpoint>

/**
 * @brief 导航历史管理类
 * 
 * 管理相机视角的导航历史，支持前进/后退功能
 */
class NavigationHistory : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit NavigationHistory(QObject* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~NavigationHistory();
    
    /**
     * @brief 添加新的视角到历史记录
     * @param viewpoint 相机视角
     * 
     * 会自动检查是否与上一个视角相似，如果相似则不记录（避免重复记录）
     */
    void pushViewpoint(const osgEarth::Viewpoint& viewpoint);
    
    /**
     * @brief 保存当前视角并后退到上一个视角
     * @param currentViewpoint 当前视角（将被保存到前进栈）
     * @param backViewpoint 输出后退的视角
     * @return 成功返回true，无历史记录返回false
     */
    bool goBack(const osgEarth::Viewpoint& currentViewpoint, osgEarth::Viewpoint& backViewpoint);
    
    /**
     * @brief 保存当前视角并前进到下一个视角
     * @param currentViewpoint 当前视角（将被保存到后退栈）
     * @param forwardViewpoint 输出前进的视角
     * @return 成功返回true，无历史记录返回false
     */
    bool goForward(const osgEarth::Viewpoint& currentViewpoint, osgEarth::Viewpoint& forwardViewpoint);
    
    /**
     * @brief 清除所有历史记录
     */
    void clear();
    
    /**
     * @brief 是否可以后退
     * @return 可以后退返回true
     */
    bool canGoBack() const;
    
    /**
     * @brief 是否可以前进
     * @return 可以前进返回true
     */
    bool canGoForward() const;
    
    /**
     * @brief 获取历史记录数量
     * @return 历史记录数量
     */
    int getHistoryCount() const;
    
    /**
     * @brief 历史记录项结构
     */
    struct HistoryItem {
        osgEarth::Viewpoint viewpoint;  // 视角
        int index;                      // 在完整历史中的索引
        bool isCurrent;                 // 是否为当前视角
        QString displayName;            // 显示名称
    };
    
    /**
     * @brief 获取所有历史记录列表
     * @param currentViewpoint 当前视角（用于标记当前位置）
     * @return 历史记录列表，按时间顺序排列（最旧的在前面）
     */
    QList<HistoryItem> getAllHistory(const osgEarth::Viewpoint& currentViewpoint) const;
    
    /**
     * @brief 检查视角是否在历史记录中
     * @param viewpoint 要检查的视角
     * @return 如果在历史记录中返回true，否则返回false
     */
    bool isViewpointInHistory(const osgEarth::Viewpoint& viewpoint) const;
    
    /**
     * @brief 跳转到指定视角（智能处理历史记录）
     * @param currentViewpoint 当前视角
     * @param targetViewpoint 目标视角
     * @return 如果目标视角在历史记录中返回true，否则返回false
     * 
     * 如果目标视角在历史记录中，不保存当前视角，直接跳转
     * 如果目标视角不在历史记录中，将当前视角保存到后退栈，清除前进栈
     */
    bool jumpToViewpoint(const osgEarth::Viewpoint& currentViewpoint, const osgEarth::Viewpoint& targetViewpoint);

signals:
    /**
     * @brief 历史记录状态变化信号
     * @param canBack 是否可以后退
     * @param canForward 是否可以前进
     */
    void historyStateChanged(bool canBack, bool canForward);

private:
    QStack<osgEarth::Viewpoint> backStack_;      // 后退历史栈
    QStack<osgEarth::Viewpoint> forwardStack_;   // 前进历史栈
    int maxHistorySize_;                         // 最大历史记录数量
    
    /**
     * @brief 发出状态变化信号
     */
    void emitStateChanged();
};

#endif // NAVIGATIONHISTORY_H

