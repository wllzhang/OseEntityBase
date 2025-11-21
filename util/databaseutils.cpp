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
#include <QFile>

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
    
    QString dbPath = getDatabasePath();
    if (dbPath.isEmpty()) {
        qDebug() << "DatabaseUtils: 数据库路径为空，无法打开数据库";
        return false;
    }
    
    // 检查并修复文件权限
    QFileInfo fileInfo(dbPath);
    if (fileInfo.exists()) {
        // 文件存在，检查是否可写
        QFile file(dbPath);
        if (!file.open(QIODevice::ReadWrite)) {
            qDebug() << "DatabaseUtils: 数据库文件存在但无法以读写模式打开:" << dbPath;
            
            // 尝试修复文件权限
            QFile::Permissions perms = file.permissions();
            if (!(perms & QFile::WriteUser)) {
                qDebug() << "DatabaseUtils: 检测到文件缺少写权限，尝试添加写权限...";
                perms |= QFile::WriteUser | QFile::WriteOwner;
                if (file.setPermissions(perms)) {
                    qDebug() << "DatabaseUtils: 已添加写权限，重新尝试打开...";
                    // 重新尝试打开
                    if (file.open(QIODevice::ReadWrite)) {
                        file.close();
                        qDebug() << "DatabaseUtils: 文件权限修复成功";
                    } else {
                        qDebug() << "DatabaseUtils: 添加写权限后仍无法打开文件";
                        qDebug() << "DatabaseUtils: 请手动检查文件权限或使用管理员权限运行程序";
                        return false;
                    }
                } else {
                    qDebug() << "DatabaseUtils: 无法修改文件权限，可能需要管理员权限";
                    qDebug() << "DatabaseUtils: 请手动右键文件->属性->取消只读，或使用管理员权限运行程序";
                    return false;
                }
            } else {
                qDebug() << "DatabaseUtils: 文件权限错误，请检查文件是否为只读或目录权限";
                qDebug() << "DatabaseUtils: 文件路径:" << dbPath;
                return false;
            }
        } else {
            file.close();
        }
    } else {
        // 文件不存在，检查目录是否可写
        QDir dir = fileInfo.absoluteDir();
        if (!dir.exists()) {
            // 目录不存在，尝试创建
            if (!dir.mkpath(".")) {
                qDebug() << "DatabaseUtils: 无法创建数据库目录:" << dir.absolutePath();
                return false;
            }
        }
        
        // 检查目录是否可写
        QFileInfo dirInfo(dir.absolutePath());
        if (!dirInfo.isWritable()) {
            qDebug() << "DatabaseUtils: 数据库目录不可写:" << dir.absolutePath();
            qDebug() << "DatabaseUtils: 请检查目录权限";
            return false;
        }
    }
    
    if (db.open()) {
        qDebug() << "DatabaseUtils: 成功打开数据库:" << connectionName << "路径:" << dbPath;
        return true;
    } else {
        QSqlError error = db.lastError();
        qDebug() << "DatabaseUtils: 打开数据库失败:" << connectionName;
        qDebug() << "DatabaseUtils: 错误类型:" << error.type();
        qDebug() << "DatabaseUtils: 错误信息:" << error.text();
        qDebug() << "DatabaseUtils: 数据库路径:" << dbPath;
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

