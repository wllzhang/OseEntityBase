/**
 * @file NavigationHistoryDialog.h
 * @brief 导航历史列表对话框
 * 
 * 显示所有视角历史记录，支持跳转到指定视角
 */

#ifndef NAVIGATIONHISTORYDIALOG_H
#define NAVIGATIONHISTORYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include "../geo/NavigationHistory.h"
#include <osgEarth/Viewpoint>

/**
 * @brief 导航历史列表对话框
 * 
 * 显示所有视角历史记录，支持双击或点击跳转按钮跳转到指定视角
 */
class NavigationHistoryDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param navigationHistory 导航历史管理器
     * @param currentViewpoint 当前视角
     * @param parent 父窗口
     */
    explicit NavigationHistoryDialog(NavigationHistory* navigationHistory,
                                     const osgEarth::Viewpoint& currentViewpoint,
                                     QWidget *parent = nullptr);
    
    /**
     * @brief 刷新历史记录列表
     * @param currentViewpoint 当前视角
     */
    void refreshHistory(const osgEarth::Viewpoint& currentViewpoint);

signals:
    /**
     * @brief 请求跳转到指定视角
     * @param viewpoint 目标视角
     */
    void jumpToViewpoint(const osgEarth::Viewpoint& viewpoint);

private slots:
    /**
     * @brief 列表项双击槽函数
     * @param item 被双击的列表项
     */
    void onItemDoubleClicked(QListWidgetItem* item);
    
    /**
     * @brief 跳转按钮点击槽函数
     */
    void onJumpClicked();
    
    /**
     * @brief 关闭按钮点击槽函数
     */
    void onCloseClicked();
    
    /**
     * @brief 列表项选择变化槽函数
     */
    void onSelectionChanged();

private:
    /**
     * @brief 更新历史记录列表显示
     */
    void updateHistoryList();
    
    /**
     * @brief 格式化视角信息为显示文本
     * @param item 历史记录项
     * @return 格式化后的文本
     */
    QString formatViewpointInfo(const NavigationHistory::HistoryItem& item) const;

    NavigationHistory* navigationHistory_;  // 导航历史管理器
    osgEarth::Viewpoint currentViewpoint_;  // 当前视角
    
    QListWidget* historyListWidget_;        // 历史记录列表
    QPushButton* jumpButton_;               // 跳转按钮
    QPushButton* closeButton_;              // 关闭按钮
    QLabel* infoLabel_;                     // 信息标签
    
    QList<NavigationHistory::HistoryItem> historyItems_;  // 历史记录项列表
};

#endif // NAVIGATIONHISTORYDIALOG_H

