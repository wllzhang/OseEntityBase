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
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFileInfo>
#include <QPixmap>
#include <QScrollArea>
#include <QMimeData>
#include <QDrag>
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
    void loadImageConfig();
    void populateImageList();
    void displaySelectedImage(const QString &imagePath, const QString &description);
    
private:
    DraggableListWidget *imageListWidget_;
    QLabel *imageLabel_;
    QLabel *descriptionLabel_;
    QScrollArea *scrollArea_;
    QPushButton *closeButton_;
    
    QJsonObject config_;
    QString imageDirectory_;
};

#endif // IMAGEVIEWERWINDOW_H
