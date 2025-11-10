#ifndef WEAPONMOUNTDIALOG_H
#define WEAPONMOUNTDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSpinBox>
#include <QMap>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>
#include "../util/databaseutils.h"

// 前向声明
class GeoEntity;

/**
 * @brief 武器挂载信息结构体
 *
 * 存储武器的挂载信息，包括武器ID、名称和数量
 */
struct WeaponMountInfo {
    QString weaponId;      // 武器ID
    QString weaponName;    // 武器名称
    int quantity;          // 挂载数量
};

/**
 * @brief 武器挂载对话框
 *
 * 用于为地图上的模型实体配置武器挂载信息。提供以下功能：
 * - 左侧显示武器树状结构（仅显示"空空导弹"和"空面导弹"类型）
 * - 右侧显示武器数量输入框
 * - 用户选择武器并填入数量后，点击保存按钮将挂载信息保存到方案中
 *
 * 主要特性：
 * - 从数据库读取武器模型（筛选类型为"空空导弹"和"空面导弹"）
 * - 树状结构展示武器（按类型分类）
 * - 数量输入和验证
 * - 挂载信息保存到实体属性中
 */
class WeaponMountDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param entity 要配置武器挂载的实体指针
     * @param parent 父widget
     */
    explicit WeaponMountDialog(GeoEntity* entity, QWidget *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~WeaponMountDialog();

private slots:
    /**
     * @brief 武器树项选择变化槽函数
     * @param current 当前选中的项
     * @param previous 之前选中的项
     */
    void onWeaponTreeSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);

    /**
     * @brief 保存按钮点击槽函数
     *
     * 保存武器挂载信息到实体属性中
     */
    void onSaveButtonClicked();

    /**
     * @brief 取消按钮点击槽函数
     */
    void onCancelButtonClicked();

private:
    /**
     * @brief 设置UI界面
     */
    void setupUI();

    /**
     * @brief 加载武器树
     *
     * 从数据库加载武器模型（仅"空空导弹"和"空面导弹"类型），并按类型分类显示
     */
    void loadWeaponTree();

    /**
     * @brief 加载已保存的挂载信息
     *
     * 从实体属性中读取已保存的武器挂载信息，并更新UI
     */
    void loadSavedMountInfo();

    /**
     * @brief 获取所有挂载信息
     *
     * 从UI中收集所有武器的挂载信息
     * @return 武器挂载信息列表
     */
    QList<WeaponMountInfo> getAllMountInfo() const;

    /**
     * @brief 从数据库获取武器的完整信息
     * @param weaponId 武器ID
     * @return 武器信息JSON对象
     */
    QJsonObject getWeaponFullInfo(const QString& weaponId) const;

    /**
     * @brief 从数据库获取组件完整信息
     * @param componentId 组件ID
     * @return 组件信息JSON对象
     */
    QJsonObject getComponentFullInfoFromDatabase(const QString& componentId) const;

    /**
     * @brief 解析组件列表字符串为ID列表
     * @param componentListStr 组件列表字符串
     * @return 组件ID列表
     */
    QStringList parseComponentList(const QString& componentListStr) const;

    GeoEntity* entity_;                    // 要配置的实体指针
    QTreeWidget* weaponTree_;              // 武器树控件
    QSpinBox* quantitySpinBox_;            // 数量输入框
    QLabel* weaponNameLabel_;              // 武器名称标签
    QPushButton* saveButton_;              // 保存按钮
    QPushButton* cancelButton_;            // 取消按钮

    // 存储武器ID到数量的映射（用于快速查找和更新）
    QMap<QString, int> weaponQuantityMap_; // 武器ID -> 数量
    QMap<QString, QString> weaponNameMap_;  // 武器ID -> 武器名称
};


#endif // WEAPONMOUNTDIALOG_H
