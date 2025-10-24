#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>
#include <osgGA/OrbitManipulator>
#include <osgGA/TerrainManipulator>
#include <osgEarth/MapNode>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

// 前置声明
class QLabel;
class QResizeEvent;
class ImageViewerWindow;
class GeoEntityManager;

// 地图模式枚举
enum MapMode {
    MAP_MODE_2D,
    MAP_MODE_3D
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void switchTo3DMode();
    void openImageViewer();
    void testSetHeading();  // 测试设置实体旋转角度

protected:
    void resizeEvent(QResizeEvent* event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void initializeViewer();
    void loadMap(const QString& earthFile);
    void setupCamera();
    void setupManipulator(MapMode mode);
    void loadEntityConfig();

private:
    Ui::MainWindow *ui;
    osg::ref_ptr<osg::Group> root_;
    osg::ref_ptr<osgViewer::Viewer> viewer_;
    osg::ref_ptr<osgGA::TrackballManipulator> trackballManipulator_;
    osg::ref_ptr<osgGA::TerrainManipulator> terrainManipulator_;
    osg::ref_ptr<osgEarth::MapNode> mapNode_;

    MapMode currentMode_;
    QString earth2DPath_;
    QString earth3DPath_;

    
    // 图片查看器窗口
    ImageViewerWindow* imageViewerWindow_;
    
    // 实体管理器
    GeoEntityManager* entityManager_;
};
#endif // MAINWINDOW_H
