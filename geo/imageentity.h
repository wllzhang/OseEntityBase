#ifndef IMAGEENTITY_H
#define IMAGEENTITY_H

#include "geoentity.h"
#include <QString>

class ImageEntity : public GeoEntity
{
    Q_OBJECT

public:
    ImageEntity(const QString& id, const QString& name, const QString& imagePath,
                double longitude, double latitude, double altitude, QObject* parent = nullptr);
    
    // 实现基类纯虚函数
    void initialize() override;
    void update() override;
    void cleanup() override;

private:
    osg::ref_ptr<osg::Node> createNode() override;
    
    QString imagePath_;
};

#endif // IMAGEENTITY_H
