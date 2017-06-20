#include <stdio.h>
#include <stdlib.h>
#define TANK_SIZE 1024
struct mac_ip{
	char Mac[32];
	int isAP;
	int times;
	struct mac_ip *next;
};
typedef struct mac_ip Mac_IP;
int hahash(const char mac[],int n)
{
	int len=strlen(mac);
	int i,sum=0;
	for(i=0;i<len;i++)
		sum+=mac[i];
	return(sum%n);
}

Mac_IP *tank[TANK_SIZE];
int mac_count = 0;
int ap_count = 0;
int insert_new(char mac[], int isAp)
{
	Mac_IP *temp;
	Mac_IP *p;
	int i,n;
	n=hahash(mac,TANK_SIZE);
	if(tank[n] == NULL){
		temp=(Mac_IP*)malloc(sizeof(Mac_IP));
		if(temp == NULL){
			printf("Malloc error!\n");
		}
		strcpy(temp->Mac,mac);
		temp->isAP = isAp;
		temp->times = 1;
		temp->next=NULL;

		tank[n]=temp;
		mac_count++;
		if(isAp)
			ap_count++;
	}
	else{
		p=tank[n];
		while(p != NULL){
			if(strcmp(p->Mac,mac) == 0)
			{
				p->times++;
				return 1;
			}
			else
				p=p->next;
		}

		temp=(Mac_IP*)malloc(sizeof(Mac_IP));
		if(temp == NULL){
			printf("Malloc error!\n");
		}
		strcpy(temp->Mac,mac);
		temp->isAP = isAp;
		temp->times = 1;
		temp->next=tank[n];
		tank[n]=temp;
		mac_count++;
		if(isAp)
			ap_count++;
	}

	return 0;
}
char *hash_search(char mac[])
{
	int n;
	Mac_IP *p;
	n=hahash(mac,TANK_SIZE);
	if(tank[n] == NULL){
		printf("No record of this MAC!\n");
		return NULL;
	}
	else{
		p=tank[n];
		while(p != NULL){
			if(strcmp(p->Mac,mac) == 0)
				return p->Mac;
			else
				p=p->next;
		}
		printf("No record of this MAC!\n");
		return NULL;
	}
}
void hash_reset()
{
	Mac_IP *p;
	Mac_IP *pn;
	mac_count = 0;
	ap_count = 0;
	int i;
	for(i=0;i<TANK_SIZE;i++)
	{
		p=tank[i];
		while(p != NULL){
				pn = p->next;
				free(p);	
				p = pn;
		}
		tank[i] = NULL;
	}
	
}

char *hash_dump()
{
	Mac_IP *p;
	int i;
	for(i=0;i<TANK_SIZE;i++)
	{
		p=tank[i];
		while(p != NULL){
			fprintf(stderr,"%s %d",p->Mac,p->times);
			if(p->isAP == 1)
				fprintf(stderr," isAP");
			fprintf(stderr,"\n");
			p = p->next;
		}
		tank[i] = NULL;
	}
	

}
void hash_init()
{
	int i;
	for(i=0;i<TANK_SIZE;i++)
		tank[i]=NULL;
}
#ifdef TEST
int main(int argc,char **argv)
{
	int i;
	char *ip=NULL;
	char key[32];

	hash_init();
	insert_new("10-2F-34-25-6D-DE","192.168.10.25");
	insert_new("10-2F-34-25-6D-DE","192.168.10.25");
	insert_new("10-2F-34-25-6D-DE","192.168.10.25");
	insert_new("10-2F-34-6D-DE-25","192.168.1.126");
	insert_new("4D-25-E4-2C-62-DE","10.0.0.234");
	for(i=0; i<100000;i++)
	{
		sprintf(key,"sample-%d",i);
		insert_new(key,"1");
		insert_new(key,"1");
	}
	
	printf("mac_count=%d\n",mac_count);
	ip=hash_search("10-2F-34-25-6D-DE");
	if(ip != NULL){
		printf("ip:%s\n",ip);
	}
	hash_reset();
	insert_new("10-2F-34-25-6D-DE","192.168.10.25");
	insert_new("10-2F-34-25-6D-DE","192.168.10.25");
	insert_new("10-2F-34-25-6D-DE","192.168.10.25");
	insert_new("10-2F-34-6D-DE-25","192.168.1.126");
	insert_new("4D-25-E4-2C-62-DE","10.0.0.234");
	for(i=0; i<100000;i++)
	{
		sprintf(key,"sample-%d",i);
		insert_new(key,"1");
		insert_new(key,"1");
	}
	
	printf("mac_count=%d\n",mac_count);
	return 0;
}
#endif
