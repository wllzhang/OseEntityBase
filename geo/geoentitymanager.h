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

class GeoEntityManager : public QObject
{
    Q_OBJECT

public:
    explicit GeoEntityManager(osg::Group* root, osgEarth::MapNode* mapNode, QObject *parent = nullptr);
    
    // 通用实体管理接口
    GeoEntity* createEntity(const QString& entityType, const QString& entityName, 
                           const QJsonObject& properties, double longitude, double latitude, double altitude);
    
    bool addEntityFromDrag(const QString& dragData, double longitude, double latitude, double altitude = 0.0);
    
    // 查询接口
    GeoEntity* getEntity(const QString& entityId);
    QStringList getEntityIds() const;
    QStringList getEntityIdsByType(const QString& entityType) const;
    
    // 批量操作
    void removeEntity(const QString& entityId);
    void clearAllEntities();
    
    // 配置管理
    void setEntityConfig(const QJsonObject& config);
    
    // 鼠标事件处理
    void onMousePress(QMouseEvent* event);
    
    // 设置Viewer用于射线相交检测
    void setViewer(osgViewer::Viewer* viewer);
    
    // 查找指定位置的实体
    GeoEntity* findEntityAtPosition(QPoint screenPos);

signals:
    void entityCreated(GeoEntity* entity);
    void entityRemoved(const QString& entityId);
    void entitySelected(GeoEntity* entity);
    void entityDeselected();
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
