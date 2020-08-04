


#include <apr_errno.h>

#include "mod_as_dbcache_queue.h"
#include "mod_as_dbcache_redis.h"
#include "mod_as_dbcache_mysql.h"

#define MOD_AS_DBCACHE_MAIN_QUEUE_MAX_SIZE      (2048)
#define MOD_AS_DBCACHE_MAIN_QUEUE_OVER_FLOW     (128)
#define MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER        (16)

#define MOD_AS_DBCACHE_RESP_QUEUE_MAX_SIZE      (32)
#define MOD_AS_DBCACHE_RESP_QUEUE_OVER_FLOW     (16)
#define MOD_AS_DBCACHE_RESP_QUEUE_NUMBER        (2048)

static switch_memory_pool_t *pmod_as_dbcache_queue_pool = NULL;

static switch_mutex_t *pmod_as_dbcache_queue_mutex_main = NULL;
static switch_mutex_t *pmod_as_dbcache_queue_mutex_resp = NULL;

static ST_DB_CACHE_MAIN_QUEUE mod_as_dbcache_queue_main[MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER] = {{NULL}};
static ST_DB_CACHE_RESP_QUEUE mod_as_dbcache_queue_resp[MOD_AS_DBCACHE_RESP_QUEUE_NUMBER] = {{NULL}};

static switch_thread_t *pmod_as_dbcache_queue_threads[MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER] = {NULL};
static int mod_as_dbcache_queue_sequence = 0;
static switch_bool_t isinit = SWITCH_FALSE;

static int kaliveinterval = 0;
static int taskid = 0;
extern int g_as_use_redis;

static ST_DB_CACHE_HANDLER *mod_as_dbcache_queue_handler_alloc()
{
	ST_DB_CACHE_HANDLER *handler= NULL;

	handler = calloc(1, sizeof(ST_DB_CACHE_HANDLER));

	return handler;
}

static void mod_as_dbcache_queue_handler_free(ST_DB_CACHE_HANDLER **handler)
{
    if((*handler) && ((*handler)->respqueue))
    {
        (*handler)->respqueue->flag = EN_FLAG_IDLE;
    }
	switch_safe_free(*handler);
}

static ST_DB_CACHE_QUEUE_RESP_DATA *mod_as_dbcache_queue_resp_data_alloc()
{
	ST_DB_CACHE_QUEUE_RESP_DATA *data= NULL;

	data = calloc(1, sizeof(ST_DB_CACHE_QUEUE_RESP_DATA));

    if(data)
    {
        data->event = calloc(1, sizeof(ST_DB_CACHE_EVENT));
    }

	return data;
}

static void mod_as_dbcache_queue_resp_data_free(ST_DB_CACHE_QUEUE_RESP_DATA **data)
{
    switch_safe_free((*data)->respdata);
    switch_safe_free((*data)->event);
	switch_safe_free(*data);
}

//main queue deal thread
static void *SWITCH_THREAD_FUNC mod_as_dbcache_queue_listen_thread(switch_thread_t *t, void *data)
{    
    void *popdata = NULL;
    ST_DB_CACHE_QUEUE_RESP_DATA *prespdata = NULL;
    ST_DB_CACHE_RESP_QUEUE *prespqueue = NULL;
    ST_DB_CACHE_EVENT *pevent = NULL;
    switch_queue_t *pqueue = NULL;
    int sequence = 0;
    int threadcontinue = 1;
    ST_DB_CACHE_MAIN_QUEUE *mainqueue = (ST_DB_CACHE_MAIN_QUEUE *)data;
    int queuesize = 0;

    t = t;

    if(NULL == mainqueue)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "mod_as_dbcache_queue_listen_thread fail, NULL == mainqueue\n");
        return NULL;
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_queue_listen_thread start: %p\n", (void *)mainqueue->pqueue);

    while (threadcontinue) {
        popdata = NULL;
        prespdata = NULL;
        prespqueue = NULL;
        pevent = NULL;        
        pqueue = NULL;
        sequence = 0;
        queuesize = 0;

        if (switch_queue_pop(mainqueue->pqueue, &popdata) != SWITCH_STATUS_SUCCESS) {
            break;
        }

        if (NULL == popdata) {
            break;
        }

        prespdata = (ST_DB_CACHE_QUEUE_RESP_DATA *)popdata;
        if(NULL == prespdata)
        {
            continue;
        }
        
        //TODO: query process
        prespqueue = prespdata->respqueue;     
        pevent = prespdata->event;
        if(NULL == prespqueue)
        {
            if(prespdata->event && (EN_TYPE_REDIS_KEEP_ALIVE == prespdata->event->type))
            {
                //keep alive redis
                if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_queue_db_query_process(prespdata))
                {
                    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(pevent->uuid_str), SWITCH_LOG_ERROR, 
                            "seq[%d], mod_as_dbcache_queue_db_query_process failed\n", sequence);
                }

                mod_as_dbcache_queue_resp_data_free(&prespdata);
            }
            else
            {
                continue;
            }
        }
        else
        {
            pqueue = prespdata->respqueue->pqueue;
            sequence = prespdata->sequence;
            if(NULL == pevent || NULL == pqueue)
            {
                continue;
            }

            //query db
            if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_queue_db_query_process(prespdata))
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(pevent->uuid_str), SWITCH_LOG_ERROR, 
                        "seq[%d], mod_as_dbcache_queue_db_query_process failed\n", sequence);
            }

            queuesize = switch_queue_size(pqueue);
            if(queuesize < MOD_AS_DBCACHE_RESP_QUEUE_MAX_SIZE)
            {
                if(SWITCH_STATUS_SUCCESS != switch_queue_push(pqueue, prespdata))
                {
                    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(pevent->uuid_str), SWITCH_LOG_ERROR, 
                            "seq[%d], switch_queue_push failed\n", sequence);
                    mod_as_dbcache_queue_resp_data_free(&prespdata);
                }
            }
            else
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(pevent->uuid_str), SWITCH_LOG_ERROR, 
                            "seq[%d], switch_queue_push failed, respqueue[%p] is full[%d]\n", 
                            sequence, (void*)pqueue, queuesize);
                mod_as_dbcache_queue_resp_data_free(&prespdata);
            }
        }

        mainqueue->lasttime = time(NULL);
        
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_queue_listen_thread stop: %p\n", (void *)mainqueue->pqueue);
    
    return NULL;
}

SWITCH_STANDARD_SCHED_FUNC(task_keep_redis_alive)
{
    int lasttime = 0;
    int curtime = 0;
    int cursize = 0;

//    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
//            "SWITCH_STANDARD_SCHED_FUNC(task_keep_redis_alive)\n");    

    //check the lasttime of main queue, and send 'GET MOD_AS_DBCACHE_QUEUE_KEEP_ALIVE' to redis
    for(int i = 0; i < MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER; i++)
    {
        if(!isinit)
        {
            return;
        }
        curtime = time(NULL);
        lasttime = mod_as_dbcache_queue_main[i].lasttime;
        cursize = switch_queue_size(mod_as_dbcache_queue_main[i].pqueue);
        if(((curtime - lasttime) > kaliveinterval) && 0 == cursize)
        {
            //switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
            //    "mod_as_dbcache_queue_send_keep_alive, main_queue[%d]\n", i);    
            mod_as_dbcache_queue_send_keep_alive(&(mod_as_dbcache_queue_main[i]));
        }
    }   
    
    task->runtime = switch_epoch_time_now(NULL) + kaliveinterval;
}

static void mod_as_dbcache_queue_start_task()
{
    
    char *ptmp = NULL;    

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG,
            "mod_as_dbcache_queue_start_task \n");

    ptmp = switch_core_get_variable("rediskeepaliveinterval");
    kaliveinterval = atoi(switch_str_nil(ptmp));
    kaliveinterval = kaliveinterval>0? kaliveinterval: REDIS_KEEP_ALIVE_INTERVAL;

    if(1 == g_as_use_redis)
    {
        taskid = switch_scheduler_add_task(switch_epoch_time_now(NULL) + kaliveinterval, 
            task_keep_redis_alive, "task_keep_redis_alive", "task_keep_redis_alive", 
            0, NULL, SSHF_NONE | SSHF_OWN_THREAD | SSHF_FREE_ARG | SSHF_NO_DEL);
    }
}

switch_status_t mod_as_dbcache_queue_init()
{
    const char *pVariable = NULL;
    
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_queue_init\n");

    if(isinit)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "isinit is true\n");
        return SWITCH_STATUS_FALSE;
    }
    isinit = SWITCH_TRUE;

/*
    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_mysql_init())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "mod_as_dbcache_mysql_init failed\n");
        goto goto_init_failed;
    }
*/

    pVariable = switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_REDIS_USE_FLAG);
    if(NULL == pVariable)
    {
        g_as_use_redis = 1;
    }
    else
    {
        g_as_use_redis = atoi(pVariable);
    }
   
    if(SWITCH_STATUS_SUCCESS != switch_core_new_memory_pool(&pmod_as_dbcache_queue_pool))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "pmod_as_dbcache_queue_pool init failed\n");
        goto goto_init_failed;
    }
    
    if(SWITCH_STATUS_SUCCESS != switch_mutex_init(&pmod_as_dbcache_queue_mutex_main, 
                                    SWITCH_MUTEX_NESTED, pmod_as_dbcache_queue_pool))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "pmod_as_dbcache_queue_mutex_main init failed\n");
        goto goto_init_failed;
    }

    if(SWITCH_STATUS_SUCCESS != switch_mutex_init(&pmod_as_dbcache_queue_mutex_resp, 
                                    SWITCH_MUTEX_NESTED, pmod_as_dbcache_queue_pool))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "pmod_as_dbcache_queue_mutex_resp init failed\n");
        goto goto_init_failed;
    }

    //resp queue
    for(int i = 0; i < MOD_AS_DBCACHE_RESP_QUEUE_NUMBER; i++)
    {
        mod_as_dbcache_queue_resp[i].flag = EN_FLAG_IDLE;
        if(SWITCH_STATUS_SUCCESS != switch_queue_create(&(mod_as_dbcache_queue_resp[i].pqueue), 
                                        MOD_AS_DBCACHE_RESP_QUEUE_MAX_SIZE + MOD_AS_DBCACHE_RESP_QUEUE_OVER_FLOW, 
                                        pmod_as_dbcache_queue_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_resp init failed\n");
            goto goto_init_failed;
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_queue_init resp queue=%p\n", 
            (void*)mod_as_dbcache_queue_resp[i].pqueue);
    }

    //main queue
    for(int i = 0; i < MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER; i++)
    {
        if(SWITCH_STATUS_SUCCESS != switch_queue_create(&(mod_as_dbcache_queue_main[i].pqueue), 
                                        MOD_AS_DBCACHE_MAIN_QUEUE_MAX_SIZE + MOD_AS_DBCACHE_MAIN_QUEUE_OVER_FLOW, 
                                        pmod_as_dbcache_queue_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_main init failed\n");
            goto goto_init_failed;
        }

        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_queue_init main queue=%p\n", 
            (void*)mod_as_dbcache_queue_resp[i].pqueue);

        if(1 == g_as_use_redis)
        {
            //get redis context
            if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_redis_cluster_connect2(&(mod_as_dbcache_queue_main[i].pcontext)))
            {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                    "mod_as_dbcache_redis_cluster_connect2 failed\n");
                goto goto_init_failed;
            }
        }

        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_mysql_connect2(&(mod_as_dbcache_queue_main[i].pmysql)))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_mysql_connect2 failed\n");
            goto goto_init_failed;
        }

        //start thread
        if(SWITCH_STATUS_SUCCESS != switch_thread_create(&(pmod_as_dbcache_queue_threads[i]), 
                                        NULL, 
                                        mod_as_dbcache_queue_listen_thread, 
                                        &(mod_as_dbcache_queue_main[i]), 
                                        pmod_as_dbcache_queue_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                    "pmod_as_dbcache_queue_threads init failed\n");
            goto goto_init_failed;
        }
    }

    mod_as_dbcache_queue_start_task();

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_queue_init success\n");
    
    return SWITCH_STATUS_SUCCESS;

goto_init_failed:
    if(NULL != pmod_as_dbcache_queue_pool)
    {
        switch_core_destroy_memory_pool(&pmod_as_dbcache_queue_pool);
        pmod_as_dbcache_queue_pool = NULL;
    }

    isinit = SWITCH_FALSE;
    
    return SWITCH_STATUS_FALSE;
}

switch_status_t mod_as_dbcache_queue_uninit()
{  
    switch_status_t st;

    if(!isinit)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "isinit is false\n");
        return SWITCH_STATUS_FALSE;
    }
    isinit = SWITCH_FALSE;

    //stop task keepalive
    switch_scheduler_del_task_id(taskid);

    //stop threads
    for(int i = 0; i < MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER; i++)
    {
        switch_queue_push(mod_as_dbcache_queue_main[i].pqueue, NULL);
    }

    for(int i = 0; i < MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER; i++)
    {
        switch_thread_join(&st, pmod_as_dbcache_queue_threads[i]);
    }

    //main queue    
    //redis context 
    
    for(int i = 0; i < MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER; i++)
    {
        if(1 == g_as_use_redis)
        {
            mod_as_dbcache_redis_cluster_disconnect2(&(mod_as_dbcache_queue_main[i].pcontext));
        }

        mod_as_dbcache_mysql_disconnect2(&(mod_as_dbcache_queue_main[i].pmysql));
        
        mod_as_dbcache_queue_main[i].pqueue = NULL;        
    }

    //resp queue
    for(int i = 0; i < MOD_AS_DBCACHE_RESP_QUEUE_NUMBER; i++)
    {
        mod_as_dbcache_queue_resp[i].flag = EN_FLAG_BUSY;
        mod_as_dbcache_queue_resp[i].pqueue = NULL;
    }

    //mutex
    switch_mutex_destroy(pmod_as_dbcache_queue_mutex_resp);
    pmod_as_dbcache_queue_mutex_resp = NULL;
    
    switch_mutex_destroy(pmod_as_dbcache_queue_mutex_main);
    pmod_as_dbcache_queue_mutex_main = NULL;

    //pool
    switch_core_destroy_memory_pool(&pmod_as_dbcache_queue_pool);
    pmod_as_dbcache_queue_pool = NULL;
    
    //mysql connect
    //mod_as_dbcache_mysql_uninit();

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_queue_query(ST_DB_CACHE_EVENT *event, void *result)
{
    ST_DB_CACHE_HANDLER *handler = NULL;
    switch_status_t res = SWITCH_STATUS_SUCCESS;

    if(!isinit)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache is not inited\n");
        return SWITCH_STATUS_FALSE;
    }

    if(NULL == event)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_query, event=NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    if(0 == g_as_use_redis && (EN_TYPE_REDIS == event->type || EN_TYPE_REDIS_KEEP_ALIVE == event->type || EN_TYPE_REDIS_MYSQL == event->type))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_DEBUG, 
                "mod_as_dbcache_queue_query, redis is not used\n");
        return SWITCH_STATUS_FALSE;
    }
    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_DEBUG, 
                "mod_as_dbcache_queue_query, type=%d, redis_command=%s, mysql_command=%s, iscache=%d\n",
                event->type, event->redis_command, event->mysql_command, event->iscache);

    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_queue_send(&handler, event, result))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_send failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != (res = mod_as_dbcache_queue_recv_wait(&handler, event)))
    {
        //switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
        //        "mod_as_dbcache_queue_recv_wait failed\n");
        return res;
    }

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_queue_send(ST_DB_CACHE_HANDLER **handler, ST_DB_CACHE_EVENT *event, void *result)
{
    ST_DB_CACHE_QUEUE_RESP_DATA *data = NULL;
    
    if(NULL == event)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "event is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

//    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_DEBUG, 
//                "mod_as_dbcache_queue_send\n");

    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_queue_get_handler(handler))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_get_handler failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_queue_get_resp_data(&data, (*handler), event, result))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_get_resp_data failed\n");

        mod_as_dbcache_queue_handler_free(handler);
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != switch_queue_push(data->mainqueue->pqueue, data))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
                "switch_queue_push failed\n");

        mod_as_dbcache_queue_handler_free(handler);
        mod_as_dbcache_queue_resp_data_free(&data);
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_queue_send_keep_alive(ST_DB_CACHE_MAIN_QUEUE *mainqueue)
{
    ST_DB_CACHE_QUEUE_RESP_DATA *data = NULL;

    if(NULL == mainqueue)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_send_keep_alive failed: NULL == mainqueue\n");
        return SWITCH_STATUS_FALSE;
    }

    data = mod_as_dbcache_queue_resp_data_alloc();
    if(NULL == data)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_resp_data_alloc failed: NULL == data\n");
        return SWITCH_STATUS_FALSE;
    }

    data->mainqueue = mainqueue;   
    data->respqueue = NULL;
    data->sequence = 0;
    data->event->type = EN_TYPE_REDIS_KEEP_ALIVE;
    switch_snprintf(data->event->redis_command, DB_QUERY_COMMAND_MAX_LEN, 
                                    "GET MOD_AS_DBCACHE_QUEUE_KEEP_ALIVE");
    data->event->pfunredis = NULL;
    data->event->pfunmysql = NULL;
    data->event->iscache = SWITCH_FALSE;
    switch_snprintf(data->event->uuid_str, SWITCH_UUID_FORMATTED_LENGTH, "MOD_AS_DBCACHE_QUEUE_KEEP_ALIVE");
    data->event->timeout = 0;
    data->resptype = EN_TYPE_ERROR;
    data->resplen = 0;
    data->respdata = NULL;
    data->res = SWITCH_STATUS_FALSE;
    data->result = NULL;

    if(SWITCH_STATUS_SUCCESS != switch_queue_push(data->mainqueue->pqueue, data))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "switch_queue_push failed\n");

        mod_as_dbcache_queue_resp_data_free(&data);
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_queue_recv_wait(ST_DB_CACHE_HANDLER **handler, ST_DB_CACHE_EVENT *event)
{
    void *popdata = NULL;
    switch_interval_time_t timeout = (event->timeout > 0)? ((event->timeout) * 1000000): 5000000;
    switch_queue_t *respqueue = (*handler)->respqueue->pqueue;
    int sequence = (*handler)->sequence;
    switch_status_t res = SWITCH_STATUS_FALSE;

    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_DEBUG, 
                "mod_as_dbcache_queue_recv_wait start\n");

    while(1)
    {
        res = switch_queue_pop_timeout(respqueue, &popdata, timeout);
        if(APR_TIMEUP == res)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_WARNING, 
                "mod_as_dbcache_queue_recv_wait timeout, res = %d\n", res);
            break;
        }
        else if(APR_EINTR == res)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_WARNING, 
                "mod_as_dbcache_queue_recv_wait eintr, res = %d\n", res);
            continue;
        }
        else if(APR_SUCCESS == res)
        {
            ST_DB_CACHE_QUEUE_RESP_DATA *prespdata = (ST_DB_CACHE_QUEUE_RESP_DATA*)popdata;
            if(NULL == prespdata)
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_WARNING, 
                    "mod_as_dbcache_queue_recv_wait, NULL == prespdata\n");
                break;
            }

//            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_DEBUG, 
//                "sequence=%d, prespdata->sequence=%d\n", sequence, prespdata->sequence);
            
            if(sequence == prespdata->sequence)
            {
                //TODO: result process     
                res = mod_as_dbcache_queue_resp_process(prespdata);
                mod_as_dbcache_queue_resp_data_free(&prespdata);
                break;
            }
            else
            {
                mod_as_dbcache_queue_resp_data_free(&prespdata);
                continue;
            }
        }
        else
        {
            break;
        }
    }

    //recover respqueue
    mod_as_dbcache_queue_handler_free(handler);

    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_DEBUG, 
                "mod_as_dbcache_queue_recv_wait stop res = %d\n", res);
    
    return res;
}

ST_DB_CACHE_RESP_QUEUE * mod_as_dbcache_queue_get_resp_queue()
{
    static int nLastIndex = 0;
    int nStartPos = 0;
    ST_DB_CACHE_RESP_QUEUE *respqueue = NULL;
    
    if(pmod_as_dbcache_queue_mutex_resp)
    {
        switch_mutex_lock(pmod_as_dbcache_queue_mutex_resp);
        
        nStartPos = nLastIndex;
        respqueue = mod_as_dbcache_queue_resp + nStartPos;

        do
        {
            ++nStartPos;
            if(nStartPos < MOD_AS_DBCACHE_RESP_QUEUE_NUMBER)
            {
                ++respqueue;
            }
            else
            {
                nStartPos = 0;
                respqueue = mod_as_dbcache_queue_resp;
            }            
        }while(EN_FLAG_BUSY == respqueue->flag && nStartPos != nLastIndex);

        if(EN_FLAG_IDLE == respqueue->flag)
        {
            respqueue->flag = EN_FLAG_BUSY;
            nLastIndex = nStartPos;
        }
        else
        {
            respqueue = NULL;
        }
        
        switch_mutex_unlock(pmod_as_dbcache_queue_mutex_resp);
    }
    else
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "NULL == pmod_as_dbcache_queue_mutex_resp\n");
    }

    return respqueue;
}

ST_DB_CACHE_MAIN_QUEUE *mod_as_dbcache_queue_get_main_queue()
{
    ST_DB_CACHE_MAIN_QUEUE *mainqueue = NULL;

    //get main queue size current
    int minindex = 0;
    int minsize = switch_queue_size(mod_as_dbcache_queue_main[0].pqueue);
    int cursize = 0;
    for(int i = 0; i < MOD_AS_DBCACHE_MAIN_QUEUE_NUMBER; i++)
    {
        cursize = switch_queue_size(mod_as_dbcache_queue_main[i].pqueue);
        if(0 == cursize)
        {
            minindex = i;
            minsize = cursize;
            break;
        }
        
        if(cursize < minsize)
        {
            minindex = i;
            minsize = cursize;
        }
    }   

    if(minsize < MOD_AS_DBCACHE_MAIN_QUEUE_MAX_SIZE)
    {
        mainqueue = &(mod_as_dbcache_queue_main[minindex]);
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
                "mod_as_dbcache_queue_get_main_queue, mainqueue=%p, minindex=%d, minsize=%d\n", 
                (void*)mainqueue, minindex, minsize);

    return mainqueue;
}

switch_status_t mod_as_dbcache_queue_get_resp_data(ST_DB_CACHE_QUEUE_RESP_DATA **data, ST_DB_CACHE_HANDLER *handler, ST_DB_CACHE_EVENT *event, void *result)
{
    time_t curtime = 0;
    *data = mod_as_dbcache_queue_resp_data_alloc();
    if(NULL == *data)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_get_resp_data failed: NULL == *data\n");
        return SWITCH_STATUS_FALSE;
    }

    (*data)->mainqueue = mod_as_dbcache_queue_get_main_queue();
    if(NULL == (*data)->mainqueue)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(event->uuid_str), SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_get_resp_data failed: NULL == (*data)->mainqueue\n");
        mod_as_dbcache_queue_resp_data_free(data);
        return SWITCH_STATUS_FALSE;
    }
    (*data)->respqueue = handler->respqueue;
    (*data)->sequence = handler->sequence;
    memcpy((*data)->event, event, sizeof(ST_DB_CACHE_EVENT));
    (*data)->resptype = EN_TYPE_ERROR;
    (*data)->resplen = 0;
    (*data)->respdata = NULL;
    (*data)->res = SWITCH_STATUS_FALSE;
    (*data)->result = result;

    //add by zr 20190923, for fs coredump with mysql timeout
    if(EN_TYPE_MYSQL == (*data)->event->type || EN_TYPE_MYSQL_INSERT_ID == (*data)->event->type)
    {
        curtime = time(NULL);
        (*data)->event->timeout += curtime;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_queue_get_handler(ST_DB_CACHE_HANDLER **handler)
{    
#define COUNT_GET_RESP_QUEUE            (4)

    int nCount = 0;
    *handler = mod_as_dbcache_queue_handler_alloc();
    if(NULL == *handler)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "*handler is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    while(NULL == (*handler)->respqueue && nCount < COUNT_GET_RESP_QUEUE)
    {
        ++nCount;
        (*handler)->respqueue = mod_as_dbcache_queue_get_resp_queue();
        if(NULL == (*handler)->respqueue && nCount < COUNT_GET_RESP_QUEUE)
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, "Need to get a response queue again, count[%d]\n", nCount);
            usleep(200000);
        }
    }

    if(NULL == (*handler)->respqueue)
    {
        mod_as_dbcache_queue_handler_free(handler);
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "respqueue is NULL\n");
        return SWITCH_STATUS_FALSE;
    }

    (*handler)->sequence = mod_as_dbcache_queue_sequence++;
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_queue_db_query_process(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_FALSE;

    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_db_query_process, prespdata is NULL\n");
        return SWITCH_STATUS_FALSE;
    }
    
    switch(prespdata->event->type)
    {
        case EN_TYPE_REDIS:
        case EN_TYPE_REDIS_KEEP_ALIVE:
            res = mod_as_dbcache_redis_cluster_command2(prespdata);
            prespdata->res = res;
            
            break;
        case EN_TYPE_MYSQL:
		case EN_TYPE_MYSQL_INSERT_ID:
            res = mod_as_dbcache_mysql_command2(prespdata);
            prespdata->res = res;
            
            break;
        case EN_TYPE_REDIS_MYSQL:
            res = mod_as_dbcache_redis_cluster_command2(prespdata);
            if(SWITCH_STATUS_SUCCESS == res)
            {
                prespdata->res = res;
            }
            else
            {
                //get value from mysql
                res = mod_as_dbcache_mysql_command2(prespdata);
                prespdata->res = res;
            }
            
            break;
        default:
            break;
    }

    return res;
}

switch_status_t mod_as_dbcache_queue_resp_process(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_FALSE;
    ST_DB_CACHE_EVENT *pEvent = NULL;

    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "mod_as_dbcache_queue_resp_process, prespdata is NULL\n");
        return SWITCH_STATUS_FALSE;
    }
    
    switch(prespdata->resptype)
    {
        case EN_TYPE_REDIS:
            pEvent = prespdata->event;
            if(pEvent && pEvent->pfunredis)
            {
                res = pEvent->pfunredis(prespdata, prespdata->result);
//                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(pEvent->uuid_str), SWITCH_LOG_DEBUG, 
//                    "mod_as_dbcache_queue_resp_process, prespdata->event->pfunredis return:%d\n", res);
                prespdata->res = res;
            }
            else
            {
                res = prespdata->res;
            }
            
            break;
        case EN_TYPE_MYSQL:
		case EN_TYPE_MYSQL_INSERT_ID:
            res = prespdata->res;
            
            break;
        default:
            break;
    }

    return res;
}



