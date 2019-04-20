#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <string.h>
#include <linux/if_ether.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <sys/time.h>
#include <net/if_arp.h>
#include <iostream>
#include <fstream>
using namespace std;
char buf[1024];
char SendBuf[1024];
const int MAX_ROUTE_INFO = 512;
const int MAX_ARP_SIZE = 512;
const int MAX_DEVICE = 512;
#define MAXINTERFACES 16
struct arp_table_item
{
    char ip_addr[16];
    char mac_addr[18];
}arp_table[MAX_ARP_SIZE];
struct myiphdr
{
    uint8_t hv=0x45;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    uint32_t saddr;
    uint32_t daddr;
};
struct myhdr                                // since that the icmphdr don't have time and data, i define my own hdr
{
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    union
    {
        struct
        {
            uint16_t id;
            uint16_t sequence;
        }echo;
        uint32_t gateway;
        struct
        {
            uint16_t unused;
            uint16_t mtu;
        }frag;
    }un;
    uint32_t  time[2];
    uint8_t data[24];                       //but the uint8_t*data will segmentation fault
};
struct mixhdr{
    uint8_t Dst[6];
    uint8_t Src[6];
    uint8_t type;
    uint8_t code;
};


int arp_item_index = 0;
int SocketId;
int seq=0;
int SendCount=0;
int RevCount=0;
int pid =0;
char * fuckping="123456781234567812345678"; 

fstream file;
sockaddr_ll DestAddr;
sockaddr_ll SrcAddr;
timeval tvstart;
timeval tvend;

char *MacAddr;
char *IpAddr;
char MyMac[20];
char MyIp[20] = "192.168.2.2";
void CreateArp();
void CreateSocket();
void InitDst();
void sigint_handle(int);
char *FindMacAddr(char * IpAddr);
uint16_t cs(uint8_t *addr,int len);
int GetMyMac(char *mac ,int len_limit);
void fill_icmp(myhdr * hdr);
void fill_mix(mixhdr * hdr);
void fill_ip(myiphdr *hdr);

int main(int argc,char *argv[])
{
    CreateArp();
    CreateSocket();
    InitDst();
    pid=getpid();
    signal(SIGINT, sigint_handle);  //注册中断处理函数
    if(argc < 2)
    {
        printf("Please input the addr like \"./ping 200.200.200.1\" !\n");
        exit(1);
    }
    IpAddr = argv[1];
    MacAddr = FindMacAddr(IpAddr);
    GetMyMac(MyMac,20);
    while(1)
    {
        mixhdr hdr;
        myhdr IcmpHeader;
        myiphdr IpHeader;
        fill_mix(&hdr);
        fill_icmp(&IcmpHeader);
        fill_ip(&IpHeader);
        memcpy(SendBuf,&hdr,sizeof(hdr));
        memcpy(SendBuf+14,&IpHeader,sizeof(iphdr));
        memcpy(SendBuf+34,&IcmpHeader,sizeof(myhdr));


        if(sendto(SocketId,&SendBuf,34+sizeof(myhdr),0,(struct sockaddr *)&DestAddr,sizeof(DestAddr)) < 0)
            perror("Sendto Error\n");
        else
            SendCount++;  
        gettimeofday(&tvstart,NULL);
        recv(SocketId,buf,sizeof(buf),0);
        gettimeofday(&tvend,NULL);
        iphdr *iphdrptr = (struct iphdr*)buf;
        icmphdr *icmphdrptr = (struct icmphdr*)(buf + (iphdrptr->ihl) * 4);
        if(icmphdrptr->un.echo.id==pid&&icmphdrptr->type==0)// check if the packet is our packet respond
        {
            RevCount++;
            myhdr *myicmphdr=(myhdr *) icmphdrptr;
            int totlen= ((buf[2]<<8)+buf[3])-20; //compute the length , -20 is the iphdr
            double UseTime;
            struct timeval *sendtime=(struct timeval *)myicmphdr->time; 
            UseTime=((&tvend)->tv_sec-sendtime->tv_sec)*1000+((&tvstart)->tv_usec-sendtime->tv_usec)/1000.0;
            printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f ms\n",totlen,argv[1],icmphdrptr->un.echo.sequence,iphdrptr->ttl,UseTime);
        }
        else
        {
            printf("WTF?!\n");
        }
        
        sleep(1);
    }
    // debug
    // printf("%s\n",MacAddr);
    // for(int i=0;i<arp_item_index;i++)
    //      printf("%s %s\n",arp_table[i].ip_addr,arp_table[i].mac_addr);
    // printf("%s\n",MyMac);
    //     printf("%02X:%02X:%02X:%02X:%02X:%02X\n",hdr.Src[0],hdr.Src[1],hdr.Src[2],hdr.Src[3],hdr.Src[4],hdr.Src[5]);
    // while(1)
    // {
    //     recv(SocketId, buf, sizeof(buf), 0);
    //     iphdr *iphdrptr = (iphdr*)buf;
    //     icmphdr *icmphdrptr = (icmphdr*)(buf + (iphdrptr->ihl) * 4);
    // }
    return 0;
}
void CreateSocket()
{
    SocketId = socket(AF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
    if(SocketId<0)
    {
        printf("create socket failed!\n");
        exit(1);
    }
}
void CreateArp()
{
    file.open("ArpConfig.txt",ios::in);
    if(!file.is_open())
    {
        printf("Can not open file: \"ArpConfig.txt\"\n");
        exit(1);
    }
    while(file>>arp_table[arp_item_index].ip_addr>>arp_table[arp_item_index++].mac_addr);
    arp_item_index-=1;
    file.close();
}
void InitDst()
{
    memset((char *)&DestAddr,0,sizeof(sockaddr_ll));
    DestAddr.sll_family=AF_PACKET;                // 始终是AF_PACKET
    DestAddr.sll_protocol=htons(ETH_P_ALL);       // 物理层协议
    DestAddr.sll_ifindex=if_nametoindex("ens33"); // 接口号.
    printf("接口号：%d\n",DestAddr.sll_ifindex);
    DestAddr.sll_halen=htons(6);                  // 地址长度.htons用作网络字段与主机字段的转换
}
void sigint_handle(int)
{
    printf("\n-------%s ping statistics---------\n",IpAddr);
    printf("%d packets transmitted, %d recived. %lf%% loss\n",SendCount,RevCount,100-100*(double)SendCount/(double)RevCount);
    exit(1);
}
char * FindMacAddr(char * IpAddr)
{
    for(int i=0;i<arp_item_index;i++)
    {
        if(strcmp(arp_table[i].ip_addr,IpAddr)==0)
        {
            return arp_table[i].mac_addr;
        }
    }
    printf("no such a ip in arp!\n");
    exit(1);
    return NULL;
}

uint16_t cs(uint8_t *addr,int len)          //checksum function
{
    int nleft=len;
    int sum=0;
    uint8_t *w=addr;
	uint8_t *p=addr;
	uint16_t temp;
    uint16_t answer=0;
    while(nleft>1)
    {  
        temp=*p++;
        temp=(temp<<8)+*p++;  
        sum+=temp;
        nleft-=2;
    }
    if( nleft==1)
    {       
        *(unsigned char *)(&answer)=*(unsigned char *)w;
        sum+=answer;
    }
    sum=(sum>>16)+(sum&0xffff); 
    sum+=(sum>>16);
    answer=~sum;
    return answer;
}

void fill_icmp(myhdr *hdr)           //fill the icmp ,make the type be 'quest', id be the pid, data be the 'fucingping' , and finally compute  checksum 
{
    hdr->type=8;
    hdr->code=0;
    hdr->un.echo.id = pid & 0xffff;
    hdr->checksum=0;
    hdr->un.echo.sequence=seq++;
    memcpy(hdr->data,fuckping,24);
    hdr->checksum=0;
    gettimeofday((struct timeval *)hdr->time,NULL);
    hdr->checksum=cs( (uint8_t*)hdr,sizeof(struct myhdr));
    uint8_t temp=((hdr->checksum)>>8);
    hdr->checksum=(hdr->checksum<<8)+temp;
}

int GetMyMac(char *mac ,int len_limit)
{
    struct ifreq ifreq;
    int sock;
    sock = socket(AF_INET ,SOCK_STREAM ,0);
    if(sock<0)
    {
        printf("create mac socket failed！\n");
        exit(1);
    }
    strcpy(ifreq.ifr_name ,"ens33");
    if(ioctl(sock,SIOCGIFHWADDR ,&ifreq) < 0)
    {
        close(sock);
        printf("no such device named ens33!\n");
        return -1;
    }
    close(sock);
    return snprintf(mac ,len_limit ,"%02X:%02X:%02X:%02X:%02X:%02X",
    (unsigned char)ifreq.ifr_hwaddr.sa_data[0] ,
    (unsigned char)ifreq.ifr_hwaddr.sa_data[1] ,
    (unsigned char)ifreq.ifr_hwaddr.sa_data[2] ,
    (unsigned char)ifreq.ifr_hwaddr.sa_data[3] ,
    (unsigned char)ifreq.ifr_hwaddr.sa_data[4] ,
    (unsigned char)ifreq.ifr_hwaddr.sa_data[5] );
}
void fill_ip(myiphdr * hdr)
{
    hdr->tos=0;
    hdr->tot_len=sizeof(iphdr)+sizeof(myhdr);
    hdr->tot_len>>=8;
    hdr->id=pid;
    hdr->frag_off=0;
    hdr->ttl=128;
    hdr->protocol=1;
    hdr->check=0;
    hdr->daddr=inet_addr(IpAddr);
    hdr->saddr=inet_addr(MyIp);
    hdr->check=cs( (uint8_t*)hdr,sizeof(myiphdr));
    uint8_t temp=((hdr->check)>>8);
    hdr->check=(hdr->check<<8)+temp;
}

void fill_mix(mixhdr *hdr)
{
    int index=0;
    char tempbuf[18];
    strcpy(tempbuf,MacAddr);
    char *p=strtok(tempbuf,":");
    while(p!=NULL)
    {
        const char * a=p;
        char *stop;
        hdr->Dst[index++]=(uint8_t)strtol(a,&stop,16);
        p=strtok(NULL,":");
    }
    index=0;
    strcpy(tempbuf,MyMac);
    p=strtok(tempbuf,":");
    while(p!=NULL)
    {
        const char * a=p;
        char *stop;
        hdr->Src[index++]=(uint8_t)strtol(a,&stop,16);
        p=strtok(NULL,":");
    }
    hdr->type=8;
    hdr->code=0;
}