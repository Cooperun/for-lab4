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

int transmit(int *sd,struct sockaddr_in *addr){
	char buf[1024];
	char *eth_head;
	char *ip_head;
	int proto;
	// char *tcp_head;
	// char *udp_head;
	// char *icmp_head;
	// char *arp_head;
	char *p;
	int temp = recvfrom(*sd,buf,sizeof(buf),0,NULL,NULL);
	if(temp != -1){
		eth_head = buf;
		p = eth_head;
		proto = (ip_head + 9)[0];
		switch(proto){
			case IPPROTO_ICMP:{
			printf("MAC address: %.2x:%02x:%02x:%02x:%02x:%02x==> %.2x:%02x:%02x:%02x:%02x:%02x\n",p[6],p[7],p[8],p[9],p[10],p[11],p[0],p[1],p[2],p[3],p[4],p[5]);
			ip_head = eth_head+14;
			p = ip_head+12;
			printf("IP:%d.%d.%d.%d==> %d.%d.%d.%d\n",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
			}break;
			default:break;
		}
	}
	else{
		printf("-1\n");
		sleep(1);
	}
}

int main(int argc,char* argv[]){
	struct sockaddr_in addr;
    init();
	int sd = socket(PF_PACKET,SOCK_RAW,htons(ETH_P_ALL));
	while(1)
		transmit(&sd,&addr);
    return 0;
}