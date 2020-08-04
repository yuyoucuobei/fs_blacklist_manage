#ifndef __MOD_AS_DBCACHE_API_H__
#define __MOD_AS_DBCACHE_API_H__

#include <switch.h>

#include "mod_as_common.h"
#include "mod_as_dbcache_define.h"
#include "mod_as_numchange_data_define.h"

void api_set_complete();

switch_status_t as_dbcache_api(_In_opt_z_ const char *cmd, _In_opt_ switch_core_session_t *session, _In_ switch_stream_handle_t *stream);

switch_status_t cmd_get_relayinfo_byout_multithread(char **argv, int argc, switch_stream_handle_t *stream); 

switch_status_t cmd_jsontest(char **argv, int argc, switch_stream_handle_t *stream); 

switch_status_t as_dbcache_get_info_by_out(_Out_ TRUNK_INFO *trunk_info,_In_ const char* uuid, switch_stream_handle_t *stream); 

switch_status_t cmd_redis_cmd(char **argv, int argc, switch_stream_handle_t *stream);

switch_status_t cmd_redis_cmd_multithread(char **argv, int argc, switch_stream_handle_t *stream);

switch_status_t redis_not_used(char **argv, int argc, switch_stream_handle_t *stream);

#endif

