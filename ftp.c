#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <ftplib.h>
#include <sys/times.h>
#include "transfer.h"

#define FTP_HOST "61.163.24.98:6069"
//#define FTP_HOST "127.0.0.1"
#define FTP_USER "xiruan"
//#define FTP_USER "ftp"
//#define FTP_PASSWORD "ftp"
#define FTP_PASSWORD "xiruan@123"

#define FTP_RECONNECT_TIMEOUT  60

static clock_t _last_conn_ticks = 0;;
static netbuf * ftph = NULL;
static int _ftp_inited = 0;

int update_ftp_conn()
{
    int secs;
    if (ftph) {
        secs = getSecondsPassed(_last_conn_ticks);
        if (secs >= FTP_RECONNECT_TIMEOUT) {
            FtpQuit(ftph);
            ftph = NULL;
            return 0;
        }
        return FTP_RECONNECT_TIMEOUT - secs;
    }
    else
        return 0;
}

int ftp_transfer(const char *local_file, const char *remote_path, const char *remote_file)
{
    int rc = -1;
    int rindex = strlen(remote_path) - 1;
    int lindex;
    char path[PATH_MAX];
    char *remote_file_tmp = NULL;
    int tmp_filename_len = strlen(remote_file) + 4;

    if (!_ftp_inited) {
        FtpInit();
        _ftp_inited = 1;
    }

    if (!ftph || getSecondsPassed(_last_conn_ticks) >= FTP_RECONNECT_TIMEOUT) {
        if (ftph) {
            FtpQuit(ftph);
            ftph = NULL;
        }
        if (!FtpConnect(FTP_HOST, &ftph))
        {
            printf("ftpconnect failed \n");
            return -1;
        }
        printf("---FtpConnect ok-----\n");
    
        if (!FtpLogin(FTP_USER, FTP_PASSWORD, ftph)) {
            printf("Failed to login\n");
            goto exit;
        }
        printf("---FtpLogin ok-----\n");


        if (!FtpOptions(FTPLIB_CONNMODE, FTPLIB_PASSIVE, ftph)) {
            printf("Failed to change to PASSIVE mode\n");
            goto exit;
        }
        _last_conn_ticks = times(NULL);
    }
    else
        printf("ftpconnect using last connection \n");

    while (rindex >= 0) {
        if (remote_path[rindex] == '/') {
            rindex --;
            continue;
        }
        memcpy(path, remote_path, rindex+1);
        path[rindex+1] = 0;
        if (FtpChdir(path, ftph))
            break;

        while (rindex >= 0 && remote_path[rindex] != '/')
            rindex --;
    }

    rindex ++;
    while (rindex < strlen(remote_path)) {
        if (remote_path[rindex] == '/') {
            rindex ++;
            continue;
        }
        lindex = rindex;
        while (rindex < strlen(remote_path) && remote_path[rindex] != '/')
            rindex ++;
        memcpy(path, remote_path + lindex, rindex-lindex);
        path[rindex-lindex] = 0;
        if (!FtpMkdir(path, ftph)) {
            printf("Failed to mkdir\n");
            goto exit;
        }
        if (!FtpChdir(path, ftph)) {
            printf("Failed to chdir\n");
            goto exit;
        }
    }
    printf("---FtpMkdir ok --ftp path=%s----\n", path);


    remote_file_tmp = (char *)malloc(tmp_filename_len + 1);
    if (tmp_filename_len != snprintf(remote_file_tmp, tmp_filename_len + 1, "%s.tmp", remote_file))
        goto exit;

    printf("---befor ftpput  local_file=%s, remote_fille=%s----\n",local_file, remote_file);
    if (!FtpPut(local_file, remote_file_tmp, FTPLIB_IMAGE, ftph))
        goto exit;

    printf("---FtpPut ok------\n");
    if (!FtpRename(remote_file_tmp, remote_file, ftph))
        goto exit;
    printf("---FtpRename ok------\n");

    rc = 0;
exit:
    if (remote_file_tmp)
        free(remote_file_tmp);
    if (rc) {
        FtpQuit(ftph);
        ftph = NULL;
    }
    printf("----ftp_transfer exit----\n");
    return rc;
}


