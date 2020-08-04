  
#ifndef __MOD_AS_DBCACHE_H__
#define __MOD_AS_DBCACHE_H__

#include <switch.h>

#include "mod_as_dbcache_define.h"
#include "mod_as_dbcache_mysql.h"
#include "mod_as_dbcache_queue.h"

//db query interface, with timeout

SWITCH_DECLARE(switch_status_t) mod_as_dbcache_query(ST_DB_CACHE_EVENT *event, void *result);


#endif

