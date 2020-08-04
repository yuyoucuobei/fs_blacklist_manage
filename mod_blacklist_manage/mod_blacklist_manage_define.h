

#pragma once


#include <switch.h>
#include <mysql/mysql.h>

#include <string>
#include <vector>

using std::string;
using std::vector;

#define USER_NAME_LEN   32

typedef struct ST_USER_INFO
{
    int userid;
    char user[USER_NAME_LEN];
    double balance;
    double credit;
    int status;
    double fee;

    ST_USER_INFO()
    {
        userid = 0;
        memset(user, 0, USER_NAME_LEN);
        balance = 0;
        credit = 0;
        status = -1;
        fee = 1;
    }
} USER_INFO;

typedef struct ST_USER_NUM_INFO
{
    int userid;
    string username;
    double balance;
    double credit;
    int status;
    double fee;
    vector<string> numbers;

    ST_USER_NUM_INFO()
    {
        userid = 0;
        username.clear();
        balance = 0;
        credit = 0;
        status = -1;
        fee = 1;
        numbers.clear();
    }
} USER_NUM_INFO;


typedef struct ST_WORK_QUEUE_INFO
{
    switch_queue_t *pqueue;
    MYSQL *pmysql;
} WORK_QUEUE_INFO;

typedef struct ST_QUEUE_DATA_RECORD
{
    int userid;
    string caller;
    string callee;
    double fee;
    int hit;
} QUEUE_DATA_RECORD;

typedef struct ST_QUEUE_DATA_DEDUCTION
{
    int userid;
    int feeflag;
} QUEUE_DATA_DEDUCTION;


