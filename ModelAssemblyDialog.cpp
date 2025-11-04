#include "ModelAssemblyDialog.h"
#include "util/databaseutils.h"
#include <QListWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QUuid>
#include <QDebug>
#include <QMenu>
#include <QAction>
#include <QInputDialog>
#include <QFileDialog>
#include <QFileInfo>

ModelAssemblyDialog::ModelAssemblyDialog(QWidget *parent)
    : QDialog(parent)
    , modelTree(new QTreeWidget(this))
    , componentTree(new QTreeWidget(this))
    , assemblyList(new QListWidget(this))
    , modelNameEdit(new QLineEdit(this))
    , modelTypeEdit(new QLineEdit(this))
    , modelLocationComboBox(new QComboBox(this))
    , modelIconEdit(new QLineEdit(this))
    , browseIconButton(new QPushButton("浏览...", this))
{
    setupDatabase();
    setupUI();
    loadModelTree();
    loadComponentTree();

    setWindowTitle("模型装配");
    resize(1000, 700);
}

ModelAssemblyDialog::~ModelAssemblyDialog()
{
}

void ModelAssemblyDialog::setupDatabase()
{
    // 使用DatabaseUtils获取数据库连接
    db = DatabaseUtils::getDatabase();
    
    if (!DatabaseUtils::openDatabase()) {
        QMessageBox::critical(this, "错误", "无法打开数据库: " + db.lastError().text());
        return;
    }
    
    qDebug() << "ModelAssemblyDialog: 数据库连接成功";
}

void ModelAssemblyDialog::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // 左侧模型树
    modelTree->setHeaderLabel("模型结构");
    modelTree->setContextMenuPolicy(Qt::CustomContextMenu);
    mainLayout->addWidget(modelTree, 1);

    // 右侧配置区域
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);

    // 模型基本信息区域
    QGroupBox *modelInfoGroup = new QGroupBox("模型基本信息", this);
    QFormLayout *modelInfoLayout = new QFormLayout(modelInfoGroup);

    modelInfoLayout->addRow("模型名称:", modelNameEdit);
    modelInfoLayout->addRow("模型类型:", modelTypeEdit);
    modelInfoLayout->addRow("部署位置:", modelLocationComboBox);
    
    // 二维军标：使用 LineEdit + 浏览按钮
    QWidget *iconWidget = new QWidget(this);
    QHBoxLayout *iconLayout = new QHBoxLayout(iconWidget);
    iconLayout->setContentsMargins(0, 0, 0, 0);
    iconLayout->setSpacing(5);
    modelIconEdit->setReadOnly(true);
    modelIconEdit->setPlaceholderText("未选择图片文件");
    iconLayout->addWidget(modelIconEdit, 1);
    iconLayout->addWidget(browseIconButton);
    modelInfoLayout->addRow("二维军标:", iconWidget);

    modelTypeEdit->setReadOnly(true);
    modelLocationComboBox->addItems({"空中", "地面", "海面"});

    rightLayout->addWidget(modelInfoGroup);

    // 组件装配区域
    QGroupBox *assemblyGroup = new QGroupBox("组件装配", this);
    QHBoxLayout *assemblyLayout = new QHBoxLayout(assemblyGroup);

    // 装配列表（左侧）
    QWidget *assemblyLeftWidget = new QWidget(this);
    QVBoxLayout *assemblyLeftLayout = new QVBoxLayout(assemblyLeftWidget);

    QLabel *assemblyListLabel = new QLabel("装配组件列表", this);
    assemblyLeftLayout->addWidget(assemblyListLabel);
    assemblyLeftLayout->addWidget(assemblyList);

    // 组件树（右侧）
    QWidget *assemblyRightWidget = new QWidget(this);
    QVBoxLayout *assemblyRightLayout = new QVBoxLayout(assemblyRightWidget);

    QLabel *componentTreeLabel = new QLabel("组件列表", this);
    assemblyRightLayout->addWidget(componentTreeLabel);
    assemblyRightLayout->addWidget(componentTree);

    assemblyLayout->addWidget(assemblyLeftWidget, 1);
    assemblyLayout->addWidget(assemblyRightWidget, 1);

    rightLayout->addWidget(assemblyGroup, 1);

    // 保存按钮
    QPushButton *saveButton = new QPushButton("保存模型配置", this);
    rightLayout->addWidget(saveButton);

    mainLayout->addWidget(rightWidget, 2);

    // 连接信号槽
    connect(modelTree, &QTreeWidget::itemClicked,
            this, &ModelAssemblyDialog::onModelTreeItemClicked);
    connect(componentTree, &QTreeWidget::itemDoubleClicked,
            this, &ModelAssemblyDialog::onComponentTreeDoubleClicked);
    connect(assemblyList, &QListWidget::itemDoubleClicked,
            this, &ModelAssemblyDialog::onAssemblyListItemDoubleClicked);
    connect(modelTree, &QTreeWidget::customContextMenuRequested,
            this, &ModelAssemblyDialog::showContextMenu);
    connect(saveButton, &QPushButton::clicked,
            this, &ModelAssemblyDialog::onSaveButtonClicked);
    connect(browseIconButton, &QPushButton::clicked,
            this, &ModelAssemblyDialog::onBrowseIconButtonClicked);
//    connect(addModelButton, &QPushButton::clicked,
//            this, &ModelAssemblyDialog::onAddModelButtonClicked);
//    connect(deleteModelButton, &QPushButton::clicked,
//            this, &ModelAssemblyDialog::onDeleteModelButtonClicked);
}

void ModelAssemblyDialog::loadModelTree()
{
    modelTree->clear();
    loadModelTypes();
    loadModels();
}

void ModelAssemblyDialog::loadModelTypes()
{
    QSqlQuery query;
//    query.exec("SELECT DISTINCT type FROM ModelType");
    query.exec("SELECT id, type FROM ModelType ORDER BY type");
    while (query.next()) {
        QString modelTypeId = query.value(0).toString();
        QString type = query.value(1).toString();
        QTreeWidgetItem *typeItem = new QTreeWidgetItem(modelTree);
        typeItem->setText(0, type);
        typeItem->setData(0, Qt::UserRole, "type");
        typeItem->setData(0, Qt::UserRole + 1, modelTypeId);
    }
}

void ModelAssemblyDialog::loadModels()
{
    QSqlQuery query;
    query.exec("SELECT mi.id, mi.name, mt.type "
               "FROM ModelInformation mi "
               "JOIN ModelType mt ON mi.modeltypeid = mt.id");

    while (query.next()) {
        QString modelId = query.value(0).toString();
        QString modelName = query.value(1).toString();
        QString modelType = query.value(2).toString();
//        qInfo() << modelId << modelName << modelType << componentlist;

        // 找到对应的父节点
        for (int i = 0; i < modelTree->topLevelItemCount(); ++i) {
            QTreeWidgetItem *typeItem = modelTree->topLevelItem(i);
            if (typeItem->text(0) == modelType) {
                QTreeWidgetItem *modelItem = new QTreeWidgetItem(typeItem);
                modelItem->setText(0, modelName);
                modelItem->setData(0, Qt::UserRole, "model");
                modelItem->setData(0, Qt::UserRole + 1, modelId);
                break;
            }
        }
    }

    modelTree->expandAll();
}


void ModelAssemblyDialog::loadComponentTree()
{
    componentTree->clear();
    componentTree->setHeaderLabel("组件结构");

    // 加载组件类型
    QSqlQuery typeQuery;
    typeQuery.exec("SELECT DISTINCT subtype FROM ComponentType");

    QMap<QString, QTreeWidgetItem*> subtypeItems;

    while (typeQuery.next()) {
        QString subtype = typeQuery.value(0).toString();
        QTreeWidgetItem *subtypeItem = new QTreeWidgetItem(componentTree);
        subtypeItem->setText(0, subtype);
        subtypeItem->setData(0, Qt::UserRole, "subtype");
        subtypeItems[subtype] = subtypeItem;

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
            wsfItem->setData(0, Qt::UserRole + 1, subtype);
        }
    }

    // 加载具体组件
    QSqlQuery componentQuery;
    componentQuery.exec("SELECT ci.componentid, ci.name, ci.type, ct.wsf, ct.subtype "
                       "FROM ComponentInformation ci "
                       "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
                       "ORDER BY ci.name");

    while (componentQuery.next()) {
        QString componentId = componentQuery.value(0).toString();
        QString name = componentQuery.value(1).toString();
        QString type = componentQuery.value(2).toString();
        QString wsf = componentQuery.value(3).toString();
        QString subtype = componentQuery.value(4).toString();

        // 找到对应的节点
        if (subtypeItems.contains(subtype)) {
            QTreeWidgetItem *subtypeItem = subtypeItems[subtype];
            for (int i = 0; i < subtypeItem->childCount(); ++i) {
                QTreeWidgetItem *wsfItem = subtypeItem->child(i);
                if (wsfItem->text(0) == wsf) {
                    QTreeWidgetItem *componentItem = new QTreeWidgetItem(wsfItem);
                    componentItem->setText(0, name + " (" + type + ")");
                    componentItem->setData(0, Qt::UserRole, "component");
                    componentItem->setData(0, Qt::UserRole + 1, componentId);
                    break;
                }
            }
        }
    }

    componentTree->expandAll();
}


void ModelAssemblyDialog::onModelTreeItemClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    currentItem = item;
    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "model") {
        QString modelId = item->data(0, Qt::UserRole + 1).toString();

        QSqlQuery query;
        query.prepare("SELECT mi.id, mi.name, mi.location, mi.icon, mi.componentlist, mt.type "
                      "FROM ModelInformation mi "
                      "JOIN ModelType mt ON mi.modeltypeid = mt.id "
                      "WHERE mi.id = ?");
        query.addBindValue(modelId);

        if (query.exec() && query.next()) {
            ModelInfo info;
            info.id = query.value(0).toString();
            info.name = query.value(1).toString();
            info.type = query.value(5).toString();
            info.location = query.value(2).toString();
            info.icon = query.value(3).toString();
            QString componentListStr = query.value(4).toString();
            info.componentList = componentListStr.split(',');
//            qInfo() << "测试" << info.id << info.name << info.componentList;
            currentModelInfo = info;
            updateModelInfo(info);

            // 创建组件装配列表
            clearAssemblyList();
            loadAssemblyList(info.componentList);
        }
    }
}

// 更新模型的通用信息
void ModelAssemblyDialog::updateModelInfo(const ModelInfo &info)
{
    modelNameEdit->setText(info.name);
    modelTypeEdit->setText(info.type);
    modelLocationComboBox->setCurrentText(info.location);
    modelIconEdit->setText(info.icon);
}

void ModelAssemblyDialog::clearAssemblyList()
{
    assemblyList->clear();
}

void ModelAssemblyDialog::loadAssemblyList(const QStringList &componentIds)
{
    if (componentIds.isEmpty()) {
        return;
    }

    // 构建IN查询语句
    QString placeholders;
    for (int i = 0; i < componentIds.size(); ++i) {
        placeholders += "?";
        if (i < componentIds.size() - 1) {
            placeholders += ",";
        }
    }

    QSqlQuery query;
    query.prepare(QString("SELECT componentid, name, type FROM ComponentInformation "
                         "WHERE componentid IN (%1)").arg(placeholders));

    for (int i = 0; i < componentIds.size(); ++i) {
        query.addBindValue(componentIds[i]);
    }

    if (query.exec()) {
        while (query.next()) {
            QString componentId = query.value(0).toString();
            QString name = query.value(1).toString();
            QString type = query.value(2).toString();

            QListWidgetItem *item = new QListWidgetItem(name + " (" + type + ")");
            item->setData(Qt::UserRole, componentId);
            assemblyList->addItem(item);
        }
    }
}

void ModelAssemblyDialog::onComponentTreeDoubleClicked(QTreeWidgetItem *item, int column)
{
    Q_UNUSED(column);

    if (currentModelInfo.id.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要装配的模型");
        return;
    }

    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType != "component") {
        return;
    }

    QString componentId = item->data(0, Qt::UserRole + 1).toString();

    // 检查是否已经装配
    for (int i = 0; i < assemblyList->count(); ++i) {
        QListWidgetItem *assemblyItem = assemblyList->item(i);
        if (assemblyItem->data(Qt::UserRole).toString() == componentId) {
            QMessageBox::information(this, "提示", "该组件已经装配到当前模型中");
            return;
        }
    }

    // 获取组件信息
    QSqlQuery query;
    query.prepare("SELECT name, type FROM ComponentInformation WHERE componentid = ?");
    query.addBindValue(componentId);

    if (query.exec() && query.next()) {
        QString name = query.value(0).toString();
        QString type = query.value(1).toString();

        QListWidgetItem *newItem = new QListWidgetItem(name + " (" + type + ")");
        newItem->setData(Qt::UserRole, componentId);
        assemblyList->addItem(newItem);

        // 更新当前模型的组件列表
        if (!currentModelInfo.componentList.contains(componentId)) {
            currentModelInfo.componentList.append(componentId);
        }
    }
}

// 将所选组件从模型配置中移除
void ModelAssemblyDialog::onAssemblyListItemDoubleClicked(QListWidgetItem *item)
{
    if (currentModelInfo.id.isEmpty()) {
        return;
    }

    QString componentId = item->data(Qt::UserRole).toString();

    int ret = QMessageBox::question(this, "确认", "确定要从模型中移除该组件吗？",
                                   QMessageBox::Yes | QMessageBox::No);

    if (ret == QMessageBox::Yes) {
        // 从装配列表中移除
        delete assemblyList->takeItem(assemblyList->row(item));

        // 更新当前模型的组件列表
        currentModelInfo.componentList.removeAll(componentId);
    }
}

void ModelAssemblyDialog::onSaveButtonClicked()
{
    if (currentModelInfo.id.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选择要保存的模型");
        return;
    }

    // 更新模型基本信息
    currentModelInfo.name = modelNameEdit->text();
    currentModelInfo.type = modelTypeEdit->text();
    currentModelInfo.location = modelLocationComboBox->currentText();
    currentModelInfo.icon = modelIconEdit->text();

    // 构建组件列表字符串
//    QString componentListStr = currentModelInfo.componentList.join(',');
    // 修复：过滤掉空字符串元素，否则会出现插入",2,3"的情况
    QString componentListStr;
    if (!currentModelInfo.componentList.isEmpty()) {
        // 过滤掉空字符串
        QStringList validComponents;
        for (const QString &componentId : currentModelInfo.componentList) {
            if (!componentId.trimmed().isEmpty()) {
                validComponents << componentId.trimmed();
            }
        }
        componentListStr = validComponents.join(',');
    } else {
        componentListStr = ""; // 明确设置为空字符串
    }

    QSqlQuery query;
    query.prepare("UPDATE ModelInformation SET name = ?, location = ?, "
                  "icon = ?, componentlist = ? WHERE id = ?");
    query.addBindValue(currentModelInfo.name);
    query.addBindValue(currentModelInfo.location);
    query.addBindValue(currentModelInfo.icon);
    query.addBindValue(componentListStr);
    query.addBindValue(currentModelInfo.id);

    if (query.exec()) {
        QMessageBox::information(this, "成功", "模型配置已保存");
        // 更新模型列表显示
        loadModelTree();
    } else {
        QMessageBox::critical(this, "错误", "保存失败: " + query.lastError().text());
    }
}

// 模型右键菜单
void ModelAssemblyDialog::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = modelTree->itemAt(pos);
    if (!item) return;
    currentItem = item;
    QMenu contextMenu(this);
    QString itemType = item->data(0, Qt::UserRole).toString();
    if (itemType == "type") {
        QAction *addAction = contextMenu.addAction("添加组件");
        QString modelTypeId = item->data(0, Qt::UserRole + 1).toString();
        connect(addAction, &QAction::triggered, this, &ModelAssemblyDialog::onAddModelAction);
    } else if (itemType == "model") {
        QAction *deleteAction = contextMenu.addAction("删除组件");
        QString modelId = item->data(0, Qt::UserRole + 1).toString();
        connect(deleteAction, &QAction::triggered, this, &ModelAssemblyDialog::ondeleteModelAction);
    }

    contextMenu.exec(modelTree->viewport()->mapToGlobal(pos));
}

void ModelAssemblyDialog::onAddModelAction()
{
    bool ok;
    QString modelName = QInputDialog::getText(this,
                                            "添加模型",
                                            "请输入模型名称:",
                                            QLineEdit::Normal,
                                            "",
                                            &ok);

    if (ok && !modelName.trimmed().isEmpty()) {
        // 添加到数据库和树形控件
        addModel(modelName.trimmed());
    } else if (ok && modelName.trimmed().isEmpty()) {
        QMessageBox::warning(this, "警告", "模型名称不能为空！");
    }
}

void ModelAssemblyDialog::addModel(QString modelName)
{
    if (!currentItem) {
        return;
    }
    QString modelTypeId = currentItem->data(0, Qt::UserRole + 1).toString();
    QSqlQuery insertQuery;
    insertQuery.prepare("INSERT INTO ModelInformation (name, modeltypeid, location, icon, componentlist) "
                  "VALUES (?, ?, '', '', '')");
    insertQuery.addBindValue(modelName);
    insertQuery.addBindValue(modelTypeId);

    if (insertQuery.exec()) {
        // 查询新模型的id
        QSqlQuery query;
        query.prepare("SELECT mi.id, mt.type, mi.location, mi.icon, mi.componentlist "
                      "FROM ModelInformation mi "
                      "JOIN ModelType mt ON mi.modeltypeid = mt.id "
                      "WHERE mi.name = ?");
        query.addBindValue(modelName);
        if(query.exec() && query.next()) {
            QString modelId = query.value(0).toString();
            QString modelType = query.value(2).toString();
            QString location = query.value(3).toString();
            QString icon = query.value(4).toString();
            QString componentListStr = query.value(5).toString();
            QStringList componentList = componentListStr.split(',');

            // 在树状结构中添加新模型
            QTreeWidgetItem *modelItem = new QTreeWidgetItem(currentItem);
            modelItem->setText(0, modelName);
            modelItem->setData(0, Qt::UserRole, "model");
            modelItem->setData(0, Qt::UserRole + 1, modelId);

            // 更新当前模型信息
            currentModelInfo.id = modelId;
            currentModelInfo.name = modelName;
            currentModelInfo.type = modelType;
            currentModelInfo.location = location;
            currentModelInfo.icon = icon;  // icon 字段存储的是绝对路径
            currentModelInfo.componentList = componentList;

            updateModelInfo(currentModelInfo);
            // 创建组件装配列表
            clearAssemblyList();
            loadAssemblyList(componentList);

            QMessageBox::information(this, "成功", "新模型已创建");

        } else {
            QMessageBox::critical(this, "错误", "创建模型失败: " + query.lastError().text());
        }

    } else {
        QMessageBox::critical(this, "错误", "创建模型失败: " + insertQuery.lastError().text());
    }
}

void ModelAssemblyDialog::ondeleteModelAction()
{
    if (!currentItem) {
        QMessageBox::warning(this, "警告", "请先选择要删除的模型");
        return;
    }

    QString modelId = currentItem->data(0, Qt::UserRole + 1).toString();
    QString modelName = currentItem->text(0);

    int ret = QMessageBox::question(this, "确认删除",
                                   QString("确定要删除模型 \"%1\" 吗？此操作不可恢复。").arg(modelName),
                                   QMessageBox::Yes | QMessageBox::No);

    if (ret != QMessageBox::Yes) {
        return;
    }

    // 开始事务，确保数据一致性
    DatabaseUtils::beginTransaction();

    try {
        // 删除数据库中的模型信息
        QSqlQuery deleteQuery;
        deleteQuery.prepare("DELETE FROM ModelInformation WHERE id = ?");
        deleteQuery.addBindValue(modelId);
        if (!deleteQuery.exec()) {
            throw QString("删除模型失败: ") + deleteQuery.lastError().text();
        }

        // 检查是否真的删除了记录
        if (deleteQuery.numRowsAffected() == 0) {
            throw QString("未找到要删除的模型信息");
        }

        // 提交事务
        DatabaseUtils::commitTransaction();

        // 从树形结构中移除
        // 获取模型父节点
        QTreeWidgetItem *parentItem = currentItem->parent();
        if (parentItem) {
            int index = parentItem->indexOfChild(currentItem);
            if (index >= 0) {
                delete currentItem;
                currentItem = nullptr;
                currentModelInfo = ModelInfo(); // 清空当前模型信息
                // 清空当前显示
                modelNameEdit->clear();
                modelTypeEdit->clear();
                modelLocationComboBox->setCurrentIndex(0);
                modelIconEdit->clear();
                clearAssemblyList();

                QMessageBox::information(this, "成功", "模型删除成功");
            }
        }

    }
    catch (const QString &error) {
        // 回滚事务
        DatabaseUtils::rollbackTransaction();
        QMessageBox::critical(this, "错误", error);
    }

}


ModelInfo ModelAssemblyDialog::getCurrentModelInfo() const
{
    return currentModelInfo;
}

void ModelAssemblyDialog::onBrowseIconButtonClicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "选择二维军标图片",
        "",
        "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)"
    );
    
    if (!fileName.isEmpty()) {
        // 使用 QFileInfo 规范化路径，确保使用绝对路径
        QFileInfo fileInfo(fileName);
        QString absolutePath = fileInfo.absoluteFilePath();
        
        // 验证文件是否存在
        if (fileInfo.exists() && fileInfo.isFile()) {
            modelIconEdit->setText(absolutePath);
        } else {
            QMessageBox::warning(this, "错误", "选择的文件不存在或无效");
        }
    }
}
