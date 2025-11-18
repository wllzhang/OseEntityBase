/**
 * @file BaseMapDialog.h
 * @brief 底图管理对话框头文件
 * 
 * 定义BaseMapDialog类，用于显示和切换底图
 */

#ifndef BASEMAPDIALOG_H
#define BASEMAPDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>

class BaseMapManager;

/**
 * @brief 底图管理对话框
 * 
 * 提供底图列表显示和切换功能
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
     * @brief 底图列表项双击槽函数
     * @param item 列表项
     */
    void onItemDoubleClicked(QListWidgetItem* item);
    
    /**
     * @brief 确定按钮点击槽函数
     */
    void onOkClicked();
    
    /**
     * @brief 取消按钮点击槽函数
     */
    void onCancelClicked();
    
    /**
     * @brief 底图切换完成槽函数
     * @param mapName 底图名称
     */
    void onBaseMapSwitched(const QString& mapName);

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
     * @brief 高亮当前选中的底图
     */
    void highlightCurrentBaseMap();

    BaseMapManager* baseMapManager_;  // 底图管理器
    QListWidget* listWidget_;        // 底图列表控件
    QLabel* currentMapLabel_;         // 当前底图标签
    QString selectedMapName_;         // 选中的底图名称
};

#endif // BASEMAPDIALOG_H

