#ifndef ENTITYMANAGEMENTDIALOG_H
#define ENTITYMANAGEMENTDIALOG_H

#include <QDialog>
#include <QList>
#include <QTreeWidget>
#include <QString>
#include <QMap>

class GeoEntity;
class QPushButton;

struct RouteGroupData {
    QString groupId;
    QString groupName;
    QList<GeoEntity*> waypoints;
};

class EntityManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntityManagementDialog(QWidget* parent = nullptr);

    void refresh(const QList<GeoEntity*>& entities,
                 const QMap<QString, QList<RouteGroupData>>& entityRouteMap,
                 const QString& selectedUid);
    void updateEntityVisibility(const QString& uid, bool visible);
    void setSelectedUid(const QString& uid);

signals:
    void requestFocus(const QString& uid);
    void requestEdit(const QString& uid);
    void requestDelete(const QString& uid);
    void requestVisibilityChange(const QString& uid, bool visible);
    void requestSelection(const QString& uid);
    void requestRefresh();
    void requestWeaponQuantityChange(const QString& entityUid,
                                     const QString& weaponId,
                                     const QString& weaponName,
                                     int quantity);
    void requestHover(const QString& uid, bool hovered);

private slots:
    void handleItemChanged(QTreeWidgetItem* item, int column);
    void handleSelectionChanged();
    void handleItemEntered(QTreeWidgetItem* item, int column);
    void onFocusClicked();
    void onEditClicked();
    void onDeleteClicked();
    void onRefreshClicked();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    QString currentEntityUid() const;
    QTreeWidgetItem* findItemByUid(const QString& uid) const;
    void updateButtonsState();
    void populateTree(const QList<GeoEntity*>& entities,
                      const QMap<QString, QList<RouteGroupData>>& entityRouteMap,
                      const QString& selectedUid);

    QTreeWidget* tree_;
    QPushButton* focusButton_;
    QPushButton* editButton_;
    QPushButton* deleteButton_;
    QPushButton* refreshButton_;
    bool updating_;
    QString hoveredUid_;
};

#endif // ENTITYMANAGEMENTDIALOG_H


