
#include "mod_blacklist_manage_deduction.h"
#include "mod_blacklist_manage_mysql.h"
#include "mod_blacklist_manage_data.h"
#include "apr_errno.h"

#define QUEUE_MAX_SIZE      (1024*1024)
#define QUEUE_MAX_NUMBER    (16)

static switch_memory_pool_t *precord_pool = NULL;
static WORK_QUEUE_INFO work_queue_info[QUEUE_MAX_NUMBER] = {NULL, NULL};
static switch_thread_t *work_threadid[QUEUE_MAX_NUMBER] = {NULL};

static void *SWITCH_THREAD_FUNC work_thread(switch_thread_t *t, void *data)
{    
    void *ppopdata = NULL;
    QUEUE_DATA_DEDUCTION *pqueuedata = NULL;
    int threadcontinue = 1;
    char cmd[256] = {0};
    int insertcount = 0;
    switch_interval_time_t timeout = 1000000;
    
    WORK_QUEUE_INFO *pqueueinfo = (WORK_QUEUE_INFO*)data;
    if(NULL == pqueueinfo)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "work_thread fail, NULL == pqueueinfo\n");
        return NULL;
    }

    if(NULL == pqueueinfo->pqueue || NULL == pqueueinfo->pmysql)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "work_thread fail, NULL == pqueueinfo->pqueue || NULL == pqueueinfo->pmysql\n");
        return NULL;
    }

    while (threadcontinue)
    {
        ppopdata = NULL;
        pqueuedata = NULL;

        switch_status_t res = switch_queue_pop_timeout(pqueueinfo->pqueue, &ppopdata, timeout);
        if (APR_TIMEUP == res) 
        {
            if(insertcount > 0)
            {
                if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pqueueinfo->pmysql, "COMMIT"))
                {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                        "bm_mysql_command_batch failed, cmd=COMMIT\n");
                }
                insertcount = 0;
            }
            continue;
        }
        else if(APR_EINTR == res)
        {
            continue;
        }
        else if(APR_EOF == res)
        {
            break;
        }
        //else APR_SUCCESS

        if (NULL == ppopdata) 
        {
            //TODO: COMMIT all cmd
            if(insertcount > 0)
            {
                if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pqueueinfo->pmysql, "COMMIT"))
                {
                    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                        "bm_mysql_command_batch failed, cmd=COMMIT\n");
                }
                insertcount = 0;
            }
            break;
        }

        pqueuedata = (QUEUE_DATA_DEDUCTION *)ppopdata;
        if(NULL == pqueuedata)
        {
            continue;
        }

        if(0 == insertcount)
        {
            if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pqueueinfo->pmysql, "START TRANSACTION"))
            {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                    "bm_mysql_command_batch failed, cmd=START TRANSACTION\n");
            }
        }

        if(0 == pqueuedata->feeflag)
        {
            snprintf(cmd, sizeof(cmd), 
                "UPDATE user_info SET balance=balance-fee WHERE id=%d",
                pqueuedata->userid);
        }
        else
        {
            snprintf(cmd, sizeof(cmd), 
                "UPDATE user_info SET credit=credit-fee WHERE id=%d",
                pqueuedata->userid);
        }

        if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pqueueinfo->pmysql, cmd))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "bm_mysql_command_batch failed, cmd=%s\n", cmd);
        }

        insertcount++;

        if(insertcount >= 10)
        {
            if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pqueueinfo->pmysql, "COMMIT"))
            {
                switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                    "bm_mysql_command_batch failed, cmd=COMMIT\n");
            }
            insertcount = 0;
        }

        if(NULL != pqueuedata)
        {
            delete pqueuedata;
            pqueuedata = NULL;
        }
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "work_thread stop: %p\n", (void *)pqueueinfo->pqueue);
    
    return NULL;
}

//init
switch_status_t blacklist_manage_deduction_init()
{
    //init pool
    if(SWITCH_STATUS_SUCCESS != switch_core_new_memory_pool(&precord_pool))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "precord_pool init failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    for(int i=0; i<QUEUE_MAX_NUMBER; i++)
    {
        if(SWITCH_STATUS_SUCCESS != switch_queue_create(&(work_queue_info[i].pqueue), 
                                                        QUEUE_MAX_SIZE, 
                                                        precord_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "switch_queue_create failed\n");
            goto goto_init_failed;
        }

        if(SWITCH_STATUS_SUCCESS != bm_mysql_connect(&(work_queue_info[i].pmysql)))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "bm_mysql_connect failed\n");
            goto goto_init_failed;
        }

        //start thread
        if(SWITCH_STATUS_SUCCESS != switch_thread_create(&(work_threadid[i]), 
                                        NULL, 
                                        work_thread, 
                                        &(work_queue_info[i]), 
                                        precord_pool))
        {
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                    "switch_thread_create failed\n");
            goto goto_init_failed;
        }
    }

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "blacklist_manage_deduction_init success\n");
    return SWITCH_STATUS_SUCCESS;

goto_init_failed:
    if(NULL != precord_pool)
    {
        switch_core_destroy_memory_pool(&precord_pool);
        precord_pool = NULL;
    }

    return SWITCH_STATUS_FALSE;
}

switch_status_t blacklist_manage_deduction_uninit()
{    
    switch_status_t ret;
    
    for(int i=0; i<QUEUE_MAX_NUMBER; i++)
    {
        switch_queue_push(work_queue_info[i].pqueue, NULL);
    }

    for(int i=0; i<QUEUE_MAX_NUMBER; i++)
    {
        switch_thread_join(&ret, work_threadid[i]);
    }
    
    for(int i=0; i<QUEUE_MAX_NUMBER; i++)
    {
        bm_mysql_disconnect(&(work_queue_info[i].pmysql));
        work_queue_info[i].pqueue = NULL;        
    }

    //pool
    switch_core_destroy_memory_pool(&precord_pool);
    precord_pool = NULL;
}


switch_status_t blacklist_manage_deduction_write(int userid, 
                                                   int feeflag )
{
    QUEUE_DATA_DEDUCTION *pdata = new QUEUE_DATA_DEDUCTION();
    if(NULL == pdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "QUEUE_DATA_DEDUCTION new failed\n");
        return SWITCH_STATUS_MEMERR;
    }
    pdata->userid = userid;
    pdata->feeflag = feeflag;

    static int queue_index = 0;
    queue_index += 1;
    if(queue_index >= QUEUE_MAX_NUMBER)
    {
        queue_index = 0;
    }

    if(NULL == work_queue_info[queue_index].pqueue
        || SWITCH_STATUS_SUCCESS != switch_queue_push(work_queue_info[queue_index].pqueue, pdata))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "switch_queue_push failed\n");
        return SWITCH_STATUS_FALSE;
    }
    int cursize = switch_queue_size(work_queue_info[queue_index].pqueue);
    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_NOTICE, 
        "################ deduction work_queue_info[%d].pqueue, size=%d\n", queue_index, cursize);
    
    return SWITCH_STATUS_SUCCESS;
}

