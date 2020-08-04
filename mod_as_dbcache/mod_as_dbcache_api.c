
#include <switch.h>
#include "mod_as_dbcache_api.h"
#include "mod_as_dbcache.h"

extern int g_as_use_redis;

typedef switch_status_t (*api_command_t) (char **argv, int argc, switch_stream_handle_t *stream);

typedef struct ST_CMD_THREAD_PARAM
{
    int threadNum;
    switch_stream_handle_t *stream;
}CMD_THREAD_PARAM;

void api_set_complete()
{
    switch_console_set_complete("add as_dbcache get_relayinfo_byout_multithread [threadnum]");  
    switch_console_set_complete("add as_dbcache jsontest");  
    switch_console_set_complete("add as_dbcache cmd");  
    switch_console_set_complete("add as_dbcache cmd_multi [threadnum]");  
}

switch_status_t as_dbcache_api(_In_opt_z_ const char *cmd, _In_opt_ switch_core_session_t *session, _In_ switch_stream_handle_t *stream)
{
    char *argv[1024] = { 0 };
    int argc = 0;
    char *mycmd = NULL;
    switch_status_t status = SWITCH_STATUS_SUCCESS;
    api_command_t func = NULL;
    int lead = 1;
    static const char usage_string[] = "USAGE:\n"
        "--------------------------------------------------------------------------------\n"
        "as_dbcache get_relayinfo_byout_multithread [threadnum]\n"
        "as_dbcache jsontest\n"
        "as_dbcache cmd\n"
        "as_dbcache cmd_multi [threadnum]\n"
        "--------------------------------------------------------------------------------\n";

    session = session;
    
    if (zstr(cmd)) 
    {
        stream->write_function(stream, "%s", usage_string);
        goto GOTO_as_common_api_function_done_flag;
    }

    if (!(mycmd = strdup(cmd))) 
    {
        status = SWITCH_STATUS_MEMERR;
        goto GOTO_as_common_api_function_done_flag;
    }

    if (!(argc = switch_separate_string(mycmd, ' ', argv, (sizeof(argv) / sizeof(argv[0])))) || !argv[0]) 
    {
        stream->write_function(stream, "%s", usage_string);
        goto GOTO_as_common_api_function_done_flag;
    }

    if (0 == strcmp(argv[0], "get_relayinfo_byout_multithread")) 
    {
        func = cmd_get_relayinfo_byout_multithread;
    }
    else if(0 == strcmp(argv[0], "jsontest"))
    {
        func = cmd_jsontest;
    }
    else if(0 == strcmp(argv[0], "cmd"))
    {
        if(1 == g_as_use_redis)
        {
            func = cmd_redis_cmd;
        }
        else
        {
            func = redis_not_used;
        }
    }
    else if(0 == strcmp(argv[0], "cmd_multi"))
    {
        if(1 == g_as_use_redis)
        {
            func = cmd_redis_cmd_multithread;
        }
        else
        {
            func = redis_not_used;
        }
    }

    if (func) 
    {
        status = func(&argv[lead], argc - lead, stream);
    } 
    else 
    {
        stream->write_function(stream, "Unknown Command [%s]\n", argv[0]);
    }

GOTO_as_common_api_function_done_flag:
    switch_safe_free(mycmd);
    return status;
}

void *SWITCH_THREAD_FUNC thread_func_get_relayinfo_byout(switch_thread_t *t, void *data)
{
    CMD_THREAD_PARAM *pThreadParam = (CMD_THREAD_PARAM *)data;
    int threadNum = 0;
    switch_stream_handle_t* stream = NULL;
    int i = 1000;
    int successcount = 0;
    int failcount = 0;
    char uuid[64] = {0};

    t = t;

    if(NULL == pThreadParam || NULL == pThreadParam->stream)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "thread_func_get_relayinfo_byout stop NULL == stream \n");
        return NULL;
    }
    stream = pThreadParam->stream;
    threadNum = pThreadParam->threadNum;

    while(i--)
    {
        TRUNK_INFO trunkinfo;
        memset(&trunkinfo, 0, sizeof(TRUNK_INFO));
        switch_snprintf(trunkinfo.szOutExternalIp, AS_OUT_IN_IP_LEN, "%d", i);
        switch_snprintf(trunkinfo.szOutExternalPort, AS_OUT_IN_PORT_LEN, "%d", i);
        switch_snprintf(trunkinfo.szOutLocalIp, AS_OUT_IN_IP_LEN, "%d", i);
        switch_snprintf(trunkinfo.szOutLocalPort, AS_OUT_IN_PORT_LEN, "%d", i);
        
        switch_snprintf(uuid, 64, "uuid_%d_%d", i, threadNum);

        if(SWITCH_STATUS_SUCCESS != as_dbcache_get_info_by_out(&trunkinfo, uuid, stream))
        {
            failcount++;
        }
        else
        {
            successcount++;
        }
    }

    stream->write_function(stream, 
                "thread_func_get_relayinfo_byout(%d) stop, successcount = %d, failcount = %d\n",
                threadNum, successcount, failcount);
    return NULL;
}

static switch_memory_pool_t *pcmd_data_pool = NULL;
#define DATA_MULTI_THREAD_NUM   (1000)
static switch_thread_t *pthreadids[DATA_MULTI_THREAD_NUM] = {0};

switch_status_t cmd_get_relayinfo_byout_multithread(char **argv, int argc, switch_stream_handle_t *stream)
{
    int threadcount = 0;
    CMD_THREAD_PARAM cmdTheadParam[DATA_MULTI_THREAD_NUM];
    memset(&cmdTheadParam, 0, sizeof(cmdTheadParam));
    
    if (NULL == argv || NULL == stream)
    {
        return SWITCH_STATUS_FALSE;
    }

    argc = argc;

    threadcount = atoi(switch_str_nil(argv[0]));
    memset(pthreadids, 0, DATA_MULTI_THREAD_NUM);
    if(NULL == pcmd_data_pool)
	{
        if(SWITCH_STATUS_SUCCESS != switch_core_new_memory_pool(&pcmd_data_pool))
        {
            stream->write_function(stream, "pcmd_data_pool init failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }
    
    threadcount = (threadcount>DATA_MULTI_THREAD_NUM)? DATA_MULTI_THREAD_NUM: threadcount;
    threadcount = (threadcount<1)? 1: threadcount;
    for(int i = 0; i < threadcount; i++)
    {
        cmdTheadParam[i].stream = stream;
        cmdTheadParam[i].threadNum = i;
        if(SWITCH_STATUS_SUCCESS != switch_thread_create(&(pthreadids[i]), 
                                                    NULL, 
                                                    thread_func_get_relayinfo_byout, 
                                                    &cmdTheadParam[i], 
                                                    pcmd_data_pool))
        {
            stream->write_function(stream, "switch_thread_create failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }

    for(int i = 0; i < threadcount; i++)
    {
        switch_status_t status;
        switch_thread_join(&status, pthreadids[i]);
    }

    if(NULL != pcmd_data_pool)
    {
        switch_core_destroy_memory_pool(&pcmd_data_pool);
        pcmd_data_pool = NULL;
    }

    stream->write_function(stream, "SUCCESS!\n");
    return SWITCH_STATUS_SUCCESS;
}


//#define JSON_BLACK_LIST ({"1111", "2222", "3333"})
switch_status_t cmd_jsontest(char **argv, int argc, switch_stream_handle_t *stream)
{
    char blacklist[1024] = {0};
    cJSON *cjbl = NULL;
    int count = 0;
    char *p1 = NULL;
    char *p2 = NULL;
    char *p3 = NULL;
    char *p4 = NULL;
    argv = argv;
    argc = argc;
    stream = stream;

    switch_snprintf(blacklist, 1024, "[{\"p1\":\"1\",\"p2\":\"111\",\"p3\":\"075561944000\",\"p4\":\"075561944999\"}]");

    cjbl = cJSON_Parse(blacklist);
    if(NULL == cjbl)
    {
        stream->write_function(stream, "cmd_jsontest, NULL == cjbl, blacklist=%s\n", blacklist); 
        return SWITCH_STATUS_FALSE;
    }
    
    count = cJSON_GetArraySize(cjbl);

    stream->write_function(stream, "cmd_jsontest, count=%d, blacklist=%s\n", count, blacklist);

    for(int i = 0; i < count; i++)
    {
        cJSON *cjitem = cJSON_GetArrayItem(cjbl, i);
        if(NULL == cjitem)
        {
            stream->write_function(stream, "cmd_jsontest, NULL == cjitem\n"); 
            continue;
        }
        p1 = (char *)cJSON_GetObjectCstr(cjitem, "p1");
        p2 = (char *)cJSON_GetObjectCstr(cjitem, "p2");
        p3 = (char *)cJSON_GetObjectCstr(cjitem, "p3");
        p4 = (char *)cJSON_GetObjectCstr(cjitem, "p4");

        stream->write_function(stream, "cmd_jsontest, i=%d, p1=%s, p2=%s, p3=%s, p4=%s\n", i, p1, p2, p3, p4); 
    }
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t as_dbcache_get_relayid_redis_res_proc(_Inout_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
    if (NULL == data || NULL == result)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "Input parameter(s) is NULL. data:%p, result:%p\n", 
            (void*)data, (void*)result);
        return SWITCH_STATUS_INTR;
    }

    if(NULL == data->event || NULL == data->event->uuid_str)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
            "NULL == data->event || NULL == data->event->uuid_str\n");
        return SWITCH_STATUS_INTR;
    }

    if (EN_TYPE_REDIS != data->resptype)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING, 
            "This is a redis proc fun. rsptype:%d\n", data->resptype);
        return SWITCH_STATUS_FALSE;
    }
    
    if (NULL == data->respdata)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING, 
            "Redis return value is NULL.\n");
        return SWITCH_STATUS_NOTFOUND;
    }

    switch_copy_string(result, data->respdata, AS_RELAY_ID_LEN);
    
    return SWITCH_STATUS_SUCCESS;
}

static switch_status_t as_dbcache_get_trunk_info_redis_res_proc(_Inout_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
    TRUNK_INFO *ptrunkinfo = NULL;
    cJSON *cjrelayinfo = NULL;
    int argc = 0;
    char *argv[2] = {NULL};
    char *outremoteipport = NULL;
    char *outlocalipport = NULL;
    char *inremoteipport = NULL;
    char *inlocalipport = NULL;
    char *sipuri = NULL;

    if (NULL == data || NULL == result)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "Input parameter(s) is NULL. data:%p, result:%p\n", 
            (void*)data, (void*)result);
        return SWITCH_STATUS_INTR;
    }

    if(NULL == data->event || NULL == data->event->uuid_str)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR,
            "NULL == data->event || NULL == data->event->uuid_str\n");
        return SWITCH_STATUS_INTR;
    }

    if (EN_TYPE_REDIS != data->resptype)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING, 
            "This is a redis proc fun. rsptype:%d\n", data->resptype);
        return SWITCH_STATUS_FALSE;
    }
    
    if (NULL == data->respdata)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING, 
            "Redis return value is NULL.\n");
        return SWITCH_STATUS_NOTFOUND;
    }

    ptrunkinfo = (TRUNK_INFO *)result;
    cjrelayinfo = cJSON_Parse(data->respdata);
    if(NULL == cjrelayinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING, 
            "NULL = cjrelayinfo, data->respdata = %s\n", (char *)data->respdata);
        return SWITCH_STATUS_FALSE;
    }

    outremoteipport = (char *)cJSON_GetObjectCstr(cjrelayinfo, AS_JSON_OUT_EXTERNAL_IPPORT);
    if (2 != (argc = switch_separate_string(outremoteipport, ':', argv, switch_arraylen(argv))) || !argv[0])
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING,
            "Redis return value format is illegal. remoteipport:%s, argc:%d != 2, cmd:%s\n", 
            outremoteipport, argc, data->event ? data->event->redis_command : "(null)");
        goto goto_fail_result;
    }
    switch_copy_string(ptrunkinfo->szOutExternalIp, switch_str_nil(argv[0]), AS_OUT_IN_IP_LEN);
    switch_copy_string(ptrunkinfo->szOutExternalPort, switch_str_nil(argv[1]), AS_OUT_IN_PORT_LEN);

    outlocalipport = (char *)cJSON_GetObjectCstr(cjrelayinfo, AS_JSON_OUT_LOCAL_IPPORT);
    if (2 != (argc = switch_separate_string(outlocalipport, ':', argv, switch_arraylen(argv))) || !argv[0])
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING,
            "Redis return value format is illegal. localipport:%s, argc:%d != 2, cmd:%s\n", 
            outlocalipport, argc, data->event ? data->event->redis_command : "(null)");
        goto goto_fail_result;
    }
    switch_copy_string(ptrunkinfo->szOutLocalIp, switch_str_nil(argv[0]), AS_OUT_IN_IP_LEN);
    switch_copy_string(ptrunkinfo->szOutLocalPort, switch_str_nil(argv[1]), AS_OUT_IN_PORT_LEN);

    inremoteipport = (char *)cJSON_GetObjectCstr(cjrelayinfo, AS_JSON_IN_EXTERNAL_IPPORT);
    if (2 != (argc = switch_separate_string(inremoteipport, ':', argv, switch_arraylen(argv))) || !argv[0])
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING,
            "Redis return value format is illegal. remoteipport:%s, argc:%d != 2, cmd:%s\n", 
            inremoteipport, argc, data->event ? data->event->redis_command : "(null)");
        goto goto_fail_result;
    }
    switch_copy_string(ptrunkinfo->szInExternalIp, switch_str_nil(argv[0]), AS_OUT_IN_IP_LEN);
    switch_copy_string(ptrunkinfo->szInExternalPort, switch_str_nil(argv[1]), AS_OUT_IN_PORT_LEN);

    inlocalipport = (char *)cJSON_GetObjectCstr(cjrelayinfo, AS_JSON_IN_LOCAL_IPPORT);
    if (2 != (argc = switch_separate_string(inlocalipport, ':', argv, switch_arraylen(argv))) || !argv[0])
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(data->event->uuid_str), SWITCH_LOG_WARNING,
            "Redis return value format is illegal. localipport:%s, argc:%d != 2, cmd:%s\n", 
            inlocalipport, argc, data->event ? data->event->redis_command : "(null)");
        goto goto_fail_result;
    }
    switch_copy_string(ptrunkinfo->szInLocalIp, switch_str_nil(argv[0]), AS_OUT_IN_IP_LEN);
    switch_copy_string(ptrunkinfo->szInLocalPort, switch_str_nil(argv[1]), AS_OUT_IN_PORT_LEN);

    ptrunkinfo->unTrunkBusiness = cJSON_GetObjectItem(cjrelayinfo, AS_JSON_BUSINESS)->valueint;

    sipuri = (char *)cJSON_GetObjectCstr(cjrelayinfo, AS_JSON_SIP_URI);
    switch_copy_string(ptrunkinfo->trunkSipUriInfo.m_szSipUri, sipuri, AS_SIP_URI_LEN);

    ptrunkinfo->unRuri = cJSON_GetObjectItem(cjrelayinfo, AS_JSON_R_URI)->valueint;

    cJSON_Delete(cjrelayinfo);    
    return SWITCH_STATUS_SUCCESS;

goto_fail_result:
    cJSON_Delete(cjrelayinfo);
    return SWITCH_STATUS_IGNORE;
    
}

switch_status_t as_dbcache_get_info_by_out(_Out_ TRUNK_INFO *trunk_info,_In_ const char* uuid, switch_stream_handle_t* stream)
{
    ST_DB_CACHE_EVENT eventrelayid = {0, {0}, {0}, 0, 0, 0, {0}, {0}, 0};
    ST_DB_CACHE_EVENT event = {0, {0}, {0}, 0, 0, 0, {0}, {0}, 0};
    char relayid[AS_RELAY_ID_LEN] = {0};

    if (NULL == trunk_info || NULL == uuid || NULL == stream)
    {
        stream->write_function(stream, 
            "Input parameter(s) is NULL. trunk_info:%p, uuid = %p, stream = %p\n", (void*)trunk_info, (void*)uuid, (void*)stream);
        return SWITCH_STATUS_INTR;
    }

    //先查询relayid
    eventrelayid.type = EN_TYPE_REDIS;
    switch_snprintf(eventrelayid.redis_command, DB_QUERY_COMMAND_MAX_LEN, 
                                    "GET SIP:FSPROXY_OUT:%s:%s_%s:%s",
                                    trunk_info->szOutExternalIp,
                                    trunk_info->szOutExternalPort,
                                    trunk_info->szOutLocalIp,
                                    trunk_info->szOutLocalPort);
    eventrelayid.pfunredis = as_dbcache_get_relayid_redis_res_proc;
    eventrelayid.pfunmysql = NULL;
    eventrelayid.iscache = SWITCH_FALSE;
    switch_snprintf(eventrelayid.uuid_str, SWITCH_UUID_FORMATTED_LENGTH, "%s", uuid);
    eventrelayid.timeout = 3;
    if (SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&eventrelayid, (void*)relayid))
    {
        stream->write_function(stream, "get relayid failed\n");
        return SWITCH_STATUS_FALSE;
    }

    /* 向DB查询数据(redis) */
    event.type = EN_TYPE_REDIS;
    switch_snprintf(event.redis_command, DB_QUERY_COMMAND_MAX_LEN, "GET SIP:BASICRELAY_INFO:%s", relayid);
    event.pfunredis = as_dbcache_get_trunk_info_redis_res_proc;
    event.pfunmysql = NULL;
    event.iscache = SWITCH_FALSE;
    switch_snprintf(event.uuid_str, SWITCH_UUID_FORMATTED_LENGTH, "%s", uuid);
    event.timeout = 3;
    if (SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, (void*)trunk_info))
    {
        return SWITCH_STATUS_FALSE;
    }
    stream->write_function(stream,
            "trunk_info:\n" \
            "szOutExternalIp:%s \n" \
            "szOutExternalPort:%s \n" \
            "szOutLocalIp:%s \n" \
            "szOutLocalPort:%s \n" \
            "szInExternalIp:%s \n" \
            "szInExternalPort:%s \n" \
            "szInLocalIp:%s \n" \
            "szInLocalPort:%s \n" \
            "unTrunkBusiness:%u \n" ,
            trunk_info->szOutExternalIp,
            trunk_info->szOutExternalPort,
            trunk_info->szOutLocalIp,
            trunk_info->szOutLocalPort,
            trunk_info->szInExternalIp,
            trunk_info->szInExternalPort,
            trunk_info->szInLocalIp,
            trunk_info->szInLocalPort,
            trunk_info->unTrunkBusiness
            );

	return SWITCH_STATUS_SUCCESS;
}

static switch_status_t cmd_redis_cmd_redis_res_proc(_Inout_ ST_DB_CACHE_QUEUE_RESP_DATA *data, _Out_ void *result)
{
    switch_stream_handle_t* stream = (switch_stream_handle_t*)result;
    if(NULL != stream)
    {
        stream->write_function(stream, "msg: %s\n", (char*)(data->respdata));
    }
    else
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "NULL == stream ; msg: %s\n", (char*)(data->respdata));
    }
    
    result = result;

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t cmd_redis_cmd(char **argv, int argc, switch_stream_handle_t *stream)
{
    int i = 0;
    int len = 0;
    ST_DB_CACHE_EVENT event = {0, {0}, {0}, 0, 0, 0, {0}, {0}, 0};

    for (i = 0; i < argc; ++i)
    {
        len = strlen(event.redis_command);
        switch_snprintf(event.redis_command + len, DB_QUERY_COMMAND_MAX_LEN - len, "%s%s", i == 0 ? "" : " ", argv[i]);
    }
       
    event.type = EN_TYPE_REDIS;
    event.pfunredis = cmd_redis_cmd_redis_res_proc;
    event.iscache = SWITCH_FALSE;
    switch_copy_string(event.uuid_str, "test", SWITCH_UUID_FORMATTED_LENGTH);
    event.timeout = 3;
    
    mod_as_dbcache_query(&event, (void*)stream); 

    return SWITCH_STATUS_SUCCESS;
}

void *SWITCH_THREAD_FUNC thread_func_cmd_loop(switch_thread_t *t, void *data)
{
    CMD_THREAD_PARAM *pThreadParam = (CMD_THREAD_PARAM *)data;
    int threadNum = 0;
    switch_stream_handle_t* stream = NULL;
    int i = 75;
    int successcount = 0;
    int failcount = 0;
    switch_uuid_t uuid_t;
    char uuid[64] = {0};
    int interval = 400 * 1000;

    ST_DB_CACHE_EVENT event = {0, {0}, {0}, 0, 0, 0, {0}, {0}, 0};

    t = t;

    if(NULL == pThreadParam || NULL == pThreadParam->stream)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "thread_func_cmd_loop stop NULL == stream \n");
        return NULL;
    }
    
    stream = pThreadParam->stream;
    threadNum = pThreadParam->threadNum;
    
    event.type = EN_TYPE_REDIS;
    switch_snprintf(event.redis_command, DB_QUERY_COMMAND_MAX_LEN, "get SIPURI:0123456789");
    event.pfunredis = cmd_redis_cmd_redis_res_proc;
    event.iscache = SWITCH_FALSE;    
    event.timeout = 2;
   
    while(i--)
    {
        switch_uuid_get(&uuid_t);
        switch_uuid_format(uuid, &uuid_t);
        switch_copy_string(event.uuid_str, uuid, SWITCH_UUID_FORMATTED_LENGTH);
        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_query(&event, stream))
        {
            failcount++;
        }
        else
        {
            successcount++;
        }

        usleep(interval);
    }

    stream->write_function(stream, 
                "thread_func_cmd_loop(%d) stop, successcount = %d, failcount = %d\n",
                threadNum, successcount, failcount);

    return NULL;
}


switch_status_t cmd_redis_cmd_multithread(char **argv, int argc, switch_stream_handle_t *stream)
{
    int threadcount = 0;
    CMD_THREAD_PARAM cmdTheadParam[DATA_MULTI_THREAD_NUM];
    memset(&cmdTheadParam, 0, sizeof(cmdTheadParam));
    
    if (NULL == argv || NULL == stream)
    {
        return SWITCH_STATUS_FALSE;
    }

    argc = argc;

    threadcount = atoi(switch_str_nil(argv[0]));
    memset(pthreadids, 0, DATA_MULTI_THREAD_NUM);
    if(NULL == pcmd_data_pool)
	{
        if(SWITCH_STATUS_SUCCESS != switch_core_new_memory_pool(&pcmd_data_pool))
        {
            stream->write_function(stream, "pcmd_data_pool init failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }
    
    threadcount = (threadcount>DATA_MULTI_THREAD_NUM)? DATA_MULTI_THREAD_NUM: threadcount;
    threadcount = (threadcount<1)? 1: threadcount;
    for(int i = 0; i < threadcount; i++)
    {
        cmdTheadParam[i].stream = stream;
        cmdTheadParam[i].threadNum = i;

        if(SWITCH_STATUS_SUCCESS != switch_thread_create(&(pthreadids[i]), 
                                                    NULL, 
                                                    thread_func_cmd_loop, 
                                                    &cmdTheadParam[i], 
                                                    pcmd_data_pool))
        {
            stream->write_function(stream, "switch_thread_create failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }

    for(int i = 0; i < threadcount; i++)
    {
        switch_status_t status;
        switch_thread_join(&status, pthreadids[i]);
    }

    if(NULL != pcmd_data_pool)
    {
        switch_core_destroy_memory_pool(&pcmd_data_pool);
        pcmd_data_pool = NULL;
    }
    stream->write_function(stream, "SUCCESS !\n");
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t redis_not_used(char **argv, int argc, switch_stream_handle_t *stream)
{
    argv = argv;
    argc  = argc ;
    stream->write_function(stream, "redis is not used\n");
    return SWITCH_STATUS_SUCCESS;
}

