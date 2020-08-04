/* 
 * 
 */

#include "mod_as_dbcache.h"
#include "mod_as_dbcache_api.h"

int g_as_use_redis = 1;

SWITCH_MODULE_LOAD_FUNCTION(libmod_as_dbcache_load);
SWITCH_MODULE_SHUTDOWN_FUNCTION(libmod_as_dbcache_shutdown);

/*****************************************************************************
 �� �� ��  : SWITCH_MODULE_DEFINITION
 ��������  : ģ�麯������
             �궨��ԭ�Ͳ鿴switch_types.h 2467��
 �������  : name: ģ����
             load: ע��load����
             shutdown: ע��shutdown����
             runtime: ע��runtime����
 �������  : ��
 �� �� ֵ  : ��
*****************************************************************************/
SWITCH_MODULE_DEFINITION(libmod_as_dbcache, libmod_as_dbcache_load, libmod_as_dbcache_shutdown, NULL);

SWITCH_STANDARD_API(as_dbcache_api_function)
{
    return as_dbcache_api(cmd, session, stream);
}


/*****************************************************************************
 �� �� ��  : libmod_as_dbcache_load
 ��������  : ģ�����
             �궨��ԭ�Ͳ鿴switch_types.h 2428��
 �������  : pool: �ڴ��
 �������  : module_interface: ģ��ָ��
 �� �� ֵ  : �ɹ�����SWITCH_STATUS_SUCCESS ʧ�ܷ��ش�����
*****************************************************************************/
SWITCH_MODULE_LOAD_FUNCTION(libmod_as_dbcache_load)
{
    switch_api_interface_t* api_interface = NULL;
    /* connect my internal structure to the blank pointer passed to me */
    *module_interface = switch_loadable_module_create_module_interface(pool, modname);
    
    /******************* ע��API *******************/
    SWITCH_ADD_API(api_interface, 
        "as_dbcache", 
        "as_dbcache api", 
        as_dbcache_api_function, 
        "<cmd> <args>");
    /* ע���ն������Զ���ȫ */
    api_set_complete();
    
    return mod_as_dbcache_queue_init();
}

/*****************************************************************************
 �� �� ��  : libmod_as_dbcache_shutdown
 ��������  : ģ���˳�
             �궨��ԭ�Ͳ鿴switch_types.h 2430��
 �������  : pool: �ڴ��
 �������  : module_interface: ģ��ָ��
 �� �� ֵ  : �ɹ�����SWITCH_STATUS_SUCCESS ʧ�ܷ��ش�����
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
