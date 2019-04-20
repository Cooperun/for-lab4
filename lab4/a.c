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

// typedef struct arg
// {
// 	int fd;
// 	int sock_raw_fd;
// }SOCK_FD;

int main(){
    char buf[1024]={};
    struct sockaddr_in addr;
    struct icmphdr *hdr;
    // printf("%ld\n",sizeof(hdr));
    unsigned char arp[64]={
	//-------MAC头部---------14
	0x00,0x0c,0x29,0x2c,0x4a,0x67,//dst MAC广播包，全FF
	0x00,0x0c,0x29,0x08,0x8d,0x20,//src MAC自己的MAC地址
	0x08,0x00,//pro_type
    // };

	0x45,0x00,0x00,0x54,0x33,0x3f,0x40,0x00,0x40,0x01,0x82,0x16,
    0xc0,0xa8,0x02,0x02,
    0xc0,0xa8,0x02,0x01
	};
   int sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));

    struct sockaddr_ll sll;
	bzero(&sll,sizeof(sll));

	sll.sll_ifindex = if_nametoindex("ens33");

    while(1){
        int a = sendto(sd,arp, sizeof(arp),0,
            (struct sockaddr*)&sll,sizeof(sll));
        printf("%d\n",a);
    }
    return 0;
}