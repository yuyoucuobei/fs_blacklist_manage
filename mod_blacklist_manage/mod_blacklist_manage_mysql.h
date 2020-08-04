
#pragma once

#include <mysql/mysql.h>
#include <switch.h>

#include "mod_blacklist_manage_define.h"


#define BM_CONFIG_MYSQL_HOST                "MysqlHost"           //mysql host addr
#define BM_CONFIG_MYSQL_PORT                "MysqlPort"           //mysql port
#define BM_CONFIG_MYSQL_USER                "MysqlUser"           //mysql user
#define BM_CONFIG_MYSQL_PASSWD              "MysqlPasswd"         //mysql password
#define BM_CONFIG_MYSQL_NAME                "MysqlName"           //mysql name

switch_status_t bm_mysql_connect(MYSQL **pmysql);

switch_status_t bm_mysql_disconnect(MYSQL **pmysql);

switch_status_t bm_mysql_command_batch(MYSQL *pmysql, const string &cmd);


switch_status_t bm_mysql_init();

switch_status_t bm_mysql_uninit();

