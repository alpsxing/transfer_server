#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>
#include<ctype.h>
#include <time.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <unistd.h>
#include <algorithm>
#include "transfer.h"
#include "cJSON.h"

using namespace std;

#define MYPORT        10054
#define QUEUE         10
#define BUFFER_SIZE   204800

#define UP_INVL       300
#define MAX_COUNT   10000

#define DEFAULT_EQUIP   "./data/equip.json"
string cszl_file="./data/cszl008.txt";

string wlrz_file="./updata/wlrz001.txt";
string sbzl_file="./updata/sbzl010.txt";

static pthread_mutex_t file_mutex;

string protocolsrc="";
int total_count = 0;
int tmp_count = 0;

typedef struct
{
    short datatype; 
    short termtype;  // 0:未识别  1:终端  2: AP
    unsigned char mac[6];
    char max_signal;
    char latest_signal;
    int itime;
    // 16 byte
    unsigned char issid[256];       //  3
    unsigned char essid[64];       // 6
    unsigned char bssid[12];	// 7
    unsigned char channel;        // 8 
    unsigned char security[2];  // 9
}MacRecord;

struct MsgHeader {
    short version;
    unsigned char dev[6];   //设备mac
    short type;    // 1 mac采集   2虚拟身份 
    int len;
    unsigned char hexmac[17]; 
};
int key = 0x66;
struct EquipJson
{
    cJSON *json;
    int arraysize;
    int equipflag;
};

struct EquipJson equipjson;

template <class Type>
inline string numToString(const Type& value)
{
    stringstream strStream;
    strStream << value; 
    return strStream.str();
};
inline string hexToString(int value)
{
    stringstream strStream;
    strStream << std::hex << std::setw(2) << std::setfill('0') << value; 
    return strStream.str();
};

void procsbzl010file()
{
    cJSON *root = NULL, *node=NULL;
    int i=0, j=0;
    char *out=NULL;
    FILE *fp=NULL;

    root = cJSON_CreateArray();
    
    for (i = 0; i < equipjson.arraysize; i++)
    {
        j = equipjson.equipflag>>i;
        if((j&0x01) == 1)
        {
            node = cJSON_GetArrayItem(equipjson.json, i);
            cJSON_AddItemReferenceToArray(root, node);
        }
    }
    
    out = cJSON_Print(root);
    printf("010file:%s\n",out );
    fp = fopen(sbzl_file.c_str(), "w+");
    if(!fp)
        return;
    fwrite(out,1,strlen(out),fp);
    fclose(fp);
    free(out);
    cJSON_Delete(root);

    equipjson.equipflag = 0;
    
}
#if 0
void procsbzl010file(struct MsgHeader *msgh)
{
    string src="";
    FILE *fp=NULL;
    int i=0;

    src += "[{\"EQUIPMENT_NUM\":\"01088888826DB8AE43497\",\"EQUIPMENT_NAME\":\"WIFI\",\"MAC\":\"";
    for(i = 0; i < 6; i ++)
    {
        if(i == 5)
            src += hexToString(msgh->dev[i]);
        else
            src += hexToString(msgh->dev[i])+"-";
    }
    src += "\",\"IP\":\"\",\"SECURITY_FACTORY_ORGCODE\":\"010888888\",\"VENDOR_NAME\":\"\",\"VENDOR_NUM\":\"";
    src += "\",\"SERVICE_CODE\":\"01012345678900\",\"PROVINCE_CODE\":\"410000\",\"CITY_CODE\":\"410823\",\"AREA_CODE\":\"000001";
    src += "\",\"INSTALL_DATE\":\"\",\"INSTALL_POINT\":\"\",\"EQUIPMENT_TYPE\":\"00\",\"LONGITUDE\":\"\",\"LATITUDE\":\"";
    src += "\",\"SUBWAY_STATION\":\"\",\"SUBWAY_LINE_INFO\":\"\",\"SUBWAY_VEHICLE_INFO\":\"";
    src += "\",\"SUBWAY_COMPARTMENT_NUM\":\"\",\"CAR_CODE\":\"\",\"UPLOAD_TIME_INTERVAL\":\"";
    src += "\",\"COLLECTION_RADIUS\":\"\",\"CREATE_TIME\":\"2017-06-14 11:20:00\",\"CREATER\":\"";
    src += "\",\"LAST_CONNECT_TIME\":\"\",\"REMARK\":\"\",\"WDA_VERSION\":\"\",\"FIRMWARE_VERSION\":\"\"}]";

    pthread_mutex_lock(&file_mutex);
    fp = fopen(sbzl_file.c_str(), "w+");
    if(!fp)
        return;
    fwrite(src.c_str(),1, src.length(),fp);
    fclose(fp);
    pthread_mutex_unlock(&file_mutex);
}
#endif
void procwlrz001file()
{
    printf("total_count=%d, tmp_count=%d\n",total_count,tmp_count);
    //printf("JSON:%s\n", protocolsrc.c_str());
    FILE *fp=NULL;
    pthread_mutex_lock(&file_mutex);
    fp = fopen(wlrz_file.c_str(), "a+");
    if(!fp)
        return;
    fwrite(protocolsrc.c_str(),1, protocolsrc.length(),fp);
    fclose(fp);  
    pthread_mutex_unlock(&file_mutex);
    tmp_count = 0;
    protocolsrc = "";
}

void procprotocol(MacRecord *mac, struct MsgHeader *msgh)
{
    int i=0;
    
    std::string src=""; 
    string tmp="";
    
    if(tmp_count != 0)
        src += ",";
    tmp_count ++;
    
    src += "{\"MAC\":\"";
    for(i = 0; i < 6; i ++)
    {
        if(i == 5)
            src += hexToString(mac->mac[i]);
        else
            src += hexToString(mac->mac[i])+"-";
    }
    src += "\",\"TYPE\":\"";
    if(mac->termtype == 2)
        src += "1";
    else 
        src += "2";
    src += "\",\"START_TIME\":\"";
    src += numToString<int>(mac->itime);
    src += "\",\"END_TIME\":\"";
    src += numToString<int>(mac->itime);
    src += "\",\"POWER\":\"";
    src += numToString<int>(mac->latest_signal);
    src += "\",\"BSSID\":\"";
    tmp ="";
    if(mac->bssid[0] != '\0')
    {
        for(i = 0; i < 12; i ++)
        {
            tmp += mac->bssid[i];
            tmp += mac->bssid[i+1];
            if(i == 10)
                break;
            tmp += "-";
            i++;
        }
    }
    else
        tmp += "\"00-00-00-00-00-00\"";
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);  
    
    src += tmp;
    src += "\",\"ESSID\":\"";
    src += std::string((char *)mac->essid);
    src += "\",\"HISTORY_ESSID\":\"";
    src += string((char *)mac->issid);
    src += "\",\"MODEL\":\"\",\"OS_VERSION\":\"\",\"IMEI\":\"\",\"IMSI\":\"";
    src += "\",\"STATION\":\"\",\"XPOINT\":\"0\",\"YPOINT\":\"0\"\"PHONE\":\"\",\"DEVMAC\":\"";
    src += string((char *)msgh->hexmac);
    src += "\",\"DEVICENUM\":\"";
    src += ORG_ID;
    tmp = "";
    for(i = 0; i < 6; i ++)
    {
        tmp += hexToString(msgh->dev[i]);
    }
    transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);  
    src += tmp;
    src += "\",\"SERVICECODE\":\"01012345678900";
    src += "\",\"PROTOCOL_TYPE\":\"\",\"ACCOUNT\":\"\",\"FLAG\":\"\",\"URL\":\"\",\"COMPANY_ID\":\"";
    src += "\",\"AP_CHANNEL\":\"\",\"AP_ENCRYTYPE\":\"\",\"CONSULT_XPOINT\":\"\",\"CONSULT_YPOINT\":\"\"}";

    //printf("src=%s\n", src.c_str());
    protocolsrc += src;
    total_count++;
}
void printmacrecord(MacRecord *mac,  int ilen)
{
    printf("type=%d, mac=%02x-%02x-%02x-%02x-%02x-%02x, signal(%d,%d),%d,%s,%s, bssid: %s, totallen=%d\n",
        mac->termtype,mac->mac[0],mac->mac[1],mac->mac[2],mac->mac[3],mac->mac[4],mac->mac[5],
        mac->max_signal,mac->latest_signal,mac->itime, mac->issid,mac->essid,mac->bssid, ilen);
}
void procmac(unsigned char *buff, struct MsgHeader *msgh)
{
    MacRecord mach;
    int itype =0;
    int  ilen=0, totallen=0;
    unsigned char *p = buff;
    totallen = msgh->len -14;
macrecord:
    if(totallen < 16)
        return;
    memset(&mach, 0, sizeof(MacRecord));
    mach.datatype = p[0]<<8|p[1];
    mach.termtype = p[2]<<8|p[3];
    memcpy(mach.mac, p+4, 6);
    mach.max_signal = p[10];
    mach.latest_signal= p[11];
    mach.itime = p[12]<<24|p[13]<<16|p[14]<<8|p[15];
    
    p += 16;
    totallen -= 16;
    if(mach.termtype == 1)    //终端
    {
   type_sta:
        itype = (p[0]<<8) |p[1];
        ilen = (p[2]<<8) |p[3];
        if(itype == 3)   //类型
        {
            memcpy(mach.issid, p+4, ilen);
        }
        else if(itype == 6)
        {
            memcpy(mach.essid, p+4, ilen);
        }
        else if(itype == 7)
        {
            memcpy(mach.bssid, p+4, ilen);
        }
        else
        {
            // 8
            p = p+4+ilen;
            totallen = totallen-4-ilen;
            //printmacrecord(&mach, totallen);
            procprotocol(&mach, msgh);
            goto macrecord;
        }
        p = p+4+ilen;
        totallen = totallen-4-ilen;
        goto type_sta;
    }
    else if(mach.termtype == 2)     // AP
    {
    type_ap:
        itype = (p[0]<<8) |p[1];
        ilen = (p[2]<<8) |p[3];
        if(itype == 6)
        {
            memcpy(mach.essid, p+4, ilen);
        }
        else if(itype == 7)
        {
            memcpy(mach.bssid, p+4, ilen);
        }
        else
        {
            // 8 + 9
            p = p+12;
            totallen= totallen - 12;
            //printmacrecord(&mach, totallen);
            procprotocol(&mach, msgh);
            goto macrecord;
        }
        p = p+4+ilen;
        totallen = totallen-4-ilen;
        goto type_ap;
     }
    else
    {
        p = p+6;
        totallen = totallen - 6;
        //printmacrecord(&mach, totallen);
        goto macrecord;
    }
}

void procmsg(unsigned char * buff)
{
    struct MsgHeader msgh  ;
    int i = 0, ret=0;
    cJSON *node;

    memset(&msgh, 0, sizeof(struct MsgHeader));
    msgh.version = buff[0]<<8|buff[1];
    memcpy(msgh.dev, buff+2, 6);
    msgh.type = buff[8]<<8|buff[9];
    msgh.len = buff[10]<<24|buff[11]<<16|buff[12]<<8|buff[13];
    string src="";
    for(i = 0; i < 6; i ++)
    {
        if(i == 5)
            src += hexToString(msgh.dev[i]);
        else
            src += hexToString(msgh.dev[i])+"-";
    }
    transform(src.begin(), src.end(), src.begin(), ::toupper);  
    memcpy(msgh.hexmac, src.c_str(), 17);
    
    printf("msgheader: version=%d;  type=%d;  len=%d;  devflag=%s\n", (msgh.version), (msgh.type), (msgh.len), msgh.hexmac);
    char *p;
    for(i=0; i<equipjson.arraysize; i++)
    {
        node = cJSON_GetArrayItem(equipjson.json, i);
        p = cJSON_GetObjectItem(node, "MAC")->valuestring;
        printf("equipjson i=%d, v=%s\n", i, p);
        if(memcmp(msgh.hexmac, p,17)==0)
        {
            equipjson.equipflag |= (1<<i);
            break;
        }
    }
    printf("equipflag=%02x\n", equipjson.equipflag);
    
    //procsbzl010file(&msgh);

    if(msgh.type == 2){
        printf("data: %s\n\n", buff+14);
    }
    if(msgh.type == 1){
        procmac(buff+14, &msgh);
        procwlrz001file();
    }
    else{
        printf("\n");
    }
}

unsigned char * crypt(unsigned char * buff, int len)
{
    int i;
    for(i = 0; i < len; i++){
        *(buff+i) = *(buff+i) ^ key;
    }
    #if 0
    printf("receiver msg: \n");
    for(i=0; i<len; i++)
    {
         printf("%02x ", buff[i]);
         if(i%16 == 0)
            printf("\n");
    }
    printf("\n");
    #endif
    procmsg(buff);
    return buff;
}
void proc_equip_service_file()
{
    FILE *fp=NULL;
    long elen;
    char *ebuf;
    cJSON *json;
    int i=0;

    fp=fopen(DEFAULT_EQUIP,"r");
    if(fp==NULL) 
        return;

    if (fseek(fp,0,SEEK_END))
        goto exit;

    elen = ftell(fp);
    if (elen <= 0)
        goto exit;

    rewind(fp);
    
    ebuf=(char*)malloc(elen);
    if (!ebuf)
        goto exit;
    memset(ebuf, 0, elen);
    
    if(elen != fread(ebuf,1,elen,fp))
        goto exit;

   equipjson.json = NULL;
   equipjson.arraysize = 0;
   equipjson.equipflag = 0;
    
    equipjson.json=cJSON_Parse(ebuf);
    if (!equipjson.json) 
    {
        printf("Error before: [%s]\n",cJSON_GetErrorPtr());
        goto exit;
    }
    equipjson.arraysize = cJSON_GetArraySize(equipjson.json);

exit:  
    if(fp)
        fclose(fp);
    if(ebuf)
        free(ebuf);
    return;
}

static int  start_transfer_time=0;
void * control_if_transfer_thread(void *)
{
    int itime =0;
    int should_transfer=0;
    string wlrz001_file="";
    
    while(1)
    {
        itime = (int)time(NULL);
        if(itime >= start_transfer_time && total_count>0)
        {
            start_transfer_time = itime + UP_INVL;
            should_transfer = 1;
        }
        if(total_count >= MAX_COUNT)
            should_transfer = 1;
        
        if(should_transfer == 1)
        {
            pthread_mutex_lock(&file_mutex);
            should_transfer = 0;
            wlrz001_file = wlrz_file;
            total_count = 0;
            wlrz_file = "wlrz010" + numToString<int>(time(NULL))+".txt";
            
            FILE *fp = NULL;
            fp = fopen(wlrz_file.c_str(), "w+");
            fwrite("[",1, 1,fp);
            fclose(fp);  
            fp = fopen(wlrz001_file.c_str(), "a+");
            fwrite("]",1, 1,fp);
            fclose(fp);  
            pthread_mutex_unlock(&file_mutex);
            procsbzl010file();
            printf("----should_transfer------\n");
            startTransferThread();
            sleep(2);
            addTransferTask(cszl_file.c_str(), FILE_TYPE_CSZL);
            sleep(1);
            addTransferTask(sbzl_file.c_str(), FILE_TYPE_SBZL);
            sleep(1);
            addTransferTask(wlrz001_file.c_str(), FILE_TYPE_TZCJRZ);
            sleep(2);
            stopTransferThread();
        }
        sleep(2);
    }
    
}

int main()
{
    ///定义sockfd
    int server_sockfd = socket(AF_INET,SOCK_STREAM, 0);
    
    ///定义sockaddr_in
    struct sockaddr_in server_sockaddr;
    server_sockaddr.sin_family      = AF_INET;
    server_sockaddr.sin_port        = htons(MYPORT);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

    ///bind，成功返回0，出错返回-1
    if(bind(server_sockfd, (struct sockaddr *)&server_sockaddr, sizeof(server_sockaddr))==-1){
        perror("bind");
        exit(1);
    }

    ///listen，成功返回0，出错返回-1
    if(listen(server_sockfd, QUEUE) == -1){
        perror("listen");
        exit(1);
    }
    
    proc_equip_service_file();

    wlrz_file = "wlrz010" + numToString<int>(time(NULL))+".txt";
    FILE *fp=NULL;
    fp = fopen(wlrz_file.c_str(), "w+");
    fwrite("[",1, 1,fp);
    fclose(fp);

    fp = fopen(OKFILE_NAME, "w+");
    fclose(fp);

    
    pthread_t control_tid;
    pthread_mutex_init(&file_mutex, NULL);

    start_transfer_time = (int)time(NULL) + UP_INVL;
    pthread_create(&control_tid, NULL, control_if_transfer_thread, NULL);

    ///客户端套接字
    unsigned char buffer[BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t length = sizeof(client_addr);

    int conn;
    while((conn = accept(server_sockfd, (struct sockaddr*)&client_addr, &length))){
        ///成功返回非负描述字，出错返回-1
        int ret=0, len=0, ilen=0, flag=0;
        memset(buffer, 0, sizeof(buffer));

        while((ret = recv(conn, buffer+ilen, sizeof(buffer)-ilen, 0)) > 0)
        {
            if(ret > 0)
            {
                ilen += ret;
                if(ilen > 14 && flag == 0)
                {
                    len = (buffer[10]^ key)<<24|(buffer[11]^ key)<<16|(buffer[12]^ key)<<8|(buffer[13]^ key);
                    flag = 1;
                }
                
                printf("recv:%d, ilen=%d,  len=%d\n", ret,ilen,len);
            }
            if(ilen > len&& flag==1)
                crypt(buffer, ilen);
        }
        close(conn);

    }
    close(server_sockfd);
    cJSON_Delete(equipjson.json);
    return 0;
}
