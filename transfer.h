#ifndef _TRANSFER_TRANSFER_H_
#define _TRANSFER_TRANSFER_H_

enum {
    FILE_TYPE_TZCJRZ = 0,
    FILE_TYPE_WLXNSFGJ = 1,
    FILE_TYPE_JSTXNR = 2,
    FILE_TYPE_SWRZ = 3,
    FILE_TYPE_ZDSXXRZ = 4,
    FILE_TYPE_PTNR = 5,
    FILE_TYPE_SSGJZ = 6,
    FILE_TYPE_CSZL = 7,
    FILE_TYPE_CSZT = 8,
    FILE_TYPE_SBZL = 9,
    FILE_TYPE_ZDJSJZT = 10,
    FILE_TYPE_CJSBYDGJ = 11,
    FILE_TYPE_RZSJ = 12,
    FILE_TYPE_SJTZSJ = 13,
    FILE_TYPE_SFGXSJ = 14,
    FILE_TYPE_SPCLSJ = 15,
    FILE_TYPE_WJZSJ = 16,
    FILE_TYPE_PTNRFJ = 17,
    FILE_TYPE_MAX,
};

#define OKFILE_NAME "./data/ok.log"

#define ORG_ID     "723005104"      //厂商组织机构代码
#define DEV_TYPE "123"               //数据采集系统类型
#define ORIG_ID   "410823"          //数据产生源标识

int startTransferThread();
int stopTransferThread();
int addTransferTask(const char *filename, unsigned int filetype);

#endif
