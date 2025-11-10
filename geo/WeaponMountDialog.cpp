#include "WeaponMountDialog.h"
#include "../geo/geoentity.h"
#include "../util/databaseutils.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QHeaderView>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

WeaponMountDialog::WeaponMountDialog(GeoEntity* entity, QWidget *parent)
    : QDialog(parent)
    , entity_(entity)
    , weaponTree_(new QTreeWidget(this))
    , quantitySpinBox_(new QSpinBox(this))
    , weaponNameLabel_(new QLabel("未选择武器", this))
    , saveButton_(new QPushButton("保存", this))
    , cancelButton_(new QPushButton("取消", this))
{
    if (!entity_) {
        QMessageBox::critical(this, "错误", "实体指针为空");
        return;
    }

    setupUI();
    loadWeaponTree();
    loadSavedMountInfo();

    setWindowTitle("武器挂载配置 - " + entity_->getName());
    resize(800, 600);
}

WeaponMountDialog::~WeaponMountDialog()
{
}

void WeaponMountDialog::setupUI()
{
    QHBoxLayout *mainLayout = new QHBoxLayout(this);

    // 左侧：武器树
    QWidget *leftWidget = new QWidget(this);
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    leftLayout->setContentsMargins(0, 0, 0, 0);

    QLabel *treeLabel = new QLabel("武器列表", this);
    leftLayout->addWidget(treeLabel);

    weaponTree_->setHeaderLabel("武器结构");
    weaponTree_->setRootIsDecorated(true);
    weaponTree_->setAlternatingRowColors(true);
    weaponTree_->setSelectionMode(QAbstractItemView::SingleSelection);
    leftLayout->addWidget(weaponTree_);

    mainLayout->addWidget(leftWidget, 1);

    // 右侧：数量输入区域
    QWidget *rightWidget = new QWidget(this);
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);

    QGroupBox *infoGroup = new QGroupBox("武器挂载信息", this);
    QFormLayout *infoLayout = new QFormLayout(infoGroup);

    weaponNameLabel_->setWordWrap(true);
    infoLayout->addRow("选中武器:", weaponNameLabel_);

    quantitySpinBox_->setMinimum(0);
    quantitySpinBox_->setMaximum(9999);
    quantitySpinBox_->setValue(0);
    quantitySpinBox_->setEnabled(false);
    infoLayout->addRow("挂载数量:", quantitySpinBox_);

    rightLayout->addWidget(infoGroup);
    rightLayout->addStretch();

    // 按钮区域
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(saveButton_);
    buttonLayout->addWidget(cancelButton_);
    rightLayout->addLayout(buttonLayout);

    mainLayout->addWidget(rightWidget, 1);

    // 连接信号和槽
    connect(weaponTree_, &QTreeWidget::itemSelectionChanged, this, [this]() {
        QList<QTreeWidgetItem*> selectedItems = weaponTree_->selectedItems();
        if (!selectedItems.isEmpty()) {
            QTreeWidgetItem* item = selectedItems.first();
            QString itemType = item->data(0, Qt::UserRole).toString();
            if (itemType == "weapon") {
                onWeaponTreeSelectionChanged(item, nullptr);
            } else {
                // 如果选中的是类型节点，取消选择
                weaponTree_->clearSelection();
                weaponNameLabel_->setText("未选择武器");
                quantitySpinBox_->setEnabled(false);
                quantitySpinBox_->setValue(0);
            }
        }
    });

    // 连接数量输入框的值变化信号，实时更新weaponQuantityMap_
   connect(quantitySpinBox_, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
       QList<QTreeWidgetItem*> selectedItems = weaponTree_->selectedItems();
       if (!selectedItems.isEmpty()) {
           QTreeWidgetItem* item = selectedItems.first();
           QString itemType = item->data(0, Qt::UserRole).toString();
           if (itemType == "weapon") {
               QString weaponId = item->data(0, Qt::UserRole + 1).toString();
               weaponQuantityMap_[weaponId] = value;
           }
       }
   });


    connect(saveButton_, &QPushButton::clicked, this, &WeaponMountDialog::onSaveButtonClicked);
    connect(cancelButton_, &QPushButton::clicked, this, &WeaponMountDialog::onCancelButtonClicked);
}

void WeaponMountDialog::loadWeaponTree()
{
    weaponTree_->clear();
    weaponQuantityMap_.clear();
    weaponNameMap_.clear();

    if (!DatabaseUtils::openDatabase()) {
        QMessageBox::critical(this, "错误", "无法打开数据库");
        return;
    }

    QSqlQuery query;
    // 查询类型为"空空导弹"或"空面导弹"的模型
    query.prepare("SELECT mi.id, mi.name, mt.type "
                  "FROM ModelInformation mi "
                  "JOIN ModelType mt ON mi.modeltypeid = mt.id "
                  "WHERE mt.type IN ('空空导弹', '空面导弹') "
                  "ORDER BY mt.type, mi.name");

    if (!query.exec()) {
        QMessageBox::critical(this, "错误", "查询武器数据失败: " + query.lastError().text());
        return;
    }

    // 用于存储类型节点
    QMap<QString, QTreeWidgetItem*> typeItems;

    while (query.next()) {
        QString weaponId = query.value(0).toString();
        QString weaponName = query.value(1).toString();
        QString weaponType = query.value(2).toString();

        // 存储武器信息
        weaponNameMap_[weaponId] = weaponName;
        weaponQuantityMap_[weaponId] = 0; // 初始数量为0

        // 获取或创建类型节点
        QTreeWidgetItem* typeItem = nullptr;
        if (typeItems.contains(weaponType)) {
            typeItem = typeItems[weaponType];
        } else {
            typeItem = new QTreeWidgetItem(weaponTree_);
            typeItem->setText(0, weaponType);
            typeItem->setData(0, Qt::UserRole, "type");
            typeItems[weaponType] = typeItem;
        }

        // 创建武器节点
        QTreeWidgetItem* weaponItem = new QTreeWidgetItem(typeItem);
        weaponItem->setText(0, weaponName);
        weaponItem->setData(0, Qt::UserRole, "weapon");
        weaponItem->setData(0, Qt::UserRole + 1, weaponId);
    }

    weaponTree_->expandAll();
    weaponTree_->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

    qDebug() << "加载了" << weaponNameMap_.size() << "个武器";
}

void WeaponMountDialog::loadSavedMountInfo()
{
    if (!entity_) {
        return;
    }

    // 从实体属性中读取武器挂载信息
    QVariant mountInfoVariant = entity_->getProperty("weaponMounts");
    if (!mountInfoVariant.isValid()) {
        return;
    }

    QJsonObject mountInfoObj = mountInfoVariant.toJsonObject();
    if (mountInfoObj.isEmpty()) {
        return;
    }

    // 解析JSON对象
    QJsonArray weaponsArray = mountInfoObj["weapons"].toArray();
    for (const auto& weaponValue : weaponsArray) {
        QJsonObject weaponObj = weaponValue.toObject();
        QString weaponId = weaponObj["weaponId"].toString();
        int quantity = weaponObj["quantity"].toInt();

        if (weaponQuantityMap_.contains(weaponId)) {
            weaponQuantityMap_[weaponId] = quantity;
        }
    }

    qDebug() << "加载了" << weaponsArray.size() << "个已保存的武器挂载信息";
}

void WeaponMountDialog::onWeaponTreeSelectionChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    Q_UNUSED(previous);

    if (!current) {
        weaponNameLabel_->setText("未选择武器");
        quantitySpinBox_->setEnabled(false);
        quantitySpinBox_->setValue(0);
        return;
    }

    QString itemType = current->data(0, Qt::UserRole).toString();
    if (itemType != "weapon") {
        weaponNameLabel_->setText("未选择武器");
        quantitySpinBox_->setEnabled(false);
        quantitySpinBox_->setValue(0);
        return;
    }

    QString weaponId = current->data(0, Qt::UserRole + 1).toString();
    QString weaponName = current->text(0);

    weaponNameLabel_->setText(weaponName);
    quantitySpinBox_->setEnabled(true);

    // 设置当前数量（从已保存的信息中读取，如果没有则为0）
    int quantity = weaponQuantityMap_.value(weaponId, 0);
    quantitySpinBox_->setValue(quantity);
}


QList<WeaponMountInfo> WeaponMountDialog::getAllMountInfo() const
{
    QList<WeaponMountInfo> mountInfoList;

    // 遍历所有武器，收集数量大于0的武器
    for (auto it = weaponQuantityMap_.constBegin(); it != weaponQuantityMap_.constEnd(); ++it) {
        if (it.value() > 0) {
            WeaponMountInfo info;
            info.weaponId = it.key();
            info.weaponName = weaponNameMap_.value(it.key(), "");
            info.quantity = it.value();
            mountInfoList.append(info);
        }
    }

    return mountInfoList;
}

void WeaponMountDialog::onSaveButtonClicked()
{
    if (!entity_) {
            QMessageBox::warning(this, "警告", "实体指针为空");
            return;
    }

    // 获取所有挂载信息
    QList<WeaponMountInfo> mountInfoList = getAllMountInfo();

    // 构建JSON对象
    QJsonObject mountInfoObj;
    QJsonArray weaponsArray;

    for (const auto& info : mountInfoList) {
        QJsonObject weaponObj;
        weaponObj["weaponId"] = info.weaponId;
        weaponObj["weaponName"] = info.weaponName;
        weaponObj["quantity"] = info.quantity;

        QJsonObject weaponDetails = getWeaponFullInfo(info.weaponId);
        if (!weaponDetails.isEmpty()) {
            QString weaponType = weaponDetails.value("type").toString();
            if (!weaponType.isEmpty()) {
                weaponObj["weaponType"] = weaponType;
            }
            weaponObj["weaponDetails"] = weaponDetails;
        }
        weaponsArray.append(weaponObj);
    }

    mountInfoObj["weapons"] = weaponsArray;

    // 保存到实体属性
    entity_->setProperty("weaponMounts", mountInfoObj);

    QMessageBox::information(this, "成功", QString("已保存 %1 种武器的挂载信息").arg(mountInfoList.size()));

    accept();

}

void WeaponMountDialog::onCancelButtonClicked()
{
    reject();
}

QJsonObject WeaponMountDialog::getWeaponFullInfo(const QString& weaponId) const
{
    QJsonObject result;

    if (weaponId.isEmpty()) {
        return result;
    }

    if (!DatabaseUtils::openDatabase()) {
        qWarning() << "无法打开数据库，无法获取武器信息:" << weaponId;
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT mi.id, mi.name, mt.type, mi.location, mi.icon, mi.componentlist "
                  "FROM ModelInformation mi "
                  "JOIN ModelType mt ON mi.modeltypeid = mt.id "
                  "WHERE mi.id = ?");
    query.addBindValue(weaponId);

    if (!query.exec()) {
        qWarning() << "查询武器信息失败:" << weaponId << query.lastError().text();
        return result;
    }

    if (!query.next()) {
        qWarning() << "未找到武器信息:" << weaponId;
        return result;
    }

    QString modelId = query.value(0).toString();
    QString modelName = query.value(1).toString();
    QString type = query.value(2).toString();
    QString location = query.value(3).toString();
    QString icon = query.value(4).toString();
    QString componentListStr = query.value(5).toString();

    result["modelId"] = modelId;
    result["modelName"] = modelName;
    result["type"] = type;
    result["location"] = location;
    result["icon"] = icon;

    QStringList componentIds = parseComponentList(componentListStr);
    QJsonArray componentListArray;
    for (const QString& compId : componentIds) {
        componentListArray.append(compId);
    }
    result["componentList"] = componentListArray;

    QJsonArray componentsArray;
    for (const QString& compId : componentIds) {
        QJsonObject compInfo = getComponentFullInfoFromDatabase(compId);
        if (!compInfo.isEmpty()) {
            componentsArray.append(compInfo);
        }
    }
    result["components"] = componentsArray;

    return result;
}

QJsonObject WeaponMountDialog::getComponentFullInfoFromDatabase(const QString& componentId) const
{
    QJsonObject result;

    if (componentId.isEmpty()) {
        return result;
    }

    if (!DatabaseUtils::openDatabase()) {
        qWarning() << "无法打开数据库，无法获取组件信息:" << componentId;
        return result;
    }

    QSqlQuery query;
    query.prepare("SELECT ci.componentid, ci.name, ci.type, ci.configinfo, "
                  "ct.wsf, ct.subtype, ct.template "
                  "FROM ComponentInformation ci "
                  "JOIN ComponentType ct ON ci.componenttypeid = ct.ctypeid "
                  "WHERE ci.componentid = ?");
    query.addBindValue(componentId);

    if (!query.exec()) {
        qWarning() << "查询组件信息失败:" << componentId << query.lastError().text();
        return result;
    }

    if (!query.next()) {
        qWarning() << "未找到组件信息:" << componentId;
        return result;
    }

    result["componentId"] = query.value(0).toString();
    result["name"] = query.value(1).toString();
    result["type"] = query.value(2).toString();
    result["wsf"] = query.value(4).toString();
    result["subtype"] = query.value(5).toString();

    QString configStr = query.value(3).toString();
    if (!configStr.isEmpty()) {
        QJsonParseError parseError;
        QJsonDocument configDoc = QJsonDocument::fromJson(configStr.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            result["configInfo"] = configDoc.object();
        } else {
            qWarning() << "解析组件配置失败:" << componentId << parseError.errorString();
        }
    }

    return result;
}

QStringList WeaponMountDialog::parseComponentList(const QString& componentListStr) const
{
    QStringList componentIds;

    if (componentListStr.isEmpty()) {
        return componentIds;
    }

    const QStringList rawIds = componentListStr.split(',', Qt::SkipEmptyParts);
    for (const QString& rawId : rawIds) {
        QString trimmed = rawId.trimmed();
        if (!trimmed.isEmpty()) {
            componentIds.append(trimmed);
        }
    }

    return componentIds;
}
