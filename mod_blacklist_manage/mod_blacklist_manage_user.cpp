


#include "mod_blacklist_manage_user.h"


static USER_INFO_MANAGE		g_user_info_manage;
static uint32_t             g_updatethreadid = 0;

static char *apr_strdup(apr_pool_t *ppool, const char *str)
{
    char *pnewstr = NULL;
    if(NULL == ppool || NULL == str)
    {
        return NULL;
    }
    
    pnewstr = (char*)apr_pcalloc(ppool, strlen(str) + 1);
    if(NULL == pnewstr)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "apr_pcalloc %s fialed \n", str);
        return NULL;
    }
    strncpy(pnewstr, str, strlen(str));
    return pnewstr;
}


static switch_status_t bm_user_update_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
	MYSQL_RES *mysql_res = NULL;
    MYSQL_ROW mysql_row;
    USER_INFO *puserinfo = NULL;
    
    if (NULL == data || NULL == result)
    {
        return SWITCH_STATUS_INTR;
    }

    if (NULL == data->respdata)
    {
        return SWITCH_STATUS_NOTFOUND;
    }

    SQL_RESULT *psqlresult = (SQL_RESULT*)result;
    if(NULL == psqlresult)
    {
        return SWITCH_STATUS_INTR;
    }
    apr_pool_t *ppool = psqlresult->pnewpool;
    apr_hash_t *phash_user = psqlresult->pnewhash_user;
    apr_hash_t *phash_num = psqlresult->pnewhash_num;
    
    mysql_res = (MYSQL_RES*)data->respdata;
    if (NULL == mysql_res || mysql_num_fields(mysql_res) != 6)
    {
        return SWITCH_STATUS_IGNORE;
    }
    while (NULL != (mysql_row = mysql_fetch_row(mysql_res)))
    {
    	if(NULL == (puserinfo = (USER_INFO*)apr_pcalloc(ppool, sizeof(USER_INFO))))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "apr_palloc fialed\n");
            continue;
        }
        puserinfo->userid = atoi(switch_str_nil(mysql_row[0]));
        memcpy(puserinfo->user, switch_str_nil(mysql_row[1]), USER_NAME_LEN);
        puserinfo->balance = atof(switch_str_nil(mysql_row[2]));
        puserinfo->credit = atof(switch_str_nil(mysql_row[3]));
        puserinfo->status = atoi(switch_str_nil(mysql_row[4]));
        puserinfo->fee = atof(switch_str_nil(mysql_row[5]));

        apr_hash_set(phash_user, apr_strdup(ppool, switch_str_nil(mysql_row[0])), 
            APR_HASH_KEY_STRING, puserinfo);
    }    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t bm_num_update_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
	MYSQL_RES *mysql_res = NULL;
    MYSQL_ROW mysql_row;
    USER_INFO *puserinfo = NULL;
    
    if (NULL == data || NULL == result)
    {
        return SWITCH_STATUS_INTR;
    }

    if (NULL == data->respdata)
    {
        return SWITCH_STATUS_NOTFOUND;
    }

    SQL_RESULT *psqlresult = (SQL_RESULT*)result;
    if(NULL == psqlresult)
    {
        return SWITCH_STATUS_INTR;
    }
    apr_pool_t *ppool = psqlresult->pnewpool;
    apr_hash_t *phash_user = psqlresult->pnewhash_user;
    apr_hash_t *phash_num = psqlresult->pnewhash_num;
    
    mysql_res = (MYSQL_RES*)data->respdata;
    if (NULL == mysql_res || mysql_num_fields(mysql_res) != 2)
    {
        return SWITCH_STATUS_IGNORE;
    }
    while (NULL != (mysql_row = mysql_fetch_row(mysql_res)))
    {
    	if(NULL == (puserinfo = (USER_INFO*)apr_hash_get(phash_user, switch_str_nil(mysql_row[0]), APR_HASH_KEY_STRING)))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "apr_hash_get fialed, userid=%s\n", switch_str_nil(mysql_row[0]));
            continue;
        }

        apr_hash_set(phash_num, apr_strdup(ppool, switch_str_nil(mysql_row[1])), 
            APR_HASH_KEY_STRING, puserinfo);
    }    
    return SWITCH_STATUS_SUCCESS;
}


static switch_status_t bm_user_update_from_mysql(SQL_RESULT &sqlresult)
{ 
    if(NULL == sqlresult.pnewpool || NULL == sqlresult.pnewhash_user || NULL == sqlresult.pnewhash_num)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
                "ppool = %p, phash_user = %p, phash_num = %p\n", 
                (void *)sqlresult.pnewpool, (void *)sqlresult.pnewhash_user, (void *)sqlresult.pnewhash_num);
        return SWITCH_STATUS_FALSE;
    }
    
    ST_DB_CACHE_EVENT event ;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "SELECT id, user, balance, credit, status, fee FROM user_info");
    
    event.pfunmysql = bm_user_update_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &sqlresult))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "SELECT userid, number FROM user_num");
    event.pfunmysql = bm_num_update_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &sqlresult))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}


switch_status_t blacklist_manage_user_update()
{
    SQL_RESULT sqlresult;
    static time_t last_update_time = 0;
    time_t curtime = time(NULL);
    if((curtime-last_update_time) < 60)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
					  "blacklist_manage_user_update fail, update too frequent\n");
        return SWITCH_STATUS_IGNORE;
    }
    last_update_time = curtime;

	// 如果当前使用的是pool1 就刷新 pool2,刷新完就指向 pool2
	if(g_user_info_manage.phandlepool == g_user_info_manage.ppool1 &&  
	   g_user_info_manage.phandlehash_user == g_user_info_manage.phash_user1 && 
	   g_user_info_manage.phandlehash_num == g_user_info_manage.phash_num1)
	{
		if(NULL != g_user_info_manage.ppool2)
		{
			apr_pool_destroy(g_user_info_manage.ppool2);
			g_user_info_manage.ppool2 = NULL;
			g_user_info_manage.phash_user2 = NULL;
            g_user_info_manage.phash_num2 = NULL;
		}

		if (SWITCH_STATUS_SUCCESS != apr_pool_create(&g_user_info_manage.ppool2, NULL))
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
					  "switch_core_new_memory_pool fail\n");
			return SWITCH_STATUS_FALSE;
		}

		if(NULL == (g_user_info_manage.phash_user2 = apr_hash_make(g_user_info_manage.ppool2)))
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"switch_core_hash_init fail\n");
			apr_pool_destroy(g_user_info_manage.ppool2);
			g_user_info_manage.ppool2 = NULL;
			g_user_info_manage.phash_user2 = NULL;
            g_user_info_manage.phash_num2 = NULL;
			return SWITCH_STATUS_FALSE;
		}

        if(NULL == (g_user_info_manage.phash_num2 = apr_hash_make(g_user_info_manage.ppool2)))
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"switch_core_hash_init fail\n");
			apr_pool_destroy(g_user_info_manage.ppool2);
			g_user_info_manage.ppool2 = NULL;
			g_user_info_manage.phash_user2 = NULL;
            g_user_info_manage.phash_num2 = NULL;
			return SWITCH_STATUS_FALSE;
		}

        sqlresult.pnewpool = g_user_info_manage.ppool2;
        sqlresult.pnewhash_user = g_user_info_manage.phash_user2;
        sqlresult.pnewhash_num = g_user_info_manage.phash_num2;
	}
	else
	{
		if(NULL != g_user_info_manage.ppool1)
		{
			apr_pool_destroy(g_user_info_manage.ppool1);
			g_user_info_manage.ppool1 = NULL;
			g_user_info_manage.phash_user1 = NULL;
            g_user_info_manage.phash_num1 = NULL;
		}

		if (SWITCH_STATUS_SUCCESS != apr_pool_create(&g_user_info_manage.ppool1, NULL))
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
					  "switch_core_new_memory_pool fail\n");
			return SWITCH_STATUS_FALSE;
		}

		if(NULL == (g_user_info_manage.phash_user1 = apr_hash_make(g_user_info_manage.ppool1)))
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"switch_core_hash_init fail\n");
			apr_pool_destroy(g_user_info_manage.ppool1);
			g_user_info_manage.ppool1 = NULL;
			g_user_info_manage.phash_user1 = NULL;
            g_user_info_manage.phash_num1 = NULL;
			return SWITCH_STATUS_FALSE;
		}

        if(NULL == (g_user_info_manage.phash_num1 = apr_hash_make(g_user_info_manage.ppool1)))
		{
			switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"switch_core_hash_init fail\n");
			apr_pool_destroy(g_user_info_manage.ppool1);
			g_user_info_manage.ppool1 = NULL;
			g_user_info_manage.phash_user1 = NULL;
            g_user_info_manage.phash_num1 = NULL;
			return SWITCH_STATUS_FALSE;
		}

        sqlresult.pnewpool = g_user_info_manage.ppool1;
        sqlresult.pnewhash_user = g_user_info_manage.phash_user1;
        sqlresult.pnewhash_num = g_user_info_manage.phash_num1;
	}

    if(SWITCH_STATUS_SUCCESS != bm_user_update_from_mysql(sqlresult))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "bm_user_update_from_mysql fail\n");
        return SWITCH_STATUS_FALSE;
    }
    
	//切换当前操作指针
    g_user_info_manage.phandlepool = sqlresult.pnewpool;
    g_user_info_manage.phandlehash_user = sqlresult.pnewhash_user;
    g_user_info_manage.phandlehash_num = sqlresult.pnewhash_num;

    return SWITCH_STATUS_SUCCESS;
}


SWITCH_STANDARD_SCHED_FUNC(bm_user_update_task)
{
	if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_update())    
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
			"blacklist_manage_user_update failed\n");	
    }

	task->runtime = switch_epoch_time_now(NULL) + (60*10);
}


switch_status_t blacklist_manage_user_init()
{
	if(APR_SUCCESS != apr_pool_create(&g_user_info_manage.ppool1, NULL))
	{
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"apr_pool_create failed\n");
		return SWITCH_STATUS_FALSE;
	}

	if(NULL == (g_user_info_manage.phash_user1= apr_hash_make(g_user_info_manage.ppool1)))
	{
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"apr_hash_make fail\n");
		return SWITCH_STATUS_FALSE;
	}

    if(NULL == (g_user_info_manage.phash_num1= apr_hash_make(g_user_info_manage.ppool1)))
	{
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"apr_hash_make fail\n");
		return SWITCH_STATUS_FALSE;
	}

    g_user_info_manage.phandlepool = g_user_info_manage.ppool1;
    g_user_info_manage.phandlehash_user = g_user_info_manage.phash_user1;
    g_user_info_manage.phandlehash_num = g_user_info_manage.phash_num1;

	g_updatethreadid= switch_scheduler_add_task(switch_epoch_time_now(NULL), 
        bm_user_update_task, "bm_user_update_task", "bm_user_update_task", 
        0, NULL, SSHF_NONE | SSHF_OWN_THREAD | SSHF_FREE_ARG | SSHF_NO_DEL);
        
	return SWITCH_STATUS_SUCCESS;
}


switch_status_t blacklist_manage_user_uninit()
{
	if(g_updatethreadid > 0)
	{
		switch_scheduler_del_task_id(g_updatethreadid);
		g_updatethreadid = 0;
	}

    if(g_user_info_manage.ppool1)             
    {
        apr_pool_destroy(g_user_info_manage.ppool1);
        g_user_info_manage.ppool1 = NULL;
		g_user_info_manage.phash_user1 = NULL;
        g_user_info_manage.phash_num1 = NULL;
    }
	if(g_user_info_manage.ppool2)             
    {
        apr_pool_destroy(g_user_info_manage.ppool2);
        g_user_info_manage.ppool2 = NULL;
		g_user_info_manage.phash_user2 = NULL;
        g_user_info_manage.phash_num2 = NULL;
    }
	g_user_info_manage.phandlepool = NULL;
    g_user_info_manage.phandlehash_user= NULL;
    g_user_info_manage.phandlehash_num= NULL;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_create(int userid, const string &username, double fee)
{
    USER_INFO *puserinfo = NULL;
    if(NULL == (puserinfo = (USER_INFO*)apr_pcalloc(g_user_info_manage.phandlepool, sizeof(USER_INFO))))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "apr_palloc fialed\n");
        return SWITCH_STATUS_FALSE;
    }
    puserinfo->userid = userid;
    memcpy(puserinfo->user, username.c_str(), USER_NAME_LEN);
    puserinfo->balance = 0;
    puserinfo->credit = 0;
    puserinfo->status = 0;
    puserinfo->fee = fee;
    
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    apr_hash_set(g_user_info_manage.phandlehash_user, apr_strdup(g_user_info_manage.phandlepool, id), 
        APR_HASH_KEY_STRING, puserinfo);

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_status(int userid, int status)
{
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_user, 
                                                id, APR_HASH_KEY_STRING);
    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from user failed, id=%d\n", userid);
		return SWITCH_STATUS_FALSE;
    }

    pinfo->status = status;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_balance(int userid, double money)
{
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_user, 
                                                id, APR_HASH_KEY_STRING);
    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from user failed, id=%d\n", userid);
		return SWITCH_STATUS_FALSE;
    }

    pinfo->balance += money;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_credit(int userid, double money)
{
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_user, 
                                                id, APR_HASH_KEY_STRING);
    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from user failed, id=%d\n", userid);
		return SWITCH_STATUS_FALSE;
    }

    pinfo->credit += money;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_fee(int userid, double fee)
{
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_user, 
                                                id, APR_HASH_KEY_STRING);
    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from user failed, id=%d\n", userid);
		return SWITCH_STATUS_FALSE;
    }

    pinfo->fee = fee;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_num_add(int userid, const string &num)
{
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_user, 
                                                id, APR_HASH_KEY_STRING);
    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from user failed, id=%d\n", userid);
		return SWITCH_STATUS_FALSE;
    }

    apr_hash_set(g_user_info_manage.phandlehash_num, apr_strdup(g_user_info_manage.phandlepool, num.c_str()), 
        APR_HASH_KEY_STRING, pinfo);
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_num_del(int userid, const string &num)
{
    apr_hash_set(g_user_info_manage.phandlehash_num, num.c_str(), 
        APR_HASH_KEY_STRING, NULL);
    
    return SWITCH_STATUS_SUCCESS;
}


switch_status_t blacklist_manage_user_get(const string &num, USER_INFO &userinfo)
{
    if(num.empty())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"num.empty()\n");
		return SWITCH_STATUS_FALSE;
    }

    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_num, 
                                                num.c_str(), APR_HASH_KEY_STRING);

    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from num failed, num=%s\n", num.c_str());
		return SWITCH_STATUS_FALSE;
    }

    userinfo.userid = pinfo->userid;
    memcpy(userinfo.user, pinfo->user, USER_NAME_LEN);
    userinfo.balance = pinfo->balance;
    userinfo.credit = pinfo->credit;
    userinfo.status = pinfo->status;
    userinfo.fee = pinfo->fee;
    
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t blacklist_manage_user_deduction(int userid, int feeflag)
{
    char id[16];
    snprintf(id, sizeof(id), "%d", userid);
    USER_INFO *pinfo = (USER_INFO *)apr_hash_get(g_user_info_manage.phandlehash_user, 
                                                id, APR_HASH_KEY_STRING);
    if(NULL == pinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
				"get user info from user failed, id=%d\n", userid);
		return SWITCH_STATUS_FALSE;
    }

    if(0 == feeflag)
    {
        pinfo->balance = pinfo->balance - pinfo->fee;
    }
    else
    {
        pinfo->credit = pinfo->credit - pinfo->fee;
    }
    
    return SWITCH_STATUS_SUCCESS;
}


