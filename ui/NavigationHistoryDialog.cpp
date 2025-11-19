/**
 * @file NavigationHistoryDialog.cpp
 * @brief 导航历史列表对话框实现
 */

#include "NavigationHistoryDialog.h"
#include <QMessageBox>
#include <QHeaderView>
#include <QDebug>
#include <osgEarth/Units>

NavigationHistoryDialog::NavigationHistoryDialog(NavigationHistory* navigationHistory,
                                                 const osgEarth::Viewpoint& currentViewpoint,
                                                 QWidget *parent)
    : QDialog(parent)
    , navigationHistory_(navigationHistory)
    , currentViewpoint_(currentViewpoint)
    , historyListWidget_(nullptr)
    , jumpButton_(nullptr)
    , closeButton_(nullptr)
    , infoLabel_(nullptr)
{
    setWindowTitle("视角历史记录");
    setModal(false);
    resize(500, 600);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 添加说明标签
    infoLabel_ = new QLabel("双击列表项或选中后点击跳转按钮可跳转到对应视角", this);
    infoLabel_->setStyleSheet("color: #666; font-size: 10pt; padding: 5px;");
    infoLabel_->setWordWrap(true);
    mainLayout->addWidget(infoLabel_);
    
    // 创建历史记录列表
    historyListWidget_ = new QListWidget(this);
    historyListWidget_->setSelectionMode(QAbstractItemView::SingleSelection);
    historyListWidget_->setAlternatingRowColors(true);
    historyListWidget_->setStyleSheet(
        "QListWidget {"
        "    border: 1px solid #ccc;"
        "    border-radius: 4px;"
        "    padding: 5px;"
        "}"
        "QListWidget::item {"
        "    padding: 8px;"
        "    border-bottom: 1px solid #eee;"
        "}"
        "QListWidget::item:selected {"
        "    background-color: #4A90E2;"
        "    color: white;"
        "}"
        "QListWidget::item:hover {"
        "    background-color: #E8F4F8;"
        "}"
    );
    
    connect(historyListWidget_, &QListWidget::itemDoubleClicked, 
            this, &NavigationHistoryDialog::onItemDoubleClicked);
    connect(historyListWidget_, &QListWidget::itemSelectionChanged,
            this, &NavigationHistoryDialog::onSelectionChanged);
    
    mainLayout->addWidget(historyListWidget_);
    
    // 添加按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    
    jumpButton_ = new QPushButton("跳转", this);
    jumpButton_->setDefault(true);
    jumpButton_->setMinimumWidth(80);
    jumpButton_->setEnabled(false);
    connect(jumpButton_, &QPushButton::clicked, this, &NavigationHistoryDialog::onJumpClicked);
    
    closeButton_ = new QPushButton("关闭", this);
    closeButton_->setMinimumWidth(80);
    connect(closeButton_, &QPushButton::clicked, this, &NavigationHistoryDialog::onCloseClicked);
    
    buttonLayout->addWidget(jumpButton_);
    buttonLayout->addWidget(closeButton_);
    mainLayout->addLayout(buttonLayout);
    
    // 更新历史记录列表
    updateHistoryList();
}

void NavigationHistoryDialog::refreshHistory(const osgEarth::Viewpoint& currentViewpoint)
{
    currentViewpoint_ = currentViewpoint;
    updateHistoryList();
}

void NavigationHistoryDialog::updateHistoryList()
{
    if (!navigationHistory_) {
        return;
    }
    
    // 清空列表
    historyListWidget_->clear();
    historyItems_.clear();
    
    // 获取所有历史记录
    historyItems_ = navigationHistory_->getAllHistory(currentViewpoint_);
    
    if (historyItems_.isEmpty()) {
        QListWidgetItem* emptyItem = new QListWidgetItem("暂无历史记录");
        emptyItem->setFlags(Qt::NoItemFlags);
        historyListWidget_->addItem(emptyItem);
        return;
    }
    
    // 添加历史记录到列表
    for (const auto& item : historyItems_) {
        QString displayText = formatViewpointInfo(item);
        QListWidgetItem* listItem = new QListWidgetItem(displayText);
        
        // 如果是当前视角，使用特殊样式
        if (item.isCurrent) {
            listItem->setBackground(QBrush(QColor(255, 255, 200)));  // 浅黄色背景
            listItem->setForeground(QBrush(QColor(0, 0, 0)));        // 黑色文字
            listItem->setFont(QFont("", -1, QFont::Bold));           // 粗体
        }
        
        historyListWidget_->addItem(listItem);
    }
    
    // 自动选中当前视角项
    for (int i = 0; i < historyItems_.size(); ++i) {
        if (historyItems_[i].isCurrent) {
            historyListWidget_->setCurrentRow(i);
            historyListWidget_->scrollToItem(historyListWidget_->item(i));
            break;
        }
    }
}

QString NavigationHistoryDialog::formatViewpointInfo(const NavigationHistory::HistoryItem& item) const
{
    QString result;
    
    // 添加标记
    if (item.isCurrent) {
        result += "[当前] ";
    }
    
    // 添加名称
    result += item.displayName;
    
    // 添加视角信息
    const auto& vp = item.viewpoint;
    if (vp.focalPoint().isSet()) {
        const auto& focal = vp.focalPoint().value();
        double lon = focal.x();
        double lat = focal.y();
        double alt = focal.z();
        
        result += QString("\n  位置: 经度 %1°, 纬度 %2°, 高度 %3m")
                     .arg(lon, 0, 'f', 6)
                     .arg(lat, 0, 'f', 6)
                     .arg(alt, 0, 'f', 2);
    }
    
    if (vp.heading().isSet()) {
        double heading = vp.heading().value().as(osgEarth::Units::DEGREES);
        result += QString(" | 航向: %1°").arg(heading, 0, 'f', 2);
    }
    
    if (vp.pitch().isSet()) {
        double pitch = vp.pitch().value().as(osgEarth::Units::DEGREES);
        result += QString(" | 俯仰: %1°").arg(pitch, 0, 'f', 2);
    }
    
    if (vp.range().isSet()) {
        double range = vp.range().value().as(osgEarth::Units::METERS);
        if (range >= 1000.0) {
            result += QString(" | 视距: %1km").arg(range / 1000.0, 0, 'f', 2);
        } else {
            result += QString(" | 视距: %1m").arg(range, 0, 'f', 2);
        }
    }
    
    return result;
}

void NavigationHistoryDialog::onItemDoubleClicked(QListWidgetItem* item)
{
    Q_UNUSED(item);
    onJumpClicked();
}

void NavigationHistoryDialog::onJumpClicked()
{
    int currentRow = historyListWidget_->currentRow();
    if (currentRow < 0 || currentRow >= historyItems_.size()) {
        return;
    }
    
    const auto& selectedItem = historyItems_[currentRow];
    
    // 如果是当前视角，不需要跳转
    if (selectedItem.isCurrent) {
        QMessageBox::information(this, "提示", "当前已在此视角");
        return;
    }
    
    // 发出跳转信号
    emit jumpToViewpoint(selectedItem.viewpoint);
}

void NavigationHistoryDialog::onCloseClicked()
{
    close();
}

void NavigationHistoryDialog::onSelectionChanged()
{
    int currentRow = historyListWidget_->currentRow();
    bool hasSelection = (currentRow >= 0 && currentRow < historyItems_.size());
    
    if (hasSelection) {
        const auto& selectedItem = historyItems_[currentRow];
        // 如果是当前视角，禁用跳转按钮
        jumpButton_->setEnabled(!selectedItem.isCurrent);
    } else {
        jumpButton_->setEnabled(false);
    }
}

