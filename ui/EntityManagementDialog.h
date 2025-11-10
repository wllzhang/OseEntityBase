#ifndef ENTITYMANAGEMENTDIALOG_H
#define ENTITYMANAGEMENTDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>
#include <QTreeWidget>
#include <QSet>
#include <QString>

class GeoEntity;
class QPushButton;

class EntityManagementDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EntityManagementDialog(QWidget* parent = nullptr);

    void refresh(const QList<GeoEntity*>& entities,
                 const QList<QPair<QString, QList<GeoEntity*>>>& waypointGroups,
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
    void fillWaypointGroup(QTreeWidgetItem* parentItem, const QList<GeoEntity*>& waypoints, const QString& selectedUid, QSet<QString>& waypointUids);
    void updateButtonsState();
    void populateTree(const QList<GeoEntity*>& entities,
                      const QList<QPair<QString, QList<GeoEntity*>>>& waypointGroups,
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


