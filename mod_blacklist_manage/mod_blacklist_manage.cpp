

#include "mod_blacklist_manage.h"
#include "mod_blacklist_manage_api.h"
#include "mod_blacklist_manage_app.h"
#include "mod_blacklist_manage_record.h"
#include "mod_blacklist_manage_deduction.h"
#include "mod_blacklist_manage_user.h"
#include "mod_blacklist_manage_mysql.h"


SWITCH_MODULE_LOAD_FUNCTION(mod_blacklist_manage_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_blacklist_manage_shutdown);

/*****************************************************************************
 函 数 名  : SWITCH_MODULE_DEFINITION
 功能描述  : 模块函数定义
             宏定义原型查看switch_types.h 2467行
 输入参数  : name: 模块名
             load: 注册load函数
             shutdown: 注册shutdown函数
             runtime: 注册runtime函数
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
SWITCH_MODULE_DEFINITION(mod_blacklist_manage, mod_blacklist_manage_load, mod_blacklist_manage_shutdown, NULL);

SWITCH_STANDARD_API(bl_user_api_function)
{
    return bl_user_api(cmd, session, stream);
}


SWITCH_STANDARD_APP(blacklist_manage_check_function)
{
    blacklist_manage_check(session, data);
}


/*****************************************************************************
 函 数 名  : mod_blacklist_manage_load
 功能描述  : 模块加载
             宏定义原型查看switch_types.h 2428行
 输入参数  : pool: 内存池
 输出参数  : module_interface: 模块指针
 返 回 值  : 成功返回SWITCH_STATUS_SUCCESS 失败返回错误码
*****************************************************************************/
SWITCH_MODULE_LOAD_FUNCTION(mod_blacklist_manage_load)
{
    switch_api_interface_t* api_interface = NULL;
    switch_application_interface_t* app_interface = NULL;
    /* connect my internal structure to the blank pointer passed to me */
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);

    /******************* 注册APP *******************/
    SWITCH_ADD_APP(app_interface, "blacklist_manage_check", "blacklist_manage_check", "NULL",
           blacklist_manage_check_function, "NULL", SAF_SUPPORT_NOMEDIA | SAF_ROUTING_EXEC);

    /******************* 注册API *******************/
    SWITCH_ADD_API(api_interface, 
        "bl_user", 
        "bl_user api", 
        bl_user_api_function, 
        "<cmd> <args>");
    
    /* 注册终端命令自动补全 */
    api_set_complete();

    if(SWITCH_STATUS_SUCCESS != bm_mysql_init())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_init failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    if(SWITCH_STATUS_SUCCESS != blacklist_manage_record_init())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "blacklist_manage_record_init failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_deduction_init())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "blacklist_manage_deduction_init failed\n");
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_init())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "blacklist_manage_user_init failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    return SWITCH_STATUS_SUCCESS;
}

/*****************************************************************************
 函 数 名  : mod_blacklist_manage_shutdown
 功能描述  : 模块退出
             宏定义原型查看switch_types.h 2430行
 输入参数  : pool: 内存池
 输出参数  : module_interface: 模块指针
 返 回 值  : 成功返回SWITCH_STATUS_SUCCESS 失败返回错误码
*****************************************************************************/
SWITCH_MODULE_SHUTDOWN_FUNCTION(mod_blacklist_manage_shutdown)
{
    //uninit        
    blacklist_manage_user_uninit();
    blacklist_manage_deduction_uninit();
	blacklist_manage_record_uninit();
    bm_mysql_uninit();
	return SWITCH_STATUS_SUCCESS;
}


/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4 noet:
 */
 
