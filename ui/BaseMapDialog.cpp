/**
 * @file BaseMapDialog.cpp
 * @brief 底图管理对话框实现文件
 * 
 * 实现BaseMapDialog类的所有功能
 */

#include "BaseMapDialog.h"
#include "../geo/basemapmanager.h"
#include <QDebug>
#include <QBrush>
#include <QColor>

BaseMapDialog::BaseMapDialog(BaseMapManager* baseMapManager, QWidget *parent)
    : QDialog(parent)
    , baseMapManager_(baseMapManager)
    , listWidget_(nullptr)
    , currentMapLabel_(nullptr)
    , selectedMapName_()
{
    if (!baseMapManager_) {
        qDebug() << "BaseMapDialog: BaseMapManager为空";
    }
    
    setupUI();
    updateBaseMapList();
    
    // 连接底图切换信号
    if (baseMapManager_) {
        connect(baseMapManager_, &BaseMapManager::baseMapSwitched,
                this, &BaseMapDialog::onBaseMapSwitched);
    }
}

BaseMapDialog::~BaseMapDialog()
{
}

void BaseMapDialog::setupUI()
{
    setWindowTitle("底图管理");
    setMinimumSize(400, 500);
    resize(450, 550);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 当前底图标签
    currentMapLabel_ = new QLabel("当前底图：无底图", this);
    currentMapLabel_->setStyleSheet("font-size: 14px; font-weight: bold; padding: 5px;");
    mainLayout->addWidget(currentMapLabel_);
    
    // 底图列表
    listWidget_ = new QListWidget(this);
    listWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    connect(listWidget_, &QListWidget::itemDoubleClicked,
            this, &BaseMapDialog::onItemDoubleClicked);
    mainLayout->addWidget(listWidget_);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    
    QPushButton* okButton = new QPushButton("确定", this);
    QPushButton* cancelButton = new QPushButton("取消", this);
    
    connect(okButton, &QPushButton::clicked, this, &BaseMapDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &BaseMapDialog::onCancelClicked);
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
    
    // 更新当前底图显示
    if (baseMapManager_) {
        QString currentMap = baseMapManager_->getCurrentBaseMapName();
        currentMapLabel_->setText("当前底图：" + currentMap);
    }
}

void BaseMapDialog::updateBaseMapList()
{
    if (!baseMapManager_ || !listWidget_) {
        return;
    }
    
    listWidget_->clear();
    
    QList<BaseMapSource> baseMaps = baseMapManager_->getAvailableBaseMaps();
    for (const auto& source : baseMaps) {
        QListWidgetItem* item = new QListWidgetItem(source.name, listWidget_);
        item->setData(Qt::UserRole, source.name);
        listWidget_->addItem(item);
    }
    
    highlightCurrentBaseMap();
}

void BaseMapDialog::highlightCurrentBaseMap()
{
    if (!baseMapManager_ || !listWidget_) {
        return;
    }
    
    QString currentMap = baseMapManager_->getCurrentBaseMapName();
    
    for (int i = 0; i < listWidget_->count(); ++i) {
        QListWidgetItem* item = listWidget_->item(i);
        if (item && item->text() == currentMap) {
            item->setSelected(true);
            listWidget_->setCurrentItem(item);
            listWidget_->scrollToItem(item);
            
            // 设置高亮样式
            item->setBackground(QBrush(QColor(200, 230, 255)));
            break;
        }
    }
}

void BaseMapDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    if (!item || !baseMapManager_) {
        return;
    }
    
    QString mapName = item->text();
    if (baseMapManager_->switchToBaseMap(mapName)) {
        selectedMapName_ = mapName;
        // 双击后直接关闭对话框
        accept();
    }
}

void BaseMapDialog::onOkClicked()
{
    if (!baseMapManager_ || !listWidget_) {
        reject();
        return;
    }
    
    QListWidgetItem* currentItem = listWidget_->currentItem();
    if (!currentItem) {
        // 如果没有选中项，保持当前底图不变
        accept();
        return;
    }
    
    QString mapName = currentItem->text();
    if (baseMapManager_->switchToBaseMap(mapName)) {
        selectedMapName_ = mapName;
        accept();
    } else {
        // 切换失败，不关闭对话框
        qDebug() << "BaseMapDialog: 切换底图失败:" << mapName;
    }
}

void BaseMapDialog::onCancelClicked()
{
    reject();
}

void BaseMapDialog::onBaseMapSwitched(const QString& mapName)
{
    // 更新当前底图标签
    if (currentMapLabel_) {
        currentMapLabel_->setText("当前底图：" + mapName);
    }
    
    // 更新列表高亮
    highlightCurrentBaseMap();
}

