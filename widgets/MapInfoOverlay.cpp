/**
 * @file MapInfoOverlay.cpp
 * @brief 地图信息叠加层组件实现文件
 * 
 * 实现MapInfoOverlay类的所有功能
 */

#include "MapInfoOverlay.h"
#include "../geo/mapstatemanager.h"
#include "../plan/planfilemanager.h"
#include <QDebug>
#include <QtMath>
#include <QResizeEvent>
#include <QFileInfo>
#include <QTimer>
#include <QShowEvent>
#include <QMouseEvent>
#include <QCoreApplication>
#include <QPaintEvent>
#include <QPainter>

// 指北针widget类实现
class CompassWidget : public QWidget
{
public:
    explicit CompassWidget(QWidget* parent = nullptr) : QWidget(parent), heading_(0.0) {}
    void setHeading(double heading) {
        if (heading_ != heading) {
            heading_ = heading;
            update();
        }
    }
protected:
    void paintEvent(QPaintEvent* event) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        int compassSize = 80;
        int margin = 10;
        QPoint compassCenter(compassSize / 2 + margin, compassSize / 2 + margin);
        drawCompass(painter, compassCenter, compassSize / 2);
    }
private:
    double heading_;
    void drawCompass(QPainter& painter, const QPoint& center, int radius);
};

// 比例尺widget类实现
class ScaleWidget : public QWidget
{
public:
    explicit ScaleWidget(QWidget* parent = nullptr) : QWidget(parent), scaleRange_(0.0) {}
    void setScaleRange(double range) {
        if (scaleRange_ != range) {
            scaleRange_ = range;
            update();
        }
    }
protected:
    void paintEvent(QPaintEvent* event) override {
        if (scaleRange_ <= 0) {
            return;
        }
        
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        
        QPoint scalePos(10, 10);
        drawScaleBar(painter, scalePos);
    }
private:
    double scaleRange_;
    void drawScaleBar(QPainter& painter, const QPoint& pos);
    QString formatDistance(double distanceMeters);
};


MapInfoOverlay::MapInfoOverlay(QWidget *parent)
    : QWidget(parent)
    , mouseCoordLabel_(nullptr)
    , headingLabel_(nullptr)
    , pitchLabel_(nullptr)
    , rangeLabel_(nullptr)
    , showCompass_(true)
    , compassHeading_(0.0)
    , showScale_(true)
    , scaleRange_(0.0)
    , mapStateManager_(nullptr)
    , planFileManager_(nullptr)
    , infoPanel_(nullptr)
    , compassWidget_(nullptr)
    , scaleWidget_(nullptr)
{
    // MapInfoOverlay本身不再覆盖整个窗口，只用来管理子widget
    setAttribute(Qt::WA_TransparentForMouseEvents, true);
    setFocusPolicy(Qt::NoFocus);
    hide();  // 本身不显示
    
    setupUI();
}

MapInfoOverlay::~MapInfoOverlay()
{
}

void MapInfoOverlay::setupUI()
{
    // 创建自定义的信息面板widget，在paintEvent中绘制半透明背景
    // 注意：parent设置为nullptr，稍后在OsgMapWidget中设置正确的parent
    class InfoPanelWidget : public QWidget {
    public:
        InfoPanelWidget(QWidget* parent = nullptr) : QWidget(parent) {
            setAttribute(Qt::WA_TranslucentBackground, true);
            setAutoFillBackground(false);
        }
    protected:
        void paintEvent(QPaintEvent* event) override {
            QPainter painter(this);
            painter.setRenderHint(QPainter::Antialiasing);
            
            // 绘制半透明圆角矩形背景
            QRect bgRect = rect();
            painter.setBrush(QBrush(QColor(0, 0, 0, 180)));
            painter.setPen(QPen(QColor(255, 255, 255, 200), 1));
            painter.drawRoundedRect(bgRect, 8, 8);
            
            QWidget::paintEvent(event);
        }
    };
    
    // 创建信息面板
    InfoPanelWidget* infoPanel = new InfoPanelWidget(nullptr);
    infoPanel->setStyleSheet(
        "QLabel {"
        "   color: #FFFFFF;"
        "   background: transparent;"
        "   border: none;"
        "   font-size: 10pt;"
        "   font-family: 'Microsoft YaHei', 'SimSun', sans-serif;"
        "   padding: 2px 5px;"
        "}"
    );
    QHBoxLayout* infoLayout = new QHBoxLayout(infoPanel);
    infoLayout->setSpacing(8);
    infoLayout->setContentsMargins(8, 5, 8, 5);
    
    // 鼠标坐标
    mouseCoordLabel_ = new QLabel("鼠标: 0°E, 0°N", infoPanel);
    mouseCoordLabel_->setStyleSheet(
        "color: #90EE90; "
        "font-weight: bold; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0, 0, 0, 200);"
    );
    infoLayout->addWidget(mouseCoordLabel_);
    
    // 分隔符
    QLabel* separator1 = new QLabel("|", infoPanel);
    separator1->setStyleSheet("color: rgba(255, 255, 255, 150); background: transparent;");
    infoLayout->addWidget(separator1);
    
    // 航向角
    headingLabel_ = new QLabel("航向: 0°", infoPanel);
    headingLabel_->setStyleSheet(
        "color: #FFFFFF; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0, 0, 0, 200);"
    );
    infoLayout->addWidget(headingLabel_);
    
    // 俯仰角
    pitchLabel_ = new QLabel("俯仰: 0°", infoPanel);
    pitchLabel_->setStyleSheet(
        "color: #FFFFFF; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0, 0, 0, 200);"
    );
    infoLayout->addWidget(pitchLabel_);
    
    // 距离
    rangeLabel_ = new QLabel("距离: 0m", infoPanel);
    rangeLabel_->setStyleSheet(
        "color: #FFFFFF; "
        "background: transparent; "
        "text-shadow: 1px 1px 2px rgba(0, 0, 0, 200);"
    );
    infoLayout->addWidget(rangeLabel_);
    
    infoPanel->resize(600, 35);  // 一行显示，调整大小
    infoPanel_ = infoPanel;  // 保存引用
    
    // 创建指北针widget
    CompassWidget* compass = new CompassWidget(nullptr);
    compass->setAttribute(Qt::WA_TranslucentBackground, true);
    compass->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    compass->setFocusPolicy(Qt::NoFocus);
    compass->resize(100, 100);
    compassWidget_ = compass;
    
    // 创建比例尺widget
    ScaleWidget* scale = new ScaleWidget(nullptr);
    scale->setAttribute(Qt::WA_TranslucentBackground, true);
    scale->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    scale->setFocusPolicy(Qt::NoFocus);
    scale->resize(200, 50);
    scaleWidget_ = scale;
    
    // 注意：位置更新在OsgMapWidget中完成，这里不设置位置
}

void MapInfoOverlay::setMapStateManager(MapStateManager* mapStateManager)
{
    mapStateManager_ = mapStateManager;
    if (mapStateManager_) {
        // 连接信号
        connect(mapStateManager_, &MapStateManager::mousePositionChanged,
                this, &MapInfoOverlay::updateMouseCoordinates);
        connect(mapStateManager_, &MapStateManager::stateChanged,
                this, &MapInfoOverlay::updateAllInfo);
    }
}

void MapInfoOverlay::setPlanFileManager(PlanFileManager* planFileManager)
{
    planFileManager_ = planFileManager;
    // 方案名称已移到工具栏，这里不再处理
}

void MapInfoOverlay::updateMouseCoordinates(double longitude, double latitude, double altitude)
{
    // 移除高度显示，只显示经纬度
    QString text = QString("鼠标: %1°E, %2°N")
                   .arg(longitude, 0, 'f', 5)
                   .arg(latitude, 0, 'f', 5);
    if (mouseCoordLabel_) {
        mouseCoordLabel_->setText(text);
    }
}

void MapInfoOverlay::updateCameraParameters(double heading, double pitch, double range)
{
    compassHeading_ = heading;
    scaleRange_ = range;
    
    if (headingLabel_) {
        headingLabel_->setText(QString("航向: %1°").arg(heading, 0, 'f', 1));
    }
    if (pitchLabel_) {
        pitchLabel_->setText(QString("俯仰: %1°").arg(pitch, 0, 'f', 1));
    }
    if (rangeLabel_) {
        // 格式化距离显示
        QString distanceText;
        if (range >= 1000.0) {
            distanceText = QString("%1 km").arg(range / 1000.0, 0, 'f', 1);
        } else {
            distanceText = QString("%1 m").arg(range, 0, 'f', 0);
        }
        rangeLabel_->setText(QString("距离: %1").arg(distanceText));
    }
    
    // 更新指北针和比例尺widget
    if (compassWidget_ && showCompass_) {
        // 使用static_cast，因为我们知道compassWidget_确实指向CompassWidget
        CompassWidget* compass = static_cast<CompassWidget*>(compassWidget_);
        compass->setHeading(heading);
    }
    if (scaleWidget_ && showScale_) {
        // 使用static_cast，因为我们知道scaleWidget_确实指向ScaleWidget
        ScaleWidget* scale = static_cast<ScaleWidget*>(scaleWidget_);
        scale->setScaleRange(range);
    }
}


void MapInfoOverlay::updateAllInfo()
{
    if (!mapStateManager_) {
        return;
    }
    
    const auto& state = mapStateManager_->getCurrentState();
    updateMouseCoordinates(state.mouseLongitude, state.mouseLatitude, state.mouseAltitude);
    updateCameraParameters(state.heading, state.pitch, state.range);
}


bool MapInfoOverlay::event(QEvent* event)
{
    // MapInfoOverlay本身不显示且设置了WA_TransparentForMouseEvents
    // 所有事件都应该传递到下层，由OsgMapWidget处理
    // 信息面板、指北针、比例尺都有自己的事件处理
    return QWidget::event(event);
}


void MapInfoOverlay::updateOverlayWidgetsPosition(int parentWidth, int parentHeight)
{
    // 更新信息面板位置（右下角）
    if (infoPanel_ && infoPanel_->parentWidget()) {
        // 确保widget已经有正确的parent
        int panelHeight = 35;
        int margin = 15;
        int x = parentWidth - 600 - margin;
        int y = parentHeight - panelHeight - margin;
        infoPanel_->setGeometry(x, y, 600, panelHeight);
    }
    
    // 更新指北针widget位置（右上角）
    if (compassWidget_ && compassWidget_->parentWidget()) {
        int compassSize = 100;
        int margin = 20;
        int x = parentWidth - compassSize - margin;
        int y = margin;
        compassWidget_->setGeometry(x, y, compassSize, compassSize);
        compassWidget_->update();  // 触发重绘
    }
    
    // 更新比例尺widget位置（左下角）
    if (scaleWidget_ && scaleWidget_->parentWidget()) {
        int scaleWidth = 200;
        int scaleHeight = 50;
        int margin = 20;
        int x = margin;
        int y = parentHeight - scaleHeight - margin;
        scaleWidget_->setGeometry(x, y, scaleWidth, scaleHeight);
        scaleWidget_->update();  // 触发重绘
    }
}

// CompassWidget的drawCompass实现
void CompassWidget::drawCompass(QPainter& painter, const QPoint& center, int radius)
{
    painter.save();
    
    // 绘制外圈（透明背景，只绘制边框）
    QPen pen(QColor(255, 255, 255, 200), 2);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);  // 透明背景
    painter.drawEllipse(center, radius, radius);
    
    // 绘制内圈（半透明蓝色背景）
    painter.setBrush(QBrush(QColor(100, 150, 255, 120)));  // 降低不透明度
    painter.drawEllipse(center, radius - 5, radius - 5);
    
    // 绘制指北针箭头（根据航向角旋转）
    painter.translate(center);
    painter.rotate(-heading_); // 负号是因为屏幕坐标系Y轴向下
    
    // 绘制N标记（指向正北）
    painter.setPen(QPen(QColor(255, 0, 0), 3));
    QPointF northPoint(0, -radius + 10);
    painter.drawLine(QPointF(0, 0), northPoint);
    
    // 绘制箭头
    QPolygonF arrow;
    arrow << QPointF(0, -radius + 10)
          << QPointF(-5, -radius + 20)
          << QPointF(0, -radius + 15)
          << QPointF(5, -radius + 20);
    painter.setBrush(QBrush(QColor(255, 0, 0)));
    painter.drawPolygon(arrow);
    
    // 绘制N文字
    painter.setPen(QPen(QColor(255, 255, 255), 2));
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(QRectF(-10, -radius + 5, 20, 15), Qt::AlignCenter, "N");
    
    painter.restore();  // 恢复变换矩阵
    
    // 绘制方向标记（东南西北）
    painter.setPen(QPen(QColor(255, 255, 255, 180), 1));
    painter.setFont(QFont("Arial", 8));
    
    QString directions[] = {"N", "E", "S", "W"};
    double angles[] = {0.0, 90.0, 180.0, 270.0};
    
    for (int i = 0; i < 4; i++) {
        double dirAngleRad = qDegreesToRadians(angles[i] - heading_);
        double dirX = center.x() + (radius - 15) * qSin(dirAngleRad);
        double dirY = center.y() - (radius - 15) * qCos(dirAngleRad);
        
        painter.drawText(QRectF(dirX - 10, dirY - 7, 20, 14), Qt::AlignCenter, directions[i]);
    }
}

// ScaleWidget的drawScaleBar实现
void ScaleWidget::drawScaleBar(QPainter& painter, const QPoint& pos)
{
    // 计算比例尺长度（像素）
    double scaleLengthMeters = scaleRange_ * 0.1;  // 比例尺长度约为视距的10%
    if (scaleLengthMeters < 100) {
        scaleLengthMeters = 100;
    } else if (scaleLengthMeters > 100000) {
        scaleLengthMeters = 100000;
    }
    
    // 简化：假设1像素 = 1米（实际应该根据地图投影计算）
    double scaleLengthPx = scaleLengthMeters / 100.0;  // 粗略估算
    
    // 绘制比例尺线条（透明背景）
    painter.setPen(QPen(QColor(255, 255, 255, 220), 2));
    painter.setBrush(Qt::NoBrush);  // 透明背景
    
    int barHeight = 8;
    int barWidth = qMin(int(scaleLengthPx), 150);
    
    // 只绘制线条，不绘制填充矩形
    QRect barRect(pos.x(), pos.y(), barWidth, barHeight);
    painter.drawRect(barRect);
    
    // 绘制刻度
    painter.setPen(QPen(QColor(255, 255, 255, 220), 1));
    painter.drawLine(pos.x(), pos.y(), pos.x(), pos.y() + barHeight + 5);
    painter.drawLine(pos.x() + barWidth, pos.y(), pos.x() + barWidth, pos.y() + barHeight + 5);
    
    // 绘制文字
    QString distanceText = formatDistance(scaleLengthMeters);
    QFontMetrics fm(QFont("Arial", 9));
    QRect textBoundingRect = fm.boundingRect(distanceText);
    
    int textMargin = 10;
    QRect textRect(pos.x(), pos.y() + barHeight + 8, 
                   qMax(barWidth + textMargin, textBoundingRect.width() + textMargin * 2), 
                   textBoundingRect.height() + textMargin);
    
    // 绘制半透明背景
    painter.fillRect(textRect, QColor(0, 0, 0, 120));
    
    // 绘制文字
    painter.setPen(QPen(QColor(255, 255, 255, 255), 1));
    painter.setFont(QFont("Arial", 9));
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignTop, distanceText);
}

QString ScaleWidget::formatDistance(double distanceMeters)
{
    if (distanceMeters >= 1000.0) {
        return QString("%1 km").arg(distanceMeters / 1000.0, 0, 'f', 1);
    } else {
        return QString("%1 m").arg(distanceMeters, 0, 'f', 0);
    }
}
