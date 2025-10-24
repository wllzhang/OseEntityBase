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

signals:
    void entityCreated(GeoEntity* entity);
    void entityRemoved(const QString& entityId);

private:
    osg::ref_ptr<osg::Group> root_;
    osg::ref_ptr<osgEarth::MapNode> mapNode_;
    osg::ref_ptr<osg::Group> entityGroup_;
    
    QJsonObject entityConfig_;
    QMap<QString, GeoEntity*> entities_;
    int entityCounter_;
    
    QString generateEntityId(const QString& entityType, const QString& entityName);
    QString getImagePathFromConfig(const QString& entityName);
};

#endif // GEOENTITYMANAGER_H
