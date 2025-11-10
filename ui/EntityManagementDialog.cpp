#include "EntityManagementDialog.h"

#include "../geo/geoentity.h"

#include <QTreeWidgetItemIterator>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QLabel>
#include <QSet>
#include <QEvent>

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
                                     const QList<QPair<QString, QList<GeoEntity*>>>& waypointGroups,
                                     const QString& selectedUid)
{
    populateTree(entities, waypointGroups, selectedUid);
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

void EntityManagementDialog::fillWaypointGroup(QTreeWidgetItem* parentItem,
                                               const QList<GeoEntity*>& waypoints,
                                               const QString& selectedUid,
                                               QSet<QString>& waypointUids)
{
    if (!parentItem) {
        return;
    }

    for (GeoEntity* entity : waypoints) {
        if (!entity) {
            continue;
        }

        waypointUids.insert(entity->getUid());

        QTreeWidgetItem* child = new QTreeWidgetItem(parentItem);
        child->setText(ColumnName, entity->getName());
        child->setText(ColumnType, QString::fromUtf8(u8"航迹点"));
        child->setText(ColumnUid, entity->getUid());
        child->setData(ColumnName, RoleUid, entity->getUid());
        child->setData(ColumnName, RoleIsEntity, true);
        child->setCheckState(ColumnName, entity->isVisible() ? Qt::Checked : Qt::Unchecked);
        child->setFlags(child->flags() | Qt::ItemIsUserCheckable);

        if (!selectedUid.isEmpty() && entity->getUid() == selectedUid) {
            tree_->setCurrentItem(child);
        }
    }
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
                                          const QList<QPair<QString, QList<GeoEntity*>>>& waypointGroups,
                                          const QString& selectedUid)
{
    updating_ = true;
    if (!hoveredUid_.isEmpty()) {
        emit requestHover(hoveredUid_, false);
        hoveredUid_.clear();
    }
    tree_->blockSignals(true);
    tree_->clear();

    QSet<QString> waypointUids;

    for (const auto& groupPair : waypointGroups) {
        const QString& groupName = groupPair.first;
        const QList<GeoEntity*>& wps = groupPair.second;
        if (wps.isEmpty()) {
            continue;
        }

        QTreeWidgetItem* groupItem = new QTreeWidgetItem(tree_);
        groupItem->setText(ColumnName, groupName);
        groupItem->setText(ColumnType, QString::fromUtf8(u8"航迹点组"));
        groupItem->setData(ColumnName, RoleIsEntity, false);
        groupItem->setData(ColumnName, RoleUid, QString());
        groupItem->setFlags(groupItem->flags() & ~(Qt::ItemIsUserCheckable));
        groupItem->setExpanded(true);

        fillWaypointGroup(groupItem, wps, selectedUid, waypointUids);
    }

    for (GeoEntity* entity : entities) {
        if (!entity) {
            continue;
        }
        if (waypointUids.contains(entity->getUid())) {
            continue;
        }

        QTreeWidgetItem* item = new QTreeWidgetItem(tree_);
        item->setText(ColumnName, entity->getName());
        const QString typeId = entity->getType();
        QString typeDisplay;
        if (typeId == QStringLiteral("waypoint")) {
            typeDisplay = QString::fromUtf8(u8"航迹点");
        } else if (typeId == QStringLiteral("image")) {
            typeDisplay = QString::fromUtf8(u8"图片实体");
        } else {
            typeDisplay = typeId;
        }
        item->setText(ColumnType, typeDisplay);
        item->setText(ColumnUid, entity->getUid());
        item->setData(ColumnName, RoleUid, entity->getUid());
        item->setData(ColumnName, RoleIsEntity, true);
        item->setCheckState(ColumnName, entity->isVisible() ? Qt::Checked : Qt::Unchecked);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);

        if (!selectedUid.isEmpty() && entity->getUid() == selectedUid) {
            tree_->setCurrentItem(item);
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

