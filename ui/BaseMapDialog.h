/**
 * @file BaseMapDialog.h
 * @brief 底图管理对话框头文件
 * 
 * 定义BaseMapDialog类，用于管理多个底图图层的叠加显示
 */

#ifndef BASEMAPDIALOG_H
#define BASEMAPDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>
#include "../geo/basemapmanager.h"

class BaseMapManager;

/**
 * @brief 底图管理对话框
 * 
 * 提供底图图层的叠加管理功能：
 * - 显示所有已加载的底图图层
 * - 控制每个图层的显示/隐藏和透明度
 * - 添加、编辑、删除底图图层
 * - 从模板快速添加底图
 */
class BaseMapDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param baseMapManager 底图管理器
     * @param parent 父窗口
     */
    explicit BaseMapDialog(BaseMapManager* baseMapManager, QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~BaseMapDialog();

private slots:
    /**
     * @brief 添加按钮点击槽函数
     */
    void onAddClicked();
    
    /**
     * @brief 从模板添加按钮点击槽函数
     */
    void onAddFromTemplateClicked();
    
    /**
     * @brief 编辑按钮点击槽函数
     */
    void onEditClicked();
    
    /**
     * @brief 删除按钮点击槽函数
     */
    void onDeleteClicked();
    
    /**
     * @brief 上移按钮点击槽函数（图层向上移动，显示更上层）
     */
    void onMoveUpClicked();
    
    /**
     * @brief 下移按钮点击槽函数（图层向下移动，显示更下层）
     */
    void onMoveDownClicked();
    
    /**
     * @brief 确定按钮点击槽函数
     */
    void onOkClicked();
    
    /**
     * @brief 取消按钮点击槽函数
     */
    void onCancelClicked();
    
    /**
     * @brief 底图图层可见性改变槽函数
     * @param item 树项
     * @param column 列
     */
    void onItemChanged(QTreeWidgetItem* item, int column);
    
    /**
     * @brief 底图图层添加完成槽函数
     * @param mapName 底图名称
     */
    void onBaseMapAdded(const QString& mapName);
    
    /**
     * @brief 底图图层移除完成槽函数
     * @param mapName 底图名称
     */
    void onBaseMapRemoved(const QString& mapName);
    
    /**
     * @brief 底图图层更新完成槽函数
     * @param mapName 底图名称
     */
    void onBaseMapUpdated(const QString& mapName);

private:
    /**
     * @brief 初始化UI
     */
    void setupUI();
    
    /**
     * @brief 更新底图列表显示
     */
    void updateBaseMapList();
    
    /**
     * @brief 创建底图树项
     * @param name 底图名称
     * @param config 底图配置
     * @return 树项指针
     */
    QTreeWidgetItem* createBaseMapItem(const QString& name, const BaseMapSource& config);
    
    /**
     * @brief 获取当前选中的底图名称
     * @return 底图名称，未选中返回空字符串
     */
    QString getSelectedBaseMapName() const;
    
    /**
     * @brief 显示底图配置编辑对话框
     * @param source 底图配置（用于编辑时传入现有配置）
     * @return 编辑后的配置，取消返回空配置
     */
    BaseMapSource showConfigDialog(const BaseMapSource& source = BaseMapSource());

    BaseMapManager* baseMapManager_;  // 底图管理器
    QTreeWidget* treeWidget_;         // 底图树控件
    QPushButton* addButton_;           // 添加按钮
    QPushButton* addFromTemplateButton_; // 从模板添加按钮
    QPushButton* editButton_;          // 编辑按钮
    QPushButton* deleteButton_;        // 删除按钮
    QPushButton* moveUpButton_;        // 上移按钮
    QPushButton* moveDownButton_;      // 下移按钮
};

#endif // BASEMAPDIALOG_H
