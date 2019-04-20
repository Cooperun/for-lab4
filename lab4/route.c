// #pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>

#include <arpa/inet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
// #include <net/if.h>

#define MAX_ROUTE_INFO 10
#define MAX_ARP_SIZE 10
#define MAX_DEVICE 10


struct route_item{
    char destination[16];
    char gateway[16];
    char netmask[16];
    char interface[16];
}route_info[MAX_ROUTE_INFO];
// the sum of the items in the route table
int route_item_index=0;

//the informaiton of the " my arp cache"
struct arp_table_item{
    char ip_addr[16];
    char mac_addr[18];
}arp_table[MAX_ARP_SIZE];
// the sum of the items in the arp cache
int arp_item_index =0;

// the storage of the device , got information from configuration file : if.info
struct device_item{
    char interface[14];
    char mac_addr[18];
}device[MAX_DEVICE];
//the sum of the interface
int device_index=0;

void init(){
    FILE *fp = fopen("route.txt","r");
    while(feof(fp)==0) 
    {
        fscanf(fp,"%s", route_info[route_item_index].destination);
		fscanf(fp,"%s",route_info[route_item_index].gateway);
		fscanf(fp,"%s",route_info[route_item_index].netmask);
		fscanf(fp,"%s",route_info[route_item_index].interface);
        route_item_index++;
    }
    fclose(fp);

	fp = fopen("arp.txt","r");
	while(!feof(fp)){
		fscanf(fp,"%s",arp_table[arp_item_index].ip_addr);
		fscanf(fp,"%s",arp_table[arp_item_index].mac_addr);
		arp_item_index++;
	}
	fclose(fp);

	fp = fopen("device.txt","r");
	while(!feof(fp)){
		fscanf(fp,"%s",device[device_index].interface);
		fscanf(fp,"%s",device[device_index].mac_addr);
		device_index++;
	}
	fclose(fp);
}

void fill_icmp(struct icmphdr* hdr){
    // 清空 
    memset(hdr, 0, sizeof(*hdr));
    // 初始化 ICMP Header
    hdr->type = ICMP_ECHO;
    hdr->code = 0;
    hdr->checksum = 0;
    hdr->un.echo.id = 0;
    hdr->un.echo.sequence = 0;
    // hdr->checksum = checksum((unsigned short*)hdr, sizeof(*hdr));
}

int main(){
    char buf[1024]={};
    struct sockaddr_in addr;
    struct icmphdr *hdr;
    // printf("%ld\n",sizeof(hdr));
    unsigned char arp[98]={
	//-------MAC头部---------14
	0x00,0x0c,0x29,0x2c,0x4a,0x67,//dst MAC广播包，全FF
	0x00,0x0c,0x29,0x08,0x8d,0x20,//src MAC自己的MAC地址
	0x08,0x00,//pro_type
    0x45,0x00,0x00,0x54,0xbf,0xb0,0x40,0x00,0x40,0x01,0xf5,0xa4,0xc0,0xa8,0x02,0x02,
    0xc0,0xa8,0x02,0x01,
    0x08,0x00,0x84,0x43,0x07,0x96,0x00,0x0a,0x09,0x52,0xb9,0x5c,0x00,0x00,0x00,0x00,
    0xdf,0x9a,0x0b,0x00,0x00,0x00,0x00,0x00,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,
    0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,
    0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,
    };

    int sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    struct sockaddr_ll sll;
	bzero(&sll,sizeof(sll));

	sll.sll_ifindex = if_nametoindex("ens33");

    while(1){
        int a = sendto(sd,arp, 84,0,
            (struct sockaddr*)&sll,sizeof(sll));

        sleep(1);
        printf("%d\n",a);
    }
    return 0;
}