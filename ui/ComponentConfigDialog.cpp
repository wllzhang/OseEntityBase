/**
 * @file ComponentConfigDialog.cpp
 * @brief 组件配置对话框实现文件
 * 
 * 实现ComponentConfigDialog类的所有功能
 */

#include "ComponentConfigDialog.h"
#include "../util/databaseutils.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QMenu>
#include <QApplication>
#include <QUuid>
#include <QDebug>
#include <QFile>
#include <QGroupBox>
#include <QScrollArea>

ComponentConfigDialog::ComponentConfigDialog(QWidget *parent)
    : QDialog(parent)
    , parameterWidget(new QWidget(this))
    , parameterFormLayout(new QFormLayout(parameterWidget))
{
    setupDatabase();
    setupUI();
    loadComponentTree();

    setWindowTitle("组件参数配置");
    resize(1600, 1000);
}

ComponentConfigDialog::~ComponentConfigDialog()
{
    if (db.isOpen()) {
        db.close();
    }
}

void ComponentConfigDialog::setupDatabase()
{
    // 使用DatabaseUtils获取数据库连接
    db = DatabaseUtils::getDatabase();
    
    if (!DatabaseUtils::openDatabase()) {
        QMessageBox::critical(this, "错误", "无法打开数据库: " + db.lastError().text());
        return;
    }
    
    qDebug() << "ComponentConfigDialog: 数据库连接成功";
}

void ComponentConfigDialog::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true); // 让内部widget可调整大小

    // 左侧组件树
    componentTree = new QTreeWidget(this);
    componentTree->setHeaderLabel("组件结构");
    componentTree->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(componentTree, 1);

    // 提升渲染性能，避免高度波动
    componentTree->setUniformRowHeights(true);
    componentSearchEdit = new QLineEdit(this);
    componentSearchEdit->setPlaceholderText("搜索组件名称");
    QWidget *componentTreePanel = new QWidget(this);
    QVBoxLayout *componentTreeLayout = new QVBoxLayout(componentTreePanel);
    componentTreeLayout->setContentsMargins(0, 0, 0, 0);
    componentTreeLayout->setSpacing(6);
    componentTreeLayout->addWidget(componentSearchEdit);
    componentTreeLayout->addWidget(componentTree, 1);
    mainLayout->addWidget(componentTreePanel, 1);

    // 右侧配置区域
    rightLayout = new QVBoxLayout();
    QWidget *rightWidget = new QWidget(this);
    rightWidget->setLayout(rightLayout);
    mainLayout->addWidget(rightWidget, 2);

    // 通用信息区域
    QGroupBox *generalGroup = new QGroupBox("通用信息", this);
    QFormLayout *generalLayout = new QFormLayout(generalGroup);
    nameEdit = new QLineEdit(this);
    typeComboBox = new QComboBox(this);
    wsfEdit = new QLineEdit(this);
    commentEdit = new QLineEdit(this);
    generalLayout->addRow("组件名称:", nameEdit);
    generalLayout->addRow("组件类型:", typeComboBox);
    generalLayout->addRow("WSF:", wsfEdit);
    generalLayout->addRow("注释:", commentEdit);
    wsfEdit->setReadOnly(true);

    rightLayout->addWidget(generalGroup);

    // 参数配置区域
    QGroupBox *paramGroup = new QGroupBox("参数配置", this);
    QVBoxLayout *paramLayout = new QVBoxLayout(paramGroup);
    paramLayout->addWidget(parameterWidget);

//    rightLayout->addWidget(paramGroup, 1);

    scrollArea->setWidget(paramGroup);
    // 设置滚动条策略，例如垂直滚动条一直显示，水平滚动条自动显示
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    rightLayout->addWidget(scrollArea, 1);

    // 保存按钮
    QPushButton *saveButton = new QPushButton("保存", this);
    rightLayout->addWidget(saveButton);

    // 连接信号槽
    connect(componentSearchEdit, &QLineEdit::textChanged, this, &ComponentConfigDialog::onComponentSearchTextChanged);
    connect(componentTree, &QTreeWidget::itemClicked, this, &ComponentConfigDialog::onTreeItemClicked);
    connect(componentTree, &QTreeWidget::customContextMenuRequested, this, &ComponentConfigDialog::showContextMenu);
    connect(saveButton, &QPushButton::clicked, this, &ComponentConfigDialog::onSaveButtonClicked);
}

void ComponentConfigDialog::loadComponentTree()
{
    componentTree->clear();
    loadComponentTypes();
    loadComponents();
}

void ComponentConfigDialog::loadComponentTypes()
{
    QSqlQuery query;
    query.exec("SELECT DISTINCT subtype FROM ComponentType");

    while (query.next()) {
        QString subtype = query.value(0).toString();
        QTreeWidgetItem *subtypeItem = new QTreeWidgetItem(componentTree);
        subtypeItem->setText(0, subtype);
        subtypeItem->setData(0, Qt::UserRole, "subtype");

        // 加载该子类型下的WSF
        QSqlQuery wsfQuery;
        wsfQuery.prepare("SELECT DISTINCT wsf FROM ComponentType WHERE subtype = ?");
        wsfQuery.addBindValue(subtype);
        wsfQuery.exec();

        while (wsfQuery.next()) {
            QString wsf = wsfQuery.value(0).toString();
            QTreeWidgetItem *wsfItem = new QTreeWidgetItem(subtypeItem);
            wsfItem->setText(0, wsf);
            wsfItem->setData(0, Qt::UserRole, "wsf");
        }
    }
}

void ComponentConfigDialog::loadComponents()
{
    QSqlQuery query;
    query.exec("SELECT ci.componentid, ci.name, ci.type, ct.wsf, ct.subtype "
               "FROM ComponentInformation ci "
               "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid");

    while (query.next()) {
        QString componentId = query.value(0).toString();
        QString name = query.value(1).toString();
        QString type = query.value(2).toString();
        QString wsf = query.value(3).toString();
        QString subtype = query.value(4).toString();

        // 找到对应的WSF节点
        for (int i = 0; i < componentTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *subtypeItem = componentTree->topLevelItem(i);
            if (subtypeItem->text(0) == subtype) {
                for (int j = 0; j < subtypeItem->childCount(); ++j) {
                    QTreeWidgetItem *wsfItem = subtypeItem->child(j);
                    if (wsfItem->text(0) == wsf) {
                        QTreeWidgetItem *componentItem = new QTreeWidgetItem(wsfItem);
                        componentItem->setText(0, name);
                        componentItem->setData(0, Qt::UserRole, "component");
                        componentItem->setData(0, Qt::UserRole + 1, componentId);
                        break;
                    }
                }
                break;
            }
        }
    }

    componentTree->expandAll();
}

void ComponentConfigDialog::onTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    currentItem = item;
    QString itemType = item->data(0, Qt::UserRole).toString();

    if (itemType == "component") {
        QString componentId = item->data(0, Qt::UserRole + 1).toString();

        QSqlQuery query;
        query.prepare("SELECT ci.componentid, ci.name, ci.type, ci.configinfo, "
                      "ct.wsf, ct.subtype, ct.template "
                      "FROM ComponentInformation ci "
                      "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
                      "WHERE ci.componentid = ?");
        query.addBindValue(componentId);

        if (query.exec() && query.next()) {
            ComponentInfo info;
            info.componentId = query.value(0).toString();
            info.name = query.value(1).toString();
            info.type = query.value(2).toString();
            info.wsf = query.value(4).toString();
            info.subtype = query.value(5).toString();

            // 解析配置信息
            QJsonDocument configDoc = QJsonDocument::fromJson(query.value(3).toString().toUtf8());
            info.configInfo = configDoc.object();

            // 解析模板信息
            QJsonDocument templateDoc = QJsonDocument::fromJson(query.value(6).toString().toUtf8());
            info.templateInfo = templateDoc.object();

            currentComponentInfo = info;
            updateComponentInfo(info);

            // 创建参数表单
            clearParameterForm();
            createParameterForm(info.templateInfo, info.configInfo);
        }
    }
}


void ComponentConfigDialog::onComponentSearchTextChanged(const QString &text)
{
    const QString keyword = text.trimmed();  // 去除首尾空格以保证匹配准确

    componentTree->setUpdatesEnabled(false);    // 批量更新前先关闭刷新，避免闪烁

    if (keyword.isEmpty()) {
        resetComponentTreeFilter();
        componentTree->setUpdatesEnabled(true);
        return;
    }

    for (int i = 0; i < componentTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = componentTree->topLevelItem(i);
        bool matched = filterComponentTreeItem(item, keyword);
        item->setHidden(!matched);
    }

    componentTree->setUpdatesEnabled(true);
}

void ComponentConfigDialog::resetComponentTreeFilter()
{
    // 遍历组件树的所有顶级项
    for (int i = 0; i < componentTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = componentTree->topLevelItem(i);
        // 递归设置该项及其所有子项为显示状态
        setTreeItemHiddenRecursive(item, false);
        item->setExpanded(true);  // 将当前顶级项设置为展开状态
    }
}

bool ComponentConfigDialog::filterComponentTreeItem(QTreeWidgetItem *item, const QString &keyword)
{
    bool matched = item->text(0).contains(keyword, Qt::CaseInsensitive);    // 当前节点文本匹配

    for (int i = 0; i < item->childCount(); ++i) {
        QTreeWidgetItem *child = item->child(i);
        // 递归判断子节点是否匹配关键字
        bool childMatched = filterComponentTreeItem(child, keyword);
        matched = matched || childMatched;
    }

    item->setHidden(!matched);
    if (matched && item->childCount() > 0) {
        item->setExpanded(true);    // 设置节点展开
    } else if (!matched) {
        item->setExpanded(false);   // 设置节点隐藏
    }

    return matched;
}

void ComponentConfigDialog::setTreeItemHiddenRecursive(QTreeWidgetItem *item, bool hidden)
{
    item->setHidden(hidden);
    if (!hidden && item->childCount() > 0) {
        item->setExpanded(true);  // 恢复可见节点默认展开
    } else if (hidden) {
        item->setExpanded(false); // 隐藏节点时无需展开
    }
    for (int i = 0; i < item->childCount(); ++i) {
        setTreeItemHiddenRecursive(item->child(i), hidden);
    }
}


void ComponentConfigDialog::updateComponentInfo(const ComponentInfo &info)
{
    nameEdit->setText(info.name);

    typeComboBox->clear();
    typeComboBox->addItem(info.type);
    typeComboBox->setCurrentText(info.type);

    wsfEdit->setText(info.wsf);
    commentEdit->setText(info.subtype);
}

void ComponentConfigDialog::clearParameterForm()
{
    // 清除旧的参数控件
    QLayoutItem *child;
    while ((child = parameterFormLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->setParent(nullptr);
            delete child->widget();
        }
        delete child;
    }
    paramWidgets.clear();
}

void ComponentConfigDialog::createParameterForm(const QJsonObject &templateInfo, const QJsonObject &configInfo)
{
    for (auto it = templateInfo.begin(); it != templateInfo.end(); ++it) {
        QString paramName = it.key();
        QJsonObject paramConfig = it.value().toObject();

        int type = paramConfig["type"].toInt();
        QStringList values;
        if (paramConfig.contains("value")) {
            values = paramConfig["value"].toString().split(',');
        }

        QVariant currentValue = getParameterValue(paramName, type, values, paramConfig, configInfo);
        QWidget *widget = createFormWidget(paramName, type, values, currentValue, paramConfig);

        if (widget) {
            parameterFormLayout->addRow(paramName + ":", widget);
            paramWidgets[paramName] = widget;

            // 为嵌套JSON类型存储模板信息
            if (type == 6 && paramConfig.contains("value")) {
                nestedTemplates[paramName] = paramConfig["value"].toObject();
            }
        }
    }

    // 添加弹性空间
    parameterFormLayout->addRow(new QWidget(), new QWidget());
}


QVariant ComponentConfigDialog::getParameterValue(const QString &paramName, int type,
                                                 const QStringList &values,
                                                 const QJsonObject &paramConfig,
                                                 const QJsonObject &configInfo)
{
    if (!configInfo.contains(paramName)) {
        return getDefaultValue(type, values, paramConfig);
    }

    QJsonValue jsonValue = configInfo[paramName];

    switch (type) {
    case 0: // QLineEdit
        return convertToString(jsonValue);

    case 1: // QComboBox
        return convertToComboBoxIndex(jsonValue, values);

    case 2: // QSpinBox
        return convertToInt(jsonValue);

    case 3: // QCheckBox
        return convertToBool(jsonValue);

    case 4: // QDoubleSpinBox
        return convertToDouble(jsonValue);

    case 5: // 范围输入框
        return convertToRangeString(jsonValue);

    case 6: // 嵌套Json
        return convertToNestedObject(jsonValue, paramConfig);

    case 7: // 组件类型下拉框
        return convertToComponentComboBox(jsonValue);

    default:
        qWarning() << "Unknown parameter type:" << type << "for parameter:" << paramName;
        return QVariant();
    }
}

QVariant ComponentConfigDialog::getDefaultValue(int type, const QStringList &values, const QJsonObject &paramConfig)
{
    switch (type) {
    case 0: return "";                    // QLineEdit
    case 1: return 0;                     // QComboBox
    case 2: return 0;                     // QSpinBox
    case 3: return false;                 // QCheckBox
    case 4: return 0.0;                   // QDoubleSpinBox
    case 5: return "0,0";                 // 范围输入框
    case 6: {                             // 嵌套Json
        QVariantMap nestedDefault;
        if (paramConfig.contains("value")) {
            QJsonObject nestedTemplate = paramConfig["value"].toObject();
            for (auto it = nestedTemplate.begin(); it != nestedTemplate.end(); ++it) {
                QJsonObject nestedConfig = it.value().toObject();
                int nestedType = nestedConfig["type"].toInt();
                QStringList nestedValues;
                if (nestedConfig.contains("value")) {
                    nestedValues = nestedConfig["value"].toString().split(',');
                }
                nestedDefault[it.key()] = getDefaultValue(nestedType, nestedValues, nestedConfig);
            }
        }
        return nestedDefault;
    }
    default: return QVariant();
    }
}

// 值转换辅助函数
QString ComponentConfigDialog::convertToString(const QJsonValue &jsonValue)
{
//    qInfo() << jsonValue;
    if (jsonValue.isString()) return jsonValue.toString();
    if (jsonValue.isDouble()) return QString::number(jsonValue.toDouble());
    if (jsonValue.isBool()) return jsonValue.toBool() ? "true" : "false";
    return "";
}

int ComponentConfigDialog::convertToComboBoxIndex(const QJsonValue &jsonValue, const QStringList &values)
{
    if (jsonValue.isDouble()) {
        int index = jsonValue.toInt();
        return (index >= 0 && index < values.size()) ? index : 0;
    }
    if (jsonValue.isString()) {
        QString strValue = jsonValue.toString();
        int index = values.indexOf(strValue);
        return (index >= 0) ? index : 0;
    }
    return 0;
}

int ComponentConfigDialog::convertToInt(const QJsonValue &jsonValue)
{
//    qInfo() << jsonValue;
    if (jsonValue.isDouble()) return jsonValue.toInt();
    if (jsonValue.isString()) return jsonValue.toString().toInt();
    return 0;
}

bool ComponentConfigDialog::convertToBool(const QJsonValue &jsonValue)
{
    if (jsonValue.isBool()) return jsonValue.toBool();
    if (jsonValue.isDouble()) return jsonValue.toInt() != 0;
    if (jsonValue.isString()) {
        QString strValue = jsonValue.toString().toLower();
        return (strValue == "true" || strValue == "1" || strValue == "是" || strValue == "yes");
    }
    return false;
}

double ComponentConfigDialog::convertToDouble(const QJsonValue &jsonValue)
{
//    qInfo() << jsonValue;

    if (jsonValue.isDouble()) return jsonValue.toDouble();
    if (jsonValue.isString()) return jsonValue.toString().toDouble();
    return 0.0;
}

QString ComponentConfigDialog::convertToRangeString(const QJsonValue &jsonValue)
{
//    qInfo() << jsonValue;

    if (jsonValue.isString()) return jsonValue.toString();

    // 如果是数组，转换为逗号分隔的字符串
    if (jsonValue.isArray()) {
        QJsonArray array = jsonValue.toArray();
        if (array.size() >= 2) {
            return QString("%1,%2").arg(array[0].toVariant().toString()).arg(array[1].toVariant().toString());
        }
    }
    return "0,0";
}

QVariant ComponentConfigDialog::convertToNestedObject(const QJsonValue &jsonValue, const QJsonObject &paramConfig)
{
    if (jsonValue.isObject()) return jsonValue.toObject().toVariantMap();
    if (jsonValue.isString()) {
        // 尝试解析JSON字符串
        QJsonDocument doc = QJsonDocument::fromJson(jsonValue.toString().toUtf8());
        if (doc.isObject()) return doc.object().toVariantMap();
    }

    // 返回默认嵌套对象
    QVariantMap nestedDefault;
    if (paramConfig.contains("value")) {
        QJsonObject nestedTemplate = paramConfig["value"].toObject();
        for (auto it = nestedTemplate.begin(); it != nestedTemplate.end(); ++it) {
            QJsonObject nestedConfig = it.value().toObject();
            int nestedType = nestedConfig["type"].toInt();
            QStringList nestedValues;
            if (nestedConfig.contains("value")) {
                nestedValues = nestedConfig["value"].toString().split(',');
            }
            nestedDefault[it.key()] = getDefaultValue(nestedType, nestedValues, nestedConfig);
        }
    }
    return nestedDefault;
}

QString ComponentConfigDialog::convertToComponentComboBox(const QJsonValue &jsonValue)
{
    QString name = "";
    if (!jsonValue.isNull())
    {
        QSqlQuery query;
        query.prepare("SELECT ci.name, ci.configinfo "
                      "FROM ComponentInformation ci "
                      "WHERE ci.componentid = ? ");
        query.addBindValue(jsonValue.toInt());

        if (query.exec() && query.next()) {
            name = query.value(0).toString();
        } else {
            qWarning() << "找不到id为：" << jsonValue.toInt() << "的组件";
        }
    }
    else {
        qWarning() << "找不到id为：" << jsonValue.toInt() << "的组件";
    }
    return name;
}

QWidget* ComponentConfigDialog::createFormWidget(QString paramName, int type, const QStringList &values,
                                                const QVariant &currentValue,
                                                const QJsonObject &paramConfig)
{
//    qInfo() << currentValue;
    switch (type) {
    case 0: return createLineEditWidget(currentValue);
    case 1: return createComboBoxWidget(values, currentValue);
    case 2: return createSpinBoxWidget(currentValue);
    case 3: return createCheckBoxWidget(currentValue);
    case 4: return createDoubleSpinBoxWidget(currentValue);
    case 5: return createRangeWidget(currentValue);
    case 6: return createNestedJsonWidget(paramName, currentValue, paramConfig);
    case 7: return createComponentComboBoxWidget(values, currentValue);
    default:
        qWarning() << "Unsupported widget type:" << type;
        return createUnsupportedWidget(type);
    }
}


// values：组件的afsimtype, currentValue：下拉框中显示的当前所选组件名称
QWidget* ComponentConfigDialog::createComponentComboBoxWidget(const QStringList &values, const QVariant &currentValue)
{
    QComboBox *combo = new QComboBox(this);

    // 检查输入是否为空
    if (values.isEmpty() || currentValue.toString() == "") {
        return combo;
    }

    QString sql = "SELECT ci.name "
        "FROM ComponentInformation ci "
        "INNER JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
        "WHERE ct.afsimtype IN (";

    // 生成占位符
    QStringList placeholders;
    for (int i = 0; i < values.size(); ++i) {
        placeholders << "?";
    }
    sql += placeholders.join(",") + ")";

    QSqlQuery query;
    query.prepare(sql);

    for (const QString& value : values) {
        query.addBindValue(value);
    }

    QStringList nameList;
    if (query.exec()) {
        while (query.next()) {
            QString name = query.value(0).toString();
            if (!name.isEmpty()) {
                nameList.append(name);
            }
        }
    } else {
        qWarning() << "数据库查询失败:" << query.lastError().text();
    }

    combo->addItems(nameList);
    combo->setCurrentText(currentValue.toString());
    return combo;
}

QWidget* ComponentConfigDialog::createLineEditWidget(const QVariant &currentValue)
{
    QLineEdit *edit = new QLineEdit(this);
    edit->setText(currentValue.toString());
    return edit;
}

QWidget* ComponentConfigDialog::createComboBoxWidget(const QStringList &values, const QVariant &currentValue)
{
    QComboBox *combo = new QComboBox(this);
    combo->addItems(values);

    int index = currentValue.toInt();
    if (index >= 0 && index < combo->count()) {
        combo->setCurrentIndex(index);
    }
    return combo;
}

QWidget* ComponentConfigDialog::createSpinBoxWidget(const QVariant &currentValue)
{
    QSpinBox *spinBox = new QSpinBox(this);
    spinBox->setRange(std::numeric_limits<int>::min(), std::numeric_limits<int>::max());
    spinBox->setValue(currentValue.toInt());
    return spinBox;
}

QWidget* ComponentConfigDialog::createCheckBoxWidget(const QVariant &currentValue)
{
    QCheckBox *checkBox = new QCheckBox(this);
    checkBox->setChecked(currentValue.toBool());
    return checkBox;
}

QWidget* ComponentConfigDialog::createDoubleSpinBoxWidget(const QVariant &currentValue)
{
    QDoubleSpinBox *spinBox = new QDoubleSpinBox(this);
    spinBox->setRange(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());
    spinBox->setDecimals(6);
    spinBox->setValue(currentValue.toDouble());
    return spinBox;
}

QWidget* ComponentConfigDialog::createRangeWidget(const QVariant &currentValue)
{
    QWidget *rangeWidget = new QWidget(this);
    rangeWidget->setProperty("isRangeWidget", true);
    QHBoxLayout* rangeLayout = new QHBoxLayout(rangeWidget);
    rangeLayout->setContentsMargins(0, 0, 0, 0);

    QLineEdit *minEdit = new QLineEdit();
    QLineEdit *maxEdit = new QLineEdit();

    // 设置验证器，只允许数字输入
    QDoubleValidator *validator = new QDoubleValidator(this);
    minEdit->setValidator(validator);
    maxEdit->setValidator(validator);

    QString rangeStr = currentValue.toString();
    QStringList rangeValues = rangeStr.split(",");

    if (rangeValues.size() >= 2) {
        minEdit->setText(rangeValues[0].trimmed());
        maxEdit->setText(rangeValues[1].trimmed());
    } else {
        minEdit->setText("0");
        maxEdit->setText("0");
    }

    rangeLayout->addWidget(minEdit);
    rangeLayout->addWidget(new QLabel(" ~ "));
    rangeLayout->addWidget(maxEdit);
    rangeLayout->setStretchFactor(minEdit, 1);
    rangeLayout->setStretchFactor(maxEdit, 1);

    return rangeWidget;
}

QWidget* ComponentConfigDialog::createNestedJsonWidget(QString parentName, const QVariant &currentValue, const QJsonObject &paramConfig)
{
//    qInfo() << paramConfig;
    QGroupBox *groupBox = new QGroupBox(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(groupBox);

    if (paramConfig.contains("value")) {
        QJsonObject nestedTemplate = paramConfig["value"].toObject();
        QVariantMap nestedConfig = currentValue.toMap();

        QFormLayout *formLayout = new QFormLayout();

        for (auto it = nestedTemplate.begin(); it != nestedTemplate.end(); ++it) {
            QString nestedParamName = it.key();
            QJsonObject nestedParamConfig = it.value().toObject();
            int nestedType = nestedParamConfig["type"].toInt();
            QStringList nestedValues;
            if (nestedParamConfig.contains("value")) {
                nestedValues = nestedParamConfig["value"].toString().split(',');
            }

            QVariant nestedCurrentValue = nestedConfig.value(nestedParamName);
            QWidget *nestedWidget = createFormWidget(parentName, nestedType, nestedValues, nestedCurrentValue, nestedParamConfig);

            if (nestedWidget) {
                formLayout->addRow(nestedParamName + ":", nestedWidget);
                // 存储嵌套控件的引用，使用组合键
                QString fullParamName = QString("%1.%2").arg(parentName).arg(nestedParamName);
                paramWidgets[fullParamName] = nestedWidget;
            }
        }

        mainLayout->addLayout(formLayout);
    }

    return groupBox;
}

QWidget* ComponentConfigDialog::createUnsupportedWidget(int type)
{
    QLabel *label = new QLabel(this);
    label->setText(QString("不支持的控件类型: %1").arg(type));
    label->setStyleSheet("color: red; font-style: italic;");
    return label;
}


void ComponentConfigDialog::onSaveButtonClicked()
{
    if (currentComponentInfo.componentId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要配置的组件");
        return;
    }

    // 收集参数值
    QJsonObject configInfo = collectFormData();

    // 验证必要字段
    if (!validateFormData(configInfo)) {
        QMessageBox::warning(this, "警告", "请填写所有必填字段");
        return;
    }

    // 更新数据库
    QSqlQuery query;
    query.prepare("UPDATE ComponentInformation SET name = ?, type = ?, configinfo = ? WHERE componentid = ?");
    query.addBindValue(currentComponentInfo.name);
    query.addBindValue(currentComponentInfo.type);
    query.addBindValue(QJsonDocument(configInfo).toJson(QJsonDocument::Compact));
    query.addBindValue(currentComponentInfo.componentId);
    if (query.exec()) {
        QMessageBox::information(this, "成功", "组件配置已保存");
        // 更新树显示
        if (currentItem) {
//            currentItem->setText(0, nameEdit->text() + " (" + typeComboBox->currentText() + ")");
            currentItem->setText(0, nameEdit->text());
        }
    } else {
        QMessageBox::critical(this, "错误", "保存失败: " + query.lastError().text());
    }
}

QJsonObject ComponentConfigDialog::collectFormData()
{
    QJsonObject configInfo;

    // 首先处理普通字段（不包含点号的字段名）
    for (auto it = paramWidgets.begin(); it != paramWidgets.end(); ++it) {
        QString paramName = it.key();

        // 跳过嵌套JSON字段（包含点号的字段在后续处理）
        if (paramName.contains('.')) {
            continue;
        }

        QWidget *widget = it.value();
        QVariant value = getWidgetValue(widget, paramName);

        // 如果是嵌套JSON类型，初始化一个空的JSON对象
        if (nestedTemplates.contains(paramName)) {
            configInfo[paramName] = QJsonObject();
        } else {
            configInfo[paramName] = QJsonValue::fromVariant(value);
        }
    }

    // 然后处理嵌套字段
    for (auto it = paramWidgets.begin(); it != paramWidgets.end(); ++it) {
        QString paramName = it.key();

        // 只处理嵌套JSON字段（包含点号的字段名）
        if (!paramName.contains('.')) {
            continue;
        }

        QStringList parts = paramName.split('.');
        if (parts.size() != 2) {
            qWarning() << "Invalid nested parameter name:" << paramName;
            continue;
        }

        QString parentName = parts[0];
        QString childName = parts[1];
//        qInfo() << "parentName:" << parentName << "childName:" << childName;
//        qInfo() << nestedTemplates;
        // 检查父字段是否是已知的嵌套JSON字段
        if (nestedTemplates.contains(parentName)) {
//            qInfo() << "parentName:" << parentName;
            QWidget *widget = it.value();
            QVariant value = getWidgetValue(widget, paramName);

            // 获取或创建嵌套的JSON对象
            QJsonObject nestedObject = configInfo[parentName].toObject();
            nestedObject[childName] = QJsonValue::fromVariant(value);
            configInfo[parentName] = nestedObject;
        }
    }

    return configInfo;
}

QVariant ComponentConfigDialog::getWidgetValue(QWidget *widget, const QString &paramName)
{
    if (QLineEdit *edit = qobject_cast<QLineEdit*>(widget)) {
        return edit->text();
    } else if (QComboBox *combo = qobject_cast<QComboBox*>(widget)) {
        return combo->currentIndex();
    } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget)) {
        return spinBox->value();
    } else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(widget)) {
        return checkBox->isChecked();
    } else if (QDoubleSpinBox *doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        return doubleSpinBox->value();
    } else if (widget->property("isRangeWidget").toBool()) {
        // 范围输入框 - 由两个QLineEdit组成
        QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(widget->layout());
        if (layout && layout->count() >= 3) {
            QLineEdit *minEdit = qobject_cast<QLineEdit*>(layout->itemAt(0)->widget());
            QLineEdit *maxEdit = qobject_cast<QLineEdit*>(layout->itemAt(2)->widget());
            if (minEdit && maxEdit) {
                return QString("%1,%2").arg(minEdit->text()).arg(maxEdit->text());
            }
        }
        return "0,0";
    } else if (QGroupBox *groupBox = qobject_cast<QGroupBox*>(widget)) {
        // 嵌套JSON字段 - 已经在collectFormData中通过子控件处理
        // 这里返回一个空的QVariantMap，实际数据已通过子控件收集
        return QVariantMap();
    } else {
        qWarning() << "Unknown widget type for parameter:" << paramName;
        return QVariant();
    }
}

bool ComponentConfigDialog::validateFormData(const QJsonObject &configInfo)
{
    // 检查必要字段
    for (auto it = paramWidgets.begin(); it != paramWidgets.end(); ++it) {
        QString paramName = it.key();

        // 跳过嵌套字段的子字段（只检查顶级字段）
        if (paramName.contains('.')) {
            continue;
        }

        QWidget *widget = it.value();

        // 检查QLineEdit是否为空
        if (QLineEdit *edit = qobject_cast<QLineEdit*>(widget)) {
            if (edit->text().trimmed().isEmpty()) {
                QMessageBox::warning(this, "验证错误",
                                   QString("参数\"%1\"不能为空").arg(paramName));
                return false;
            }
        }

        // 检查范围输入框
        if (widget->property("isRangeWidget").toBool()) {
            QHBoxLayout *layout = qobject_cast<QHBoxLayout*>(widget->layout());
            if (layout && layout->count() >= 3) {
                QLineEdit *minEdit = qobject_cast<QLineEdit*>(layout->itemAt(0)->widget());
                QLineEdit *maxEdit = qobject_cast<QLineEdit*>(layout->itemAt(2)->widget());
                if (minEdit && maxEdit) {
                    bool minOk, maxOk;
                    double minVal = minEdit->text().toDouble(&minOk);
                    double maxVal = maxEdit->text().toDouble(&maxOk);

                    if (!minOk || !maxOk) {
                        QMessageBox::warning(this, "验证错误",
                                           QString("参数\"%1\"必须包含有效的数字").arg(paramName));
                        return false;
                    }

                    if (minVal > maxVal) {
                        QMessageBox::warning(this, "验证错误",
                                           QString("参数\"%1\"的最小值不能大于最大值").arg(paramName));
                        return false;
                    }
                }
            }
        }
    }

    return true;
}


// 组件右键菜单
void ComponentConfigDialog::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = componentTree->itemAt(pos);
    if (!item) return;

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType != "component") return;

    QMenu contextMenu(this);
    QAction *copyAction = contextMenu.addAction("复制组件");
    QAction *deleteAction = contextMenu.addAction("删除组件");

    connect(copyAction, &QAction::triggered, this, &ComponentConfigDialog::copyComponent);
    connect(deleteAction, &QAction::triggered, this, &ComponentConfigDialog::deleteComponent);

    contextMenu.exec(componentTree->viewport()->mapToGlobal(pos));
}

void ComponentConfigDialog::copyComponent()
{
    if (!currentItem || currentComponentInfo.componentId.isEmpty()) {
        return;
    }

    // 生成新的组件名称
    QString baseName = currentComponentInfo.name;
    QString newName = baseName + "_copy";
    QString newComponentTypeId = "";
    QString newType = "";
    QString newConfigInfo = "";
    // 检查名称是否已存在，如果存在则继续添加数字后缀
    int counter = 1;
    QSqlQuery checkQuery;
    checkQuery.prepare("SELECT COUNT(*) FROM ComponentInformation WHERE name = ?");
    while (true) {
        checkQuery.addBindValue(newName);
        if (checkQuery.exec() && checkQuery.next() && checkQuery.value(0).toInt() == 0) {
            break;
        }
        newName = baseName + "_copy" + QString::number(counter++);
    }

    // 获取组件父节点（WSF节点）
    QTreeWidgetItem *parentItem = currentItem->parent();
    if (!parentItem) return;

    // 提取组件信息
    QSqlQuery tempQuery;
    tempQuery.prepare("SELECT ci.componenttypeid, ci.type, ci.configinfo FROM ComponentInformation ci WHERE ci.componentid = ?");
    tempQuery.addBindValue(currentComponentInfo.componentId);
    if(tempQuery.exec() && tempQuery.next()) {
        newComponentTypeId = tempQuery.value(0).toString();
        newType = tempQuery.value(1).toString();
        newConfigInfo = tempQuery.value(2).toString();
//        qInfo() << "测试：" << currentComponentInfo.componentId << tempQuery.value(0) << tempQuery.value(1) << tempQuery.value(2);
    }

    // 插入新组件到数据库
    QSqlQuery insertQuery;
    insertQuery.prepare("INSERT INTO ComponentInformation (name, componenttypeid, type, configinfo) "
                      "VALUES (?, ?, ?, ?)");
    insertQuery.addBindValue(newName);
    insertQuery.addBindValue(newComponentTypeId);
    insertQuery.addBindValue(newType);
    insertQuery.addBindValue(newConfigInfo);

    if (insertQuery.exec()) {
        QSqlQuery query;
        query.prepare("SELECT componentid "
                      "FROM ComponentInformation WHERE name = ?");
        query.addBindValue(newName);

        if (query.exec() && query.next()) {
            // 在树中添加新组件
            QTreeWidgetItem *newComponentItem = new QTreeWidgetItem(parentItem);
            newComponentItem->setText(0, newName + " (" + currentComponentInfo.type + ")");
            newComponentItem->setData(0, Qt::UserRole, "component");
            QString newComponentId = query.value(0).toString();
            newComponentItem->setData(0, Qt::UserRole + 1, newComponentId);
        }
        QMessageBox::information(this, "成功", "组件复制成功");
    } else {
        QMessageBox::critical(this, "错误", "复制失败: " + insertQuery.lastError().text());
    }
}

void ComponentConfigDialog::deleteComponent()
{
    if (!currentItem || currentComponentInfo.componentId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要删除的组件");
        return;
    }

    // 确认删除对话框
   QMessageBox::StandardButton reply;
   reply = QMessageBox::question(this, "确认删除",
                               "确定要删除组件 '" + currentComponentInfo.name + "' 吗？\n此操作不可恢复！",
                               QMessageBox::Yes | QMessageBox::No);

   if (reply != QMessageBox::Yes) {
       return;
   }

   // 检查组件是否被其他模型引用
   if (isComponentUsed(currentComponentInfo.componentId)) {
       QMessageBox::warning(this, "警告",
                          "无法删除组件 '" + currentComponentInfo.name + "'\n"
                          "该组件正在被其他模型使用，请先解除关联关系！");
       return;
   }

   // 开始事务，确保数据一致性
   QSqlDatabase::database().transaction();

   try {
       // 删除数据库中的组件信息
       QSqlQuery deleteQuery;
       deleteQuery.prepare("DELETE FROM ComponentInformation WHERE componentid = ?");
       deleteQuery.addBindValue(currentComponentInfo.componentId);

       if (!deleteQuery.exec()) {
           throw QString("删除组件失败: ") + deleteQuery.lastError().text();
       }

       // 检查是否真的删除了记录
       if (deleteQuery.numRowsAffected() == 0) {
           throw QString("未找到要删除的组件记录");
       }

       // 提交事务
       QSqlDatabase::database().commit();

       // 从树形结构中移除
       // 获取组件父节点（WSF节点）
       QTreeWidgetItem *parentItem = currentItem->parent();
       if (parentItem) {
           int index = parentItem->indexOfChild(currentItem);
           if (index >= 0) {
               delete currentItem;
               currentItem = nullptr;
               currentComponentInfo = ComponentInfo(); // 清空当前组件信息
           }
       }

       QMessageBox::information(this, "成功", "组件删除成功");
   }
   catch (const QString &error) {
       // 回滚事务
       QSqlDatabase::database().rollback();
       QMessageBox::critical(this, "错误", error);
   }

}

bool ComponentConfigDialog::isComponentUsed(QString componentId)
{
    // 待补充
    return false;
}

QString ComponentConfigDialog::generateComponentId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

ComponentInfo ComponentConfigDialog::getCurrentComponentInfo() const
{
    return currentComponentInfo;
}
