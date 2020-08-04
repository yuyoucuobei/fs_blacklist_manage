


#pragma once

#include "mod_blacklist_manage_define.h"

void api_set_complete();

switch_status_t bl_user_api(_In_opt_z_ const char *cmd, _In_opt_ switch_core_session_t *session, _In_ switch_stream_handle_t *stream);


switch_status_t user_create(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_status(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_balance(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_credit(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_fee(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_num_add(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_num_del(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_getinfo(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_checkinfo(char **argv, int argc, switch_stream_handle_t *stream);
switch_status_t user_reload(char **argv, int argc, switch_stream_handle_t *stream);






