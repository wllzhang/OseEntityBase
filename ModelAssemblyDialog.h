#ifndef MODELASSEMBLYDIALOG_H
#define MODELASSEMBLYDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QTreeWidget>
#include <QSqlDatabase>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include "util/databaseutils.h"

struct ModelInfo {
    QString id;
    QString name;
    QString type;
    QString location;
    QString icon;
    QStringList componentList;
};

class ModelAssemblyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModelAssemblyDialog(QWidget *parent = nullptr);
    ~ModelAssemblyDialog();

private slots:
    void onModelTreeItemClicked(QTreeWidgetItem *item, int column);
    void onComponentTreeDoubleClicked(QTreeWidgetItem *item, int column);
    void onAssemblyListItemDoubleClicked(QListWidgetItem *item);
    void onSaveButtonClicked();
    void onAddModelAction();
    void ondeleteModelAction();
    void onBrowseIconButtonClicked();

private:
    void setupUI();
    void setupDatabase();
    void loadModelTree();
    void loadModelTypes();
    void loadModels();
    void loadComponentTree();   // 加载组件树状结构

    void updateModelInfo(const ModelInfo &info);    // 更新模型的通用信息
    void clearAssemblyList();
    void loadAssemblyList(const QStringList &componentIds);
    void showContextMenu(const QPoint &pos);
    void addModel(QString modelName);
    ModelInfo getCurrentModelInfo() const;
//    QString generateModelId();

    QSqlDatabase db;
    QTreeWidget *modelTree;
    QTreeWidget *componentTree;
    QListWidget *assemblyList;  // 组件装配列表

    // 模型基本信息控件
    QLineEdit *modelNameEdit;
    QLineEdit *modelTypeEdit;
    QComboBox *modelLocationComboBox;
    QLineEdit *modelIconEdit;
    QPushButton *browseIconButton;

    ModelInfo currentModelInfo;
    QTreeWidgetItem *currentItem;

};


#endif // MODELASSEMBLYDIALOG_H
