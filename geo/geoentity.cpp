/**
 * @file geoentity.cpp
 * @brief 通用地理实体实现文件
 * 
 * 实现GeoEntity类的所有功能
 */

#include "geoentity.h"
#include "geoutils.h"
#include <QDebug>
#include <QColor>

GeoEntity::GeoEntity(const QString& id, const QString& name, const QString& type,
                     double longitude, double latitude, double altitude, QObject* parent)
    : QObject(parent)
    , entityId_(id)
    , entityName_(name)
    , entityType_(type)
    , longitude_(longitude)
    , latitude_(latitude)
    , altitude_(altitude)
    , heading_(0.0)
    , visible_(true)
    , selected_(false)
{
    // 设置默认属性
    setProperty("size", 100.0);
    setProperty("color", QColor(255, 255, 255));
    setProperty("opacity", 1.0);
}

void GeoEntity::setPosition(double longitude, double latitude, double altitude)
{
    longitude_ = longitude;
    latitude_ = latitude;
    altitude_ = altitude;
    
    // 自我更新渲染节点
    updateNode();
    
    // 发出信号
    emit positionChanged(longitude, latitude, altitude);
}

void GeoEntity::getPosition(double& longitude, double& latitude, double& altitude) const
{
    longitude = longitude_;
    latitude = latitude_;
    altitude = altitude_;
}

void GeoEntity::setHeading(double headingDegrees)
{
    heading_ = headingDegrees;
    
    // 自我更新渲染节点
    updateNode();
    
    // 发出信号
    emit headingChanged(heading_);
}

void GeoEntity::setVisible(bool visible)
{
    visible_ = visible;
    
    // 自我更新渲染节点
    if (node_) {
        node_->setNodeMask(visible ? 0xffffffff : 0x0);
    }
    
    // 发出信号
    emit visibilityChanged(visible);
}

void GeoEntity::setSelected(bool selected)
{
    selected_ = selected;
    
    // 自我更新渲染节点（比如改变颜色或边框）
    updateNode();
    
    // 发出信号
    emit selectionChanged(selected);
}

void GeoEntity::setProperty(const QString& key, const QVariant& value)
{
    properties_[key] = value;
    
    // 自我更新渲染节点
    updateNode();
    
    // 发出信号
    emit propertyChanged(key, value);
}

QVariant GeoEntity::getProperty(const QString& key) const
{
    return properties_.value(key, QVariant());
}

/**
 * @brief 初始化实体资源与节点
 * 
 * 默认实现流程：
 * 1. 调用 createNode() 创建渲染节点（由子类实现）
 * 2. 如果节点创建成功，设置初始可见性和选中状态
 * 3. 调用 setupNodeTransform() 设置节点的初始位置和旋转
 * 4. 调用 onInitialized() 回调，供子类扩展特定初始化逻辑
 * 
 * @note 子类通常只需实现 createNode() 和 onInitialized()，
 *       无需重写此方法，除非需要完全自定义初始化流程。
 */
void GeoEntity::initialize()
{
    // 创建渲染节点
    node_ = createNode();
    
    if (node_) {
        // 设置初始状态
        setVisible(true);
        setSelected(false);
        
        // 设置节点的初始变换（如果是PAT节点）
        setupNodeTransform(node_.get());
    }
    
    // 调用子类特定的初始化回调
    onInitialized();
}

/**
 * @brief 根据业务状态更新实体
 * 
 * 默认实现流程：
 * 1. 调用 updateNode() 更新节点的位置和旋转变换
 * 2. 调用 onUpdated() 回调，供子类扩展特定更新逻辑（如更新标签）
 * 
 * @note 子类通常只需重写 onUpdated() 来处理特定更新需求，
 *       无需重写此方法，除非需要完全自定义更新流程。
 */
void GeoEntity::update()
{
    // 更新节点变换
    updateNode();
    
    // 调用子类特定的更新回调
    onUpdated();
}

/**
 * @brief 释放资源与场景引用
 * 
 * 默认实现流程：
 * 1. 调用 onBeforeCleanup() 回调，供子类清理特定资源（如高亮节点、子节点）
 * 2. 清除节点引用（node_ = nullptr）
 * 3. 调用 onAfterCleanup() 回调，供子类做额外的清理工作
 * 
 * @note 子类通常只需重写 onBeforeCleanup() 和 onAfterCleanup()，
 *       无需重写此方法，除非需要完全自定义清理流程。
 * 
 * @warning 清理后节点将从场景图中移除，不应再使用节点引用。
 */
void GeoEntity::cleanup()
{
    // 子类清理特定资源（如高亮节点）
    onBeforeCleanup();
    
    // 清除节点引用
    node_ = nullptr;
    
    // 子类额外清理
    onAfterCleanup();
}

/**
 * @brief 根据当前状态刷新渲染节点
 * 
 * 更新节点的位置和旋转变换，使其与实体的地理位置和航向角同步。
 * 如果节点是 PositionAttitudeTransform 类型，则更新其位置和旋转属性。
 * 
 * @note 此方法在位置、航向或属性变化时自动调用。
 *       子类通常不需要直接调用此方法，除非需要强制刷新。
 */
void GeoEntity::updateNode()
{
    if (!node_) return;
    
    // 更新PositionAttitudeTransform的位置和旋转
    setupNodeTransform(node_.get());
}

/**
 * @brief 设置节点的位置和旋转变换
 * 
 * 如果节点是 PositionAttitudeTransform 类型，则：
 * 1. 将实体的地理坐标（经纬度、高度）转换为 OSG 世界坐标
 * 2. 根据航向角（heading_）计算旋转四元数（绕Z轴旋转）
 * 3. 设置 PAT 节点的位置和姿态
 * 
 * @param node 要设置的节点（如果是 PAT 则设置，否则忽略）
 * 
 * @note 此方法通常在 createNode() 后调用以设置初始变换，
 *       或在位置/航向变化时调用以更新变换。
 *       子类创建的节点如果是 PAT 类型，应使用此方法设置变换。
 */
void GeoEntity::setupNodeTransform(osg::Node* node)
{
    if (!node) return;
    
    osg::PositionAttitudeTransform* pat = dynamic_cast<osg::PositionAttitudeTransform*>(node);
    if (pat) {
        // 更新位置：使用工具函数进行地理坐标到世界坐标的转换
        osg::Vec3d worldPos = GeoUtils::geoToWorldCoordinates(longitude_, latitude_, altitude_);
        pat->setPosition(worldPos);
        
        // 更新旋转：将航向角（度）转换为弧度，创建绕Z轴的旋转四元数
        double angleRad = heading_ * M_PI / 180.0;
        osg::Quat rotation(angleRad, osg::Vec3d(0.0, 0.0, 1.0));
        pat->setAttitude(rotation);
    }
}

/**
 * @brief 创建并初始化 PositionAttitudeTransform 节点
 * 
 * 创建 PAT 节点并自动设置初始位置和旋转，子类可直接使用。
 * 
 * @return 已设置好变换的 PAT 节点（位置和旋转已根据实体状态初始化）
 * 
 * @note 子类在 createNode() 中需要 PAT 节点时，可直接调用此方法，
 *       然后在此节点上添加子节点（如几何体、高亮边框等）。
 * 
 * @example
 * @code
 * osg::ref_ptr<osg::PositionAttitudeTransform> pat = createPATNode();
 * pat->addChild(myGeometry);
 * return pat.get();
 * @endcode
 */
osg::ref_ptr<osg::PositionAttitudeTransform> GeoEntity::createPATNode()
{
    osg::ref_ptr<osg::PositionAttitudeTransform> pat = new osg::PositionAttitudeTransform();
    setupNodeTransform(pat.get());
    return pat;
}
