/**
 * @file LocationJumpDialog.cpp
 * @brief 经纬度定位跳转对话框实现
 */

#include "LocationJumpDialog.h"
#include <QMessageBox>

LocationJumpDialog::LocationJumpDialog(double longitude, 
                                       double latitude, 
                                       double altitude, 
                                       double range,
                                       QWidget *parent)
    : QDialog(parent)
    , longitudeSpinBox_(nullptr)
    , latitudeSpinBox_(nullptr)
    , altitudeSpinBox_(nullptr)
    , rangeSpinBox_(nullptr)
    , okButton_(nullptr)
    , cancelButton_(nullptr)
{
    setWindowTitle("定位跳转");
    setModal(true);
    resize(350, 200);

    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 创建表单布局
    QFormLayout* formLayout = new QFormLayout;
    formLayout->setSpacing(10);

    // 经度输入框（-180 到 180）
    longitudeSpinBox_ = new QDoubleSpinBox(this);
    longitudeSpinBox_->setRange(-180.0, 180.0);
    longitudeSpinBox_->setDecimals(6);
    longitudeSpinBox_->setSuffix("°");
    longitudeSpinBox_->setValue(longitude); // 使用传入的当前经度作为默认值
    formLayout->addRow("经度 (Longitude):", longitudeSpinBox_);

    // 纬度输入框（-90 到 90）
    latitudeSpinBox_ = new QDoubleSpinBox(this);
    latitudeSpinBox_->setRange(-90.0, 90.0);
    latitudeSpinBox_->setDecimals(6);
    latitudeSpinBox_->setSuffix("°");
    latitudeSpinBox_->setValue(latitude); // 使用传入的当前纬度作为默认值
    formLayout->addRow("纬度 (Latitude):", latitudeSpinBox_);

    // 高度输入框（-10000 到 100000）
    altitudeSpinBox_ = new QDoubleSpinBox(this);
    altitudeSpinBox_->setRange(-10000.0, 100000.0);
    altitudeSpinBox_->setDecimals(2);
    altitudeSpinBox_->setSuffix(" m");
    altitudeSpinBox_->setValue(altitude); // 使用传入的当前高度作为默认值
    formLayout->addRow("高度 (Altitude):", altitudeSpinBox_);

    // 视距输入框（1000 到 100000000）
    rangeSpinBox_ = new QDoubleSpinBox(this);
    rangeSpinBox_->setRange(1000.0, 100000000.0);
    rangeSpinBox_->setDecimals(0);
    rangeSpinBox_->setSuffix(" m");
    rangeSpinBox_->setValue(range); // 使用传入的当前视距作为默认值
    formLayout->addRow("视距 (Range):", rangeSpinBox_);

    mainLayout->addLayout(formLayout);

    // 添加提示标签
    QLabel* hintLabel = new QLabel("提示：输入经纬度坐标，点击确定跳转到指定位置", this);
    hintLabel->setStyleSheet("color: #666; font-size: 10pt;");
    hintLabel->setWordWrap(true);
    mainLayout->addWidget(hintLabel);

    // 添加按钮布局
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();

    okButton_ = new QPushButton("确定", this);
    okButton_->setDefault(true);
    okButton_->setMinimumWidth(80);
    connect(okButton_, &QPushButton::clicked, this, &LocationJumpDialog::onOkClicked);

    cancelButton_ = new QPushButton("取消", this);
    cancelButton_->setMinimumWidth(80);
    connect(cancelButton_, &QPushButton::clicked, this, &LocationJumpDialog::onCancelClicked);

    buttonLayout->addWidget(okButton_);
    buttonLayout->addWidget(cancelButton_);
    mainLayout->addLayout(buttonLayout);
}

double LocationJumpDialog::getLongitude() const
{
    return longitudeSpinBox_ ? longitudeSpinBox_->value() : 0.0;
}

double LocationJumpDialog::getLatitude() const
{
    return latitudeSpinBox_ ? latitudeSpinBox_->value() : 0.0;
}

double LocationJumpDialog::getAltitude() const
{
    return altitudeSpinBox_ ? altitudeSpinBox_->value() : 0.0;
}

double LocationJumpDialog::getRange() const
{
    return rangeSpinBox_ ? rangeSpinBox_->value() : 10000000.0;
}

void LocationJumpDialog::onOkClicked()
{
    // 验证输入值
    double lon = getLongitude();
    double lat = getLatitude();
    
    if (lon < -180.0 || lon > 180.0) {
        QMessageBox::warning(this, "输入错误", "经度必须在 -180 到 180 之间");
        return;
    }
    
    if (lat < -90.0 || lat > 90.0) {
        QMessageBox::warning(this, "输入错误", "纬度必须在 -90 到 90 之间");
        return;
    }
    
    accept();
}

void LocationJumpDialog::onCancelClicked()
{
    reject();
}

