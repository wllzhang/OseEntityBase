/**
 * @file EntityPropertyDialog.h
 * @brief 实体属性编辑对话框头文件
 * 
 * 定义EntityPropertyDialog类，用于编辑实体的所有属性
 */

#ifndef ENTITYPROPERTYDIALOG_H
#define ENTITYPROPERTYDIALOG_H

#include <QDialog>
#include <QTabWidget>
#include <QJsonObject>
#include <QMap>
#include <QVariant>

// 前向声明
class GeoEntity;
class PlanFileManager;
class QLineEdit;
class QLabel;
class QDoubleSpinBox;
class QCheckBox;
class QComboBox;
class QPushButton;
class QListWidget;
class QScrollArea;
class QFormLayout;
class QVBoxLayout;
class QWidget;

/**
 * @brief 实体属性编辑对话框
 * 
 * 提供4个标签页编辑实体的所有属性：
 * 1. 基本信息：实体名称、模型信息
 * 2. 规划属性：位置、航向、可见性
 * 3. 模型组装属性：部署位置、二维军标、组件列表
 * 4. 组件配置：动态表单，根据templateInfo生成
 */
class EntityPropertyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntityPropertyDialog(GeoEntity* entity, PlanFileManager* planFileManager, QWidget *parent = nullptr);
    ~EntityPropertyDialog();

private slots:
    void onBrowseIconButtonClicked();
    void onAddComponentClicked();
    void onRemoveComponentClicked();
    void onSaveClicked();
    void onCancelClicked();

private:
    void setupUI();
    void loadEntityData();
    void loadModelAssemblyData();
    void loadComponentConfigs();
    
    // 动态表单生成（参考ComponentConfigDialog）
    void clearComponentConfigForms();
    void createComponentConfigForm(const QString& componentId, const QJsonObject& templateInfo, const QJsonObject& configInfo);
    QWidget* createFormWidget(int type, const QStringList& values = QStringList(), const QVariant& currentValue = QVariant());
    
    // 数据保存
    void savePlanningProperties();
    void saveModelAssemblyProperties();
    void saveComponentConfigs();
    
    // 辅助方法
    QJsonObject getModelAssemblyFromDatabase(const QString& modelId);
    QJsonObject getComponentConfigFromDatabase(const QString& componentId);
    QJsonObject getComponentTemplateFromDatabase(const QString& componentId);
    QJsonObject getComponentFullInfoFromDatabase(const QString& componentId);  // 获取完整的组件信息
    bool jsonObjectsDiffer(const QJsonObject& obj1, const QJsonObject& obj2);
    
    // 基本信息标签页
    void setupBasicInfoTab();
    
    // 规划属性标签页
    void setupPlanningTab();
    
    // 模型组装属性标签页
    void setupModelAssemblyTab();
    
    // 组件配置标签页
    void setupComponentConfigTab();

private:
    GeoEntity* entity_;
    PlanFileManager* planFileManager_;
    
    QTabWidget* tabWidget_;
    
    // 基本信息标签页控件
    QLineEdit* nameEdit_;
    QLabel* modelInfoLabel_;
    
    // 规划属性标签页控件
    QDoubleSpinBox* longitudeSpinBox_;
    QDoubleSpinBox* latitudeSpinBox_;
    QDoubleSpinBox* altitudeSpinBox_;
    QDoubleSpinBox* headingSpinBox_;
    QCheckBox* visibleCheckBox_;
    
    // 模型组装属性标签页控件
    QComboBox* locationComboBox_;
    QLineEdit* iconEdit_;
    QPushButton* browseIconButton_;
    QListWidget* componentListWidget_;
    QPushButton* addComponentButton_;
    QPushButton* removeComponentButton_;
    
    // 组件配置标签页
    QScrollArea* componentConfigScrollArea_;
    QWidget* componentConfigContainer_;
    QVBoxLayout* componentConfigLayout_;
    QMap<QString, QWidget*> componentConfigWidgets_;  // componentId -> 配置表单widget
    QMap<QString, QMap<QString, QWidget*>> componentParamWidgets_;  // componentId -> (paramName -> widget)
    
    // 数据缓存
    QString modelId_;
    QString modelName_;
    QJsonObject dbModelAssembly_;  // 数据库中的模型组装默认值（仅用于初始加载）
    QMap<QString, QJsonObject> dbComponentConfigs_;  // 数据库中的组件配置默认值（仅用于初始加载）
    QMap<QString, QJsonObject> componentTemplates_;  // 组件的模板信息（用于编辑和保存）
    QMap<QString, QJsonObject> componentFullInfo_;   // 组件的完整信息（componentId, name, type, configInfo, templateInfo等）
};

#endif // ENTITYPROPERTYDIALOG_H

