


#include "mod_as_dbcache_mysql.h"
#include "mod_as_dbcache_redis.h"

//mysql connect handler
static MYSQL *pmod_as_dbcache_mysql = NULL;
static switch_memory_pool_t *pmod_as_dbcache_mysql_pool = NULL;
static switch_mutex_t *pmod_as_dbcache_mysql_mutex = NULL;
extern int g_as_use_redis;

//init the global param and connect
switch_status_t mod_as_dbcache_mysql_init()
{    
    if(NULL == pmod_as_dbcache_mysql_pool)
	{
        if(SWITCH_STATUS_SUCCESS != switch_core_new_memory_pool(&pmod_as_dbcache_mysql_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "pmod_as_dbcache_mysql_pool init failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }
    
    if(NULL == pmod_as_dbcache_mysql_mutex)
	{        
        if(SWITCH_STATUS_SUCCESS != switch_mutex_init(&pmod_as_dbcache_mysql_mutex, 
                                                        SWITCH_MUTEX_NESTED, pmod_as_dbcache_mysql_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "pmod_as_dbcache_mysql_mutex init failed\n");
            return SWITCH_STATUS_FALSE;
        }
	}
    
    return mod_as_dbcache_mysql_connect();
}

//uninit
switch_status_t mod_as_dbcache_mysql_uninit()
{
    if(NULL != pmod_as_dbcache_mysql_mutex)
    {
        switch_mutex_destroy(pmod_as_dbcache_mysql_mutex);
        pmod_as_dbcache_mysql_mutex = NULL;
    }

    if(NULL != pmod_as_dbcache_mysql_pool)
    {
        switch_core_destroy_memory_pool(&pmod_as_dbcache_mysql_pool);
        pmod_as_dbcache_mysql_pool = NULL;
    }
    
    return mod_as_dbcache_mysql_disconnect();
}

//connect mysql
switch_status_t mod_as_dbcache_mysql_connect()
{
    char *pmysqladdr = NULL;
    char *pmysqlport = NULL;
    char *pmysqluser = NULL;
    char *pmysqlpw = NULL;
    char *pmysqlname = NULL;
    unsigned int mysqlport = 0;
    char value = 1;

    pmysqladdr = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_HOST));
    pmysqlport = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_PORT));
    pmysqluser = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_USER));
    pmysqlpw = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_PASSWD));
    pmysqlname = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_NAME));
    mysqlport = atoi(pmysqlport);
    
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
        "mod_as_dbcache_mysql_connect: host=%s, port=%d, user=%s, pw=%s, name=%s\n",
        pmysqladdr, mysqlport, pmysqluser, pmysqlpw, pmysqlname);
    
    //mysql connect 
    pmod_as_dbcache_mysql = mysql_init(NULL); 
    if (!mysql_real_connect(pmod_as_dbcache_mysql, pmysqladdr, pmysqluser, pmysqlpw, pmysqlname, mysqlport, NULL, 0))
    { 
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "mysql connect failed: %s!\n", mysql_error(pmod_as_dbcache_mysql));
        return SWITCH_STATUS_FALSE;
    }

    //mysql set reconnect        
    mysql_options(pmod_as_dbcache_mysql, MYSQL_OPT_RECONNECT, (char *)&value);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
        "mod_as_dbcache_mysql_connect success\n");

    return SWITCH_STATUS_SUCCESS;
}

//disconnect mysql
switch_status_t mod_as_dbcache_mysql_disconnect()
{
    if(NULL != pmod_as_dbcache_mysql)
    {
        mysql_close(pmod_as_dbcache_mysql);
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_mysql_disconnect success\n");

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_mysql_command(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_FALSE;
	int key_id = 0;
    char *command = NULL;
    void *result = NULL;
    MYSQL_RES *pmysqlres = NULL;
    time_t curtime = 0;
    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
            "prespdata is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    //TODO :select 查询需要进行prespdata->event->pfunmysql 的判断
    
    command = prespdata->event->mysql_command;
    result = prespdata->result;
    
    if(NULL == command)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "command is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_mysql_command, command=%s\n", command);

    if(NULL == pmod_as_dbcache_mysql)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "pmod_as_dbcache_mysql is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    switch_mutex_lock(pmod_as_dbcache_mysql_mutex);
    
    mysql_ping(pmod_as_dbcache_mysql);
    
    //mysql query
    if(0 != mysql_real_query(pmod_as_dbcache_mysql, command, strlen(command)))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                        "mysql query failed: %s, command: %s!\n", 
                        mysql_error(pmod_as_dbcache_mysql), command);
        switch_mutex_unlock(pmod_as_dbcache_mysql_mutex);
        return SWITCH_STATUS_FALSE;
    }
	//mysql插入并需要返回主键id时调用此类型
	if (prespdata->event->type == EN_TYPE_MYSQL_INSERT_ID)
	{
		prespdata->resptype = EN_TYPE_MYSQL_INSERT_ID;
		key_id = mysql_insert_id(pmod_as_dbcache_mysql);
		prespdata->respdata = &key_id;
		if(NULL != prespdata->event->pfunmysql && NULL != prespdata)
		{
			res = prespdata->event->pfunmysql(prespdata, result);
		}
		else
		{
			res = SWITCH_STATUS_SUCCESS;
		}
		
		switch_mutex_unlock(pmod_as_dbcache_mysql_mutex);
		prespdata->respdata = NULL;
		return res;
	}
    //mysql get result
    pmysqlres = mysql_store_result(pmod_as_dbcache_mysql);
    if(NULL == pmysqlres)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                        "mysql query : pmysqlres is null\n");
        //switch_mutex_unlock(pmod_as_dbcache_mysql_mutex);
        //return SWITCH_STATUS_FALSE;
    }

    prespdata->resptype = EN_TYPE_MYSQL;
    prespdata->respdata = pmysqlres;

    //add by zr 20190923, for fs coredump with mysql timeout
    curtime = time(NULL);
    if(curtime >= prespdata->event->timeout)
    {
        res = SWITCH_STATUS_TIMEOUT;
    }
    else
    {
        if(NULL != prespdata->event->pfunmysql && NULL != prespdata)
        {
            res = prespdata->event->pfunmysql(prespdata, result);
        }
        else
        {
            res = SWITCH_STATUS_SUCCESS;
        }
    }
    
    if(pmysqlres)
    {
        mysql_free_result(pmysqlres);
    }
    switch_mutex_unlock(pmod_as_dbcache_mysql_mutex);

    if(SWITCH_STATUS_SUCCESS == res && prespdata->event->iscache)
    {
        if(1 == g_as_use_redis)
        {
            mod_as_dbcache_redis_cluster_cache_command(prespdata);
        }
        else
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_ERROR, 
                  "mysql query : redis is not used\n");
        }
    }    
    prespdata->respdata = NULL;
    
    return res;
}


switch_status_t mod_as_dbcache_mysql_connect2(MYSQL **pmysql)
{
    char *pmysqladdr = NULL;
    char *pmysqlport = NULL;
    char *pmysqluser = NULL;
    char *pmysqlpw = NULL;
    char *pmysqlname = NULL;
    unsigned int mysqlport = 0;
    char value = 1;

    pmysqladdr = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_HOST));
    pmysqlport = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_PORT));
    pmysqluser = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_USER));
    pmysqlpw = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_PASSWD));
    pmysqlname = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_MYSQL_NAME));
    mysqlport = atoi(pmysqlport);
    
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
        "mod_as_dbcache_mysql_connect: host=%s, port=%d, user=%s, pw=%s, name=%s\n",
        pmysqladdr, mysqlport, pmysqluser, pmysqlpw, pmysqlname);
    
    //mysql connect 
    *pmysql = mysql_init(NULL); 
    if (!mysql_real_connect(*pmysql, pmysqladdr, pmysqluser, pmysqlpw, pmysqlname, mysqlport, NULL, 0))
    { 
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "mysql connect failed: %s!\n", mysql_error(*pmysql));
        return SWITCH_STATUS_FALSE;
    }

    //mysql set reconnect        
    mysql_options(*pmysql, MYSQL_OPT_RECONNECT, (char *)&value);

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
        "mod_as_dbcache_mysql_connect success\n");

    return SWITCH_STATUS_SUCCESS;
}

//disconnect mysql
switch_status_t mod_as_dbcache_mysql_disconnect2(MYSQL **pmysql)
{
    if(NULL != *pmysql)
    {
        mysql_close(*pmysql);
        *pmysql = NULL;
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_mysql_disconnect success\n");

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_mysql_command2(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_FALSE;
	int key_id = 0;
    char *command = NULL;
    void *result = NULL;
    MYSQL_RES *pmysqlres = NULL;
    time_t curtime = 0;
    MYSQL *pmysql;
    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
            "prespdata is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    //TODO :select 查询需要进行prespdata->event->pfunmysql 的判断
    
    command = prespdata->event->mysql_command;
    result = prespdata->result;
    pmysql = prespdata->mainqueue->pmysql;
    
    if(NULL == command)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "command is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_mysql_command, command=%s\n", command);

    if(NULL == pmysql)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "pmysql is NULL\n");
        return SWITCH_STATUS_FALSE;
    }
    
    //mysql_ping(pmysql);
    
    //mysql query
    if(0 != mysql_real_query(pmysql, command, strlen(command)))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                        "mysql query failed: %s, command: %s!\n", 
                        mysql_error(pmysql), command);
        return SWITCH_STATUS_FALSE;
    }
	//mysql插入并需要返回主键id时调用此类型
	if (prespdata->event->type == EN_TYPE_MYSQL_INSERT_ID)
	{
		prespdata->resptype = EN_TYPE_MYSQL_INSERT_ID;
		key_id = mysql_insert_id(pmysql);
		prespdata->respdata = &key_id;
		if(NULL != prespdata->event->pfunmysql && NULL != prespdata)
		{
			res = prespdata->event->pfunmysql(prespdata, result);
		}
		else
		{
			res = SWITCH_STATUS_SUCCESS;
		}
		
		prespdata->respdata = NULL;
		return res;
	}
    //mysql get result
    pmysqlres = mysql_store_result(pmysql);
    if(NULL == pmysqlres)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                        "mysql query : pmysqlres is null\n");
        //return SWITCH_STATUS_FALSE;
    }

    prespdata->resptype = EN_TYPE_MYSQL;
    prespdata->respdata = pmysqlres;

    //add by zr 20190923, for fs coredump with mysql timeout
    curtime = time(NULL);
    if(curtime >= prespdata->event->timeout)
    {
        res = SWITCH_STATUS_TIMEOUT;
    }
    else
    {
        if(NULL != prespdata->event->pfunmysql && NULL != prespdata)
        {
            res = prespdata->event->pfunmysql(prespdata, result);
        }
        else
        {
            res = SWITCH_STATUS_SUCCESS;
        }
    }
    
    if(pmysqlres)
    {
        mysql_free_result(pmysqlres);
    }

    if(SWITCH_STATUS_SUCCESS == res && prespdata->event->iscache)
    {
        if(1 == g_as_use_redis)
        {
            mod_as_dbcache_redis_cluster_cache_command(prespdata);
        }
        else
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_ERROR, 
                  "mysql query : redis is not used\n");
        }
    }    
    prespdata->respdata = NULL;
    
    return res;
}


