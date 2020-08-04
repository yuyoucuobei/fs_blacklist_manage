
#ifndef __MOD_AS_DBCACHE_REDIS_H__
#define __MOD_AS_DBCACHE_REDIS_H__

#include <switch.h>

#include "mod_as_dbcache_define.h"

#define MOD_AS_DBCACHE_CONFIG_REDIS_CLUSTER_ADDRS       "RedisClusterAddrs"   //redis cluster addrs
#define MOD_AS_DBCACHE_CONFIG_REDIS_USE_FLAG            "redis_use"
#define REDIS_KEY_TIMEOUT	(7200)

//init the global param and connect
switch_status_t mod_as_dbcache_redis_init();

//uninit
switch_status_t mod_as_dbcache_redis_uninit();

//connect redis cluster 
switch_status_t mod_as_dbcache_redis_cluster_connect();

switch_status_t mod_as_dbcache_redis_cluster_connect2(redisClusterContext **rccontext);

//disconnect redis cluster
switch_status_t mod_as_dbcache_redis_cluster_disconnect();

switch_status_t mod_as_dbcache_redis_cluster_disconnect2(redisClusterContext **rccontext);

//redis cluster command 
switch_status_t mod_as_dbcache_redis_cluster_command(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);

switch_status_t mod_as_dbcache_redis_cluster_cache_command(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);

switch_status_t mod_as_dbcache_redis_cluster_command2(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);

switch_status_t mod_as_dbcache_redis_cluster_cache_command2(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);


#endif


