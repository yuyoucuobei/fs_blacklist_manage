


#include "mod_blacklist_manage_mysql.h"
#include "mod_blacklist_manage_data.h"

switch_status_t bm_mysql_connect(MYSQL **pmysql)
{
    unsigned int mysqlport = 0;
    char value = 1;

    const char *pmysqladdr = switch_str_nil(switch_core_get_variable(BM_CONFIG_MYSQL_HOST));
    const char *pmysqlport = switch_str_nil(switch_core_get_variable(BM_CONFIG_MYSQL_PORT));
    const char *pmysqluser = switch_str_nil(switch_core_get_variable(BM_CONFIG_MYSQL_USER));
    const char *pmysqlpw = switch_str_nil(switch_core_get_variable(BM_CONFIG_MYSQL_PASSWD));
    const char *pmysqlname = switch_str_nil(switch_core_get_variable(BM_CONFIG_MYSQL_NAME));
    mysqlport = atoi(pmysqlport);

    //mysql connect 
    *pmysql = mysql_init(NULL); 
    if (!mysql_real_connect(*pmysql, pmysqladdr, pmysqluser, pmysqlpw, pmysqlname, mysqlport, NULL, 0))
    { 
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "mysql connect failed: %s!\n", mysql_error(*pmysql));
        return SWITCH_STATUS_FALSE;
    }

    //mysql set reconnect        
    mysql_options(*pmysql, MYSQL_OPT_RECONNECT, (char *)&value);

    return SWITCH_STATUS_SUCCESS;
}

//disconnect mysql
switch_status_t bm_mysql_disconnect(MYSQL **pmysql)
{
    if(NULL != *pmysql)
    {
        mysql_close(*pmysql);
        *pmysql = NULL;
    }

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_mysql_command_batch(MYSQL *pmysql, const string &cmd)
{    
    if(cmd.empty())
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
            "cmd.empty()\n");
        return SWITCH_STATUS_FALSE;
    }

    if(NULL == pmysql)
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
            "pmysql is NULL\n");
        return SWITCH_STATUS_FALSE;
    }
    
    //mysql_ping(pmysql);

    //mysql query
    if(0 != mysql_real_query(pmysql, cmd.c_str(), cmd.length()))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_WARNING, 
                        "mysql query failed: %s, command: %s!\n", 
                        mysql_error(pmysql), cmd.c_str());
        return SWITCH_STATUS_FALSE;
    }

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t create_table_user_info(MYSQL *pmysql)
{    
    string cmd = "CREATE TABLE IF NOT EXISTS `user_info` ( " \
      "`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT, " \
      "`user` VARCHAR(64) NOT NULL COMMENT '�û���', " \
      "`balance` DOUBLE NOT NULL DEFAULT '0' COMMENT '�˻����', " \
      "`credit` DOUBLE NOT NULL DEFAULT '0' COMMENT '����', " \
      "`status` INT(10) UNSIGNED NOT NULL DEFAULT '0' COMMENT '�˻�״̬��0-������1-���ᣬ2-���ڣ�3��Ƿ��', " \
      "`fee` DOUBLE NOT NULL DEFAULT '0' COMMENT '���ã�����������1��', " \
      "`createtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '����ʱ��', " \
      "UNIQUE KEY `id` (`id`) " \
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8 ";

    if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pmysql, cmd))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_command_batch failed, cmd:%s\n", cmd.c_str());
        return SWITCH_STATUS_FALSE;
    }

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t create_table_user_num(MYSQL *pmysql)
{    
    string cmd = "CREATE TABLE IF NOT EXISTS `user_num` ( " \
      "`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT, " \
      "`userid` INT(10) UNSIGNED NOT NULL COMMENT '�û�ID', " \
      "`number` VARCHAR(32) NOT NULL UNIQUE COMMENT '��������', " \
      "`createtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '����ʱ��', " \
      "UNIQUE KEY `id` (`id`) " \
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8 ";

    if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pmysql, cmd))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_command_batch failed, cmd:%s\n", cmd.c_str());
        return SWITCH_STATUS_FALSE;
    }

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t create_table_bl_check_today(MYSQL *pmysql)
{    
    char today[16];
    get_time_str(today, sizeof(today));
    string cmd = string("CREATE TABLE IF NOT EXISTS `bl_check_") + today + "` ( " \
      "`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT, " \
      "`userid` INT(10) UNSIGNED NOT NULL COMMENT '�û�ID', " \
      "`caller` VARCHAR(32) NOT NULL COMMENT '���к���', " \
      "`callee` VARCHAR(32) NOT NULL COMMENT '���к���', " \
      "`fee` DOUBLE NOT NULL DEFAULT '0' COMMENT '���η���', " \
      "`hit` INT(11) DEFAULT '0' COMMENT '���������У�0-δ���У�1-����', " \
      "`createtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '����ʱ��', " \
      "UNIQUE KEY `id` (`id`) " \
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8 ";

    if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pmysql, cmd))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_command_batch failed, cmd:%s\n", cmd.c_str());
        return SWITCH_STATUS_FALSE;
    }

    return SWITCH_STATUS_SUCCESS;
}

switch_status_t create_table_bl_check_tomorrow(MYSQL *pmysql)
{    
    char tomorrow[16];
    get_time_str_tomorrow(tomorrow, sizeof(tomorrow));
    string cmd = string("CREATE TABLE IF NOT EXISTS `bl_check_") + tomorrow + "` ( " \
      "`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT, " \
      "`userid` INT(10) UNSIGNED NOT NULL COMMENT '�û�ID', " \
      "`caller` VARCHAR(32) NOT NULL COMMENT '���к���', " \
      "`callee` VARCHAR(32) NOT NULL COMMENT '���к���', " \
      "`fee` DOUBLE NOT NULL DEFAULT '0' COMMENT '���η���', " \
      "`hit` INT(11) DEFAULT '0' COMMENT '���������У�0-δ���У�1-����', " \
      "`createtime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT '����ʱ��', " \
      "UNIQUE KEY `id` (`id`) " \
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8 ";

    if(SWITCH_STATUS_SUCCESS != bm_mysql_command_batch(pmysql, cmd))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_command_batch failed, cmd:%s\n", cmd.c_str());
        return SWITCH_STATUS_FALSE;
    }

    return SWITCH_STATUS_SUCCESS;
}

SWITCH_STANDARD_SCHED_FUNC(create_table_bl_check_tomorrow_task)
{
	MYSQL *pmysql = NULL;
    if(SWITCH_STATUS_SUCCESS != bm_mysql_connect(&pmysql))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_connect failed\n");
        task->runtime = switch_epoch_time_now(NULL) + (60);
        return;
    }

    if(SWITCH_STATUS_SUCCESS != create_table_bl_check_tomorrow(pmysql))
    {
        bm_mysql_disconnect(&pmysql);
        task->runtime = switch_epoch_time_now(NULL) + (60);
        return;
    }

    bm_mysql_disconnect(&pmysql);

	task->runtime = switch_epoch_time_now(NULL) + (86400);
}

switch_status_t bm_mysql_init()
{
    MYSQL *pmysql = NULL;
    if(SWITCH_STATUS_SUCCESS != bm_mysql_connect(&pmysql))
    {
        switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, 
            "bm_mysql_connect failed\n");
        return SWITCH_STATUS_FALSE;
    }
    
    //ģ��������ʱ���Զ�����4�ű�user_info, user_num, bl_check${today}, bl_check${tomorrow}
    if(SWITCH_STATUS_SUCCESS != create_table_user_info(pmysql))
    {
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != create_table_user_num(pmysql))
    {
        return SWITCH_STATUS_FALSE;
    }

    if(SWITCH_STATUS_SUCCESS != create_table_bl_check_today(pmysql))
    {
        return SWITCH_STATUS_FALSE;
    }

    bm_mysql_disconnect(&pmysql);

    //��������ÿ���Զ����� bl_check${tomorrow}
    switch_scheduler_add_task(switch_epoch_time_now(NULL), 
        create_table_bl_check_tomorrow_task, "create_table_bl_check_tomorrow_task", "create_table_bl_check_tomorrow_task", 
        0, NULL, SSHF_NONE | SSHF_OWN_THREAD | SSHF_FREE_ARG | SSHF_NO_DEL);
    
    return SWITCH_STATUS_SUCCESS;
}

switch_status_t bm_mysql_uninit()
{
    return SWITCH_STATUS_SUCCESS;
}

