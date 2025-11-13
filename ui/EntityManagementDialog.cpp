#include "EntityManagementDialog.h"

#include "../geo/geoentity.h"

#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QEvent>
#include <QSpinBox>
#include <QJsonObject>
#include <QJsonArray>

namespace {
constexpr int ColumnName = 0;
constexpr int ColumnType = 1;
constexpr int ColumnUid = 2;
const int RoleUid = Qt::UserRole;
const int RoleIsEntity = Qt::UserRole + 1;
}

EntityManagementDialog::EntityManagementDialog(QWidget* parent)
    : QDialog(parent)
    , tree_(new QTreeWidget(this))
    , focusButton_(new QPushButton("聚焦", this))
    , editButton_(new QPushButton("编辑属性", this))
    , deleteButton_(new QPushButton("删除", this))
    , refreshButton_(new QPushButton("刷新", this))
    , updating_(false)
{
    setWindowTitle("实体管理");
    resize(580, 420);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    tree_->setColumnCount(3);
    tree_->setHeaderLabels({"名称", "类型", "UID"});
    tree_->header()->setStretchLastSection(true);
    tree_->setSelectionMode(QAbstractItemView::SingleSelection);
    tree_->setSelectionBehavior(QAbstractItemView::SelectRows);
    tree_->setAllColumnsShowFocus(true);
    tree_->setExpandsOnDoubleClick(true);
    tree_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tree_->setMouseTracking(true);
    tree_->viewport()->installEventFilter(this);
    mainLayout->addWidget(tree_);

    QHBoxLayout* buttonLayout = new QHBoxLayout;
    focusButton_->setEnabled(false);
    editButton_->setEnabled(false);
    deleteButton_->setEnabled(false);

    buttonLayout->addWidget(focusButton_);
    buttonLayout->addWidget(editButton_);
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addStretch();
    buttonLayout->addWidget(refreshButton_);

    mainLayout->addLayout(buttonLayout);

    connect(tree_, &QTreeWidget::itemChanged, this, &EntityManagementDialog::handleItemChanged);
    connect(tree_, &QTreeWidget::itemSelectionChanged, this, &EntityManagementDialog::handleSelectionChanged);
    connect(tree_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem*, int){ onFocusClicked(); });
    connect(tree_, &QTreeWidget::itemEntered, this, &EntityManagementDialog::handleItemEntered);

    connect(focusButton_, &QPushButton::clicked, this, &EntityManagementDialog::onFocusClicked);
    connect(editButton_, &QPushButton::clicked, this, &EntityManagementDialog::onEditClicked);
    connect(deleteButton_, &QPushButton::clicked, this, &EntityManagementDialog::onDeleteClicked);
    connect(refreshButton_, &QPushButton::clicked, this, &EntityManagementDialog::onRefreshClicked);
}

void EntityManagementDialog::refresh(const QList<GeoEntity*>& entities,
                                     const QMap<QString, QList<RouteGroupData>>& entityRouteMap,
                                     const QString& selectedUid)
{
    populateTree(entities, entityRouteMap, selectedUid);
}

void EntityManagementDialog::updateEntityVisibility(const QString& uid, bool visible)
{
    if (uid.isEmpty()) {
        return;
    }

    QTreeWidgetItemIterator it(tree_);
    while (*it) {
        QTreeWidgetItem* item = *it;
        if (item->data(ColumnName, RoleIsEntity).toBool() && item->data(ColumnName, RoleUid).toString() == uid) {
            updating_ = true;
            tree_->blockSignals(true);
            item->setCheckState(ColumnName, visible ? Qt::Checked : Qt::Unchecked);
            tree_->blockSignals(false);
            updating_ = false;
            break;
        }
        ++it;
    }
}

void EntityManagementDialog::setSelectedUid(const QString& uid)
{
    if (uid.isEmpty()) {
        tree_->clearSelection();
        updateButtonsState();
        return;
    }

    if (QTreeWidgetItem* item = findItemByUid(uid)) {
        updating_ = true;
        tree_->blockSignals(true);
        tree_->setCurrentItem(item);
        tree_->blockSignals(false);
        updating_ = false;
        updateButtonsState();
    }
}

void EntityManagementDialog::handleItemChanged(QTreeWidgetItem* item, int column)
{
    if (updating_ || !item || column != ColumnName) {
        return;
    }

    if (!item->data(ColumnName, RoleIsEntity).toBool()) {
        return;
    }

    const QString uid = item->data(ColumnName, RoleUid).toString();
    if (uid.isEmpty()) {
        return;
    }

    bool visible = (item->checkState(ColumnName) == Qt::Checked);
    emit requestVisibilityChange(uid, visible);
}

void EntityManagementDialog::handleSelectionChanged()
{
    if (updating_) {
        return;
    }
    updateButtonsState();
    emit requestSelection(currentEntityUid());
}

void EntityManagementDialog::handleItemEntered(QTreeWidgetItem* item, int)
{
    QString newUid;
    if (item && item->data(ColumnName, RoleIsEntity).toBool()) {
        newUid = item->data(ColumnName, RoleUid).toString();
    }

    if (hoveredUid_ == newUid) {
        return;
    }

    if (!hoveredUid_.isEmpty()) {
        emit requestHover(hoveredUid_, false);
    }

    hoveredUid_ = newUid;

    if (!hoveredUid_.isEmpty()) {
        emit requestHover(hoveredUid_, true);
    }
}

void EntityManagementDialog::onFocusClicked()
{
    const QString uid = currentEntityUid();
    if (uid.isEmpty()) {
        return;
    }
    emit requestFocus(uid);
}

void EntityManagementDialog::onEditClicked()
{
    const QString uid = currentEntityUid();
    if (uid.isEmpty()) {
        return;
    }
    emit requestEdit(uid);
}

void EntityManagementDialog::onDeleteClicked()
{
    const QString uid = currentEntityUid();
    if (uid.isEmpty()) {
        return;
    }
    emit requestDelete(uid);
}

void EntityManagementDialog::onRefreshClicked()
{
    emit requestRefresh();
}

bool EntityManagementDialog::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == tree_->viewport()) {
        if (event->type() == QEvent::Leave) {
            if (!hoveredUid_.isEmpty()) {
                emit requestHover(hoveredUid_, false);
                hoveredUid_.clear();
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

QString EntityManagementDialog::currentEntityUid() const
{
    QTreeWidgetItem* item = tree_->currentItem();
    if (!item || !item->data(ColumnName, RoleIsEntity).toBool()) {
        return QString();
    }
    return item->data(ColumnName, RoleUid).toString();
}

QTreeWidgetItem* EntityManagementDialog::findItemByUid(const QString& uid) const
{
    if (uid.isEmpty()) {
        return nullptr;
    }
    QTreeWidgetItemIterator it(tree_);
    while (*it) {
        QTreeWidgetItem* item = *it;
        if (item->data(ColumnName, RoleIsEntity).toBool() && item->data(ColumnName, RoleUid).toString() == uid) {
            return item;
        }
        ++it;
    }
    return nullptr;
}

void EntityManagementDialog::updateButtonsState()
{
    QTreeWidgetItem* item = tree_->currentItem();
    bool isEntity = item && item->data(ColumnName, RoleIsEntity).toBool();
    focusButton_->setEnabled(isEntity);
    editButton_->setEnabled(isEntity);
    deleteButton_->setEnabled(isEntity);
}

void EntityManagementDialog::populateTree(const QList<GeoEntity*>& entities,
                                          const QMap<QString, QList<RouteGroupData>>& entityRouteMap,
                                          const QString& selectedUid)
{
    updating_ = true;
    if (!hoveredUid_.isEmpty()) {
        emit requestHover(hoveredUid_, false);
        hoveredUid_.clear();
    }
    tree_->blockSignals(true);
    tree_->clear();

    QHash<QString, GeoEntity*> entityMap;
    entityMap.reserve(entities.size());
    for (GeoEntity* entity : entities) {
        if (entity) {
            entityMap.insert(entity->getUid(), entity);
        }
    }

    for (GeoEntity* entity : entities) {
        if (!entity) {
            continue;
        }

        if (entity->getProperty(QStringLiteral("lineEndpoint")).toBool()) {
            continue;
        }

        const QString typeId = entity->getType();

        if (typeId == QStringLiteral("waypoint")) {
            const QString groupId = entity->getProperty("waypointGroupId").toString();
            if (!groupId.isEmpty()) {
                continue;
            }

            QTreeWidgetItem* waypointItem = new QTreeWidgetItem(tree_);
            waypointItem->setText(ColumnName, entity->getName());
            waypointItem->setText(ColumnType, QString::fromUtf8(u8"航点"));
            waypointItem->setText(ColumnUid, entity->getUid());
            waypointItem->setData(ColumnName, RoleUid, entity->getUid());
            waypointItem->setData(ColumnName, RoleIsEntity, true);
            waypointItem->setCheckState(ColumnName, entity->isVisible() ? Qt::Checked : Qt::Unchecked);
            waypointItem->setFlags(waypointItem->flags() | Qt::ItemIsUserCheckable);

            if (!selectedUid.isEmpty() && entity->getUid() == selectedUid) {
                tree_->setCurrentItem(waypointItem);
            }
            continue;
        }

        QTreeWidgetItem* entityItem = new QTreeWidgetItem(tree_);
        entityItem->setText(ColumnName, entity->getName());
        QString typeDisplay;
        if (typeId == QStringLiteral("image")) {
            typeDisplay = QString::fromUtf8(u8"图片实体");
        } else if (typeId == QStringLiteral("line")) {
            typeDisplay = QString::fromUtf8(u8"直线");
        } else {
            typeDisplay = typeId;
        }
        entityItem->setText(ColumnType, typeDisplay);
        entityItem->setText(ColumnUid, entity->getUid());
        entityItem->setData(ColumnName, RoleUid, entity->getUid());
        entityItem->setData(ColumnName, RoleIsEntity, true);
        entityItem->setCheckState(ColumnName, entity->isVisible() ? Qt::Checked : Qt::Unchecked);
        entityItem->setFlags(entityItem->flags() | Qt::ItemIsUserCheckable);
        entityItem->setExpanded(true);

        if (typeId == QStringLiteral("line")) {
            const QString startUid = entity->getProperty("lineStartWaypointUid").toString();
            const QString endUid = entity->getProperty("lineEndWaypointUid").toString();

            auto createEndpointItem = [&](const QString& uidValue, const QString& roleLabel) {
                if (uidValue.isEmpty()) {
                    return;
                }
                GeoEntity* endpoint = entityMap.value(uidValue, nullptr);
                if (!endpoint) {
                    return;
                }
                QTreeWidgetItem* endpointItem = new QTreeWidgetItem(entityItem);
                endpointItem->setText(ColumnName, endpoint->getName());
                endpointItem->setText(ColumnType, QString::fromUtf8(u8"航点 (%1)").arg(roleLabel));
                endpointItem->setText(ColumnUid, endpoint->getUid());
                endpointItem->setData(ColumnName, RoleUid, endpoint->getUid());
                endpointItem->setData(ColumnName, RoleIsEntity, true);
                endpointItem->setCheckState(ColumnName, endpoint->isVisible() ? Qt::Checked : Qt::Unchecked);
                endpointItem->setFlags(endpointItem->flags() | Qt::ItemIsUserCheckable);

                double lon = 0.0, lat = 0.0, alt = 0.0;
                endpoint->getPosition(lon, lat, alt);
                endpointItem->setToolTip(ColumnName, QString("经度: %1\n纬度: %2\n高度: %3")
                                                         .arg(QString::number(lon, 'f', 6))
                                                         .arg(QString::number(lat, 'f', 6))
                                                         .arg(QString::number(alt, 'f', 2)));
            };

            createEndpointItem(startUid, QString::fromUtf8(u8"起点"));
            createEndpointItem(endUid, QString::fromUtf8(u8"终点"));
            continue;
        }

        if (!selectedUid.isEmpty() && entity->getUid() == selectedUid) {
            tree_->setCurrentItem(entityItem);
        }

        if (typeId == QStringLiteral("waypoint")) {
            continue;
        }

        const QList<RouteGroupData> routeGroups = entityRouteMap.value(entity->getUid());
        if (!routeGroups.isEmpty()) {
            for (const RouteGroupData& group : routeGroups) {
                QString groupLabel = group.groupName.isEmpty() ? group.groupId : group.groupName;
                QTreeWidgetItem* groupItem = new QTreeWidgetItem(entityItem);
                groupItem->setText(ColumnName, groupLabel);
                groupItem->setText(ColumnType, QString::fromUtf8(u8"航线组"));
                groupItem->setData(ColumnName, RoleIsEntity, false);
                groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsUserCheckable);
                groupItem->setExpanded(true);

                if (!group.waypoints.isEmpty()) {
                    for (GeoEntity* waypoint : group.waypoints) {
                        if (!waypoint) {
                            continue;
                        }
                        QTreeWidgetItem* wpItem = new QTreeWidgetItem(groupItem);
                        wpItem->setText(ColumnName, waypoint->getName());
                        wpItem->setText(ColumnType, QString::fromUtf8(u8"航迹点"));
                        wpItem->setText(ColumnUid, waypoint->getUid());
                        wpItem->setData(ColumnName, RoleUid, waypoint->getUid());
                        wpItem->setData(ColumnName, RoleIsEntity, false);
                        wpItem->setCheckState(ColumnName, waypoint->isVisible() ? Qt::Checked : Qt::Unchecked);
                        wpItem->setFlags(wpItem->flags() | Qt::ItemIsUserCheckable);
                    }
                } else {
                    QTreeWidgetItem* emptyWpItem = new QTreeWidgetItem(groupItem);
                    emptyWpItem->setText(ColumnName, QString::fromUtf8(u8"(无航迹点)"));
                    emptyWpItem->setData(ColumnName, RoleIsEntity, false);
                    emptyWpItem->setFlags(emptyWpItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
                }
            }
        } else {
            QTreeWidgetItem* emptyRouteGroupItem = new QTreeWidgetItem(entityItem);
            emptyRouteGroupItem->setText(ColumnName, QString::fromUtf8(u8"(无航线组)"));
            emptyRouteGroupItem->setText(ColumnType, QString::fromUtf8(u8"航线组"));
            emptyRouteGroupItem->setData(ColumnName, RoleIsEntity, false);
            emptyRouteGroupItem->setFlags(emptyRouteGroupItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
        }

        QTreeWidgetItem* weaponsItem = new QTreeWidgetItem(entityItem);
        weaponsItem->setText(ColumnName, QString::fromUtf8(u8"武器挂载"));
        weaponsItem->setText(ColumnType, QString::fromUtf8(u8"组合"));
        weaponsItem->setData(ColumnName, RoleIsEntity, false);
        weaponsItem->setFlags(weaponsItem->flags() & ~Qt::ItemIsUserCheckable);
        weaponsItem->setExpanded(true);

        QJsonObject weaponMounts = entity->getProperty("weaponMounts").toJsonObject();
        QJsonArray weaponsArray = weaponMounts["weapons"].toArray();
        if (!weaponsArray.isEmpty()) {
            for (const QJsonValue& weaponValue : weaponsArray) {
                QJsonObject weaponObj = weaponValue.toObject();
                QString weaponId = weaponObj["weaponId"].toString();
                QString weaponName = weaponObj["weaponName"].toString();
                int quantity = weaponObj["quantity"].toInt();

                QTreeWidgetItem* weaponItem = new QTreeWidgetItem(weaponsItem);
                weaponItem->setText(ColumnName, weaponName.isEmpty() ? weaponId : weaponName);
                weaponItem->setText(ColumnType, QString::fromUtf8(u8"武器"));
                weaponItem->setText(ColumnUid, weaponId);
                weaponItem->setData(ColumnName, RoleUid, entity->getUid());
                weaponItem->setData(ColumnName, RoleIsEntity, false);

                QSpinBox* spinBox = new QSpinBox(tree_);
                spinBox->setRange(0, 9999);
                spinBox->setValue(quantity);
                spinBox->setProperty("entityUid", entity->getUid());
                spinBox->setProperty("weaponId", weaponId);
                spinBox->setProperty("weaponName", weaponName);
                tree_->setItemWidget(weaponItem, ColumnUid, spinBox);

                connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                        this, [this](int value) {
                            QSpinBox* spin = qobject_cast<QSpinBox*>(sender());
                            if (!spin) {
                                return;
                            }
                            const QString entityUid = spin->property("entityUid").toString();
                            const QString weaponId = spin->property("weaponId").toString();
                            const QString weaponName = spin->property("weaponName").toString();
                            emit requestWeaponQuantityChange(entityUid, weaponId, weaponName, value);
                        });
            }
        } else {
            QTreeWidgetItem* emptyWeaponItem = new QTreeWidgetItem(weaponsItem);
            emptyWeaponItem->setText(ColumnName, QString::fromUtf8(u8"(未配置武器)"));
            emptyWeaponItem->setData(ColumnName, RoleIsEntity, false);
            emptyWeaponItem->setFlags(emptyWeaponItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
        }
    }

    tree_->blockSignals(false);
    updating_ = false;
    updateButtonsState();

    if (!selectedUid.isEmpty()) {
        setSelectedUid(selectedUid);
    }

    if (!tree_->currentItem()) {
        QTreeWidgetItemIterator it(tree_);
        while (*it) {
            if ((*it)->data(ColumnName, RoleIsEntity).toBool()) {
                tree_->setCurrentItem(*it);
                break;
            }
            ++it;
        }
    }
}

//void EntityManagementDialog::populateTree(const QList<GeoEntity*>& entities,
//                                          const QMap<QString, QList<RouteGroupData>>& entityRouteMap,
//                                          const QString& selectedUid)
//{
//    updating_ = true;
//    if (!hoveredUid_.isEmpty()) {
//        emit requestHover(hoveredUid_, false);
//        hoveredUid_.clear();
//    }
//    tree_->blockSignals(true);
//    tree_->clear();

//    for (GeoEntity* entity : entities) {
//        if (!entity) {
//            continue;
//        }

//        // 增加
//        if (entity->getProperty(QStringLiteral("lineEndpoint")).toBool()) {
//            continue;
//        }

//        const QString typeId = entity->getType();

////        if (typeId == QStringLiteral("waypoint")) {
////            continue;
////        }
//        if (typeId == QStringLiteral("waypoint")) {
//            const QString groupId = entity->getProperty("waypointGroupId").toString();
//            if (!groupId.isEmpty()) {
//                continue;
//            }

//            QTreeWidgetItem* waypointItem = new QTreeWidgetItem(tree_);
//            waypointItem->setText(ColumnName, entity->getName());
//            waypointItem->setText(ColumnType, QString::fromUtf8(u8"航点"));
//            waypointItem->setText(ColumnUid, entity->getUid());
//            waypointItem->setData(ColumnName, RoleUid, entity->getUid());
//            waypointItem->setData(ColumnName, RoleIsEntity, true);
//            waypointItem->setCheckState(ColumnName, entity->isVisible() ? Qt::Checked : Qt::Unchecked);
//            waypointItem->setFlags(waypointItem->flags() | Qt::ItemIsUserCheckable);

//            if (!selectedUid.isEmpty() && entity->getUid() == selectedUid) {
//                tree_->setCurrentItem(waypointItem);
//            }
//            continue;
//        }

//        QTreeWidgetItem* entityItem = new QTreeWidgetItem(tree_);
//        entityItem->setText(ColumnName, entity->getName());
//        QString typeDisplay;
//        if (typeId == QStringLiteral("image")) {
//            typeDisplay = QString::fromUtf8(u8"图片实体");
//        }
//        else if (typeId == QStringLiteral("line")) {
//            typeDisplay = QString::fromUtf8(u8"直线");
//        }
//        else {
//            typeDisplay = typeId;
//        }
//        entityItem->setText(ColumnType, typeDisplay);
//        entityItem->setText(ColumnUid, entity->getUid());
//        entityItem->setData(ColumnName, RoleUid, entity->getUid());
//        entityItem->setData(ColumnName, RoleIsEntity, true);
//        entityItem->setCheckState(ColumnName, entity->isVisible() ? Qt::Checked : Qt::Unchecked);
//        entityItem->setFlags(entityItem->flags() | Qt::ItemIsUserCheckable);
//        entityItem->setExpanded(true);

//        if (!selectedUid.isEmpty() && entity->getUid() == selectedUid) {
//            tree_->setCurrentItem(entityItem);
//        }

//        if (typeId == QStringLiteral("waypoint")) {
//            continue;
//        }

//        const QList<RouteGroupData> routeGroups = entityRouteMap.value(entity->getUid());
//        if (!routeGroups.isEmpty()) {
//            for (const RouteGroupData& group : routeGroups) {
//                QString groupLabel = group.groupName.isEmpty() ? group.groupId : group.groupName;
//                QTreeWidgetItem* groupItem = new QTreeWidgetItem(entityItem);
//                groupItem->setText(ColumnName, groupLabel);
//                groupItem->setText(ColumnType, QString::fromUtf8(u8"航线组"));
//                groupItem->setData(ColumnName, RoleIsEntity, false);
//                groupItem->setFlags(groupItem->flags() & ~Qt::ItemIsUserCheckable);
//                groupItem->setExpanded(true);

//                if (!group.waypoints.isEmpty()) {
//                    for (GeoEntity* waypoint : group.waypoints) {
//                        if (!waypoint) {
//                            continue;
//                        }
//                        QTreeWidgetItem* wpItem = new QTreeWidgetItem(groupItem);
//                        wpItem->setText(ColumnName, waypoint->getName());
//                        wpItem->setText(ColumnType, QString::fromUtf8(u8"航迹点"));
//                        wpItem->setText(ColumnUid, waypoint->getUid());
//                        wpItem->setData(ColumnName, RoleUid, waypoint->getUid());
//                        wpItem->setData(ColumnName, RoleIsEntity, false);
//                        wpItem->setCheckState(ColumnName, waypoint->isVisible() ? Qt::Checked : Qt::Unchecked);
//                        wpItem->setFlags(wpItem->flags() | Qt::ItemIsUserCheckable);
//                    }
//                } else {
//                    QTreeWidgetItem* emptyWpItem = new QTreeWidgetItem(groupItem);
//                    emptyWpItem->setText(ColumnName, QString::fromUtf8(u8"(无航迹点)"));
//                    emptyWpItem->setData(ColumnName, RoleIsEntity, false);
//                    emptyWpItem->setFlags(emptyWpItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
//                }
//            }
//        } else {
//            QTreeWidgetItem* emptyRouteGroupItem = new QTreeWidgetItem(entityItem);
//            emptyRouteGroupItem->setText(ColumnName, QString::fromUtf8(u8"(无航线组)"));
//            emptyRouteGroupItem->setText(ColumnType, QString::fromUtf8(u8"航线组"));
//            emptyRouteGroupItem->setData(ColumnName, RoleIsEntity, false);
//            emptyRouteGroupItem->setFlags(emptyRouteGroupItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
//        }

//        QTreeWidgetItem* weaponsItem = new QTreeWidgetItem(entityItem);
//        weaponsItem->setText(ColumnName, QString::fromUtf8(u8"武器挂载"));
//        weaponsItem->setText(ColumnType, QString::fromUtf8(u8"组合"));
//        weaponsItem->setData(ColumnName, RoleIsEntity, false);
//        weaponsItem->setFlags(weaponsItem->flags() & ~Qt::ItemIsUserCheckable);
//        weaponsItem->setExpanded(true);

//        QJsonObject weaponMounts = entity->getProperty("weaponMounts").toJsonObject();
//        QJsonArray weaponsArray = weaponMounts["weapons"].toArray();
//        if (!weaponsArray.isEmpty()) {
//            for (const QJsonValue& weaponValue : weaponsArray) {
//                QJsonObject weaponObj = weaponValue.toObject();
//                QString weaponId = weaponObj["weaponId"].toString();
//                QString weaponName = weaponObj["weaponName"].toString();
//                int quantity = weaponObj["quantity"].toInt();

//                QTreeWidgetItem* weaponItem = new QTreeWidgetItem(weaponsItem);
//                weaponItem->setText(ColumnName, weaponName.isEmpty() ? weaponId : weaponName);
//                weaponItem->setText(ColumnType, QString::fromUtf8(u8"武器"));
//                weaponItem->setText(ColumnUid, weaponId);
//                weaponItem->setData(ColumnName, RoleUid, entity->getUid());
//                weaponItem->setData(ColumnName, RoleIsEntity, false);

//                QSpinBox* spinBox = new QSpinBox(tree_);
//                spinBox->setRange(0, 9999);
//                spinBox->setValue(quantity);
//                spinBox->setProperty("entityUid", entity->getUid());
//                spinBox->setProperty("weaponId", weaponId);
//                spinBox->setProperty("weaponName", weaponName);
//                tree_->setItemWidget(weaponItem, ColumnUid, spinBox);

//                connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
//                        this, [this](int value) {
//                            QSpinBox* spin = qobject_cast<QSpinBox*>(sender());
//                            if (!spin) {
//                                return;
//                            }
//                            const QString entityUid = spin->property("entityUid").toString();
//                            const QString weaponId = spin->property("weaponId").toString();
//                            const QString weaponName = spin->property("weaponName").toString();
//                            emit requestWeaponQuantityChange(entityUid, weaponId, weaponName, value);
//                        });
//            }
//        } else {
//            QTreeWidgetItem* emptyWeaponItem = new QTreeWidgetItem(weaponsItem);
//            emptyWeaponItem->setText(ColumnName, QString::fromUtf8(u8"(未配置武器)"));
//            emptyWeaponItem->setData(ColumnName, RoleIsEntity, false);
//            emptyWeaponItem->setFlags(emptyWeaponItem->flags() & ~(Qt::ItemIsSelectable | Qt::ItemIsEditable | Qt::ItemIsUserCheckable));
//        }
//    }

//    tree_->blockSignals(false);
//    updating_ = false;
//    updateButtonsState();

//    if (!selectedUid.isEmpty()) {
//        setSelectedUid(selectedUid);
//    }

//    if (!tree_->currentItem()) {
//        QTreeWidgetItemIterator it(tree_);
//        while (*it) {
//            if ((*it)->data(ColumnName, RoleIsEntity).toBool()) {
//                tree_->setCurrentItem(*it);
//                break;
//            }
//            ++it;
//        }
//    }
//}

