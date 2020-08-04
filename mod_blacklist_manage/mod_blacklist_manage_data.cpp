

#include "mod_blacklist_manage_data.h"
#include "mod_blacklist_manage_user.h"
#include "mod_blacklist_manage_deduction.h"

extern "C"
{
#include "mod_as_dbcache.h"
}


static switch_status_t bm_user_create_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void * result)
{
	if (NULL == data || data->respdata == NULL)
	{
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "mysql callback data is NULL \n");
        return SWITCH_STATUS_FALSE;
	}

    int *userid = (int*)result;
	*userid =  *(int*)data->respdata;
    
	return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_create(const string &username, double fee)
{    
    ST_DB_CACHE_EVENT event ;
    int userid;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL_INSERT_ID;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "INSERT INTO user_info(USER, balance, credit, STATUS, fee) VALUE('%s', 0, 0, 0, %f)",
			        username.c_str(), fee);
    
    event.pfunmysql = bm_user_create_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &userid))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_create(userid, username, fee))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_create failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_status(int userid, int status)
{
    ST_DB_CACHE_EVENT event ;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "UPDATE user_info SET status=%d WHERE id=%d",
			        status, userid);
    
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, NULL))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_status(userid, status))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_status failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_balance(int userid, double money)
{
    ST_DB_CACHE_EVENT event;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "UPDATE user_info SET balance=balance+%f WHERE id=%d",
			        money, userid);
    
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, NULL))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_balance(userid, money))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_balance failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_credit(int userid, double money)
{
    ST_DB_CACHE_EVENT event;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "UPDATE user_info SET credit=credit+%f WHERE id=%d",
			        money, userid);
    
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, NULL))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_credit(userid, money))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_credit failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_fee(int userid, double fee)
{
    ST_DB_CACHE_EVENT event;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "UPDATE user_info SET fee=%f WHERE id=%d",
			        fee, userid);
    
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, NULL))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_fee(userid, fee))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_fee failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_num_add(int userid, const string &num)
{
    ST_DB_CACHE_EVENT event;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "INSERT INTO user_num(userid, number) VALUE(%d, '%s')",
			        userid, num.c_str());
    
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, NULL))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_num_add(userid, num))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_num_add failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_num_del(int userid, const string &num)
{
    ST_DB_CACHE_EVENT event;
    
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "DELETE FROM user_num WHERE number=%s",
			        num.c_str());
    
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;
    
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, NULL))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_num_del(userid, num))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "blacklist_manage_user_num_del failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t serialize_user_num_data(const USER_NUM_INFO &info, string &userinfo)
{
	cJSON *pcj = cJSON_CreateObject();
    cJSON_AddNumberToObject(pcj, "id", info.userid);  
    cJSON_AddStringToObject(pcj, "username", info.username.c_str());	
    cJSON_AddNumberToObject(pcj, "balance", info.balance); 
    cJSON_AddNumberToObject(pcj, "credit", info.credit); 
    cJSON_AddNumberToObject(pcj, "status", info.status); 
    cJSON_AddNumberToObject(pcj, "fee", info.fee);

    cJSON *pcj_nums = cJSON_CreateArray();
    cJSON_AddItemToObject(pcj, "nums", pcj_nums);	

    vector<string>::const_iterator iter;
	for(iter=info.numbers.begin(); iter!=info.numbers.end(); iter++)
    {
        string number = *iter;
		cJSON *pcj_num = cJSON_CreateObject();
		cJSON_AddItemToArray(pcj_nums, pcj_num);
		cJSON_AddStringToObject(pcj_num, "number", number.c_str());
	}

    char *pJsonDataStr = cJSON_Print(pcj);
    if(NULL == pJsonDataStr)
    {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "serialize_user_num_data failed\n");
		cJSON_Delete(pcj);
		pcj = NULL;
        return SWITCH_STATUS_FALSE;
    }

	userinfo = pJsonDataStr;

	if(pJsonDataStr)
    {
		free(pJsonDataStr);
		pJsonDataStr = NULL;
	}

	if(pcj)
    {
		cJSON_Delete(pcj);
		pcj = NULL;
	}
	
	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t bm_user_getinfo_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
	MYSQL_RES *mysql_res = NULL;
    MYSQL_ROW mysql_row;
    
    if (NULL == data || NULL == result)
    {
        return SWITCH_STATUS_INTR;
    }

    if (NULL == data->respdata)
    {
        return SWITCH_STATUS_NOTFOUND;
    }

    USER_NUM_INFO *pinfo = (USER_NUM_INFO*)result;
    if(NULL == pinfo)
    {
        return SWITCH_STATUS_INTR;
    }
    
    mysql_res = (MYSQL_RES*)data->respdata;
    if (NULL == mysql_res || mysql_num_fields(mysql_res) != 6)
    {
        return SWITCH_STATUS_IGNORE;
    }
    mysql_row = mysql_fetch_row(mysql_res);
    if(NULL == mysql_row)
    {
        return SWITCH_STATUS_NOTFOUND;
    }
    pinfo->username = switch_str_nil(mysql_row[1]);
    pinfo->balance = atof(switch_str_nil(mysql_row[2]));
    pinfo->credit = atof(switch_str_nil(mysql_row[3]));
    pinfo->status = atoi(switch_str_nil(mysql_row[4]));
    pinfo->fee = atof(switch_str_nil(mysql_row[5]));
    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t bm_user_get_num_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
	MYSQL_RES *mysql_res = NULL;
    MYSQL_ROW mysql_row;
    
    if (NULL == data || NULL == result)
    {
        return SWITCH_STATUS_INTR;
    }

    if (NULL == data->respdata)
    {
        return SWITCH_STATUS_NOTFOUND;
    }

    USER_NUM_INFO *pinfo = (USER_NUM_INFO*)result;
    if(NULL == pinfo)
    {
        return SWITCH_STATUS_INTR;
    }
    
    mysql_res = (MYSQL_RES*)data->respdata;
    if (NULL == mysql_res || mysql_num_fields(mysql_res) != 2)
    {
        return SWITCH_STATUS_IGNORE;
    }
    
    while(NULL != (mysql_row = mysql_fetch_row(mysql_res)))
    {
        pinfo->numbers.push_back(switch_str_nil(mysql_row[1]));
    }
    
    return SWITCH_STATUS_SUCCESS;
}


switch_status_t bm_user_getinfo(int userid, string &userinfo)
{
    ST_DB_CACHE_EVENT event;
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "SELECT id, user, balance, credit, status, fee FROM user_info WHERE id=%d",
			        userid);
    
    event.pfunmysql = bm_user_getinfo_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;

    USER_NUM_INFO user_num_info;
    user_num_info.userid = userid;
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &user_num_info))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "SELECT userid, number FROM user_num WHERE userid=%d",
			        userid);
    
    event.pfunmysql = bm_user_get_num_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;

    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &user_num_info))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != serialize_user_num_data(user_num_info, userinfo))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "serialize_user_num_data failed\n");
        return SWITCH_STATUS_FALSE;
    }

    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t bm_user_getinfo_all_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
	MYSQL_RES *mysql_res = NULL;
    MYSQL_ROW mysql_row;
    
    if (NULL == data || NULL == result)
    {
        return SWITCH_STATUS_INTR;
    }

    if (NULL == data->respdata)
    {
        return SWITCH_STATUS_NOTFOUND;
    }

    vector<USER_NUM_INFO> *pvec_info = (vector<USER_NUM_INFO> *)result;
    if(NULL == pvec_info)
    {
        return SWITCH_STATUS_INTR;
    }
    
    mysql_res = (MYSQL_RES*)data->respdata;
    if (NULL == mysql_res || mysql_num_fields(mysql_res) != 6)
    {
        return SWITCH_STATUS_IGNORE;
    }
    while(NULL != (mysql_row = mysql_fetch_row(mysql_res)))
    {
        USER_NUM_INFO info;
        info.userid = atoi(switch_str_nil(mysql_row[0]));
        info.username = switch_str_nil(mysql_row[1]);
        info.balance = atof(switch_str_nil(mysql_row[2]));
        info.credit = atof(switch_str_nil(mysql_row[3]));
        info.status = atoi(switch_str_nil(mysql_row[4]));
        info.fee = atof(switch_str_nil(mysql_row[5]));

        pvec_info->push_back(info);
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_getinfo_all(string &userinfo)
{
    ST_DB_CACHE_EVENT event;
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "SELECT id, user, balance, credit, status, fee FROM user_info");
    
    event.pfunmysql = bm_user_getinfo_all_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;

    vector<USER_NUM_INFO> vec_user_num_info;
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &vec_user_num_info))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    userinfo = string("[") + "\n";
    vector<USER_NUM_INFO>::iterator iter;
    for(iter=vec_user_num_info.begin(); iter!=vec_user_num_info.end(); iter++)
    {
        USER_NUM_INFO *pinfo = &(*iter);
        memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
        event.type = EN_TYPE_MYSQL;
        switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
    			        "SELECT userid, number FROM user_num WHERE userid=%d",
    			        pinfo->userid);
        
        event.pfunmysql = bm_user_get_num_mysql_callback;
        event.iscache = SWITCH_FALSE;
        strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
        event.timeout = 10;

        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, pinfo))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                    "mod_as_dbcache_query failed\n");
            return SWITCH_STATUS_FALSE;
        }

        string tmpinfo;
        if(SWITCH_STATUS_SUCCESS != serialize_user_num_data(*pinfo, tmpinfo))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                    "serialize_user_num_data failed\n");
            return SWITCH_STATUS_FALSE;
        }

        userinfo += tmpinfo + "," + "\n";
    }

    size_t found = userinfo.rfind(",");
    if (found != string::npos)
    {
        userinfo.replace (found, 1, "\n]");
    }
    else
    {
        userinfo += "]";
    }
    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t bm_user_checkinfo_mysql_callback(_In_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
	MYSQL_RES *mysql_res = NULL;
    MYSQL_ROW mysql_row;
    
    if (NULL == data || NULL == result)
    {
        return SWITCH_STATUS_INTR;
    }

    if (NULL == data->respdata)
    {
        return SWITCH_STATUS_NOTFOUND;
    }

    USER_CHECKINFO *pcheckinfo = (USER_CHECKINFO*)result;
    if(NULL == pcheckinfo)
    {
        return SWITCH_STATUS_INTR;
    }
    
    mysql_res = (MYSQL_RES*)data->respdata;
    if (NULL == mysql_res || mysql_num_fields(mysql_res) != 3)
    {
        return SWITCH_STATUS_IGNORE;
    }
    mysql_row = mysql_fetch_row(mysql_res);
    if(NULL == mysql_row)
    {
        return SWITCH_STATUS_NOTFOUND;
    }
    pcheckinfo->total = atoi(switch_str_nil(mysql_row[0]));
    pcheckinfo->black = atoi(switch_str_nil(mysql_row[1]));
    pcheckinfo->username = switch_str_nil(mysql_row[2]);
    pcheckinfo->normal = pcheckinfo->total - pcheckinfo->black;
    
    return SWITCH_STATUS_SUCCESS;
}


switch_status_t bm_user_checkinfo(int userid, const string &day, string &strcheckinfo)
{   
    ST_DB_CACHE_EVENT event ;
	memset(&event, 0, sizeof(ST_DB_CACHE_EVENT));   
    event.type = EN_TYPE_MYSQL;
    switch_snprintf(event.mysql_command, DB_QUERY_COMMAND_MAX_LEN, 
			        "SELECT a.total, b.black, c.user FROM " \
			        "(SELECT COUNT(userid) AS total FROM bl_check_%s WHERE userid=%d) a, " \
			        "(SELECT COUNT(hit) AS black FROM bl_check_%s WHERE userid=%d AND hit=1) b, " \
			        "(SELECT USER FROM user_info WHERE id=%d) c ",
			        day.c_str(), userid,
			        day.c_str(), userid,
			        userid);
    
    event.pfunmysql = bm_user_checkinfo_mysql_callback;
    event.iscache = SWITCH_FALSE;
    strncpy(event.uuid_str, "", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 10;

    USER_CHECKINFO checkinfo;
    checkinfo.userid = userid;
    checkinfo.day = day;
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, &checkinfo))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING,
                "mod_as_dbcache_query failed\n");
        return SWITCH_STATUS_FALSE;
    }

    char info[1024];
    snprintf(info, sizeof(info), 
        "{\"id\":%d,\"user\":\"%s\",\"day\":\"%s\",\"total\":%d,\"normal\":%d,\"black\":%d}",
        checkinfo.userid, checkinfo.username.c_str(), checkinfo.day.c_str(),
        checkinfo.total, checkinfo.normal, checkinfo.black);

    strcheckinfo = info;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_user_deduction(int userid, int feeflag)
{
    if(SWITCH_STATUS_SUCCESS != blacklist_manage_deduction_write(userid, feeflag))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "blacklist_manage_deduction_write failed, userid = %d, feeflag = %d\n", 
                userid, feeflag);
        return SWITCH_STATUS_FALSE;
    }    

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_deduction(userid, feeflag))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "blacklist_manage_user_deduction failed, userid = %d, feeflag = %d\n", 
                userid, feeflag);
        return SWITCH_STATUS_FALSE;
    }   

    return SWITCH_STATUS_SUCCESS;
}

void get_time_str(char *pdate, int len)
{
	time_t t = time(0);
    strftime(pdate, len, "%Y%m%d", localtime(&t));
}

void get_time_str_tomorrow(char *pdate, int len)
{
	time_t t = time(0) + 86400;
    strftime(pdate, len, "%Y%m%d", localtime(&t));
}


