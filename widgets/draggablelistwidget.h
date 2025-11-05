/**
 * @file draggablelistwidget.h
 * @brief 可拖拽列表控件头文件
 * 
 * 定义DraggableListWidget类，支持从列表项拖拽数据到主窗口
 */

#ifndef DRAGGABLELISTWIDGET_H
#define DRAGGABLELISTWIDGET_H

#include <QListWidget>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QDebug>

/**
 * @brief 可拖拽列表控件
 * 
 * 继承自QListWidget，支持鼠标拖拽列表项
 * 用于ImageViewerWindow，允许将图片拖拽到主窗口创建实体
 */
class DraggableListWidget : public QListWidget
{
    Q_OBJECT

public:
    explicit DraggableListWidget(QWidget *parent = nullptr);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    void startDrag(QListWidgetItem *item);
    QPoint startPos_;
};

#endif // DRAGGABLELISTWIDGET_H
