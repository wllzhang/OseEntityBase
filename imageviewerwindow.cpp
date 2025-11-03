/**
 * @file imageviewerwindow.cpp
 * @brief 图片查看器窗口实现文件
 * 
 * 实现ImageViewerWindow类的所有功能
 */

#include "imageviewerwindow.h"
#include "geo/geoutils.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QJsonParseError>

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
    loadImageConfig();
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

void ImageViewerWindow::loadImageConfig()
{
    // 直接使用绝对路径
    QString configPath = "E:/osgqtlib/osgEarthmy_osgb/images_config.json";
    
    // 使用工具函数加载JSON配置
    QString errorMsg;
    config_ = GeoUtils::loadJsonFile(configPath, &errorMsg);
    
    if (config_.isEmpty()) {
        qDebug() << "配置文件加载失败:" << errorMsg;
        return;
    }
    
    imageDirectory_ = config_["image_directory"].toString();
    qDebug() << "配置文件加载成功，图片目录:" << imageDirectory_;
}

void ImageViewerWindow::populateImageList()
{
    qDebug() << "开始填充图片列表...";
    
    if (!config_.contains("entities")) {
        qDebug() << "配置文件中没有找到entities数组";
        qDebug() << "配置对象键:" << config_.keys();
        return;
    }
    
    QJsonArray entitiesArray = config_["entities"].toArray();
    qDebug() << "实体数组大小:" << entitiesArray.size();
    
    for (int i = 0; i < entitiesArray.size(); ++i) {
        QJsonObject entityObj = entitiesArray[i].toObject();
        QString name = entityObj["name"].toString();
        QString filename = entityObj["filename"].toString();
        
        qDebug() << "处理实体项" << i << ":" << name << "文件名:" << filename;
        
        if (name.isEmpty() || filename.isEmpty()) {
            qDebug() << "跳过空名称或文件名的项";
            continue;
        }
        
        // 检查文件是否存在
        QString fullPath = QDir(imageDirectory_).absoluteFilePath(filename);
        QFileInfo fileInfo(fullPath);
        
        qDebug() << "检查文件路径:" << fullPath;
        qDebug() << "文件是否存在:" << fileInfo.exists();
        
        if (fileInfo.exists()) {
            imageListWidget_->addItem(name);
            qDebug() << "成功添加实体项:" << name;
        } else {
            qDebug() << "实体文件不存在:" << fullPath;
        }
    }
    
    qDebug() << "实体列表填充完成，列表项数量:" << imageListWidget_->count();
}

void ImageViewerWindow::onImageSelected()
{
    int currentRow = imageListWidget_->currentRow();
    if (currentRow < 0) return;
    
    QJsonArray entitiesArray = config_["entities"].toArray();
    if (currentRow >= entitiesArray.size()) return;
    
    QJsonObject entityObj = entitiesArray[currentRow].toObject();
    QString filename = entityObj["filename"].toString();
    QString description = entityObj["description"].toString();
    
    QString fullPath = imageDirectory_ + filename;
    displaySelectedImage(fullPath, description);
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
