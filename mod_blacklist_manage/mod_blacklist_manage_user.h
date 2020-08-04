
#pragma once

#include <switch.h>
#include "apr_pools.h"
#include "apr_hash.h"
#include "mod_blacklist_manage_define.h"

extern "C" {
#include "mod_as_dbcache.h"
}


typedef struct ST_SQL_RESULT
{
    apr_pool_t *pnewpool;           // 内存池
    apr_hash_t *pnewhash_user;      // hash指针
    apr_hash_t *pnewhash_num;
} SQL_RESULT;


typedef struct ST_USER_INFO_MANAGE
{
    apr_pool_t              *phandlepool;       // 用于操作的内存指针；动态的分配指向内存池1 或内存池2      
    apr_hash_t              *phandlehash_user;       // 用于操作的hash 指针    ；动态的分配指向hash1  或hash2     
    apr_hash_t              *phandlehash_num;

    apr_pool_t              *ppool1;            // 内存池1
    apr_hash_t              *phash_user1;           
    apr_hash_t              *phash_num1; 
    
    apr_pool_t              *ppool2;            // 内存池2
    apr_hash_t              *phash_user2;           
    apr_hash_t              *phash_num2; 
    
} USER_INFO_MANAGE;

switch_status_t blacklist_manage_user_init();
switch_status_t blacklist_manage_user_uninit();
switch_status_t blacklist_manage_user_update();

switch_status_t blacklist_manage_user_create(int userid, const string &username, double fee);
switch_status_t blacklist_manage_user_status(int userid, int status);
switch_status_t blacklist_manage_user_balance(int userid, double money);
switch_status_t blacklist_manage_user_credit(int userid, double money);
switch_status_t blacklist_manage_user_fee(int userid, double fee);
switch_status_t blacklist_manage_user_num_add(int userid, const string &num);
switch_status_t blacklist_manage_user_num_del(int userid, const string &num);

switch_status_t blacklist_manage_user_get(const string &num, USER_INFO &userinfo);
switch_status_t blacklist_manage_user_deduction(int userid, int feeflag);


