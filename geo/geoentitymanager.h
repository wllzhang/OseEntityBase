#ifndef GEOENTITYMANAGER_H
#define GEOENTITYMANAGER_H

#include <osg/Group>
#include <osgEarth/MapNode>
#include <osgEarth/Bounds>
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QMap>
#include <QMouseEvent>
#include <osgViewer/Viewer>
#include "geoentity.h"

/**
 * @brief 地理实体管理器
 * 
 * 统一管理所有地理实体的创建、删除、查询和交互。
 * 负责实体与场景图的交互，处理鼠标事件，发出实体状态信号。
 * 
 * 主要功能：
 * - 实体的创建和管理
 * - 实体的查询和遍历
 * - 实体的删除和清空
 * - 实体选择和鼠标事件处理
 * - 通过信号槽机制通知其他组件
 * 
 * @note 使用信号槽机制与 MainWindow 通信，保持职责分离
 */
class GeoEntityManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param root OSG场景根节点
     * @param mapNode osgEarth地图节点
     * @param parent Qt父对象
     */
    explicit GeoEntityManager(osg::Group* root, osgEarth::MapNode* mapNode, QObject *parent = nullptr);
    
    /**
     * @brief 创建实体
     * @param entityType 实体类型（如"aircraft"）
     * @param entityName 实体名称
     * @param properties 实体属性
     * @param longitude 经度
     * @param latitude 纬度
     * @param altitude 高度
     * @return 创建的实体指针，失败返回nullptr
     */
    GeoEntity* createEntity(const QString& entityType, const QString& entityName, 
                           const QJsonObject& properties, double longitude, double latitude, double altitude);
    
    /**
     * @brief 从拖拽数据添加实体
     * @param dragData 拖拽数据字符串
     * @param longitude 经度
     * @param latitude 纬度
     * @param altitude 高度
     * @return 成功返回true
     */
    bool addEntityFromDrag(const QString& dragData, double longitude, double latitude, double altitude = 0.0);
    
    /**
     * @brief 获取实体
     * @param entityId 实体ID
     * @return 实体指针，不存在返回nullptr
     */
    GeoEntity* getEntity(const QString& entityId);
    
    /**
     * @brief 获取所有实体ID列表
     * @return 实体ID列表
     */
    QStringList getEntityIds() const;
    
    /**
     * @brief 按类型获取实体ID列表
     * @param entityType 实体类型
     * @return 实体ID列表
     */
    QStringList getEntityIdsByType(const QString& entityType) const;
    
    /**
     * @brief 删除实体
     * @param entityId 实体ID
     */
    void removeEntity(const QString& entityId);
    
    /**
     * @brief 清空所有实体
     */
    void clearAllEntities();
    
    /**
     * @brief 设置实体配置
     * @param config JSON配置对象
     */
    void setEntityConfig(const QJsonObject& config);
    
    /**
     * @brief 处理鼠标按下事件
     * @param event 鼠标事件
     */
    void onMousePress(QMouseEvent* event);
    
    /**
     * @brief 设置Viewer用于射线相交检测
     * @param viewer OSG Viewer指针
     */
    void setViewer(osgViewer::Viewer* viewer);
    
    /**
     * @brief 查找指定位置的实体
     * @param screenPos 屏幕坐标
     * @return 找到的实体指针，未找到返回nullptr
     */
    GeoEntity* findEntityAtPosition(QPoint screenPos);

signals:
    /**
     * @brief 实体创建信号
     * @param entity 创建的实体指针
     */
    void entityCreated(GeoEntity* entity);
    
    /**
     * @brief 实体删除信号
     * @param entityId 删除的实体ID
     */
    void entityRemoved(const QString& entityId);
    
    /**
     * @brief 实体选中信号
     * @param entity 选中的实体指针
     */
    void entitySelected(GeoEntity* entity);
    
    /**
     * @brief 实体取消选中信号
     */
    void entityDeselected();
    
    /**
     * @brief 实体右键点击信号
     * @param entity 被点击的实体指针
     * @param screenPos 屏幕坐标
     */
    void entityRightClicked(GeoEntity* entity, QPoint screenPos);

private:
    osg::ref_ptr<osg::Group> root_;
    osg::ref_ptr<osgEarth::MapNode> mapNode_;
    osg::ref_ptr<osg::Group> entityGroup_;
    
    // 用于射线相交检测
    osgViewer::Viewer* viewer_;
    
    QJsonObject entityConfig_;
    QMap<QString, GeoEntity*> entities_;
    int entityCounter_;
    
    // 当前选中的实体
    GeoEntity* selectedEntity_;
    
    QString generateEntityId(const QString& entityType, const QString& entityName);
    QString getImagePathFromConfig(const QString& entityName);
};

#endif // GEOENTITYMANAGER_H
