#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/times.h>
#include "ulist.h"
#include "ftp.h"
#include "transfer.h"
#include "crypt.h"

static clock_t _sys_ticks_per_sec = 0;

typedef struct {
    const char *name;
    const char *code;
} data_type_info_t;

static const data_type_info_t _data_type_info[] = {
    {"WLRZ", "001"},       // 0
    {"FJGJ", "002"},       // 1
    {"JSTX", "003"},       // 2
    {"XWRZ", "004"},       // 3
    {"SJRZ", "005"},       // 4
    {"PTNR", "006"},       // 5
    {"SGJZ", "007"},       // 6
    {"CSZL", "008"},       // 7
    {"CSZT", "009"},       // 8
    {"SBZL", "010"},       // 9
    {"JSJZT", "011"},      // 10
    {"SBGJ", "012"},       // 11
    {"RZSJ", "013"},       // 12
    {"SJTZ", "014"},       // 13
    {"SFGX", "015"},       // 14
    {"SPCL", "016"},       // 15
    {"SMS", "017"},        // 16
    {"PNFJ", "999"}        // 17
};

typedef struct {
    unsigned int type;
    char *name;
    char *path;
    char *remote_name;
    char *remote_ok;
    time_t tm;
    struct list_head node;
} transfer_file_t;

static pthread_t _transfer_tid;
static int _transfer_started = 0;
static int _transfer_exit = 0;
static pthread_mutex_t _transfer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t _transfer_cond = PTHREAD_COND_INITIALIZER;
static LIST_HEAD(_transfer_list);

int getSecondsPassed(clock_t last_clock)
{
    clock_t now_clock = times(NULL);
    return (now_clock - last_clock) / _sys_ticks_per_sec;
}

static int updateTransferFile(transfer_file_t *tf)
{
    const char *ts = _data_type_info[tf->type].name;
    const char *cs = _data_type_info[tf->type].code;
    int plen = strlen(ts) + 15;
    int flen = 25 + strlen(DEV_TYPE) + strlen(ORIG_ID) + strlen(ORG_ID) + strlen(cs);
    int okflen = flen+3;
    struct tm *tm = localtime(&tf->tm);
    int ret;
    int r;

    tf->path = (char *)malloc(plen+1);
    if (!tf->path)
        return -1;
    ret = snprintf(tf->path, plen+1, "%s/%04d%02d%02d/%02d/%02d",
                   ts, tm->tm_year+1900,
                   tm->tm_mon+1,
                   tm->tm_mday,
                   tm->tm_hour,
                   tm->tm_min);
    if (ret != plen) {
        free(tf->path);
        tf->path = NULL;
        return -1;
    }

    tf->remote_name = (char *)malloc(flen+1);
    if (!tf->remote_name) {
        free(tf->path);
        tf->path = NULL;
        return -1;
    }
    r = rand()%1000;
    ret = snprintf(tf->remote_name, flen+1, "%04d%02d%02d%02d%02d%02d%03d_%s_%s_%s_%s.log",
                   tm->tm_year+1900,
                   tm->tm_mon+1,
                   tm->tm_mday,
                   tm->tm_hour,
                   tm->tm_min,
                   tm->tm_sec,
                   r, DEV_TYPE, ORIG_ID, ORG_ID, cs);
    if (ret != flen) {
        free(tf->path);
        free(tf->remote_name);
        tf->path = NULL;
        tf->remote_name = NULL;
        return -1;
    }
    
    tf->remote_ok = (char *)malloc(okflen+1);
    if (!tf->remote_ok) {
        free(tf->path);
        free(tf->remote_name);
        tf->path = NULL;
        tf->remote_name = NULL;
        return -1;
    }
    ret = snprintf(tf->remote_ok, okflen+1, "%s.ok", tf->remote_name);
    if (ret != okflen) {
        free(tf->path);
        free(tf->remote_name);
        free(tf->remote_ok);
        tf->path = NULL;
        tf->remote_name = NULL;
        tf->remote_ok = NULL;
        return -1;
    }
    
    return 0;
}

static int executeTransferFile(transfer_file_t *tf)
{
    char *ofname = NULL;
    int ofnamelen = strlen(tf->name) + 4;
    int ret = -1;
    int try_count = 0;

    ofname = (char *)malloc(ofnamelen+1);
    if (!ofname)
        goto exit;
    if (ofnamelen != snprintf(ofname, ofnamelen+1, "%s.enc", tf->name))
        goto exit;

    //if (encode_file(tf->name, ofname))
     //   goto exit;

    while (ret && try_count < 20) {
        if (try_count > 0)
            sleep(5);
        ret = ftp_transfer(tf->name, tf->path, tf->remote_name);
        try_count ++;
    }

    if (ret < 0)
        goto exit;

    ret = -1;
    try_count = 0;
    
    while (ret && try_count < 20) {
        if (try_count > 0)
            sleep(5);
        ret = ftp_transfer(OKFILE_NAME, tf->path, tf->remote_ok);
        try_count ++;
    }

exit:
    if (ofname)
        free(ofname);
    return ret;
}

static void freeTransferFile(transfer_file_t *tf)
{
    free(tf->name);
    if (tf->path)
        free(tf->path);
    if (tf->remote_name)
        free(tf->remote_name);
    if (tf->remote_ok)
        free(tf->remote_ok);
    free(tf);
}

static void *transferThreadLoop(void *arg)
{
    transfer_file_t *t, *tmp;

    _sys_ticks_per_sec = sysconf(_SC_CLK_TCK);

    while (1) {
        pthread_mutex_lock(&_transfer_mutex);
        while (_transfer_exit == 0 && list_empty(&_transfer_list))
            pthread_cond_wait(&_transfer_cond, &_transfer_mutex);
        if (_transfer_exit) {
            pthread_mutex_unlock(&_transfer_mutex);
            break;
        }
        while (!list_empty(&_transfer_list)) {
            t = list_entry(_transfer_list.next, typeof(*t), node);
            list_del(&t->node);
            pthread_mutex_unlock(&_transfer_mutex);
            if (!updateTransferFile(t)) {
                executeTransferFile(t);
            }
            freeTransferFile(t);
            pthread_mutex_lock(&_transfer_mutex);
        }
        pthread_mutex_unlock(&_transfer_mutex);
    }
    pthread_mutex_lock(&_transfer_mutex);
    list_for_each_entry_safe (t, tmp, &_transfer_list, node) {
        list_del(&t->node);
        freeTransferFile(t);
    }
    pthread_mutex_unlock(&_transfer_mutex);
    pthread_exit(NULL);
}

int startTransferThread()
{
    int ret;
    if (_transfer_started)
        return -1;

    _transfer_exit = 0;
    ret = pthread_create(&_transfer_tid, NULL, transferThreadLoop, NULL);
    if (!ret)
        _transfer_started = 1;
    return ret;
}

int stopTransferThread()
{
    if (!_transfer_started)
        return -1;

    pthread_mutex_lock(&_transfer_mutex);
    _transfer_exit = 1;
    pthread_cond_signal(&_transfer_cond);
    pthread_mutex_unlock(&_transfer_mutex);
    pthread_join(_transfer_tid, NULL);
    _transfer_started = 0;
    return 0;
}

int addTransferTask(const char *filename, unsigned int filetype)
{
    transfer_file_t *f = (transfer_file_t *)malloc(sizeof(transfer_file_t));
    if (!f)
        return -1;
    memset(f, 0, sizeof(*f));
    f->type = filetype;
    f->name = strdup(filename);
    time(&f->tm);
    pthread_mutex_lock(&_transfer_mutex);
    list_add(&f->node, &_transfer_list);
    pthread_cond_signal(&_transfer_cond);
    pthread_mutex_unlock(&_transfer_mutex);
    return 0;
}
