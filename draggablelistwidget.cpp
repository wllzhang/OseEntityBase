#include "draggablelistwidget.h"

DraggableListWidget::DraggableListWidget(QWidget *parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
    setDefaultDropAction(Qt::CopyAction);
}

void DraggableListWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        startPos_ = event->pos();
    }
    QListWidget::mousePressEvent(event);
}

void DraggableListWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (!(event->buttons() & Qt::LeftButton)) {
        QListWidget::mouseMoveEvent(event);
        return;
    }

    int distance = (event->pos() - startPos_).manhattanLength();
    if (distance < QApplication::startDragDistance()) {
        QListWidget::mouseMoveEvent(event);
        return;
    }

    QListWidgetItem *item = itemAt(startPos_);
    if (!item) {
        QListWidget::mouseMoveEvent(event);
        return;
    }

    startDrag(item);
}

void DraggableListWidget::startDrag(QListWidgetItem *item)
{
    // 创建拖拽数据
    QMimeData *mimeData = new QMimeData;
    
    // 设置自定义MIME类型
    QString dragData = QString("aircraft:%1").arg(item->text());
    mimeData->setText(dragData);
    
    // 创建拖拽对象
    QDrag *drag = new QDrag(this);
    drag->setMimeData(mimeData);
    
    // 设置拖拽图标（使用列表项的图标）
    QPixmap pixmap = item->icon().pixmap(32, 32);
    if (pixmap.isNull()) {
        pixmap = QPixmap(32, 32);
        pixmap.fill(Qt::blue);
    }
    drag->setPixmap(pixmap);
    
    // 设置热点为图标中心
    drag->setHotSpot(QPoint(16, 16));
    
    qDebug() << "开始拖拽:" << item->text();
    
    // 执行拖拽
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    
    if (dropAction == Qt::CopyAction) {
        qDebug() << "拖拽成功完成";
    } else {
        qDebug() << "拖拽被取消";
    }
}
