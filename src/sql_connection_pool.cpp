#include "../include/sql_connection_pool.h"

#include <mysql/mysql.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
#include <string>

using namespace std;

ConnectionPool::ConnectionPool() {
    m_current_number_of_connections = 0;
}

ConnectionPool::~ConnectionPool() {
    lock.lock();

    if (m_connection_list.size() > 0) {
        decltype(m_connection_list)::iterator it;

        for (it = m_connection_list.begin(); it != m_connection_list.end();
             ++it) {
            mysql_close(*it);
        }

        m_current_number_of_connections = 0;
        m_connection_list.clear();
    }

    lock.unlock();
}

// 当有请求时，从数据库连接池中返回一个可用连接，更新使用和空闲连接数
MYSQL *ConnectionPool::get_connection() {
    if (0 == m_connection_list.size()) {
        return NULL;
    }

    reserve.wait(); // P 操作

    lock.lock();

    MYSQL *connection = m_connection_list.front();
    m_connection_list.pop_front();

    ++m_current_number_of_connections;

    lock.unlock();

    return connection;
}

// 释放当前使用的连接
bool ConnectionPool::release_connection(MYSQL *connection) {
    if (connection == NULL) {
        return false;
    }

    lock.lock();

    m_connection_list.push_back(connection);
    --m_current_number_of_connections;

    lock.unlock();

    reserve.post(); // V 操作

    return true;
}

// 当前空闲的连接数
int ConnectionPool::get_number_of_free_connections() {
    return this->m_max_number_of_connections - m_current_number_of_connections;
}

ConnectionPool *ConnectionPool::get_instance() {
    static ConnectionPool connPool;

    return &connPool;
}

// 构造初始化
void ConnectionPool::init(
    string url,
    string user,
    string password,
    string database_name,
    int port,
    int max_number_of_connections,
    int close_log
) {
    m_url = url;
    m_port = port;
    m_user = user;
    m_password = password;
    m_database_name = database_name;
    m_close_log = close_log;
    m_max_number_of_connections = max_number_of_connections;

    for (int i = 0; i < max_number_of_connections; i++) {
        MYSQL *connection = mysql_init(NULL);

        if (connection == NULL) {
            LOG_ERROR("MySQL Error");

            exit(1);
        }

        connection = mysql_real_connect(
            connection,
            url.c_str(),
            user.c_str(),
            password.c_str(),
            database_name.c_str(),
            port,
            NULL,
            0
        );

        if (connection == NULL) {
            LOG_ERROR("MySQL Error");

            exit(1);
        }

        m_connection_list.push_back(connection);
    }

    reserve = sem(max_number_of_connections);
}

ConnectionRAII::ConnectionRAII(
    MYSQL **connection, ConnectionPool *connecton_pool
) {
    *connection = connecton_pool->get_connection();

    raii_connection = *connection;
    raii_connection_pool = connecton_pool;
}

ConnectionRAII::~ConnectionRAII() {
    raii_connection_pool->release_connection(raii_connection);
}