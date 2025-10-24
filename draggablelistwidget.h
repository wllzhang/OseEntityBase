#ifndef DRAGGABLELISTWIDGET_H
#define DRAGGABLELISTWIDGET_H

#include <QListWidget>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QApplication>
#include <QDebug>

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
