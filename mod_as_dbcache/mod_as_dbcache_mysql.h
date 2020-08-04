
#ifndef __MOD_AS_DBCACHE_MYSQL_H__
#define __MOD_AS_DBCACHE_MYSQL_H__

#include <mysql/mysql.h>
#include <switch.h>

#include "mod_as_dbcache_define.h"


#define MOD_AS_DBCACHE_CONFIG_MYSQL_HOST                "MysqlHost"           //mysql host addr
#define MOD_AS_DBCACHE_CONFIG_MYSQL_PORT                "MysqlPort"           //mysql port
#define MOD_AS_DBCACHE_CONFIG_MYSQL_USER                "MysqlUser"           //mysql user
#define MOD_AS_DBCACHE_CONFIG_MYSQL_PASSWD              "MysqlPasswd"         //mysql password
#define MOD_AS_DBCACHE_CONFIG_MYSQL_NAME                "MysqlName"           //mysql name


//init the global param and connect
switch_status_t mod_as_dbcache_mysql_init();

//uninit
switch_status_t mod_as_dbcache_mysql_uninit();

//connect mysql
switch_status_t mod_as_dbcache_mysql_connect();

//disconnect mysql
switch_status_t mod_as_dbcache_mysql_disconnect();

switch_status_t mod_as_dbcache_mysql_command(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);


switch_status_t mod_as_dbcache_mysql_connect2(MYSQL **pmysql);

switch_status_t mod_as_dbcache_mysql_disconnect2(MYSQL **pmysql);

switch_status_t mod_as_dbcache_mysql_command2(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);


#endif


