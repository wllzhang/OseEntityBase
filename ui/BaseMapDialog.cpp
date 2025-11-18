/**
 * @file BaseMapDialog.cpp
 * @brief 底图管理对话框实现文件
 * 
 * 实现BaseMapDialog类的所有功能
 */

#include "BaseMapDialog.h"
#include "../geo/basemapmanager.h"
#include <QDebug>
#include <QMessageBox>
#include <QInputDialog>
#include <QHeaderView>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>
#include <QLabel>
#include <QDir>
#include <QFileDialog>
#include <QCompleter>
#include <QFileSystemModel>
#include <QHBoxLayout>

BaseMapDialog::BaseMapDialog(BaseMapManager* baseMapManager, QWidget *parent)
    : QDialog(parent)
    , baseMapManager_(baseMapManager)
    , treeWidget_(nullptr)
    , addButton_(nullptr)
    , addFromTemplateButton_(nullptr)
    , editButton_(nullptr)
    , deleteButton_(nullptr)
{
    if (!baseMapManager_) {
        qDebug() << "BaseMapDialog: BaseMapManager为空";
    }
    
    setupUI();
    updateBaseMapList();
    
    // 连接信号
    if (baseMapManager_) {
        connect(baseMapManager_, &BaseMapManager::baseMapAdded,
                this, &BaseMapDialog::onBaseMapAdded);
        connect(baseMapManager_, &BaseMapManager::baseMapRemoved,
                this, &BaseMapDialog::onBaseMapRemoved);
        connect(baseMapManager_, &BaseMapManager::baseMapUpdated,
                this, &BaseMapDialog::onBaseMapUpdated);
    }
}

BaseMapDialog::~BaseMapDialog()
{
}

void BaseMapDialog::setupUI()
{
    setWindowTitle("底图管理");
    setMinimumSize(600, 500);
    resize(700, 600);
    
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(15, 15, 15, 15);
    
    // 说明标签
    QLabel* infoLabel = new QLabel("底图图层列表（支持多图层叠加显示）", this);
    infoLabel->setStyleSheet("font-size: 12px; color: #666; padding: 5px;");
    mainLayout->addWidget(infoLabel);
    
    // 底图树控件
    treeWidget_ = new QTreeWidget(this);
    treeWidget_->setHeaderLabels(QStringList() << "名称" << "可见" << "透明度" << "驱动" << "URL");
    treeWidget_->setRootIsDecorated(false);
    treeWidget_->setAlternatingRowColors(true);
    treeWidget_->header()->setStretchLastSection(true);
    connect(treeWidget_, &QTreeWidget::itemChanged,
            this, &BaseMapDialog::onItemChanged);
    mainLayout->addWidget(treeWidget_);
    
    // 按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    
    addButton_ = new QPushButton("添加", this);
    addFromTemplateButton_ = new QPushButton("从模板添加", this);
    editButton_ = new QPushButton("编辑", this);
    deleteButton_ = new QPushButton("删除", this);
    
    connect(addButton_, &QPushButton::clicked, this, &BaseMapDialog::onAddClicked);
    connect(addFromTemplateButton_, &QPushButton::clicked, this, &BaseMapDialog::onAddFromTemplateClicked);
    connect(editButton_, &QPushButton::clicked, this, &BaseMapDialog::onEditClicked);
    connect(deleteButton_, &QPushButton::clicked, this, &BaseMapDialog::onDeleteClicked);
    
    buttonLayout->addWidget(addButton_);
    buttonLayout->addWidget(addFromTemplateButton_);
    buttonLayout->addWidget(editButton_);
    buttonLayout->addWidget(deleteButton_);
    buttonLayout->addStretch();
    
    QPushButton* okButton = new QPushButton("确定", this);
    QPushButton* cancelButton = new QPushButton("取消", this);
    
    connect(okButton, &QPushButton::clicked, this, &BaseMapDialog::onOkClicked);
    connect(cancelButton, &QPushButton::clicked, this, &BaseMapDialog::onCancelClicked);
    
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    
    mainLayout->addLayout(buttonLayout);
}

void BaseMapDialog::updateBaseMapList()
{
    if (!baseMapManager_ || !treeWidget_) {
        return;
    }
    
    treeWidget_->clear();
    
    QList<QPair<QString, BaseMapSource>> loadedMaps = baseMapManager_->getLoadedBaseMaps();
    for (const auto& pair : loadedMaps) {
        createBaseMapItem(pair.first, pair.second);
    }
    
    // 调整列宽
    treeWidget_->resizeColumnToContents(0);
    treeWidget_->resizeColumnToContents(1);
    treeWidget_->resizeColumnToContents(2);
}

QTreeWidgetItem* BaseMapDialog::createBaseMapItem(const QString& name, const BaseMapSource& config)
{
    QTreeWidgetItem* item = new QTreeWidgetItem(treeWidget_);
    item->setText(0, name);
    item->setData(0, Qt::UserRole, name);
    
    // 可见性复选框
    QCheckBox* visibleCheckBox = new QCheckBox(treeWidget_);
    visibleCheckBox->setChecked(config.visible);
    visibleCheckBox->setProperty("mapName", name);
    connect(visibleCheckBox, &QCheckBox::toggled, this, [this, name](bool checked) {
        if (baseMapManager_) {
            baseMapManager_->setBaseMapVisible(name, checked);
        }
    });
    treeWidget_->setItemWidget(item, 1, visibleCheckBox);
    
    // 透明度滑块
    QSlider* opacitySlider = new QSlider(Qt::Horizontal, treeWidget_);
    opacitySlider->setRange(0, 100);
    opacitySlider->setValue(config.opacity);
    opacitySlider->setProperty("mapName", name);
    connect(opacitySlider, &QSlider::valueChanged, this, [this, name](int value) {
        if (baseMapManager_) {
            baseMapManager_->setBaseMapOpacity(name, value);
        }
        // 不显示透明度文字
    });
    treeWidget_->setItemWidget(item, 2, opacitySlider);
    // 透明度列不显示文字，只显示滑块
    item->setText(2, "");
    
    item->setText(3, config.driver);
    item->setText(4, config.url);
    
    return item;
}

QString BaseMapDialog::getSelectedBaseMapName() const
{
    if (!treeWidget_) {
        return QString();
    }
    
    QTreeWidgetItem* item = treeWidget_->currentItem();
    if (item) {
        return item->data(0, Qt::UserRole).toString();
    }
    
    return QString();
}

BaseMapSource BaseMapDialog::showConfigDialog(const BaseMapSource& source)
{
    QDialog dialog(this);
    dialog.setWindowTitle(source.name.isEmpty() ? "添加底图" : "编辑底图");
    dialog.setMinimumSize(550, 350);
    
    QFormLayout* formLayout = new QFormLayout(&dialog);
    
    QLineEdit* nameEdit = new QLineEdit(&dialog);
    nameEdit->setText(source.name);
    formLayout->addRow("名称:", nameEdit);
    
    QComboBox* driverCombo = new QComboBox(&dialog);
    driverCombo->addItems(QStringList() << "xyz" << "gdal");
    driverCombo->setCurrentText(source.driver);
    formLayout->addRow("驱动类型:", driverCombo);
    
    // URL输入框，带文件选择按钮
    QHBoxLayout* urlLayout = new QHBoxLayout();
    QLineEdit* urlEdit = new QLineEdit(&dialog);
    urlEdit->setText(source.url);
    urlEdit->setMinimumWidth(350);
    QPushButton* browseButton = new QPushButton("浏览...", &dialog);
    connect(browseButton, &QPushButton::clicked, [urlEdit, driverCombo, &dialog]() {
        QString fileName;
        if (driverCombo->currentText() == "gdal") {
            // GDAL支持多种格式，使用通用文件选择
            fileName = QFileDialog::getOpenFileName(&dialog, "选择本地文件", 
                QDir::currentPath(), 
                "所有支持的文件 (*.tif *.tiff *.img *.jpg *.jpeg *.png *.gdal);;"
                "TIFF文件 (*.tif *.tiff);;IMG文件 (*.img);;图片文件 (*.jpg *.jpeg *.png);;所有文件 (*.*)");
        } else {
            // XYZ驱动通常用于网络瓦片服务，但也可以选择本地文件
            fileName = QFileDialog::getOpenFileName(&dialog, "选择本地文件", 
                QDir::currentPath(), "所有文件 (*.*)");
        }
        if (!fileName.isEmpty()) {
            urlEdit->setText(fileName);
        }
    });
    urlLayout->addWidget(urlEdit);
    urlLayout->addWidget(browseButton);
    QWidget* urlWidget = new QWidget(&dialog);
    urlWidget->setLayout(urlLayout);
    formLayout->addRow("URL:", urlWidget);
    
    // 投影配置下拉框
    QComboBox* profileCombo = new QComboBox(&dialog);
    profileCombo->setEditable(true); // 允许自定义输入
    profileCombo->addItems(QStringList() 
        << "spherical-mercator" 
        << "wgs84" 
        << "geodetic"
        << "mercator"
        << "plate-carre"
        << "");
    profileCombo->setCurrentText(source.profile);
    formLayout->addRow("投影配置:", profileCombo);
    
    QCheckBox* cacheCheckBox = new QCheckBox(&dialog);
    cacheCheckBox->setChecked(source.cacheEnabled);
    formLayout->addRow("启用缓存:", cacheCheckBox);
    
    QSpinBox* opacitySpinBox = new QSpinBox(&dialog);
    opacitySpinBox->setRange(0, 100);
    opacitySpinBox->setValue(source.opacity);
    opacitySpinBox->setSuffix("%");
    formLayout->addRow("透明度:", opacitySpinBox);
    
    QDialogButtonBox* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    formLayout->addRow(buttonBox);
    
    if (dialog.exec() != QDialog::Accepted) {
        return BaseMapSource(); // 返回空配置表示取消
    }
    
    BaseMapSource result;
    result.name = nameEdit->text().trimmed();
    result.driver = driverCombo->currentText();
    result.url = urlEdit->text().trimmed();
    result.profile = profileCombo->currentText().trimmed();
    result.cacheEnabled = cacheCheckBox->isChecked();
    result.opacity = opacitySpinBox->value();
    result.visible = true;
    
    return result;
}

void BaseMapDialog::onAddClicked()
{
    BaseMapSource newSource = showConfigDialog();
    if (newSource.name.isEmpty()) {
        return; // 用户取消
    }
    
    if (baseMapManager_->hasBaseMap(newSource.name)) {
        QMessageBox::warning(this, "添加底图", "底图名称已存在：" + newSource.name);
        return;
    }
    
    if (newSource.driver.isEmpty() || newSource.url.isEmpty()) {
        QMessageBox::warning(this, "添加底图", "驱动类型和URL不能为空");
        return;
    }
    
    if (baseMapManager_->addBaseMapLayer(newSource)) {
        QMessageBox::information(this, "添加底图", "底图添加成功：" + newSource.name);
    } else {
        QMessageBox::warning(this, "添加底图", "底图添加失败");
    }
}

void BaseMapDialog::onAddFromTemplateClicked()
{
    if (!baseMapManager_) {
        return;
    }
    
    QList<BaseMapSource> templates = baseMapManager_->getAvailableBaseMapTemplates();
    if (templates.isEmpty()) {
        QMessageBox::information(this, "从模板添加", "没有可用的底图模板");
        return;
    }
    
    QStringList templateNames;
    for (const auto& t : templates) {
        templateNames << t.name;
    }
    
    bool ok;
    QString selected = QInputDialog::getItem(this, "从模板添加", "选择底图模板:",
                                            templateNames, 0, false, &ok);
    if (!ok || selected.isEmpty()) {
        return;
    }
    
    // 查找选中的模板
    BaseMapSource templateSource;
    for (const auto& t : templates) {
        if (t.name == selected) {
            templateSource = t;
            break;
        }
    }
    
    if (templateSource.name.isEmpty()) {
        return;
    }
    
    // 让用户输入名称（可能已存在同名底图）
    QString newName = QInputDialog::getText(this, "从模板添加", "请输入底图名称:",
                                            QLineEdit::Normal, templateSource.name, &ok);
    if (!ok || newName.trimmed().isEmpty()) {
        return;
    }
    
    newName = newName.trimmed();
    if (baseMapManager_->hasBaseMap(newName)) {
        QMessageBox::warning(this, "从模板添加", "底图名称已存在：" + newName);
        return;
    }
    
    templateSource.name = newName;
    if (baseMapManager_->addBaseMapLayer(templateSource)) {
        QMessageBox::information(this, "从模板添加", "底图添加成功：" + newName);
    } else {
        QMessageBox::warning(this, "从模板添加", "底图添加失败");
    }
}

void BaseMapDialog::onEditClicked()
{
    QString mapName = getSelectedBaseMapName();
    if (mapName.isEmpty()) {
        QMessageBox::information(this, "编辑底图", "请先选择一个底图");
        return;
    }
    
    BaseMapSource currentConfig = baseMapManager_->getBaseMapConfig(mapName);
    BaseMapSource newConfig = showConfigDialog(currentConfig);
    if (newConfig.name.isEmpty()) {
        return; // 用户取消
    }
    
    if (newConfig.driver.isEmpty() || newConfig.url.isEmpty()) {
        QMessageBox::warning(this, "编辑底图", "驱动类型和URL不能为空");
        return;
    }
    
    // 如果名称改变且新名称已存在，提示错误
    if (mapName != newConfig.name && baseMapManager_->hasBaseMap(newConfig.name)) {
        QMessageBox::warning(this, "编辑底图", "新名称已存在：" + newConfig.name);
        return;
    }
    
    if (baseMapManager_->updateBaseMapLayer(mapName, newConfig)) {
        QMessageBox::information(this, "编辑底图", "底图更新成功");
    } else {
        QMessageBox::warning(this, "编辑底图", "底图更新失败");
    }
}

void BaseMapDialog::onDeleteClicked()
{
    QString mapName = getSelectedBaseMapName();
    if (mapName.isEmpty()) {
        QMessageBox::information(this, "删除底图", "请先选择一个底图");
        return;
    }
    
    int ret = QMessageBox::question(this, "删除底图", 
                                    "确定要删除底图 \"" + mapName + "\" 吗？",
                                    QMessageBox::Yes | QMessageBox::No);
    if (ret == QMessageBox::Yes) {
        if (baseMapManager_->removeBaseMapLayer(mapName)) {
            QMessageBox::information(this, "删除底图", "底图删除成功");
        } else {
            QMessageBox::warning(this, "删除底图", "底图删除失败");
        }
    }
}

void BaseMapDialog::onOkClicked()
{
    // 保存配置
    if (baseMapManager_) {
        QString configPath = QDir::currentPath() + "/basemap_config.json";
        baseMapManager_->saveConfig(configPath);
        qDebug() << "BaseMapDialog: 配置已保存";
    }
    accept();
}

void BaseMapDialog::onCancelClicked()
{
    reject();
}

void BaseMapDialog::onItemChanged(QTreeWidgetItem* item, int column)
{
    // 此方法用于处理树项的文本变化，但可见性和透明度已通过控件直接处理
    Q_UNUSED(item);
    Q_UNUSED(column);
}

void BaseMapDialog::onBaseMapAdded(const QString& mapName)
{
    Q_UNUSED(mapName);
    updateBaseMapList();
}

void BaseMapDialog::onBaseMapRemoved(const QString& mapName)
{
    Q_UNUSED(mapName);
    updateBaseMapList();
}

void BaseMapDialog::onBaseMapUpdated(const QString& mapName)
{
    Q_UNUSED(mapName);
    updateBaseMapList();
}
