/**
 * @file geoentitymanager.h
 * @brief 地理实体管理器头文件
 * 
 * 定义GeoEntityManager类，统一管理所有地理实体的创建、删除、查询和交互
 */

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
#include <QHash>
#include <QMouseEvent>
#include <QTimer>
#include <QQueue>
#include <osgViewer/Viewer>
#include "geoentity.h"
#include "LineEntity.h"
#include <QVector>

// 前置声明，避免头文件循环依赖
class MapStateManager;

/**
 * @defgroup managers Managers
 * 管理器模块：用于协调实体、地图与应用交互。
 */

/**
 * @ingroup managers
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
                           const QJsonObject& properties, double longitude, double latitude, double altitude,
                           const QString& uidOverride = QString());
    
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
     * @param uid 实体UID
     * @return 实体指针，不存在返回nullptr
     */
    GeoEntity* getEntity(const QString& uid);
    /** @brief 通过稳定UID获取实体（别名，保持兼容） */
    GeoEntity* getEntityByUid(const QString& uid) const;
    /** @brief 获取所有实体列表 */
    QList<GeoEntity*> getAllEntities() const;
    /** @brief 获取当前选中的实体 */
    GeoEntity* getSelectedEntity() const;
    /** @brief 设置当前选中的实体 */
    void setSelectedEntity(GeoEntity* entity, bool emitSignal = true);
    /** @brief 设置实体可见性 */
    bool setEntityVisible(const QString& uid, bool visible);
    /** @brief 查询实体可见性 */
    bool isEntityVisible(const QString& uid) const;
    
    /**
     * @brief 获取所有实体UID列表
     * @return 实体UID列表
     */
    QStringList getEntityIds() const;  // 保持方法名兼容，实际返回UID列表
    
    /**
     * @brief 按类型获取实体UID列表
     * @param entityType 实体类型
     * @return 实体UID列表
     */
    QStringList getEntityIdsByType(const QString& entityType) const;  // 保持方法名兼容，实际返回UID列表
    
    /**
     * @brief 删除实体
     * @param uid 实体UID
     */
    void removeEntity(const QString& uid);
    
    /**
     * @brief 清空所有实体
     */
    void clearAllEntities();
    
    /**
     * @brief 设置实体配置
     * @param config JSON配置对象
     * @deprecated 此方法已废弃，图片路径现在从数据库查询
     */
    void setEntityConfig(const QJsonObject& config);
    
    /**
     * @brief 处理鼠标按下事件
     * @param event 鼠标事件
     */
    void onMousePress(QMouseEvent* event);
    
    /**
     * @brief 处理鼠标双击事件
     * @param event 鼠标事件
     */
    void onMouseDoubleClick(QMouseEvent* event);
    
    /**
     * @brief 设置Viewer用于射线相交检测
     * @param viewer OSG Viewer指针
     */
    void setViewer(osgViewer::Viewer* viewer);

    /**
     * @brief 设置MapStateManager用于读取相机range
     */
    void setMapStateManager(MapStateManager* mapStateManager);

    /**
     * @brief 控制是否阻止地图导航
     *
     * 某些测量模式下需要临时禁止地图拖拽/旋转。
     */
    void setBlockMapNavigation(bool block);
    
    /**
     * @brief 当前是否禁止地图导航
     */
    bool isMapNavigationBlocked() const;

    /**
     * @brief 处理延迟删除队列（应该在frame()完成后调用）
     * 公共方法，供外部在渲染完成后调用
     */
    void processPendingDeletions();
    
    /**
     * @brief 查找指定位置的实体
     * @param screenPos 屏幕坐标
     * @return 找到的实体指针，未找到返回nullptr
     */
    GeoEntity* findEntityAtPosition(QPoint screenPos, bool verbose = true);

    /** @brief 处理鼠标移动事件（用于实体悬停高亮） */
    void onMouseMove(QMouseEvent* event);

    // ===== 航点/航线 API =====
    /**
     * @brief 航点组信息结构
     */
    struct WaypointGroupInfo {
        QString groupId;
        QString name;
        QVector<class WaypointEntity*> waypoints;
        osg::ref_ptr<osg::Geode> routeNode; // 航线绘制节点
        QString routeModel;                 // 航线生成模型（linear|bezier）
    };

    /** @brief 创建航点组 */
    QString createWaypointGroup(const QString& name);
    /** @brief 在指定组中添加航点 */
    class WaypointEntity* addWaypointToGroup(const QString& groupId, double lon, double lat, double alt,
                                             const QString& uidOverride = QString(), const QString& label = QString());
    bool attachWaypointToGroup(const QString& groupId, class WaypointEntity* waypoint);
    /** @brief 从组中按序号删除航点 */
    bool removeWaypointFromGroup(const QString& groupId, int index);
    /** @brief 删除指定航点实体（自动更新所属航线） */
    bool removeWaypointEntity(class WaypointEntity* waypoint);
    /** @brief 依据模型生成组内航线（linear|bezier） */
    bool generateRouteForGroup(const QString& groupId, const QString& model /* 'linear' | 'bezier' */);
    /** @brief 将生成的航线绑定到实体（随实体移动/显示） */
    bool bindRouteToEntity(const QString& groupId, const QString& targetEntityUid);
    
    /** @brief 获取指定实体的航线组ID（通过routeBinding查找） */
    QString getRouteGroupIdForEntity(const QString& entityUid) const;
    
    /** @brief 获取所有航点组信息（用于保存） */
    QList<WaypointGroupInfo> getAllWaypointGroups() const;
    
    /** @brief 获取航点组信息 */
    WaypointGroupInfo getWaypointGroup(const QString& groupId) const;

    // 点标绘：直接添加一个带自定义标签的航点（不依赖组）
    /** @brief 添加独立航点（带标签），用于快速标绘 */
    class WaypointEntity* addStandaloneWaypoint(double lon, double lat, double alt, const QString& labelText,
                                                const QString& uidOverride = QString());

    // ===== 直线航点组 =====
    struct LineEndpointInfo {
        QString startWaypointUid;
        QString endWaypointUid;
        QMetaObject::Connection startPositionConn;
        QMetaObject::Connection endPositionConn;
        QMetaObject::Connection lineNameConn;
    };

    /** @brief 添加直线实体 */
    LineEntity* addLineEntity(const QString& name,
                              double startLon, double startLat, double startAlt,
                              double endLon, double endLat, double endAlt,
                              const QString& uidOverride = QString());

signals:
    /**
     * @brief 实体创建信号
     * @param entity 创建的实体指针
     */
    void entityCreated(GeoEntity* entity);
    
    /**
     * @brief 实体删除信号
     * @param uid 删除的实体UID
     */
    void entityRemoved(const QString& uid);
    
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
    
    /**
     * @brief 实体双击信号
     * @param entity 被双击的实体指针
     */
    void entityDoubleClicked(GeoEntity* entity);

    /**
     * @brief 地图左键点击（空白处）
     */
    void mapLeftClicked(QPoint screenPos);

    /**
     * @brief 地图右键点击（空白处）
     */
    void mapRightClicked(QPoint screenPos);

private:
    struct PickCandidate {
        GeoEntity* entity = nullptr;
        double distanceMeters = 0.0;
    };

    osg::ref_ptr<osg::Group> root_;
    osg::ref_ptr<osgEarth::MapNode> mapNode_;
    osg::ref_ptr<osg::Group> entityGroup_;
    
    // 用于射线相交检测
    osgViewer::Viewer* viewer_;
    
    // 用于读取当前相机距离range
    MapStateManager* mapStateManager_;
    
    QMap<QString, GeoEntity*> entities_;  // uid -> entity
    QHash<QString, GeoEntity*> uidToEntity_;  // 保留作为别名索引（实际与entities_相同）
    int entityCounter_;
    
    // 当前选中的实体
    GeoEntity* selectedEntity_;
    GeoEntity* hoveredEntity_;       // 当前悬停的实体
    
    // 延迟删除机制：避免在渲染过程中删除节点
    QQueue<QString> pendingDeletions_;  // 待删除的实体UID队列
    QMap<QString, GeoEntity*> pendingEntities_;  // 待删除的实体对象（保持引用直到真正删除）
    
    /** @brief 生成唯一实体ID（已废弃，统一使用uid） */
    QString generateEntityId(const QString& entityType, const QString& entityName);
    /** @brief 根据实体名从数据库获取图片路径 */
    QString getImagePathFromDatabase(const QString& entityName);
    /** @brief 根据实体名从配置获取图片路径（已废弃，转发到数据库查询） */
    QString getImagePathFromConfig(const QString& entityName);
    bool collectPickCandidates(QPoint screenPos, QVector<PickCandidate>& outCandidates, double& thresholdMeters, bool verbose = false);
    double computeSelectionThreshold() const;

    bool blockMapNavigation_ = false; ///< 是否阻止地图导航

    // 航点/航线数据
    QMap<QString, WaypointGroupInfo> waypointGroups_;
    QMap<QString, QString> routeBinding_; // groupId -> targetEntityUid

    // 直线
    QMap<QString, LineEndpointInfo> lineEndpoints_;

    /** @brief 生成线性航线节点 */
    osg::ref_ptr<osg::Geode> buildLinearRoute(const QVector<class WaypointEntity*>& wps);
    /** @brief 生成贝塞尔航线节点 */
    osg::ref_ptr<osg::Geode> buildBezierRoute(const QVector<class WaypointEntity*>& wps);

    /** @brief 查找航点所属分组和序号 */
    bool findWaypointLocation(class WaypointEntity* waypoint, QString& groupIdOut, int& indexOut) const;


    void updateLineEndpoints(const QString& lineUid);
    void updateLineEndpointDisplayNames(const QString& lineUid, const QString& lineName);
    void disconnectLineEndpointConnections(LineEndpointInfo& info);

};

#endif // GEOENTITYMANAGER_H
