/**
 * @file ModelAssemblyDialog.h
 * @brief 模型组装对话框头文件
 * 
 * 定义ModelAssemblyDialog类，用于配置和管理模型组装信息
 */

#ifndef MODELASSEMBLYDIALOG_H
#define MODELASSEMBLYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTreeWidget>
#include <QSqlDatabase>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include "../util/databaseutils.h"

/**
 * @brief 模型信息结构体
 * 
 * 存储模型的完整信息，包括ID、名称、类型、部署位置、图标和组件列表
 */
struct ModelInfo {
    QString id;                    // 模型ID
    QString name;                  // 模型名称
    QString type;                  // 模型类型
    QString location;               // 部署位置
    QString icon;                  // 二维军标图标路径（绝对路径）
    QStringList componentList;     // 组件ID列表
};

/**
 * @brief 模型组装对话框
 * 
 * 用于配置和管理模型组装信息。提供以下功能：
 * - 模型树状结构浏览（按类型分类）
 * - 模型基本信息的编辑（名称、类型、部署位置、二维军标）
 * - 组件列表的组装（从组件树拖拽或双击添加）
 * - 模型的添加、删除
 * - 模型组装信息的保存到数据库
 * 
 * 主要特性：
 * - 二维军标图标选择（文件选择对话框，存储绝对路径）
 * - 组件列表的拖拽和双击添加
 * - 数据库集成（使用DatabaseUtils）
 * - 右键菜单操作（添加、删除模型）
 */
class ModelAssemblyDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父widget
     */
    explicit ModelAssemblyDialog(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ModelAssemblyDialog();

private slots:
    /**
     * @brief 模型树项点击槽函数
     * @param item 点击的树项
     * @param column 列索引
     */
    void onModelTreeItemClicked(QTreeWidgetItem *item, int column);
    
    /**
     * @brief 组件树双击槽函数
     * 
     * 双击组件树项，将组件添加到组装列表
     * @param item 双击的树项
     * @param column 列索引
     */
    void onComponentTreeDoubleClicked(QTreeWidgetItem *item, int column);
    
    /**
     * @brief 组装列表双击槽函数
     * 
     * 双击组装列表项，从列表中移除
     * @param item 双击的列表项
     */
    void onAssemblyListItemDoubleClicked(QListWidgetItem *item);
    
    /**
     * @brief 保存按钮点击槽函数
     * 
     * 保存当前模型的组装信息到数据库
     */
    void onSaveButtonClicked();
    
    /**
     * @brief 添加模型操作
     */
    void onAddModelAction();
    
    /**
     * @brief 删除模型操作
     */
    void ondeleteModelAction();
    
    /**
     * @brief 浏览图标按钮点击槽函数
     * 
     * 打开文件选择对话框选择二维军标图标
     */
    void onBrowseIconButtonClicked();

    /**
     * @brief 模型搜索框文本变化
     * @param text 搜索关键字
     */
    void onModelSearchTextChanged(const QString &text);

    /**
     * @brief 组件搜索框文本变化
     * @param text 搜索关键字
     */
    void onComponentSearchTextChanged(const QString &text);

private:
    /**
     * @brief 设置UI界面
     */
    void setupUI();
    
    /**
     * @brief 设置数据库连接
     */
    void setupDatabase();
    
    /**
     * @brief 加载模型树
     * 
     * 从数据库加载模型并按类型分类显示
     */
    void loadModelTree();
    
    /**
     * @brief 加载模型类型列表
     */
    void loadModelTypes();
    
    /**
     * @brief 加载所有模型
     */
    void loadModels();
    
    /**
     * @brief 加载组件树状结构
     * 
     * 从数据库加载组件树，用于组装时选择组件
     */
    void loadComponentTree();

    /**
     * @brief 更新模型的通用信息到UI
     * @param info 模型信息
     */
    void updateModelInfo(const ModelInfo &info);
    
    /**
     * @brief 清空组装列表
     */
    void clearAssemblyList();
    
    /**
     * @brief 加载组装列表
     * @param componentIds 组件ID列表
     */
    void loadAssemblyList(const QStringList &componentIds);
    
    /**
     * @brief 显示右键上下文菜单
     * @param pos 菜单位置
     */
    void showContextMenu(const QPoint &pos);
    
    /**
     * @brief 添加新模型
     * @param modelName 模型名称
     */
    void addModel(QString modelName);
    
    /**
     * @brief 获取当前模型信息
     * @return 模型信息结构体
     */
    ModelInfo getCurrentModelInfo() const;

    void resetModelTreeFilter();
    bool filterTreeItem(QTreeWidgetItem *item, const QString &keyword);
    void resetComponentTreeFilter();
    bool filterComponentTreeItem(QTreeWidgetItem *item, const QString &keyword);


    QSqlDatabase db;                      // 数据库连接
    QTreeWidget *modelTree;               // 模型树控件
    QTreeWidget *componentTree;           // 组件树控件
    QLineEdit *modelSearchEdit;           // 模型搜索框
    QLineEdit *componentSearchEdit;       // 组件搜索框
    QListWidget *assemblyList;            // 组件装配列表

    // 模型基本信息控件
    QLineEdit *modelNameEdit;             // 模型名称输入框
    QLineEdit *modelTypeEdit;             // 模型类型输入框
    QComboBox *modelLocationComboBox;     // 部署位置下拉框
    QLineEdit *modelIconEdit;             // 二维军标图标路径输入框
    QPushButton *browseIconButton;       // 浏览图标按钮

    ModelInfo currentModelInfo;            // 当前模型信息
    QTreeWidgetItem *currentItem;        // 当前选中的树项

};


#endif // MODELASSEMBLYDIALOG_H
