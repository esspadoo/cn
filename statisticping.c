#include <sys/types.h>          /* See NOTES */
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdio.h>
#include <net/if.h>


/*
       struct sockaddr_ll {
  -------->    unsigned short sll_family;   
  NO           unsigned short sll_protocol; 
  -------->    int            sll_ifindex;  
  NO           unsigned short sll_hatype;   
  NO           unsigned char  sll_pkttype;  
  NO           unsigned char  sll_halen;    
  NO           unsigned char  sll_addr[8];  
           };

*/

unsigned char my_mac[6] = {0x22,0x00,0xb6,0xec,0x6a,0x3c};
unsigned char broadcast[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char my_ip[4]={ 139,162,197,68 };
unsigned char netmask[4] = { 255,255,255,0 };
unsigned char gateway[4] = { 139,162,197,1};

#define IP 0x0800
#define ARP 0x0806
#define ICMP 1
#define TCP 6
#define UDP 17

struct ip_datagram{
unsigned char ver_ihl;
unsigned char tos;
unsigned short totlen;
unsigned short id;
unsigned short flags_offs;
unsigned char ttl;
unsigned char proto;
unsigned short checksum;
unsigned int srcip;
unsigned int dstip;
unsigned char payload[1];
};


struct eth_frame {
unsigned char dst[6];
unsigned char src[6];
unsigned short  type; 
unsigned char payload[1];
};

struct arp_packet {
unsigned short htype;
unsigned short ptype;
unsigned char hlen;
unsigned char plen;
unsigned short op;
unsigned char src_mac[6];
unsigned char srcp_ip[4];
unsigned char dst_mac[6];
unsigned char dest_ip[4];
};

unsigned short checksum(unsigned char * b, int len)
{
unsigned short *p = (unsigned short*) b; 
unsigned short tot=0;
unsigned short prev=0;
int i;
for ( i = 0; i< len/2; i++){
    tot += ntohs(p[i]);
    if(prev>tot) tot++;
    prev=tot;
    }
return (0xFFFF - tot);
}

void forge_ip(struct ip_datagram * ip, unsigned short payload_len, char protocol, char * target_ip )
{
ip-> ver_ihl = 0x45;
ip-> tos = 0;
ip-> totlen = htons(20 + payload_len);
ip-> id= htons(0x1234);
ip-> flags_offs = 0;
ip-> ttl = 128;
ip-> proto = protocol;
ip-> checksum = 0;
ip-> srcip = *(unsigned int*)my_ip;
ip-> dstip = *(unsigned int*)target_ip;
ip -> checksum = htons(checksum((unsigned char *)ip, 20));
};

struct icmp_message{
unsigned char type;
unsigned char code;
unsigned short checksum;
unsigned short id;
unsigned short seq;
unsigned char payload[1];
};

void forge_icmp( struct icmp_message * icmp )
{
int i;
icmp->type = 8;
icmp->code = 0;
icmp->checksum=0;
    
icmp->id = htons(0xABCD);

icmp->seq = htons(1);
for(i=0;i<20;i++)
    icmp->payload[i] = i;
icmp->checksum=htons(checksum((unsigned char*)icmp, 28));
}


void forge_ethernet( struct eth_frame * eth, unsigned char * dest_addr, unsigned short type)
{
int i;
for(i=0;i<6;i++){
    eth->src[i] = my_mac[i];
    eth->dst[i] = dest_addr[i];
    }
eth->type = htons(type);
}

void forge_arp_request( struct arp_packet * arp, unsigned char * dest_ip )
{
int i;
arp-> htype = htons(1);
arp-> ptype = htons(0x0800);
arp-> hlen = 6;
arp-> plen = 4;
arp-> op = htons(1);
for(i=0;i<6;i++)
    arp-> src_mac[i]=my_mac[i];

for(i=0;i<4;i++)
    arp-> srcp_ip[i] = my_ip[i];

for(i=0;i<6;i++)
    arp-> dst_mac[i]=0;

for(i=0;i<4;i++)
    arp-> dest_ip[i]=dest_ip[i];
}

struct sockaddr_ll sll;
int s;

int resolve_ip (unsigned char * target_ip, unsigned char * target_mac);

int main () 
{

unsigned int c_ip = 0;
unsigned int c_arp = 0;
unsigned int c_not = 0;
unsigned int c_tcp = 0;
unsigned int c_udp = 0;
unsigned int c_icmp = 0;
unsigned int c_other = 0;
unsigned int c_total = 0;



unsigned char target_ip[4]={ 194,244,10,164 };
unsigned char target_mac[6];
unsigned char buffer[1500];
int i;
int j;
int size;
// local //  unsigned char target_ip[4]={ 139,162,197,70 };
struct eth_frame * eth;
struct ip_datagram * ip;
struct icmp_message * icmp;
int structlen;
s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
if ( s == -1 ) {
    perror("Socket Failed");
    return -1;
}


eth = (struct eth_frame *) buffer;
ip  = (struct ip_datagram *) eth->payload;
icmp = (struct icmp_message *) ip->payload;


structlen = sizeof(struct sockaddr_ll);

for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");

for(j=0;j<5000;j++){
    size = recvfrom(s, buffer, 1500 , 0, (struct sockaddr *) &sll, &structlen);
    if (size == -1 ){
         perror("Recvfrom failed");
         return -1;
    }
    
   /* for(int i =0; i<size; i++){
        printf("%.2X(%.3d) ",buffer[i],buffer[i]);
    }
    printf("\n---------------\n");*/
    //printf("TYPE: %d\n",ntohs(eth->type) == IP ? 9 : 11);


    c_total++;
    if(c_total%100 == 0){printf("\n %d RECEIVED...\n", c_total);}
    if (ntohs(eth->type) == IP){
        c_ip++;
           if(ip->proto == ICMP){c_icmp++;}
           if(ip->proto == UDP){c_udp++;}
           if(ip->proto == TCP){c_tcp++;}
           if((ip->proto != ICMP) && (ip->proto != UDP) && (ip->proto != TCP)){c_other++;}
    }
    if (ntohs(eth->type) == 0x0806){
        c_arp++;
    }
    if((ntohs(eth->type) != IP) && (ntohs(eth->type) != ARP)){c_not++;}
}
printf("\n---- STATISTIC ----\n");
printf("IP: %.2f\n", 100.0 * c_ip / c_total );
printf("ARP: %.2f\n", 100.0 * c_arp / c_total);
printf("NOT IP||ARP: %.2f\n", 100.0 * c_not / c_total);

if(c_ip >0 ){
printf("TCP: %.2f\n", 100.0 * c_tcp / c_ip);
printf("UDP: %.2f\n", 100.0 * c_udp / c_ip);
printf("ICMP: %.2f\n", 100.0 * c_icmp / c_ip);
printf("OTHER PAYLOAD: %.2f\n", 100.0 * c_other / c_ip);
}
}



