/**
 * @file LocationJumpDialog.h
 * @brief 经纬度定位跳转对话框
 * 
 * 提供输入经纬度并跳转到指定位置的功能
 */

#ifndef LOCATIONJUMPDIALOG_H
#define LOCATIONJUMPDIALOG_H

#include <QDialog>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

/**
 * @brief 经纬度定位跳转对话框
 * 
 * 允许用户输入经纬度坐标，并跳转到指定位置
 */
class LocationJumpDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param longitude 当前经度（作为默认值）
     * @param latitude 当前纬度（作为默认值）
     * @param altitude 当前高度（作为默认值）
     * @param range 当前视距（作为默认值）
     * @param parent 父窗口
     */
    explicit LocationJumpDialog(double longitude = 116.3974, 
                                double latitude = 39.9093, 
                                double altitude = 0.0, 
                                double range = 10000000.0,
                                QWidget *parent = nullptr);

    /**
     * @brief 获取输入的经度
     * @return 经度值（-180 到 180）
     */
    double getLongitude() const;

    /**
     * @brief 获取输入的纬度
     * @return 纬度值（-90 到 90）
     */
    double getLatitude() const;

    /**
     * @brief 获取输入的高度
     * @return 高度值（米）
     */
    double getAltitude() const;

    /**
     * @brief 获取输入的视距
     * @return 视距值（米）
     */
    double getRange() const;

private slots:
    /**
     * @brief 确定按钮点击槽函数
     */
    void onOkClicked();

    /**
     * @brief 取消按钮点击槽函数
     */
    void onCancelClicked();

private:
    QDoubleSpinBox* longitudeSpinBox_;  // 经度输入框
    QDoubleSpinBox* latitudeSpinBox_;   // 纬度输入框
    QDoubleSpinBox* altitudeSpinBox_;   // 高度输入框
    QDoubleSpinBox* rangeSpinBox_;      // 视距输入框
    
    QPushButton* okButton_;             // 确定按钮
    QPushButton* cancelButton_;         // 取消按钮
};

#endif // LOCATIONJUMPDIALOG_H

