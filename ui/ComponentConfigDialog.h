/**
 * @file ComponentConfigDialog.h
 * @brief 组件配置对话框头文件
 * 
 * 定义ComponentConfigDialog类，用于配置和管理模型组件参数
 */

#ifndef COMPONENTCONFIGDIALOG_H
#define COMPONENTCONFIGDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QSqlDatabase>
#include <QJsonObject>
#include "../util/databaseutils.h"

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QLabel;

/**
 * @brief 组件信息结构体
 * 
 * 存储组件的完整信息，包括ID、名称、类型、配置和模板信息
 */
struct ComponentInfo {
    QString componentId;        // 组件ID
    QString name;               // 组件名称
    QString type;               // 组件类型
    QString wsf;                // WSF文件路径
    QString subtype;            // 子类型
    QJsonObject configInfo;     // 配置信息（JSON格式）
    QJsonObject templateInfo;   // 模板信息（JSON格式，用于动态生成表单）
};

/**
 * @brief 组件配置对话框
 * 
 * 用于配置和管理模型组件的参数。提供以下功能：
 * - 组件树状结构浏览（按类型分类）
 * - 组件参数的动态表单编辑（根据templateInfo生成）
 * - 组件的添加、复制、删除
 * - 组件参数的保存到数据库
 * 
 * 主要特性：
 * - 支持多种参数类型（文本、数字、下拉框、复选框等）
 * - 动态表单生成（根据templateInfo动态创建控件）
 * - 数据库集成（使用DatabaseUtils）
 * - 右键菜单操作（复制、删除）
 */
class ComponentConfigDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父widget
     */
    explicit ComponentConfigDialog(QWidget *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~ComponentConfigDialog();

private slots:
    /**
     * @brief 组件树项点击槽函数
     * @param item 点击的树项
     * @param column 列索引
     */
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    
    /**
     * @brief 保存按钮点击槽函数
     * 
     * 保存当前组件的配置到数据库
     */
    void onSaveButtonClicked();
    
    /**
     * @brief 显示右键上下文菜单
     * @param pos 菜单位置
     */
    void showContextMenu(const QPoint &pos);
    
    /**
     * @brief 复制组件操作
     */
    void copyComponent();
    
    /**
     * @brief 删除组件操作
     */
    void deleteComponent();

    /**
     * @brief 组件搜索框文本变化槽函数
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
     * @brief 加载组件树
     * 
     * 从数据库加载组件并按类型分类显示
     */
    void loadComponentTree();
    
    /**
     * @brief 加载组件类型列表
     */
    void loadComponentTypes();
    
    /**
     * @brief 加载所有组件
     */
    void loadComponents();
    
    /**
     * @brief 清空参数表单
     */
    void clearParameterForm();
    
    /**
     * @brief 创建参数表单
     * 
     * 根据templateInfo动态生成表单控件
     * @param templateInfo 模板信息（定义参数类型和选项）
     * @param configInfo 配置信息（当前参数值，可选）
     */
    void createParameterForm(const QJsonObject &templateInfo, const QJsonObject &configInfo = QJsonObject());
    
    /**
     * @brief 创建表单控件
     * @param type 控件类型
     * @param values 选项值列表（用于下拉框等）
     * @param currentValue 当前值
     * @return 创建的控件指针
     */
    QWidget* createFormWidget(int type, const QStringList &values = QStringList(), const QVariant &currentValue = QVariant());
    
    /**
     * @brief 更新组件信息到UI
     * @param info 组件信息
     */
    void updateComponentInfo(const ComponentInfo &info);
    
    /**
     * @brief 获取当前组件信息
     * @return 组件信息结构体
     */
    ComponentInfo getCurrentComponentInfo() const;
    
    /**
     * @brief 生成新的组件ID
     * @return 生成的组件ID
     */
    QString generateComponentId();
    
    /**
     * @brief 检查组件是否被使用
     * @param componentId 组件ID
     * @return 被使用返回true
     */
    bool isComponentUsed(QString componentId);

    // 20251104 修改 增加对嵌套JSON格式的参数配置
    //    QWidget* createFormWidget(int type, const QStringList &values = QStringList(), const QVariant &currentValue = QVariant());
    QVariant getParameterValue(const QString &paramName, int type, const QStringList &values, const QJsonObject &paramConfig, const QJsonObject &configInfo);
    QVariant getDefaultValue(int type, const QStringList &values, const QJsonObject &paramConfig);
    QString convertToString(const QJsonValue &jsonValue);
    int convertToComboBoxIndex(const QJsonValue &jsonValue, const QStringList &values);
    int convertToInt(const QJsonValue &jsonValue);
    bool convertToBool(const QJsonValue &jsonValue);
    double convertToDouble(const QJsonValue &jsonValue);
    QString convertToRangeString(const QJsonValue &jsonValue);
    QVariant convertToNestedObject(const QJsonValue &jsonValue, const QJsonObject &paramConfig);
    QWidget* createFormWidget(QString paramName, int type, const QStringList &values, const QVariant &currentValue, const QJsonObject &paramConfig);
    QWidget* createLineEditWidget(const QVariant &currentValue);
    QWidget* createComboBoxWidget(const QStringList &values, const QVariant &currentValue);
    QWidget* createSpinBoxWidget(const QVariant &currentValue);
    QWidget* createCheckBoxWidget(const QVariant &currentValue);
    QWidget* createDoubleSpinBoxWidget(const QVariant &currentValue);
    QWidget* createRangeWidget(const QVariant &currentValue);
    QWidget* createNestedJsonWidget(QString parentName, const QVariant &currentValue, const QJsonObject &paramConfig);
    QWidget* createUnsupportedWidget(int type);
    // 20251104 修改 增加对嵌套JSON类型参数配置内容的保存
    QJsonObject collectFormData();
    QVariant getWidgetValue(QWidget *widget, const QString &paramName);
    bool validateFormData(const QJsonObject &configInfo);

    /**
     * @brief 重置组件树过滤状态，恢复所有节点可见
     */
    void resetComponentTreeFilter();
    /**
     * @brief 递归判断并过滤组件树节点
     * @param item 当前检索的节点
     * @param keyword 搜索关键词
     * @return 节点或其子节点是否匹配
     */
    bool filterComponentTreeItem(QTreeWidgetItem *item, const QString &keyword);
    /**
     * @brief 递归调用，设置节点隐藏状态，同时控制展开状态
     */
    void setTreeItemHiddenRecursive(QTreeWidgetItem *item, bool hidden);

    QSqlDatabase db;                        // 数据库连接
    QTreeWidget *componentTree;             // 组件树控件
    QLineEdit *componentSearchEdit;         // 组件搜索框
    QVBoxLayout *rightLayout;               // 右侧布局
    QWidget *parameterWidget;               // 参数表单widget
    QFormLayout *parameterFormLayout;       // 参数表单布局

    // 通用信息控件
    QLineEdit *nameEdit;                    // 名称输入框
    QComboBox *typeComboBox;                // 类型下拉框
    QLineEdit *wsfEdit;                     // WSF文件路径输入框
    QLineEdit *commentEdit;                 // 注释输入框

    // 动态生成的参数控件映射
    QMap<QString, QWidget*> paramWidgets;   // 参数名 -> 控件映射
    ComponentInfo currentComponentInfo;     // 当前组件信息
    QTreeWidgetItem *currentItem;          // 当前选中的树项
    QMap<QString, QJsonObject> nestedTemplates;  // 20251104 修改 存储嵌套JSON的模板结构

};

#endif // COMPONENTCONFIGDIALOG_H
