/* 
 * 
 */

#include "mod_as_dbcache.h"
#include "mod_as_dbcache_api.h"

int g_as_use_redis = 1;

SWITCH_MODULE_LOAD_FUNCTION(libmod_as_dbcache_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(libmod_as_dbcache_shutdown);

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
SWITCH_MODULE_DEFINITION(libmod_as_dbcache, libmod_as_dbcache_load, libmod_as_dbcache_shutdown, NULL);

SWITCH_STANDARD_API(as_dbcache_api_function)
{
    return as_dbcache_api(cmd, session, stream);
}


/*****************************************************************************
 函 数 名  : libmod_as_dbcache_load
 功能描述  : 模块加载
             宏定义原型查看switch_types.h 2428行
 输入参数  : pool: 内存池
 输出参数  : module_interface: 模块指针
 返 回 值  : 成功返回SWITCH_STATUS_SUCCESS 失败返回错误码
*****************************************************************************/
SWITCH_MODULE_LOAD_FUNCTION(libmod_as_dbcache_load)
{
    switch_api_interface_t* api_interface = NULL;
    /* connect my internal structure to the blank pointer passed to me */
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);
    
    /******************* 注册API *******************/
    SWITCH_ADD_API(api_interface, 
        "as_dbcache", 
        "as_dbcache api", 
        as_dbcache_api_function, 
        "<cmd> <args>");
    /* 注册终端命令自动补全 */
    api_set_complete();
    
    return mod_as_dbcache_queue_init();
}

/*****************************************************************************
 函 数 名  : libmod_as_dbcache_shutdown
 功能描述  : 模块退出
             宏定义原型查看switch_types.h 2430行
 输入参数  : pool: 内存池
 输出参数  : module_interface: 模块指针
 返 回 值  : 成功返回SWITCH_STATUS_SUCCESS 失败返回错误码
*****************************************************************************/
SWITCH_MODULE_SHUTDOWN_FUNCTION(libmod_as_dbcache_shutdown)
{
    //uninit        
	return mod_as_dbcache_queue_uninit();
}


SWITCH_DECLARE(switch_status_t) mod_as_dbcache_query(ST_DB_CACHE_EVENT *event, void *result)
{
    return mod_as_dbcache_queue_query(event, result);
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
