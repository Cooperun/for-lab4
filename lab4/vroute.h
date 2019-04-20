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
char local_ip[3][16] = {"192.168.2.1","192.168.3.1"};
int local_num = 2;
#define MAX_ROUTE_INFO 6
struct route_item
{ 
       	char destination[16]; 
       	char gateway[16];
      	char netmask[16]; 
       	char interface[16]; 
}route_info[MAX_ROUTE_INFO];//route table
int route_item_index=0;

#define MAX_ARP_SIZE 10
struct arp_table_item
{
      	char ip_addr[16]; 
       	char mac_addr[18];
}arp_table[MAX_ARP_SIZE]; // the sum of the items in the arp cache 
int arp_item_index =0;

#define MAX_DEVICE 3
struct device_item
{
      	char interface[14];
      	char mac_addr[18];
}device[MAX_DEVICE];
int device_index = 0;

//extern void ping_func(char *target);
//extern void route_func();
