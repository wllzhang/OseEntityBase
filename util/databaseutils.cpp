/**
 * @file databaseutils.cpp
 * @brief 数据库工具类实现文件
 * 
 * 实现DatabaseUtils类的所有功能
 */

#include "databaseutils.h"
#include <QDebug>
#include <QFileInfo>
#include <QDir>
#include <QCoreApplication>
#include <QSqlError>

// 静态成员变量初始化
QString DatabaseUtils::databasePath_ = "";

QString DatabaseUtils::getDatabasePath()
{
    // 如果路径未设置，初始化默认路径
    if (databasePath_.isEmpty()) {
        initializeDefaultPath();
    }
    return databasePath_;
}

void DatabaseUtils::setDatabasePath(const QString& path)
{
    databasePath_ = path;
    qDebug() << "DatabaseUtils: 设置数据库路径为:" << databasePath_;
}

void DatabaseUtils::initializeDefaultPath()
{
    // 默认使用项目根目录下的 MyDatabase.db
    // 这里使用绝对路径，根据实际情况修改
    
    // 先尝试从当前工作目录查找
    QString dbPath = QDir::currentPath() + "/MyDatabase.db";
    if (QFileInfo::exists(dbPath)) {
        databasePath_ = QFileInfo(dbPath).absoluteFilePath();
        qDebug() << "DatabaseUtils: 从当前目录找到数据库:" << databasePath_;
        return;
    }
    
    // 如果找不到，尝试从可执行文件所在目录查找
    QString appDirPath = QCoreApplication::applicationDirPath();
    dbPath = appDirPath + "/MyDatabase.db";
    if (QFileInfo::exists(dbPath)) {
        databasePath_ = QFileInfo(dbPath).absoluteFilePath();
        qDebug() << "DatabaseUtils: 从应用目录找到数据库:" << databasePath_;
        return;
    }
    
    // 如果都找不到，使用项目根目录的绝对路径（需要根据实际情况修改）
    // 这里设置为默认路径，用户可以通过 setDatabasePath 设置
    qDebug() << "DatabaseUtils: 使用默认数据库路径:" << databasePath_;
}

QSqlDatabase DatabaseUtils::getDatabase(const QString& connectionName)
{
    // 检查连接是否已存在
    if (QSqlDatabase::contains(connectionName)) {
        return QSqlDatabase::database(connectionName);
    }
    
    // 创建新连接
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    QString dbPath = getDatabasePath();
    db.setDatabaseName(dbPath);
    
    qDebug() << "DatabaseUtils: 创建数据库连接:" << connectionName << "路径:" << dbPath;
    
    return db;
}

bool DatabaseUtils::openDatabase(const QString& connectionName)
{
    QSqlDatabase db = getDatabase(connectionName);
    
    if (db.isOpen()) {
        qDebug() << "DatabaseUtils: 数据库连接已打开:" << connectionName;
        return true;
    }
    
    if (db.open()) {
        qDebug() << "DatabaseUtils: 成功打开数据库:" << connectionName;
        return true;
    } else {
        qDebug() << "DatabaseUtils: 打开数据库失败:" << connectionName << db.lastError().text();
        return false;
    }
}

void DatabaseUtils::closeDatabase(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) {
            db.close();
            qDebug() << "DatabaseUtils: 关闭数据库连接:" << connectionName;
        }
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool DatabaseUtils::isDatabaseOpen(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        return db.isOpen();
    }
    return false;
}

QSqlQuery DatabaseUtils::executeQuery(const QString& queryString, const QString& connectionName)
{
    QSqlDatabase db = getDatabase(connectionName);
    
    // 确保数据库已打开
    if (!db.isOpen()) {
        if (!openDatabase(connectionName)) {
            qDebug() << "DatabaseUtils: 无法打开数据库执行查询";
            return QSqlQuery();
        }
    }
    
    QSqlQuery query(db);
    if (!query.exec(queryString)) {
        qDebug() << "DatabaseUtils: 查询执行失败:" << queryString << query.lastError().text();
    }
    
    return query;
}

bool DatabaseUtils::beginTransaction(const QString& connectionName)
{
    QSqlDatabase db = getDatabase(connectionName);
    
    if (!db.isOpen()) {
        if (!openDatabase(connectionName)) {
            return false;
        }
    }
    
    return db.transaction();
}

bool DatabaseUtils::commitTransaction(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) {
            return db.commit();
        }
    }
    return false;
}

bool DatabaseUtils::rollbackTransaction(const QString& connectionName)
{
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase db = QSqlDatabase::database(connectionName);
        if (db.isOpen()) {
            return db.rollback();
        }
    }
    return false;
}

