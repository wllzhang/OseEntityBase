/**
 * @file main.cpp
 * @brief 应用程序入口文件
 * 
 * 程序主入口，创建QApplication和MainWindow实例
 */

#include "mainwindow.h"

#include <QApplication>

/**
 * @brief 程序主入口
 * @param argc 命令行参数数量
 * @param argv 命令行参数数组
 * @return 程序退出码
 */
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
