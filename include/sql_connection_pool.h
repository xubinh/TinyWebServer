#ifndef _CONNECTION_POOL_
#define _CONNECTION_POOL_

#include <error.h>
#include <iostream>
#include <list>
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>
#include <string>

using namespace std;

#include "locker.h"
#include "log.h"

class ConnectionPool {
private:
    int m_max_number_of_connections;     //最大连接数
    int m_current_number_of_connections; //当前已使用的连接数
    locker lock;
    list<MYSQL *> m_connection_list; //连接池
    sem reserve;

    ConnectionPool();
    ~ConnectionPool();

public:
    string m_url;           //主机地址
    string m_port;          //数据库端口号
    string m_user;          //登陆数据库用户名
    string m_password;      //登陆数据库密码
    string m_database_name; //使用数据库名
    int m_close_log;        //日志开关

    MYSQL *get_connection();              //获取数据库连接
    bool release_connection(MYSQL *conn); //释放连接
    int get_number_of_free_connections(); //获取连接

    // 单例模式
    static ConnectionPool *get_instance();

    void init(string, string, string, string, int, int, int);
};

class ConnectionRAII {
private:
    MYSQL *raii_connection;
    ConnectionPool *raii_connection_pool;

public:
    ConnectionRAII(MYSQL **, ConnectionPool *);
    ~ConnectionRAII();
};

#endif
