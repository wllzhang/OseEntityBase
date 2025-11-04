#include "ComponentConfigDialog.h"
#include "util/databaseutils.h"
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

ComponentConfigDialog::ComponentConfigDialog(QWidget *parent)
    : QDialog(parent)
    , parameterWidget(new QWidget(this))
    , parameterFormLayout(new QFormLayout(parameterWidget))
{
    setupDatabase();
    setupUI();
    loadComponentTree();

    setWindowTitle("组件参数配置");
    resize(1000, 700);
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

    // 左侧组件树
    componentTree = new QTreeWidget(this);
    componentTree->setHeaderLabel("组件结构");
    componentTree->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(componentTree, 1);

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

    rightLayout->addWidget(paramGroup, 1);

    // 保存按钮
    QPushButton *saveButton = new QPushButton("保存", this);
    rightLayout->addWidget(saveButton);

    // 连接信号槽
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

        QVariant currentValue;
        if (configInfo.contains(paramName)) {
            QJsonValue jsonValue = configInfo[paramName];
            qInfo() << jsonValue;
            // 根据控件类型正确处理值
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
                    currentValue = jsonValue.toInt(); // 索引值
                } else if (jsonValue.isString()) {
                    // 如果是字符串，尝试在values中查找对应的索引
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
       }else{
            // 如果没有配置值，设置默认值
            switch (type) {
            case 0: currentValue = ""; break;
            case 1: currentValue = 0; break;
            case 2: currentValue = 0; break;
            case 3: currentValue = false; break;
            }
        }

        QWidget *widget = createFormWidget(type, values, currentValue);
        if (widget) {
            parameterFormLayout->addRow(paramName + ":", widget);
            paramWidgets[paramName] = widget;
        }
    }
}

QWidget* ComponentConfigDialog::createFormWidget(int type, const QStringList &values, const QVariant &currentValue)
{
    switch (type) {
    case 0: { // QLineEdit
        QLineEdit *edit = new QLineEdit(this);
        edit->setText(currentValue.toString());
        return edit;
    }
    case 1: { // QComboBox
        QComboBox *combo = new QComboBox(this);
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
        QSpinBox *spinBox = new QSpinBox(this);
        spinBox->setRange(0, 10000);
        spinBox->setValue(currentValue.toInt());
        return spinBox;
    }
    case 3: { // QCheckBox
        QCheckBox *checkBox = new QCheckBox(this);
        checkBox->setChecked(currentValue.toBool());
        return checkBox;
    }
    default:
        return nullptr;
    }
}


void ComponentConfigDialog::onSaveButtonClicked()
{
    if (currentComponentInfo.componentId.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要配置的组件");
        return;
    }

    // 收集参数值
    QJsonObject configInfo;
    for (auto it = paramWidgets.begin(); it != paramWidgets.end(); ++it) {
        QString paramName = it.key();
        QWidget *widget = it.value();

        if (QLineEdit *edit = qobject_cast<QLineEdit*>(widget)) {
            configInfo[paramName] = edit->text();
        } else if (QComboBox *combo = qobject_cast<QComboBox*>(widget)) {
            configInfo[paramName] = combo->currentIndex();
        } else if (QSpinBox *spinBox = qobject_cast<QSpinBox*>(widget)) {
            configInfo[paramName] = spinBox->value();
        } else if (QCheckBox *checkBox = qobject_cast<QCheckBox*>(widget)) {
            configInfo[paramName] = checkBox->isChecked();
        }
    }

    // 更新数据库
    QSqlQuery query;
    query.prepare("UPDATE ComponentInformation SET name = ?, type = ?, configinfo = ? WHERE componentid = ?");
    query.addBindValue(nameEdit->text());
    query.addBindValue(typeComboBox->currentText());
    query.addBindValue(QJsonDocument(configInfo).toJson(QJsonDocument::Compact));
    query.addBindValue(currentComponentInfo.componentId);

    if (query.exec()) {
        QMessageBox::information(this, "成功", "组件配置已保存");
        // 更新树显示
        if (currentItem) {
            currentItem->setText(0, nameEdit->text() + " (" + typeComboBox->currentText() + ")");
        }
    } else {
        QMessageBox::critical(this, "错误", "保存失败: " + query.lastError().text());
    }
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
   DatabaseUtils::beginTransaction();

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
       DatabaseUtils::commitTransaction();

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
       DatabaseUtils::rollbackTransaction();
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
