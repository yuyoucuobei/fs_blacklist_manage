
#ifndef __MOD_AS_DBCACHE_DEFINE_H__
#define __MOD_AS_DBCACHE_DEFINE_H__

#include <switch.h>
#include <hiredis-vip/hircluster.h>
#include <mysql/mysql.h>

#define DB_QUERY_COMMAND_MAX_LEN    (1024)
#define REDIS_KEEP_ALIVE_INTERVAL   (60)

//resp queue used flag enum
typedef enum EN_DB_CACHE_RESP_QUEUE_FLAG
{
    EN_FLAG_IDLE = 0,
    EN_FLAG_BUSY = 1
} EN_DB_CACHE_RESP_QUEUE_FLAG;

//event type enum
typedef enum EN_DB_CACHE_EVENT_TYPE
{
    EN_TYPE_ERROR = 0,
    EN_TYPE_REDIS = 1,   
    EN_TYPE_MYSQL = 2,
    EN_TYPE_REDIS_MYSQL = 3,
    EN_TYPE_REDIS_KEEP_ALIVE = 4,
    EN_TYPE_MYSQL_INSERT_ID = 5,
}EN_DB_CACHE_EVENT_TYPE;

//resp queue define
typedef struct ST_DB_CACHE_RESP_QUEUE
{
    switch_queue_t *pqueue;
    int flag;               //0-idle, 1-busy
} ST_DB_CACHE_RESP_QUEUE;

typedef struct ST_DB_CACHE_MAIN_QUEUE
{
    switch_queue_t *pqueue;
    redisClusterContext *pcontext;
    MYSQL *pmysql;
    int lasttime;
}ST_DB_CACHE_MAIN_QUEUE;

//handler define
typedef struct ST_DB_CACHE_HANDLER
{
    ST_DB_CACHE_RESP_QUEUE *respqueue;
    int sequence;
} ST_DB_CACHE_HANDLER;

typedef struct ST_DB_CACHE_QUEUE_RESP_DATA ST_DB_CACHE_QUEUE_RESP_DATA;

//callback ptr, for parse mysql resp data
typedef switch_status_t (*DB_QEURY_MYSQL_PARSE_FUN_PTR)(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata, void *result);
//callback ptr, for parse redis resp data
typedef switch_status_t (*DB_QEURY_REDIS_PARSE_FUN_PTR)(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata, void *result);

//db cache event define
typedef struct ST_DB_CACHE_EVENT
{    
    int  type;    //EN_DB_CACHE_EVENT_TYPE
    char redis_command[DB_QUERY_COMMAND_MAX_LEN];       // redis query command
    char mysql_command[DB_QUERY_COMMAND_MAX_LEN];       // mysql query command 
    DB_QEURY_REDIS_PARSE_FUN_PTR pfunredis;             //callback func to parse redis result
    DB_QEURY_MYSQL_PARSE_FUN_PTR pfunmysql;             //callback func to parse mysql result    
    switch_bool_t iscache;                              //is need cache res to redis
    char redis_cache_command[DB_QUERY_COMMAND_MAX_LEN]; //redis cache command
    char uuid_str[SWITCH_UUID_FORMATTED_LENGTH + 1];    //session uuid str, for log info
    int timeout;                                        //time out length, second
} ST_DB_CACHE_EVENT;

//queue data define, for resp queue
struct ST_DB_CACHE_QUEUE_RESP_DATA
{
    ST_DB_CACHE_MAIN_QUEUE *mainqueue;
    ST_DB_CACHE_RESP_QUEUE *respqueue;
    int sequence;
    ST_DB_CACHE_EVENT *event;
    int resptype;           //EN_DB_CACHE_EVENT_TYPE
    int resplen;
    void *respdata;     //type=redis: char *, type=mysql: MYSQL_RES *
    switch_status_t res;
    void *result;
};


#endif

