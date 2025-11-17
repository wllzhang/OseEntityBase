/**
 * @file EntityPropertyDialog.cpp
 * @brief 实体属性编辑对话框实现文件
 * 
 * 实现EntityPropertyDialog类的所有功能
 */

#include "EntityPropertyDialog.h"
#include "../geo/geoentity.h"
#include "../plan/planfilemanager.h"
#include "../util/databaseutils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>
#include <QListWidget>
#include <QScrollArea>
#include <QFileDialog>
#include <QMessageBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>
#include <QTabWidget>
#include <QSpinBox>
#include <QSpacerItem>
#include <QSizePolicy>

EntityPropertyDialog::EntityPropertyDialog(GeoEntity* entity, PlanFileManager* planFileManager, QWidget *parent)
    : QDialog(parent)
    , entity_(entity)
    , planFileManager_(planFileManager)
    , tabWidget_(nullptr)
    , nameEdit_(nullptr)
    , modelInfoLabel_(nullptr)
    , longitudeSpinBox_(nullptr)
    , latitudeSpinBox_(nullptr)
    , altitudeSpinBox_(nullptr)
    , headingSpinBox_(nullptr)
    , visibleCheckBox_(nullptr)
    , locationComboBox_(nullptr)
    , iconEdit_(nullptr)
    , browseIconButton_(nullptr)
    , componentListWidget_(nullptr)
    , addComponentButton_(nullptr)
    , removeComponentButton_(nullptr)
    , componentConfigScrollArea_(nullptr)
    , componentConfigContainer_(nullptr)
    , componentConfigLayout_(nullptr)
{
    if (!entity_) {
        QMessageBox::warning(this, "错误", "实体对象为空");
        return;
    }

    // 获取模型ID和名称
    modelId_ = entity_->getProperty("modelId").toString();
    modelName_ = entity_->getName();

    setWindowTitle(QString("编辑实体属性 - %1").arg(modelName_));
    resize(800, 600);

    setupUI();
    loadEntityData();
    loadModelAssemblyData();
    loadComponentConfigs();
}

EntityPropertyDialog::~EntityPropertyDialog()
{
}

void EntityPropertyDialog::setupUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // 创建标签页
    tabWidget_ = new QTabWidget(this);
    setupBasicInfoTab();
    setupPlanningTab();
    setupModelAssemblyTab();
    setupComponentConfigTab();

    mainLayout->addWidget(tabWidget_);

    // 按钮区域
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    QPushButton* saveButton = new QPushButton("应用", this);
    QPushButton* cancelButton = new QPushButton("取消", this);
    connect(saveButton, &QPushButton::clicked, this, &EntityPropertyDialog::onSaveClicked);
    connect(cancelButton, &QPushButton::clicked, this, &EntityPropertyDialog::onCancelClicked);
    buttonLayout->addWidget(saveButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);
}

void EntityPropertyDialog::setupBasicInfoTab()
{
    QWidget* tab = new QWidget;
    QFormLayout* layout = new QFormLayout(tab);

    nameEdit_ = new QLineEdit;
    nameEdit_->setText(entity_->getProperty("displayName").toString().isEmpty() ? 
                       entity_->getName() : entity_->getProperty("displayName").toString());
    layout->addRow("实体名称:", nameEdit_);

    QString uid = entity_->getUid();
    modelInfoLabel_ = new QLabel(QString("模型: %1 (UID: %2)").arg(modelName_).arg(uid));
    modelInfoLabel_->setStyleSheet("color: #666;");
    layout->addRow("模型信息:", modelInfoLabel_);

    tabWidget_->addTab(tab, "基本信息");
}

void EntityPropertyDialog::setupPlanningTab()
{
    QWidget* tab = new QWidget;
    QFormLayout* layout = new QFormLayout(tab);

    // 位置
    longitudeSpinBox_ = new QDoubleSpinBox;
    longitudeSpinBox_->setRange(-180.0, 180.0);
    longitudeSpinBox_->setDecimals(6);
    longitudeSpinBox_->setSuffix("°");
    layout->addRow("经度:", longitudeSpinBox_);

    latitudeSpinBox_ = new QDoubleSpinBox;
    latitudeSpinBox_->setRange(-90.0, 90.0);
    latitudeSpinBox_->setDecimals(6);
    latitudeSpinBox_->setSuffix("°");
    layout->addRow("纬度:", latitudeSpinBox_);

    altitudeSpinBox_ = new QDoubleSpinBox;
    altitudeSpinBox_->setRange(-10000.0, 100000.0);
    altitudeSpinBox_->setDecimals(2);
    altitudeSpinBox_->setSuffix(" m");
    layout->addRow("高度:", altitudeSpinBox_);

    // 航向
    headingSpinBox_ = new QDoubleSpinBox;
    headingSpinBox_->setRange(0.0, 360.0);
    headingSpinBox_->setDecimals(2);
    headingSpinBox_->setSuffix("°");
    layout->addRow("航向角:", headingSpinBox_);

    // 可见性
    visibleCheckBox_ = new QCheckBox("可见");
    layout->addRow("可见性:", visibleCheckBox_);

    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    tabWidget_->addTab(tab, "规划属性");
}

void EntityPropertyDialog::setupModelAssemblyTab()
{
    QWidget* tab = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(tab);
    QFormLayout* layout = new QFormLayout;

    // 部署位置
    locationComboBox_ = new QComboBox;
    locationComboBox_->addItems(QStringList() << "空中" << "地面" << "海面");
    layout->addRow("部署位置:", locationComboBox_);

    // 二维军标
    iconEdit_ = new QLineEdit;
    iconEdit_->setReadOnly(true);
    browseIconButton_ = new QPushButton("浏览...");
    connect(browseIconButton_, &QPushButton::clicked, this, &EntityPropertyDialog::onBrowseIconButtonClicked);
    QHBoxLayout* iconLayout = new QHBoxLayout;
    iconLayout->addWidget(iconEdit_);
    iconLayout->addWidget(browseIconButton_);
    QWidget* iconWidget = new QWidget;
    iconWidget->setLayout(iconLayout);
    layout->addRow("二维军标:", iconWidget);

    mainLayout->addLayout(layout);

    // 组件列表
    QGroupBox* componentGroup = new QGroupBox("组件列表");
    QVBoxLayout* componentLayout = new QVBoxLayout(componentGroup);
    
    componentListWidget_ = new QListWidget;
    componentLayout->addWidget(componentListWidget_);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    addComponentButton_ = new QPushButton("添加组件");
    removeComponentButton_ = new QPushButton("移除组件");
    connect(addComponentButton_, &QPushButton::clicked, this, &EntityPropertyDialog::onAddComponentClicked);
    connect(removeComponentButton_, &QPushButton::clicked, this, &EntityPropertyDialog::onRemoveComponentClicked);
    buttonLayout->addWidget(addComponentButton_);
    buttonLayout->addWidget(removeComponentButton_);
    buttonLayout->addStretch();
    componentLayout->addLayout(buttonLayout);

    mainLayout->addWidget(componentGroup);

    tabWidget_->addTab(tab, "模型组装");
}

void EntityPropertyDialog::setupComponentConfigTab()
{
    QWidget* tab = new QWidget;
    QVBoxLayout* mainLayout = new QVBoxLayout(tab);

    componentConfigScrollArea_ = new QScrollArea;
    componentConfigScrollArea_->setWidgetResizable(true);
    componentConfigContainer_ = new QWidget;
    componentConfigLayout_ = new QVBoxLayout(componentConfigContainer_);
    componentConfigScrollArea_->setWidget(componentConfigContainer_);

    mainLayout->addWidget(componentConfigScrollArea_);

    tabWidget_->addTab(tab, "组件配置");
}

void EntityPropertyDialog::loadEntityData()
{
    if (!entity_) return;

    // 加载基本信息
    if (nameEdit_) {
        QString displayName = entity_->getProperty("displayName").toString();
        nameEdit_->setText(displayName.isEmpty() ? entity_->getName() : displayName);
    }

    // 加载规划属性
    double longitude, latitude, altitude;
    entity_->getPosition(longitude, latitude, altitude);
    if (longitudeSpinBox_) longitudeSpinBox_->setValue(longitude);
    if (latitudeSpinBox_) latitudeSpinBox_->setValue(latitude);
    if (altitudeSpinBox_) altitudeSpinBox_->setValue(altitude);
    if (headingSpinBox_) headingSpinBox_->setValue(entity_->getHeading());
    if (visibleCheckBox_) visibleCheckBox_->setChecked(entity_->isVisible());
}

void EntityPropertyDialog::loadModelAssemblyData()
{
    if (modelId_.isEmpty()) return;

    // 从数据库加载默认值（仅用于初始创建时）
    dbModelAssembly_ = getModelAssemblyFromDatabase(modelId_);

    // 从实体属性中获取覆盖值（如果有）
    QJsonObject entityModelAssembly = entity_->getProperty("modelAssembly").toJsonObject();
    if (!entityModelAssembly.isEmpty()) {
        // 合并覆盖值
        if (entityModelAssembly.contains("location")) {
            dbModelAssembly_["location"] = entityModelAssembly["location"];
        }
        if (entityModelAssembly.contains("icon")) {
            dbModelAssembly_["icon"] = entityModelAssembly["icon"];
        }
        
        // 如果有完整的组件信息数组，提取组件ID列表用于显示
        if (entityModelAssembly.contains("components") && entityModelAssembly["components"].isArray()) {
            QJsonArray componentsArray = entityModelAssembly["components"].toArray();
            QJsonArray componentIdList;
            for (const auto& compValue : componentsArray) {
                QJsonObject componentInfo = compValue.toObject();
                if (componentInfo.contains("componentId")) {
                    componentIdList.append(componentInfo["componentId"]);
                }
            }
            dbModelAssembly_["componentList"] = componentIdList;
        } else if (entityModelAssembly.contains("componentList")) {
            dbModelAssembly_["componentList"] = entityModelAssembly["componentList"];
        }
    }

    // 设置UI控件
    if (locationComboBox_) {
        QString location = dbModelAssembly_["location"].toString();
        int index = locationComboBox_->findText(location);
        if (index >= 0) {
            locationComboBox_->setCurrentIndex(index);
        }
    }

    if (iconEdit_) {
        iconEdit_->setText(dbModelAssembly_["icon"].toString());
    }

    if (componentListWidget_) {
        componentListWidget_->clear();
        QJsonArray componentList = dbModelAssembly_["componentList"].toArray();
        for (const auto& compId : componentList) {
            componentListWidget_->addItem(compId.toString());
        }
    }
}

void EntityPropertyDialog::loadComponentConfigs()
{
    if (modelId_.isEmpty()) return;

    clearComponentConfigForms();
    componentFullInfo_.clear();
    componentTemplates_.clear();
    componentParamWidgets_.clear();

    // 按照组件ID收集配置
    QJsonObject entityModelAssembly = entity_->getProperty("modelAssembly").toJsonObject();
    QStringList componentIds;
    QMap<QString, QJsonObject> entityConfigMap;

    // 优先处理方案文件中深拷贝的组件信息
    if (entityModelAssembly.contains("components") && entityModelAssembly["components"].isArray()) {
        QJsonArray componentsArray = entityModelAssembly["components"].toArray();
        for (const auto& compValue : componentsArray) {
            QJsonObject compObj = compValue.toObject();
            QString componentId = compObj["componentId"].toString();
            if (componentId.isEmpty()) {
                continue;
            }
            if (!componentIds.contains(componentId)) {
                componentIds.append(componentId);
            }
            if (compObj.contains("configInfo") && compObj["configInfo"].isObject()) {
                entityConfigMap.insert(componentId, compObj["configInfo"].toObject());
            }
            componentFullInfo_.insert(componentId, compObj);
        }
    }

    // 如果方案中没有组件数组，则根据模型从数据库加载组件列表
    if (componentIds.isEmpty()) {
        QJsonArray componentIdList = dbModelAssembly_["componentList"].toArray();
        for (const auto& compId : componentIdList) {
            QString id = compId.toString();
            if (!id.isEmpty() && !componentIds.contains(id)) {
                componentIds.append(id);
            }
        }
        if (componentIds.isEmpty()) {
            if (!DatabaseUtils::openDatabase()) {
                qDebug() << "无法打开数据库";
                return;
            }
            QSqlQuery query;
            query.prepare("SELECT componentlist FROM ModelInformation WHERE id = ?");
            query.addBindValue(modelId_);
            if (query.exec() && query.next()) {
                const QStringList ids = query.value(0).toString().split(',', Qt::SkipEmptyParts);
                for (const QString& id : ids) {
                    if (!id.isEmpty() && !componentIds.contains(id)) {
                        componentIds.append(id);
                    }
                }
            }
        }
    }

    for (const QString& componentId : componentIds) {
        if (componentId.isEmpty()) {
            continue;
        }

        // 获取模板信息（数据库）
        QJsonObject templateInfo = getComponentTemplateFromDatabase(componentId);
        componentTemplates_[componentId] = templateInfo;

        // 获取配置默认值
        QJsonObject configInfo;
        if (entityConfigMap.contains(componentId)) {
            configInfo = entityConfigMap.value(componentId);
        }

        QJsonObject fullInfo;
        if (componentFullInfo_.contains(componentId)) {
            fullInfo = componentFullInfo_.value(componentId);
        } else {
            fullInfo = getComponentFullInfoFromDatabase(componentId);
        }

        if (configInfo.isEmpty()) {
            configInfo = fullInfo["configInfo"].toObject();
        }
        fullInfo["componentId"] = componentId;
        fullInfo["configInfo"] = configInfo;

        componentFullInfo_[componentId] = fullInfo;

        createComponentConfigForm(componentId, templateInfo, configInfo);
    }
}

void EntityPropertyDialog::clearComponentConfigForms()
{
    // 清除所有组件配置表单
    QLayoutItem* item;
    while ((item = componentConfigLayout_->takeAt(0)) != nullptr) {
        if (item->widget()) {
            item->widget()->setParent(nullptr);
            delete item->widget();
        }
        delete item;
    }
    componentConfigWidgets_.clear();
    componentParamWidgets_.clear();
}

void EntityPropertyDialog::createComponentConfigForm(const QString& componentId, const QJsonObject& templateInfo, const QJsonObject& configInfo)
{
    // 创建组件配置组
    QGroupBox* componentGroup = new QGroupBox(QString("组件: %1").arg(componentId));
    QFormLayout* formLayout = new QFormLayout(componentGroup);

    // 根据模板信息动态生成表单控件
    for (auto it = templateInfo.begin(); it != templateInfo.end(); ++it) {
        QString paramName = it.key();
        QJsonObject paramConfig = it.value().toObject();

        int type = paramConfig["type"].toInt();
        QStringList values;
        if (paramConfig.contains("value")) {
            values = paramConfig["value"].toString().split(',');
        }

        QVariant currentValue;
        if (configInfo.contains(paramName)) {
            QJsonValue jsonValue = configInfo[paramName];
            switch (type) {
            case 0: // QLineEdit
                if (jsonValue.isString()) {
                    currentValue = jsonValue.toString();
                } else if (jsonValue.isDouble()) {
                    currentValue = QString::number(jsonValue.toDouble());
                } else if (jsonValue.isBool()) {
                    currentValue = jsonValue.toBool() ? "true" : "false";
                }
                break;
            case 1: // QComboBox
                if (jsonValue.isDouble()) {
                    currentValue = jsonValue.toInt();
                } else if (jsonValue.isString()) {
                    QString strValue = jsonValue.toString();
                    int index = values.indexOf(strValue);
                    currentValue = (index >= 0) ? index : 0;
                }
                break;
            case 2: // QSpinBox
                if (jsonValue.isDouble()) {
                    currentValue = jsonValue.toInt();
                } else if (jsonValue.isString()) {
                    currentValue = jsonValue.toString().toInt();
                }
                break;
            case 3: // QCheckBox
                if (jsonValue.isBool()) {
                    currentValue = jsonValue.toBool();
                } else if (jsonValue.isDouble()) {
                    currentValue = jsonValue.toInt() != 0;
                } else if (jsonValue.isString()) {
                    QString strValue = jsonValue.toString().toLower();
                    currentValue = (strValue == "true" || strValue == "1" || strValue == "是");
                }
                break;
            }
        } else {
            // 默认值
            switch (type) {
            case 0: currentValue = ""; break;
            case 1: currentValue = 0; break;
            case 2: currentValue = 0; break;
            case 3: currentValue = false; break;
            }
        }

        QWidget* widget = createFormWidget(type, values, currentValue);
        if (widget) {
            formLayout->addRow(paramName + ":", widget);
            componentParamWidgets_[componentId][paramName] = widget;
        }
    }

    componentConfigLayout_->addWidget(componentGroup);
    componentConfigWidgets_[componentId] = componentGroup;
}

QWidget* EntityPropertyDialog::createFormWidget(int type, const QStringList& values, const QVariant& currentValue)
{
    switch (type) {
    case 0: { // QLineEdit
        QLineEdit* edit = new QLineEdit(this);
        edit->setText(currentValue.toString());
        return edit;
    }
    case 1: { // QComboBox
        QComboBox* combo = new QComboBox(this);
        combo->addItems(values);
        if (currentValue.isValid()) {
            int index = currentValue.toInt();
            if (index >= 0 && index < combo->count()) {
                combo->setCurrentIndex(index);
            }
        }
        return combo;
    }
    case 2: { // QSpinBox
        QSpinBox* spinBox = new QSpinBox(this);
        spinBox->setRange(0, 10000);
        spinBox->setValue(currentValue.toInt());
        return spinBox;
    }
    case 3: { // QCheckBox
        QCheckBox* checkBox = new QCheckBox(this);
        checkBox->setChecked(currentValue.toBool());
        return checkBox;
    }
    default:
        return nullptr;
    }
}

void EntityPropertyDialog::onBrowseIconButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(this, 
                                                    "选择二维军标图片", 
                                                    "", 
                                                    "图片文件 (*.png *.jpg *.jpeg *.bmp);;所有文件 (*.*)");
    if (!fileName.isEmpty() && iconEdit_) {
        iconEdit_->setText(fileName);
    }
}

void EntityPropertyDialog::onAddComponentClicked()
{
    // TODO: 实现添加组件功能（需要显示组件选择对话框）
    QMessageBox::information(this, "提示", "添加组件功能待实现");
}

void EntityPropertyDialog::onRemoveComponentClicked()
{
    QListWidgetItem* item = componentListWidget_->currentItem();
    if (item) {
        QString componentId = item->text();
        delete item;
        
        // 从组件配置表单中移除
        if (componentConfigWidgets_.contains(componentId)) {
            QWidget* widget = componentConfigWidgets_[componentId];
            componentConfigLayout_->removeWidget(widget);
            widget->setParent(nullptr);
            delete widget;
            componentConfigWidgets_.remove(componentId);
            componentParamWidgets_.remove(componentId);
            dbComponentConfigs_.remove(componentId);
            componentTemplates_.remove(componentId);
        }
    }
}

void EntityPropertyDialog::onSaveClicked()
{
    if (!entity_ || !planFileManager_) {
        QMessageBox::warning(this, "错误", "实体或方案管理器为空");
        return;
    }

    // 保存规划属性
    savePlanningProperties();

    // 保存模型组装属性
    saveModelAssemblyProperties();

    // 保存组件配置
    saveComponentConfigs();

    // 通知方案管理器更新实体（会标记为未保存，触发"当前方案: %1 *未保存"提示）
    planFileManager_->updateEntityInPlan(entity_);

    QMessageBox::information(this, "成功", "实体属性已应用");
    accept();
}

void EntityPropertyDialog::onCancelClicked()
{
    reject();
}

void EntityPropertyDialog::savePlanningProperties()
{
    // 保存基本信息
    QString displayName = nameEdit_->text().trimmed();
    if (!displayName.isEmpty()) {
        entity_->setProperty("displayName", displayName);
    }

    // 保存位置和航向
    double longitude = longitudeSpinBox_->value();
    double latitude = latitudeSpinBox_->value();
    double altitude = altitudeSpinBox_->value();
    entity_->setPosition(longitude, latitude, altitude);
    entity_->setHeading(headingSpinBox_->value());
    entity_->setVisible(visibleCheckBox_->isChecked());
}

void EntityPropertyDialog::saveModelAssemblyProperties()
{
    QJsonObject modelAssembly;
    
    // 获取当前UI的值
    QString location = locationComboBox_->currentText();
    QString icon = iconEdit_->text();

    // 保存完整的组件信息数组（深层复制）
    QJsonArray componentsArray;
    for (int i = 0; i < componentListWidget_->count(); ++i) {
        QString componentId = componentListWidget_->item(i)->text();
        if (componentFullInfo_.contains(componentId)) {
            // 复制完整的组件信息
            QJsonObject componentInfo = componentFullInfo_[componentId];
            componentsArray.append(componentInfo);
        }
    }
    
    // 始终保存完整的组件信息数组
    modelAssembly["components"] = componentsArray;
    
    // 保存location和icon（如果与数据库默认值不同）
    QString dbLocation = dbModelAssembly_["location"].toString();
    QString dbIcon = dbModelAssembly_["icon"].toString();

    if (location != dbLocation) {
        modelAssembly["location"] = location;
    }
    if (icon != dbIcon) {
        modelAssembly["icon"] = icon;
    }

    // 保存到实体属性（始终保存完整的组件信息）
    entity_->setProperty("modelAssembly", modelAssembly);
}

void EntityPropertyDialog::saveComponentConfigs()
{
    QJsonObject componentConfigs;

    // 遍历所有组件配置表单，保存完整的配置信息
    for (auto it = componentParamWidgets_.begin(); it != componentParamWidgets_.end(); ++it) {
        QString componentId = it.key();
        QMap<QString, QWidget*>& paramWidgets = it.value();

        // 收集当前表单的值
        QJsonObject currentConfig;
        for (auto paramIt = paramWidgets.begin(); paramIt != paramWidgets.end(); ++paramIt) {
            QString paramName = paramIt.key();
            QWidget* widget = paramIt.value();

            QVariant value;
            if (QLineEdit* edit = qobject_cast<QLineEdit*>(widget)) {
                value = edit->text();
            } else if (QComboBox* combo = qobject_cast<QComboBox*>(widget)) {
                value = combo->currentText();
            } else if (QSpinBox* spinBox = qobject_cast<QSpinBox*>(widget)) {
                value = spinBox->value();
            } else if (QCheckBox* checkBox = qobject_cast<QCheckBox*>(widget)) {
                value = checkBox->isChecked();
            }

            // 转换为JSON值
            QJsonValue jsonValue;
            if (value.type() == QVariant::String) {
                jsonValue = value.toString();
            } else if (value.type() == QVariant::Int || value.type() == QVariant::LongLong) {
                jsonValue = value.toInt();
            } else if (value.type() == QVariant::Double) {
                jsonValue = value.toDouble();
            } else if (value.type() == QVariant::Bool) {
                jsonValue = value.toBool();
            }

            currentConfig[paramName] = jsonValue;
        }

        // 更新组件完整信息中的configInfo
        if (componentFullInfo_.contains(componentId)) {
            componentFullInfo_[componentId]["configInfo"] = currentConfig;
        }
        
        // 保存完整的配置（不再比较差异）
        componentConfigs[componentId] = currentConfig;
    }

    // 保存到实体属性（始终保存完整的组件配置）
    entity_->setProperty("componentConfigs", componentConfigs);
}

QJsonObject EntityPropertyDialog::getModelAssemblyFromDatabase(const QString& modelId)
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT location, icon, componentlist FROM ModelInformation WHERE id = ?");
    query.addBindValue(modelId);

    if (query.exec() && query.next()) {
        result["location"] = query.value(0).toString();
        result["icon"] = query.value(1).toString();
        QString componentListStr = query.value(2).toString();
        QJsonArray componentList;
        QStringList ids = componentListStr.split(',', Qt::SkipEmptyParts);
        for (const QString& id : ids) {
            componentList.append(id.trimmed());
        }
        result["componentList"] = componentList;
    }

    return result;
}

QJsonObject EntityPropertyDialog::getComponentConfigFromDatabase(const QString& componentId)
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT configinfo FROM ComponentInformation WHERE componentid = ?");
    query.addBindValue(componentId);

    if (query.exec() && query.next()) {
        QString configStr = query.value(0).toString();
        if (!configStr.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(configStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                result = doc.object();
            }
        }
    }

    return result;
}

QJsonObject EntityPropertyDialog::getComponentFullInfoFromDatabase(const QString& componentId)
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT ci.componentid, ci.name, ci.type, ci.configinfo, "
                  "ct.wsf, ct.subtype, ct.template "
                  "FROM ComponentInformation ci "
                  "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
                  "WHERE ci.componentid = ?");
    query.addBindValue(componentId);

    if (query.exec() && query.next()) {
        result["componentId"] = query.value(0).toString();
        result["name"] = query.value(1).toString();
        result["type"] = query.value(2).toString();
        result["wsf"] = query.value(4).toString();
        result["subtype"] = query.value(5).toString();
        
        // 解析配置信息
        QString configStr = query.value(3).toString();
        if (!configStr.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument configDoc = QJsonDocument::fromJson(configStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                result["configInfo"] = configDoc.object();
            }
        }
        
        // 解析模板信息
        QString templateStr = query.value(6).toString();
        if (!templateStr.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument templateDoc = QJsonDocument::fromJson(templateStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                result["templateInfo"] = templateDoc.object();
            }
        }
    }

    return result;
}

QJsonObject EntityPropertyDialog::getComponentTemplateFromDatabase(const QString& componentId)
{
    QJsonObject result;

    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库";
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT ct.template FROM ComponentInformation ci "
                  "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
                  "WHERE ci.componentid = ?");
    query.addBindValue(componentId);

    if (query.exec() && query.next()) {
        QString templateStr = query.value(0).toString();
        if (!templateStr.isEmpty()) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(templateStr.toUtf8(), &parseError);
            if (parseError.error == QJsonParseError::NoError) {
                result = doc.object();
            }
        }
    }

    return result;
}

bool EntityPropertyDialog::jsonObjectsDiffer(const QJsonObject& obj1, const QJsonObject& obj2)
{
    return obj1 != obj2;
}

