/**
 * @file ModelDeployDialog.h
 * @brief 模型部署对话框头文件
 * 
 * 定义ModelDeployDialog类，用于显示模型列表并支持拖拽部署
 */

#ifndef MODELDEPLOYDIALOG_H
#define MODELDEPLOYDIALOG_H

#include <QDialog>
#include <QPair>

class DraggableListWidget;
class QLabel;
class QScrollArea;
class QPushButton;

/**
 * @brief 模型部署对话框
 * 
 * 显示数据库中的模型列表，支持预览和拖拽部署到地图
 */
class ModelDeployDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ModelDeployDialog(QWidget *parent = nullptr);
    ~ModelDeployDialog();

private slots:
    void onModelSelected();
    void onCloseClicked();

private:
    void setupUI();
    void loadModelsFromDatabase();
    void populateModelList();
    void displaySelectedModel(const QString &modelId, const QString &modelName, const QString &iconPath);
    
private:
    DraggableListWidget *modelListWidget_;
    QLabel *imageLabel_;
    QLabel *descriptionLabel_;
    QScrollArea *scrollArea_;
    QPushButton *closeButton_;
    
    // 存储模型ID、名称和图片路径的列表
    struct ModelData {
        QString id;
        QString name;
        QString iconPath;
    };
    QList<ModelData> modelList_;
};

#endif // MODELDEPLOYDIALOG_H

