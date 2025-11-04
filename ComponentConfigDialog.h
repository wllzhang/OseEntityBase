#ifndef COMPONENTCONFIGDIALOG_H
#define COMPONENTCONFIGDIALOG_H

#include <QDialog>
#include <QTreeWidget>
#include <QSqlDatabase>
#include <QJsonObject>
#include "util/databaseutils.h"

class QLineEdit;
class QComboBox;
class QVBoxLayout;
class QHBoxLayout;
class QFormLayout;
class QLabel;

struct ComponentInfo {
    QString componentId;
    QString name;
    QString type;
    QString wsf;
    QString subtype;
    QJsonObject configInfo;
    QJsonObject templateInfo;
};

class ComponentConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ComponentConfigDialog(QWidget *parent = nullptr);
    ~ComponentConfigDialog();

private slots:
    void onTreeItemClicked(QTreeWidgetItem *item, int column);
    void onSaveButtonClicked();
    void showContextMenu(const QPoint &pos);
    void copyComponent();
    void deleteComponent();

private:
    void setupUI();
    void setupDatabase();
    void loadComponentTree();
    void loadComponentTypes();
    void loadComponents();
    void clearParameterForm();
    void createParameterForm(const QJsonObject &templateInfo, const QJsonObject &configInfo = QJsonObject());
    QWidget* createFormWidget(int type, const QStringList &values = QStringList(), const QVariant &currentValue = QVariant());
    void updateComponentInfo(const ComponentInfo &info);
    ComponentInfo getCurrentComponentInfo() const;
    QString generateComponentId();
    bool isComponentUsed(QString componentId);


    QSqlDatabase db;
    QTreeWidget *componentTree;
    QVBoxLayout *rightLayout;
    QWidget *parameterWidget;
    QFormLayout *parameterFormLayout;

    // 通用信息控件
    QLineEdit *nameEdit;
    QComboBox *typeComboBox;
    QLineEdit *wsfEdit;
    QLineEdit *commentEdit;

    // 动态生成的参数控件映射
    QMap<QString, QWidget*> paramWidgets;
    ComponentInfo currentComponentInfo;
    QTreeWidgetItem *currentItem;
};

#endif // COMPONENTCONFIGDIALOG_H
