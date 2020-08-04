
#include "mod_blacklist_manage_api.h"
#include "mod_blacklist_manage_app.h"
#include "mod_blacklist_manage_data.h"
#include "mod_blacklist_manage_user.h"


#include <stdlib.h>

typedef switch_status_t (*api_command_t) (char **argv, int argc, switch_stream_handle_t *stream);

void api_set_complete()
{
    switch_console_set_complete("add bl_user create     ${username} ${fee}");
    switch_console_set_complete("add bl_user status     ${userid} ${status}");
    switch_console_set_complete("add bl_user balance    ${userid} ${money}");
    switch_console_set_complete("add bl_user credit     ${userid} ${money}");
    switch_console_set_complete("add bl_user fee        ${userid} ${fee}");
    switch_console_set_complete("add bl_user num_add    ${userid} ${num}");
    switch_console_set_complete("add bl_user num_del    ${userid} ${num}");
    switch_console_set_complete("add bl_user getinfo    ${userid}");
    switch_console_set_complete("add bl_user checkinfo  ${userid} ${day}");
    switch_console_set_complete("add bl_user reload");
}

switch_status_t bl_user_api(_In_opt_z_ const char *cmd, _In_opt_ switch_core_session_t *session, _In_ switch_stream_handle_t *stream)
{
    char *argv[8] = { 0 };
    int argc = 0;
    char *mycmd = NULL;
    api_command_t func = NULL;
    int lead = 0;
    static const char usage_string[] = "USAGE:\n"
        "--------------------------------------------------------------------------------\n"
        "bl_user create     ${username} ${fee}\n"
        "bl_user status     ${userid} ${status}\n"
        "bl_user balance    ${userid} ${money}\n"
        "bl_user credit     ${userid} ${money}\n"
        "bl_user fee        ${userid} ${fee}\n"
        "bl_user num_add    ${userid} ${num}\n"
        "bl_user num_del    ${userid} ${num}\n"
        "bl_user getinfo    ${userid}\n"
        "bl_user checkinfo  ${userid} ${day}\n"
        "bl_user reload\n"
        "--------------------------------------------------------------------------------\n";
    
    session = session;
    
    if (zstr(cmd)) 
    {
        stream->write_function(stream, "%s", usage_string);
        goto GOTO_as_api_function_done_flag;
    }

    if (!(mycmd = strdup(cmd))) 
    {
        goto GOTO_as_api_function_done_flag;
    }

    if (!(argc = switch_separate_string(mycmd, ' ', argv, switch_arraylen(argv))) || !argv[0]) 
    {
        stream->write_function(stream, "%s", usage_string);
        goto GOTO_as_api_function_done_flag;
    }
    
	if(0 == strcmp(argv[0], "create"))
    {
        func = user_create;        
    }
    else if(0 == strcmp(argv[0], "status"))
    {
        func = user_status;
    }
    else if(0 == strcmp(argv[0], "balance"))
    {
        func = user_balance;
    }
    else if(0 == strcmp(argv[0], "credit"))
    {
        func = user_credit;
    }
    else if(0 == strcmp(argv[0], "fee"))
    {
        func = user_fee;
    }
    else if(0 == strcmp(argv[0], "num_add"))
    {
        func = user_num_add;
    }
    else if(0 == strcmp(argv[0], "num_del"))
    {
        func = user_num_del;
    }
    else if(0 == strcmp(argv[0], "getinfo"))
    {
        func = user_getinfo;
    }
    else if(0 == strcmp(argv[0], "checkinfo"))
    {
        func = user_checkinfo;
    }
    else if(0 == strcmp(argv[0], "reload"))
    {
        func = user_reload;
    }
    else
    {}

    lead += 1;
    if (func) 
    {
        func(&argv[lead], argc - lead, stream);
    } 
    else 
    {
        stream->write_function(stream, "Unknown Command [%s]\n", argv[0]);
    }

GOTO_as_api_function_done_flag:
    switch_safe_free(mycmd);
	
    return SWITCH_STATUS_SUCCESS;
}


switch_status_t user_create(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    string username = argv[0];
    double fee = atof(argv[1]);
    if(username.empty() || fee <= 0.0)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_create(username, fee))
    {
        stream->write_function(stream, "bm_user_create failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_status(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_status(atoi(argv[0]), atoi(argv[1])))
    {
        stream->write_function(stream, "bm_user_status failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_balance(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_balance(atoi(argv[0]), atof(argv[1])))
    {
        stream->write_function(stream, "bm_user_balance failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_credit(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_credit(atoi(argv[0]), atof(argv[1])))
    {
        stream->write_function(stream, "bm_user_credit failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");   
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_fee(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_fee(atoi(argv[0]), atof(argv[1])))
    {
        stream->write_function(stream, "bm_user_fee failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");  
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_num_add(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_num_add(atoi(argv[0]), argv[1]))
    {
        stream->write_function(stream, "bm_user_num_add failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");  
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_num_del(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }

    if(SWITCH_STATUS_SUCCESS != bm_user_num_del(atoi(argv[0]), argv[1]))
    {
        stream->write_function(stream, "bm_user_num_del failed\n");
        return SWITCH_STATUS_SUCCESS;
    }

    stream->write_function(stream, "+OK\n");  
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_getinfo(char **argv, int argc, switch_stream_handle_t *stream)
{
    string respjson;
    if(1 == argc)
    {
        if(SWITCH_STATUS_SUCCESS != bm_user_getinfo(atoi(switch_str_nil(argv[0])), respjson))
        {
            stream->write_function(stream, "bm_user_getinfo failed\n");
        }
    }
    else
    {
        if(SWITCH_STATUS_SUCCESS != bm_user_getinfo_all(respjson))
        {
            stream->write_function(stream, "bm_user_getinfo_all failed\n");
        }
    }

    stream->write_function(stream, "userinfo: %s\n", respjson.c_str());  
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_checkinfo(char **argv, int argc, switch_stream_handle_t *stream)
{
    if(2 != argc)
    {
        stream->write_function(stream, "params invalid\n");
        return SWITCH_STATUS_SUCCESS;
    }
    
    string respjson;
    if(SWITCH_STATUS_SUCCESS != bm_user_checkinfo(atoi(argv[0]), argv[1], respjson))
    {
        stream->write_function(stream, "bm_user_checkinfo failed\n");
    }

    stream->write_function(stream, "checkinfo: %s\n", respjson.c_str());  
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t user_reload(char **argv, int argc, switch_stream_handle_t *stream)
{
    switch_status_t res = blacklist_manage_user_update();
    if(SWITCH_STATUS_IGNORE == res)
    {
        stream->write_function(stream, "update too frequent\n");
    }
    else if(SWITCH_STATUS_SUCCESS == res)
    {
        stream->write_function(stream, "update success\n");
    }
    else
    {
        stream->write_function(stream, "update failed\n");
    }

    return SWITCH_STATUS_SUCCESS;
}



