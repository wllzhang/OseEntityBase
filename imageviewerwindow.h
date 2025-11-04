/**
 * @file imageviewerwindow.h
 * @brief 图片查看器窗口头文件
 * 
 * 定义ImageViewerWindow类，用于显示和选择可拖拽的实体图片
 */

#ifndef IMAGEVIEWERWINDOW_H
#define IMAGEVIEWERWINDOW_H

#include <QDialog>
#include <QListWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFileInfo>
#include <QPixmap>
#include <QScrollArea>
#include <QMimeData>
#include <QDrag>
#include <QPair>
#include "draggablelistwidget.h"

/**
 * @brief 图片查看器窗口
 * 
 * 显示配置的实体图片列表，支持预览和拖拽添加实体
 * 继承自QDialog，作为独立的对话框窗口
 */
class ImageViewerWindow : public QDialog
{
    Q_OBJECT

public:
    explicit ImageViewerWindow(QWidget *parent = nullptr);
    ~ImageViewerWindow();

private slots:
    void onImageSelected();
    void onCloseClicked();

private:
    void setupUI();
    void loadImageListFromDatabase();
    void populateImageList();
    void displaySelectedImage(const QString &imagePath, const QString &description);
    
private:
    DraggableListWidget *imageListWidget_;
    QLabel *imageLabel_;
    QLabel *descriptionLabel_;
    QScrollArea *scrollArea_;
    QPushButton *closeButton_;
    
    QList<QPair<QString, QString>> modelList_;  // 存储模型名称和图片路径的列表
};

#endif // IMAGEVIEWERWINDOW_H
