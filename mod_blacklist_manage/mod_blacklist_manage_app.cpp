

#include "mod_blacklist_manage_app.h"
#include "mod_blacklist_manage_data.h"
#include "mod_blacklist_manage_record.h"
#include "mod_blacklist_manage_deduction.h"
#include "mod_blacklist_manage_user.h"


switch_status_t blacklist_manage_check(switch_core_session_t *session, const char *data)
{
    // bm check
    //get channel & profile & caller & callee
    switch_channel_t* channel = NULL;
    switch_caller_profile_t* profile = NULL;
    const char* uuid = NULL;
    const char* caller_id_number = NULL;
    const char* destination_number = NULL;
    int feeflag = 0; // 0-fee balance, 1-fee credit
    int hit = 0;
    
    if (NULL == (uuid = switch_core_session_get_uuid(session))
        || NULL == (channel = switch_core_session_get_channel(session)) 
        || NULL == (profile = switch_channel_get_caller_profile(channel))
        || NULL == (destination_number = switch_caller_get_field_by_name(profile, "destination_number"))
        || NULL == (caller_id_number = switch_caller_get_field_by_name(profile, "caller_id_number")))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_ERROR, 
            "blacklist_manage_check get channel or profile info failed. "
            "channel:%p, profile:%p, dst_num:%p, caller_id_num:%p\n", 
            (void*)channel, (void*)profile, (void*)destination_number, (void*)caller_id_number);
        return SWITCH_STATUS_FALSE;
    }

    //get user info & fee 
    USER_INFO userinfo;

    if(SWITCH_STATUS_SUCCESS != blacklist_manage_user_get(caller_id_number, userinfo))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_NOTICE, 
            "bm_user_getinfo failed, caller_id_number:%s\n",
            caller_id_number);
        return SWITCH_STATUS_FALSE;
    } 

    switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_NOTICE, 
            "userinfo.userid = %d, userinfo.user = %s, userinfo.balance = %f, userinfo.credit = %f, userinfo.status = %d, userinfo.fee = %f\n",
            userinfo.userid,
            userinfo.user,
            userinfo.balance,
            userinfo.credit,
            userinfo.status,
            userinfo.fee);

    //check user status & balance & credit 
    if(0 != userinfo.status)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_NOTICE, 
            "0 != userinfo.status, %d\n", userinfo.status);
        return SWITCH_STATUS_FALSE;
    }

    if(0 >= userinfo.balance && 0 >= userinfo.credit)
    {
        //TODO: set user status to 3
        
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_WARNING, 
            "userinfo.balance = %f && userinfo.credit = %f\n", userinfo.balance, userinfo.credit);
        return SWITCH_STATUS_FALSE;
    }

    //check fee valid
    if(0 >= userinfo.fee)
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_WARNING, 
            "0 >= userinfo.fee = %f\n", userinfo.fee);
        return SWITCH_STATUS_FALSE;
    }

    if(userinfo.fee > userinfo.balance)
    {
        if(userinfo.fee > userinfo.credit)
        {
            switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_WARNING, 
                "fee > balance && fee > credit, fee = %f, balance = %f, credit = %f\n", 
                userinfo.fee, userinfo.balance, userinfo.credit);
            return SWITCH_STATUS_FALSE;
        }
        else
        {
            feeflag = 1;
        }
    }
    else
    {
        feeflag = 0;
    }

    //blacklist check <listname> <number>
    const char *listname = switch_str_nil(switch_core_get_variable("blacklist_name"));
    char carg[128] = {'\0'};
    switch_stream_handle_t stream;
    SWITCH_STANDARD_STREAM(stream);
    snprintf(carg, sizeof(carg), "check %s %s", listname, destination_number);
    if(SWITCH_STATUS_SUCCESS != switch_api_execute("blacklist", carg, NULL, &stream))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_ERROR, 
                "switch_api_execute failed, blacklist %s\n", 
                carg);
        return SWITCH_STATUS_FALSE;
    }

    //hangup, return 480/486
    //switch_channel_hangup(chan(), SWITCH_CAUSE_CALL_REJECTED);
    if(0 == strcmp((const char*)stream.data, "true"))
    {
        switch_channel_hangup(channel, SWITCH_CAUSE_USER_BUSY);
        hit = 1;
    }
    else
    {
        switch_channel_hangup(channel, SWITCH_CAUSE_NORMAL_CLEARING);
        hit = 0;
    }
    switch_safe_free(stream.data);

    //blacklist check record to mysql
    if(SWITCH_STATUS_SUCCESS != blacklist_manage_record_write(userinfo.userid, caller_id_number, destination_number, userinfo.fee, hit))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_ERROR, 
                "blacklist_manage_record_write failed, userinfo.userid = %d, caller_id_number = %s, destination_number = %s, userinfo.fee = %f, hit = %d\n", 
                userinfo.userid, caller_id_number, destination_number, userinfo.fee, hit);
        return SWITCH_STATUS_FALSE;
    }    

    //fee count
    if(SWITCH_STATUS_SUCCESS != bm_user_deduction(userinfo.userid, feeflag))
    {
        switch_log_printf(SWITCH_CHANNEL_UUID_LOG(uuid), SWITCH_LOG_ERROR, 
                "bm_user_deduction failed, userinfo.userid = %d, feeflag = %d\n", 
                userinfo.userid, feeflag);
        return SWITCH_STATUS_FALSE;
    }    
    
    return SWITCH_STATUS_SUCCESS;
}


