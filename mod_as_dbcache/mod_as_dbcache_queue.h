
#ifndef __MOD_AS_DBCACHE_QUEUE_H__
#define __MOD_AS_DBCACHE_QUEUE_H__

#include <switch.h>

#include "mod_as_dbcache_define.h"

//queue init
switch_status_t mod_as_dbcache_queue_init();

//uninit
switch_status_t mod_as_dbcache_queue_uninit();

//query interface
switch_status_t mod_as_dbcache_queue_query(ST_DB_CACHE_EVENT *event, void *result);

//send event to main queue
switch_status_t mod_as_dbcache_queue_send(ST_DB_CACHE_HANDLER **handler, ST_DB_CACHE_EVENT *event, void *result);
switch_status_t mod_as_dbcache_queue_send_keep_alive(ST_DB_CACHE_MAIN_QUEUE *mainqueue);

//wait for result, with timeout
switch_status_t mod_as_dbcache_queue_recv_wait(ST_DB_CACHE_HANDLER **handler, ST_DB_CACHE_EVENT *event);

ST_DB_CACHE_RESP_QUEUE *mod_as_dbcache_queue_get_resp_queue();

ST_DB_CACHE_MAIN_QUEUE *mod_as_dbcache_queue_get_main_queue();

switch_status_t mod_as_dbcache_queue_get_resp_data(ST_DB_CACHE_QUEUE_RESP_DATA **data, ST_DB_CACHE_HANDLER *handler, ST_DB_CACHE_EVENT *event, void *result);

switch_status_t mod_as_dbcache_queue_get_handler(ST_DB_CACHE_HANDLER **handler);

switch_status_t mod_as_dbcache_queue_db_query_process(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);

switch_status_t mod_as_dbcache_queue_resp_process(ST_DB_CACHE_QUEUE_RESP_DATA *prespdata);



#endif


