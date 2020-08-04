
#include <switch.h>

#include "mod_as_dbcache_redis.h"
#include "mod_as_dbcache_define.h"

#define REDIS_COMMAND_LEN  (256)

static redisClusterContext *pmod_as_dbcache_redis_cluster_context = NULL;

switch_status_t mod_as_dbcache_redis_init()
{
    return mod_as_dbcache_redis_cluster_connect();
}

switch_status_t mod_as_dbcache_redis_uninit()
{
    mod_as_dbcache_redis_cluster_disconnect();

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_redis_cluster_connect()
{
    char *paddrs = NULL;
    struct timeval timeout = {5, 0};	// 2second    
    switch_status_t res = SWITCH_STATUS_SUCCESS;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_connect\n");

    //check redis context is NULL first
	if(NULL != pmod_as_dbcache_redis_cluster_context)
	{
		//TODO: disconnect redis cluster
		redisClusterFree(pmod_as_dbcache_redis_cluster_context);
		pmod_as_dbcache_redis_cluster_context = NULL;
	}	

    // redis cluster connect with timeout 
    paddrs = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_REDIS_CLUSTER_ADDRS));
    if(NULL == paddrs)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "NULL == paddrs\n");
        return SWITCH_STATUS_FALSE;
    }
    
	pmod_as_dbcache_redis_cluster_context = redisClusterConnectWithTimeout(paddrs, timeout, HIRCLUSTER_FLAG_NULL);
	if(NULL == pmod_as_dbcache_redis_cluster_context || pmod_as_dbcache_redis_cluster_context->err)
	{
        //connect failed
		if(pmod_as_dbcache_redis_cluster_context)
		{
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "redis cluster connect failed: %s!\n", pmod_as_dbcache_redis_cluster_context->errstr);
			redisClusterFree(pmod_as_dbcache_redis_cluster_context);
			pmod_as_dbcache_redis_cluster_context = NULL;
		}
		else
		{
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "redis cluster connect failed: can't allocate redis contex!\n");
		}
		res = SWITCH_STATUS_FALSE;
	}    

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_connect: res=%d\n", res);
	
	return res;
}

switch_status_t mod_as_dbcache_redis_cluster_connect2(redisClusterContext **rccontext)
{
    char *paddrs = NULL;
    struct timeval timeout = {5, 0};	// 2second    
    switch_status_t res = SWITCH_STATUS_SUCCESS;
    redisClusterContext *pcontext = NULL;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_connect2(redisClusterContext **rccontext)\n");

    // redis cluster connect with timeout 
    paddrs = switch_str_nil(switch_core_get_variable(MOD_AS_DBCACHE_CONFIG_REDIS_CLUSTER_ADDRS));
    if(NULL == paddrs)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "NULL == paddrs\n");
        return SWITCH_STATUS_FALSE;
    }
    
	pcontext = redisClusterConnectWithTimeout(paddrs, timeout, HIRCLUSTER_FLAG_NULL);
	if(NULL == pcontext || pcontext->err)
	{
        //connect failed
		if(pcontext)
		{
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "redis cluster connect failed: %s!\n", pcontext->errstr);
			redisClusterFree(pcontext);
			pcontext = NULL;
		}
		else
		{
            switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
                "redis cluster connect failed: can't allocate redis contex!\n");
		}
		res = SWITCH_STATUS_FALSE;
	}    
    *rccontext = pcontext;

    switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_connect2: res=%d, pcontext=%p, *rccontext=%p\n", 
            res, (void*)pcontext, (void*)(*rccontext));
	
	return res;
}

switch_status_t mod_as_dbcache_redis_cluster_disconnect()
{
    if(NULL != pmod_as_dbcache_redis_cluster_context)
	{
		//TODO: disconnect redis cluster
		redisClusterFree(pmod_as_dbcache_redis_cluster_context);
		pmod_as_dbcache_redis_cluster_context = NULL;
	}	

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_redis_cluster_disconnect2(redisClusterContext **rccontext)
{
    redisClusterContext *pcontext = *rccontext;
    if(NULL != pcontext)
	{
		//TODO: disconnect redis cluster
		redisClusterFree(pcontext);
		pcontext = NULL;
	}	
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t mod_as_dbcache_redis_cluster_command(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_SUCCESS;
    redisReply *reply = NULL;
    char *command = NULL;
    void *result = NULL;

    if(NULL == prespdata )
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata\n");
        return SWITCH_STATUS_FALSE;
    }

    command = prespdata->event->redis_command;
    result = prespdata->result;
    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_command, command=%s\n", command);

    //check redis cluster is connected first
    if(NULL == pmod_as_dbcache_redis_cluster_context || pmod_as_dbcache_redis_cluster_context->err)
    {
        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_redis_cluster_connect())
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "mod_as_dbcache_redis_cluster_connect failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }

    //execute redis command     
    reply = (redisReply*)redisClusterCommand(pmod_as_dbcache_redis_cluster_context, command);
	if(NULL != reply)
	{
        //command execute result
        if(REDIS_REPLY_STRING == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_STRING == reply->type, reply->str=%s\n", reply->str);
            prespdata->resptype = EN_TYPE_REDIS;
            prespdata->respdata = reply->str;
            res = prespdata->event->pfunredis(prespdata, result); 
        }
        else if(REDIS_REPLY_ARRAY == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ARRAY == reply->type\n");
            res = SWITCH_STATUS_FALSE;
        }
        else if(REDIS_REPLY_INTEGER == reply->type)
        {
            char replydata[64] = {0};
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_INTEGER == reply->type, reply->integer=%lld\n", reply->integer);
            
            prespdata->resptype = EN_TYPE_REDIS;
            switch_snprintf(replydata, 64, "%d", reply->integer);
            prespdata->respdata = replydata;
            res = prespdata->event->pfunredis(prespdata, result); 
        }
        else if(REDIS_REPLY_NIL == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_NIL == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pmod_as_dbcache_redis_cluster_context->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        else if(REDIS_REPLY_STATUS == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_STATUS == reply->type, reply->str=%s\n", reply->str);
            res = SWITCH_STATUS_SUCCESS;
        }
        else if(REDIS_REPLY_ERROR == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ERROR == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pmod_as_dbcache_redis_cluster_context->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        else
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "OTHER == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pmod_as_dbcache_redis_cluster_context->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        
		freeReplyObject(reply);
        prespdata->respdata = NULL;
	}
    else
    {
        //command execute failed
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "redis command failed: %s !\n", command);
        res = SWITCH_STATUS_FALSE;
    }
    
    return res;
}

switch_status_t mod_as_dbcache_redis_cluster_cache_command(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_SUCCESS;
    redisReply *reply = NULL;
    char *command = NULL;

    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata\n");
        return SWITCH_STATUS_FALSE;
    }

    command = prespdata->event->redis_cache_command;
    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_command, command=%s\n", command);

    //check redis cluster is connected first
    if(NULL == pmod_as_dbcache_redis_cluster_context || pmod_as_dbcache_redis_cluster_context->err)
    {
        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_redis_cluster_connect())
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "mod_as_dbcache_redis_cluster_connect failed\n");
            return SWITCH_STATUS_FALSE;
        }
    }

    //execute redis command     
    reply = (redisReply*)redisClusterCommand(pmod_as_dbcache_redis_cluster_context, command);
	if(NULL != reply)
	{
        if(REDIS_REPLY_STRING == reply->type)
        {
            if(0 == strncmp("OK", reply->str, 2))
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                    "reply->str = %s\n", reply->str);
            }
            else
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "reply->str = %s\n", reply->str);
                res = SWITCH_STATUS_FALSE;
            } 
        }
        else if(REDIS_REPLY_ARRAY == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ARRAY == reply->type\n");
            res = SWITCH_STATUS_FALSE;
        }
        else if(REDIS_REPLY_INTEGER == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_INTEGER == reply->type, reply->integer=%lld\n", reply->integer);
        }
        else if(REDIS_REPLY_NIL == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_NIL == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pmod_as_dbcache_redis_cluster_context->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        else if(REDIS_REPLY_STATUS == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_STATUS == reply->type, reply->str=%s\n", reply->str);
            res = SWITCH_STATUS_SUCCESS;
        }
        else if(REDIS_REPLY_ERROR == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ERROR == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pmod_as_dbcache_redis_cluster_context->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        else
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "OTHER == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pmod_as_dbcache_redis_cluster_context->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        
		freeReplyObject(reply);
	}
    else
    {
        //command execute failed
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "redis command failed: %s !\n", command);
        res = SWITCH_STATUS_FALSE;
    }
    
    return res;
}


switch_status_t mod_as_dbcache_redis_cluster_command2(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_SUCCESS;
    redisReply *reply = NULL;
    char *command = NULL;
    //void *result = NULL;
    redisClusterContext *pcontext = NULL;
    int replylen = 16;
    unsigned int i = 0;

    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata\n");
        return SWITCH_STATUS_FALSE;
    }
    
    if(NULL == prespdata->event)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata->event\n");
        return SWITCH_STATUS_FALSE;
    }

    if(NULL == prespdata->mainqueue->pcontext)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata->mainqueue->pcontext\n");
        return SWITCH_STATUS_FALSE;
    }
    pcontext = prespdata->mainqueue->pcontext;

    command = prespdata->event->redis_command;
    //result = prespdata->result;
//    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
//            "mod_as_dbcache_redis_cluster_command, command=%s\n", command);

    //check redis cluster is connected first
    if(NULL == pcontext || pcontext->err)
    {
        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_redis_cluster_connect2(&pcontext))
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "mod_as_dbcache_redis_cluster_connect2 failed\n");
            return SWITCH_STATUS_FALSE;
        }
        prespdata->mainqueue->pcontext = pcontext;
    }

    //execute redis command     
    reply = (redisReply*)redisClusterCommand(pcontext, command);
	if(NULL != reply)
	{
        //command execute result
        if(REDIS_REPLY_STRING == reply->type)
        {
//            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
//                "REDIS_REPLY_STRING == reply->type, reply->str=%1.100s, reply->len=%d\n", reply->str, reply->len);
            prespdata->resptype = EN_TYPE_REDIS;
            prespdata->resplen = reply->len;
            prespdata->respdata = calloc(1, prespdata->resplen + 1);
            if(NULL == prespdata->respdata)
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                    "NULL == prespdata->respdata\n");
                res = SWITCH_STATUS_FALSE; 
            }
            else
            {
                switch_copy_string(prespdata->respdata, reply->str, prespdata->resplen+1);
                res = SWITCH_STATUS_SUCCESS;
            }            
        }
        else if(REDIS_REPLY_ARRAY == reply->type)
        {
            prespdata->resptype = EN_TYPE_REDIS;
            prespdata->resplen = reply->elements;
            prespdata->respdata = calloc(1, reply->elements * 64);
            if(NULL == prespdata->respdata)
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                    "NULL == prespdata->respdata\n");
                res = SWITCH_STATUS_FALSE; 
            }
            else
            {
                for (i = 0;i < reply->elements;++i)
                {
                    if (REDIS_REPLY_STRING == reply->element[i]->type)
                    {
                        switch_snprintf(((char*)prespdata->respdata) + strlen(prespdata->respdata), reply->elements * 64 - strlen(prespdata->respdata), "%s ", reply->element[i]->str);
                    }
                    else if (REDIS_REPLY_INTEGER == reply->element[i]->type)
                    {
                        switch_snprintf(((char*)prespdata->respdata) + strlen(prespdata->respdata), reply->elements * 64 - strlen(prespdata->respdata), "%d ", reply->element[i]->integer);
                    }
                }
                res = SWITCH_STATUS_SUCCESS;
            }
        }
        else if(REDIS_REPLY_INTEGER == reply->type)
        {
//            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
//                "REDIS_REPLY_INTEGER == reply->type, reply->integer=%lld\n", reply->integer);
            
            prespdata->resptype = EN_TYPE_REDIS;
            prespdata->resplen = replylen;
            prespdata->respdata = calloc(1, replylen);
            if(NULL == prespdata->respdata)
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                    "NULL == prespdata->respdata\n");
                res = SWITCH_STATUS_FALSE; 
            }
            else
            {
                switch_snprintf(prespdata->respdata, replylen, "%d", reply->integer);
                res = SWITCH_STATUS_SUCCESS;
            }                        
        }
        else if(REDIS_REPLY_NIL == reply->type)
        {
            //switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_NOTICE, 
            //    "REDIS_REPLY_NIL == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pcontext->errstr);
            prespdata->resptype = EN_TYPE_REDIS;
            prespdata->respdata = NULL;
            res = SWITCH_STATUS_SUCCESS;
        }
        else if(REDIS_REPLY_STATUS == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_STATUS == reply->type, reply->str=%s\n", reply->str);
            prespdata->resptype = EN_TYPE_REDIS;
            res = SWITCH_STATUS_SUCCESS;
        }
        else if(REDIS_REPLY_ERROR == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ERROR == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pcontext->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        else
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "OTHER == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pcontext->errstr);
            res = SWITCH_STATUS_FALSE;
        }

		freeReplyObject(reply);
	}
    else
    {
        //command execute failed
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "redis command failed: %s, err = %d, errstr = %s!\n", command, pcontext->err, pcontext->errstr);
        res = SWITCH_STATUS_FALSE;
    }
    
    return res;
}

switch_status_t mod_as_dbcache_redis_cluster_cache_command2(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata)
{
    switch_status_t res = SWITCH_STATUS_SUCCESS;
    redisReply *reply = NULL;
    char *command = NULL;
    redisClusterContext *pcontext = NULL;
    
    if(NULL == prespdata)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata\n");
        return SWITCH_STATUS_FALSE;
    }

    if(NULL == prespdata->mainqueue->pcontext)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                "NULL == prespdata->mainqueue->pcontext\n");
        return SWITCH_STATUS_FALSE;
    }
    pcontext = prespdata->mainqueue->pcontext;

    command = prespdata->event->redis_cache_command;
    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
            "mod_as_dbcache_redis_cluster_command, command=%s\n", command);

    //check redis cluster is connected first
    if(NULL == pcontext || pcontext->err)
    {
        if(SWITCH_STATUS_SUCCESS != mod_as_dbcache_redis_cluster_connect2(&pcontext))
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
            "mod_as_dbcache_redis_cluster_connect failed\n");
            return SWITCH_STATUS_FALSE;
        }
        prespdata->mainqueue->pcontext = pcontext;
    }

    //execute redis command     
    reply = (redisReply*)redisClusterCommand(pcontext, command);
	if(NULL != reply)
	{
        if(REDIS_REPLY_STRING == reply->type)
        {
            if(0 == strncmp("OK", reply->str, 2))
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                    "reply->str = %s\n", reply->str);
            }
            else
            {
                switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "reply->str = %s\n", reply->str);
                res = SWITCH_STATUS_FALSE;
            } 
        }
        else if(REDIS_REPLY_ARRAY == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ARRAY == reply->type\n");
            res = SWITCH_STATUS_FALSE;
        }
        else if(REDIS_REPLY_INTEGER == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_INTEGER == reply->type, reply->integer=%lld\n", reply->integer);
        }
        else if(REDIS_REPLY_NIL == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_NIL == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pcontext->errstr);
            res = SWITCH_STATUS_SUCCESS;
        }
        else if(REDIS_REPLY_STATUS == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_DEBUG, 
                "REDIS_REPLY_STATUS == reply->type, reply->str=%s\n", reply->str);
            res = SWITCH_STATUS_SUCCESS;
        }
        else if(REDIS_REPLY_ERROR == reply->type)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "REDIS_REPLY_ERROR == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pcontext->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        else
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "OTHER == reply->type, pmod_as_dbcache_redis_cluster_context->errstr=%s\n", pcontext->errstr);
            res = SWITCH_STATUS_FALSE;
        }
        	
		freeReplyObject(reply);
	}
    else
    {
        //command execute failed
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(prespdata->event->uuid_str), SWITCH_LOG_WARNING, 
                "redis command failed: %s !\n", command);
        res = SWITCH_STATUS_FALSE;
    }
    
    return res;
}


