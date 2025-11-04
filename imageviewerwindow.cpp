/**
 * @file imageviewerwindow.cpp
 * @brief 图片查看器窗口实现文件
 * 
 * 实现ImageViewerWindow类的所有功能
 */

#include "imageviewerwindow.h"
#include "geo/geoutils.h"
#include "util/databaseutils.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QJsonParseError>
#include <QSqlQuery>
#include <QSqlError>
#include <QFileInfo>

ImageViewerWindow::ImageViewerWindow(QWidget *parent)
    : QDialog(parent)
    , imageListWidget_(nullptr)
    , imageLabel_(nullptr)
    , descriptionLabel_(nullptr)
    , scrollArea_(nullptr)
    , closeButton_(nullptr)
{
    setWindowTitle("战斗机");
    setModal(false);
    resize(800, 600);
    
    setupUI();
    loadImageListFromDatabase();
    populateImageList();
}

ImageViewerWindow::~ImageViewerWindow()
{
}

void ImageViewerWindow::setupUI()
{
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    
    // 左侧图片列表
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    
    QLabel *listTitle = new QLabel("战斗机列表:");
    listTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    imageListWidget_ = new DraggableListWidget;
    imageListWidget_->setMaximumWidth(250);
    connect(imageListWidget_, &QListWidget::currentRowChanged, this, &ImageViewerWindow::onImageSelected);
    
    leftLayout->addWidget(listTitle);
    leftLayout->addWidget(imageListWidget_);
    
    // 右侧图片显示区域
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    
    // 描述标签
    descriptionLabel_ = new QLabel("请选择一架战斗机查看详细信息");
    descriptionLabel_->setStyleSheet("font-size: 12px; color: #666; padding: 10px;");
    descriptionLabel_->setWordWrap(true);
    
    // 图片显示区域
    scrollArea_ = new QScrollArea;
    scrollArea_->setWidgetResizable(true);
    scrollArea_->setAlignment(Qt::AlignCenter);
    scrollArea_->setStyleSheet("border: 1px solid #ccc; background-color: #f9f9f9;");
    
    imageLabel_ = new QLabel;
    imageLabel_->setAlignment(Qt::AlignCenter);
    imageLabel_->setStyleSheet("background-color: white;");
    imageLabel_->setText("未选择图片");
    scrollArea_->setWidget(imageLabel_);
    
    // 关闭按钮
    closeButton_ = new QPushButton("关闭");
    closeButton_->setMaximumWidth(100);
    connect(closeButton_, &QPushButton::clicked, this, &ImageViewerWindow::onCloseClicked);
    
    rightLayout->addWidget(descriptionLabel_);
    rightLayout->addWidget(scrollArea_, 1); // 给图片区域更多空间
    rightLayout->addWidget(closeButton_, 0, Qt::AlignRight);
    
    // 添加到主布局
    mainLayout->addWidget(leftWidget, 0);
    mainLayout->addWidget(rightWidget, 1);
}

void ImageViewerWindow::loadImageListFromDatabase()
{
    // 使用DatabaseUtils获取数据库连接
    QSqlDatabase db = DatabaseUtils::getDatabase();
    
    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库:" << db.lastError().text();
        QMessageBox::warning(this, "错误", "无法打开数据库: " + db.lastError().text());
        return;
    }
    
    qDebug() << "ImageViewerWindow: 数据库连接成功，路径:" << DatabaseUtils::getDatabasePath();
    
    QSqlQuery query;
    query.exec("SELECT name, icon FROM ModelInformation WHERE icon IS NOT NULL AND icon != ''");
    
    modelList_.clear();
    while (query.next()) {
        QString name = query.value(0).toString();
        QString iconPath = query.value(1).toString();
        
        // 验证文件是否存在
        QFileInfo fileInfo(iconPath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            modelList_.append(qMakePair(name, iconPath));
            qDebug() << "加载模型:" << name << "图片路径:" << iconPath;
        } else {
            qDebug() << "模型图片文件不存在，跳过:" << name << iconPath;
        }
    }
    
    qDebug() << "从数据库加载了" << modelList_.size() << "个模型";
    db.close();
}

void ImageViewerWindow::populateImageList()
{
    qDebug() << "开始填充图片列表...";
    
    imageListWidget_->clear();
    
    for (const auto& pair : modelList_) {
        QString name = pair.first;
        imageListWidget_->addItem(name);
        qDebug() << "添加模型到列表:" << name;
    }
    
    qDebug() << "实体列表填充完成，列表项数量:" << imageListWidget_->count();
}

void ImageViewerWindow::onImageSelected()
{
    int currentRow = imageListWidget_->currentRow();
    if (currentRow < 0 || currentRow >= modelList_.size()) return;
    
    QString name = modelList_[currentRow].first;
    QString iconPath = modelList_[currentRow].second;
    
    // 显示图片，描述使用模型名称
    displaySelectedImage(iconPath, name);
}

void ImageViewerWindow::displaySelectedImage(const QString &imagePath, const QString &description)
{
    QFileInfo fileInfo(imagePath);
    if (!fileInfo.exists()) {
        qDebug() << "图片文件不存在:" << imagePath;
        imageLabel_->setText("图片文件不存在");
        return;
    }
    
    QPixmap pixmap(imagePath);
    if (pixmap.isNull()) {
        qDebug() << "无法加载图片:" << imagePath;
        imageLabel_->setText("无法加载图片");
        return;
    }
    
    // 缩放图片以适应显示区域，保持宽高比
    QSize scrollSize = scrollArea_->size();
    QSize scaledSize = pixmap.size().scaled(scrollSize.width() - 20, scrollSize.height() - 20, 
                                          Qt::KeepAspectRatio);
    
    if (scaledSize.width() < pixmap.width() || scaledSize.height() < pixmap.height()) {
        pixmap = pixmap.scaled(scaledSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }
    
    imageLabel_->setPixmap(pixmap);
    descriptionLabel_->setText(description);
    
    qDebug() << "显示图片:" << imagePath << "描述:" << description;
}

void ImageViewerWindow::onCloseClicked()
{
    close();
}
