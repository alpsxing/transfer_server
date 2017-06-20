#include <stdio.h>
#include <linux/limits.h>
#include <stdlib.h>
#include <string.h>
#include <ftplib.h>

#define FTP_HOST "61.163.24.98:6069"
//#define FTP_HOST "127.0.0.1"
#define FTP_USER "xiruan"
//#define FTP_USER "ftp"
//#define FTP_PASSWORD "ftp"
#define FTP_PASSWORD "xiruan@123"

int ftp_transfer(const char *local_file, const char *remote_path, const char *remote_file)
{
    netbuf * ftph;
    int rc = -1;
    int rindex = strlen(remote_path) - 1;
    int lindex;
    char path[PATH_MAX];
    char *remote_file_tmp = NULL;
    int tmp_filename_len = strlen(remote_file) + 4;

    FtpInit();
    printf("---ftpinit ok-----\n");
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
    FtpQuit(ftph);
    printf("----ftp_transfer exit----\n");
    return rc;
}


