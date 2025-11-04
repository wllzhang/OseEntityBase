/**
 * @file databaseutils.h
 * @brief 数据库工具类头文件
 * 
 * 定义DatabaseUtils工具类，提供统一的数据库连接和路径管理功能
 */

#ifndef DATABASEUTILS_H
#define DATABASEUTILS_H

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>

/**
 * @brief 数据库工具类
 * 
 * 提供统一的数据库连接管理，包括：
 * - 数据库路径管理（使用绝对路径）
 * - 数据库连接获取
 * - 数据库连接状态检查
 */
class DatabaseUtils
{
public:
    /**
     * @brief 获取数据库文件路径（绝对路径）
     * @return 数据库文件的绝对路径
     */
    static QString getDatabasePath();
    
    /**
     * @brief 设置数据库文件路径（绝对路径）
     * @param path 数据库文件的绝对路径
     */
    static void setDatabasePath(const QString& path);
    
    /**
     * @brief 获取数据库连接
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 数据库连接对象
     */
    static QSqlDatabase getDatabase(const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 打开数据库连接
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 成功返回true，失败返回false
     */
    static bool openDatabase(const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 关闭数据库连接
     * @param connectionName 连接名称（可选，默认使用默认连接）
     */
    static void closeDatabase(const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 检查数据库连接是否打开
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 连接已打开返回true，否则返回false
     */
    static bool isDatabaseOpen(const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 执行SQL查询
     * @param queryString SQL查询语句
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 查询对象
     */
    static QSqlQuery executeQuery(const QString& queryString, const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 开始事务
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 成功返回true
     */
    static bool beginTransaction(const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 提交事务
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 成功返回true
     */
    static bool commitTransaction(const QString& connectionName = QSqlDatabase::defaultConnection);
    
    /**
     * @brief 回滚事务
     * @param connectionName 连接名称（可选，默认使用默认连接）
     * @return 成功返回true
     */
    static bool rollbackTransaction(const QString& connectionName = QSqlDatabase::defaultConnection);

private:
    static QString databasePath_;  // 数据库文件路径（绝对路径）
    
    /**
     * @brief 初始化默认数据库路径
     */
    static void initializeDefaultPath();
};

#endif // DATABASEUTILS_H

