

#pragma once


#include "mod_blacklist_manage_define.h"

switch_status_t blacklist_manage_record_init();

switch_status_t blacklist_manage_record_uninit();

switch_status_t blacklist_manage_record_write(int userid, 
                                                           const string &caller,
                                                           const string &callee,
                                                           double fee,
                                                           int hit );



