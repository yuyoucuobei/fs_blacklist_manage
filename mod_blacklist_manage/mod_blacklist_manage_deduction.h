

#pragma once


#include "mod_blacklist_manage_define.h"

switch_status_t blacklist_manage_deduction_init();

switch_status_t blacklist_manage_deduction_uninit();

switch_status_t blacklist_manage_deduction_write(int userid, 
                                                   int feeflag );



