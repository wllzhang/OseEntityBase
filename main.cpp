/**
 * @file main.cpp
 * @brief 应用程序入口文件
 * 
 * 程序主入口，创建QApplication和MainWidget实例
 */

#include "ui/MainWidget.h"
// #include "mainwindow.h"
#include <QApplication>
#include "util/databaseutils.h"
#include "geo/basemapmanager.h"
#include <QDebug>

/**
 * @brief 程序主入口
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    
    // 设置数据库路径（使用绝对路径）
    // 根据实际情况修改为你的项目根目录路径
    DatabaseUtils::setDatabasePath("D:/OSG/MyDatabase.db");
    qDebug() << "数据库路径设置为:" << DatabaseUtils::getDatabasePath();
    
    // 设置底图配置文件路径（使用绝对路径）
    // 根据实际情况修改为你的项目根目录路径
    BaseMapManager::setConfigFilePath("D:/OSG/osgqtlib/osgEarthmy_osgb/basemap_config.json");
    qDebug() << "底图配置文件路径设置为:" << BaseMapManager::getConfigFilePath();
    
    MainWidget w;
//     MainWindow w;
    w.show();
    return a.exec();
}
