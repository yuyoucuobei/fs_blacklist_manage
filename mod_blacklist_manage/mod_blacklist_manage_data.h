

#pragma once

#include "mod_blacklist_manage_define.h"

typedef struct ST_USER_CHECKINFO
{
    int userid;
    string username;
    string day;
    int total;
    int normal;
    int black;
} USER_CHECKINFO;


switch_status_t bm_user_create(const string &username, double fee);

switch_status_t bm_user_status(int userid, int status);

switch_status_t bm_user_balance(int userid, double money);

switch_status_t bm_user_credit(int userid, double money);

switch_status_t bm_user_fee(int userid, double fee);

switch_status_t bm_user_num_add(int userid, const string &num);

switch_status_t bm_user_num_del(int userid, const string &num);

switch_status_t bm_user_getinfo(int userid, string &userinfo);

switch_status_t bm_user_getinfo_all(string &userinfo);

switch_status_t bm_user_checkinfo(int userid, const string &day, string &checkinfo);

switch_status_t bm_user_deduction(int userid, int feeflag);

void get_time_str(char *pdate, int len);

void get_time_str_tomorrow(char *pdate, int len);


