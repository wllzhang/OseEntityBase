/**
 * @file ModelDeployDialog.cpp
 * @brief 模型部署对话框实现文件
 * 
 * 实现ModelDeployDialog类的所有功能
 */

#include "ModelDeployDialog.h"
#include "../widgets/draggablelistwidget.h"
#include "../util/databaseutils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPushButton>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QFileInfo>
#include <QPixmap>
#include <QDebug>
#include <QMessageBox>
#include <QApplication>
#include <QMimeData>
#include <QDrag>
#include <QListWidgetItem>
#include <QMouseEvent>

// 自定义可拖拽列表控件，支持传递modelId
class ModelDeployListWidget : public DraggableListWidget
{
public:
    explicit ModelDeployListWidget(QWidget *parent = nullptr) : DraggableListWidget(parent) {}

protected:
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            startPos_ = event->pos();
        }
        DraggableListWidget::mousePressEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override
    {
        if (!(event->buttons() & Qt::LeftButton)) {
            DraggableListWidget::mouseMoveEvent(event);
            return;
        }

        int distance = (event->pos() - startPos_).manhattanLength();
        if (distance < QApplication::startDragDistance()) {
            DraggableListWidget::mouseMoveEvent(event);
            return;
        }

        QListWidgetItem *item = itemAt(startPos_);
        if (!item) {
            DraggableListWidget::mouseMoveEvent(event);
            return;
        }

        startDrag(item);
    }

private:
    void startDrag(QListWidgetItem *item)
    {
        // 获取存储的modelId和modelName
        QString modelId = item->data(Qt::UserRole).toString();
        QString modelName = item->text();
        
        if (modelId.isEmpty()) {
            qDebug() << "模型ID为空，无法拖拽";
            return;
        }

        // 创建拖拽数据
        QMimeData *mimeData = new QMimeData;
        
        // 设置自定义MIME类型: "modeldeploy:{modelId}:{modelName}"
        QString dragData = QString("modeldeploy:%1:%2").arg(modelId).arg(modelName);
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
        
        qDebug() << "开始拖拽模型:" << modelName << "ID:" << modelId;
        
        // 执行拖拽
        Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
        
        if (dropAction == Qt::CopyAction) {
            qDebug() << "拖拽成功完成";
        } else {
            qDebug() << "拖拽被取消";
        }
    }

private:
    QPoint startPos_;
};

ModelDeployDialog::ModelDeployDialog(QWidget *parent)
    : QDialog(parent)
    , modelListWidget_(nullptr)
    , imageLabel_(nullptr)
    , descriptionLabel_(nullptr)
    , scrollArea_(nullptr)
    , closeButton_(nullptr)
{
    setWindowTitle("模型部署");
    setModal(false);
    resize(800, 600);
    
    setupUI();
    loadModelsFromDatabase();
    populateModelList();
}

ModelDeployDialog::~ModelDeployDialog()
{
}

void ModelDeployDialog::setupUI()
{
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout(this);
    
    // 左侧模型列表
    QWidget *leftWidget = new QWidget;
    QVBoxLayout *leftLayout = new QVBoxLayout(leftWidget);
    
    QLabel *listTitle = new QLabel("模型列表:");
    listTitle->setStyleSheet("font-weight: bold; font-size: 14px;");
    
    modelListWidget_ = new ModelDeployListWidget;
    modelListWidget_->setMaximumWidth(250);
    connect(modelListWidget_, &QListWidget::currentRowChanged, this, &ModelDeployDialog::onModelSelected);
    
    leftLayout->addWidget(listTitle);
    leftLayout->addWidget(modelListWidget_);
    
    // 右侧预览区域
    QWidget *rightWidget = new QWidget;
    QVBoxLayout *rightLayout = new QVBoxLayout(rightWidget);
    
    // 描述标签
    descriptionLabel_ = new QLabel("请选择一个模型查看详细信息");
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
    imageLabel_->setText("未选择模型");
    scrollArea_->setWidget(imageLabel_);
    
    // 关闭按钮
    closeButton_ = new QPushButton("关闭");
    closeButton_->setMaximumWidth(100);
    connect(closeButton_, &QPushButton::clicked, this, &ModelDeployDialog::onCloseClicked);
    
    rightLayout->addWidget(descriptionLabel_);
    rightLayout->addWidget(scrollArea_, 1); // 给图片区域更多空间
    rightLayout->addWidget(closeButton_, 0, Qt::AlignRight);
    
    // 添加到主布局
    mainLayout->addWidget(leftWidget, 0);
    mainLayout->addWidget(rightWidget, 1);
}

void ModelDeployDialog::loadModelsFromDatabase()
{
    // 使用DatabaseUtils获取数据库连接
    QSqlDatabase db = DatabaseUtils::getDatabase();
    
    if (!DatabaseUtils::openDatabase()) {
        qDebug() << "无法打开数据库:" << db.lastError().text();
        QMessageBox::warning(this, "错误", "无法打开数据库: " + db.lastError().text());
        return;
    }
    
    qDebug() << "ModelDeployDialog: 数据库连接成功，路径:" << DatabaseUtils::getDatabasePath();
    
    QSqlQuery query;
    query.exec("SELECT id, name, icon FROM ModelInformation WHERE icon IS NOT NULL AND icon != ''");
    
    modelList_.clear();
    while (query.next()) {
        ModelData data;
        data.id = query.value(0).toString();
        data.name = query.value(1).toString();
        data.iconPath = query.value(2).toString();
        
        // 验证文件是否存在
        QFileInfo fileInfo(data.iconPath);
        if (fileInfo.exists() && fileInfo.isFile()) {
            modelList_.append(data);
            qDebug() << "加载模型:" << data.name << "ID:" << data.id << "图片路径:" << data.iconPath;
        } else {
            qDebug() << "模型图片文件不存在，跳过:" << data.name << data.iconPath;
        }
    }
    
    qDebug() << "从数据库加载了" << modelList_.size() << "个模型";
    db.close();
}

void ModelDeployDialog::populateModelList()
{
    qDebug() << "开始填充模型列表...";
    
    modelListWidget_->clear();
    
    for (const auto& data : modelList_) {
        QListWidgetItem *item = new QListWidgetItem(data.name);
        item->setData(Qt::UserRole, data.id);  // 存储modelId
        
        // 如果有图标，加载并设置
        if (!data.iconPath.isEmpty()) {
            QFileInfo fileInfo(data.iconPath);
            if (fileInfo.exists()) {
                QPixmap pixmap(data.iconPath);
                if (!pixmap.isNull()) {
                    QIcon icon(pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    item->setIcon(icon);
                }
            }
        }
        
        modelListWidget_->addItem(item);
        qDebug() << "添加模型到列表:" << data.name << "ID:" << data.id;
    }
    
    qDebug() << "模型列表填充完成，列表项数量:" << modelListWidget_->count();
}

void ModelDeployDialog::onModelSelected()
{
    int currentRow = modelListWidget_->currentRow();
    if (currentRow < 0 || currentRow >= modelList_.size()) return;
    
    const ModelData& data = modelList_[currentRow];
    
    // 显示模型预览
    displaySelectedModel(data.id, data.name, data.iconPath);
}

void ModelDeployDialog::displaySelectedModel(const QString &modelId, const QString &modelName, const QString &iconPath)
{
    QFileInfo fileInfo(iconPath);
    if (!fileInfo.exists()) {
        qDebug() << "图片文件不存在:" << iconPath;
        imageLabel_->setText("图片文件不存在");
        descriptionLabel_->setText(QString("模型: %1\nID: %2").arg(modelName).arg(modelId));
        return;
    }
    
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) {
        qDebug() << "无法加载图片:" << iconPath;
        imageLabel_->setText("无法加载图片");
        descriptionLabel_->setText(QString("模型: %1\nID: %2").arg(modelName).arg(modelId));
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
    descriptionLabel_->setText(QString("模型名称: %1\n模型ID: %2").arg(modelName).arg(modelId));
    
    qDebug() << "显示模型:" << modelName << "ID:" << modelId << "图片:" << iconPath;
}

void ModelDeployDialog::onCloseClicked()
{
    close();
}

