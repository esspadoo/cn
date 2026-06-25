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
//max ip header 60 bytes, 20 bytes normal header
//options have 60-20-(1 padding) = 39 bytes
unsigned short options_len = 40; //with final padding

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
unsigned char options[39];
unsigned char padding;
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

void forge_ip(struct ip_datagram * ip, unsigned short payload_len, unsigned short options_len, char protocol, char * target_ip )
{
ip-> ver_ihl = 0x4F; //0x45 -> 5*4 = 20bytes -> 60/4 = 15 so F
ip-> tos = 0;
ip-> totlen = htons(20 + payload_len + options_len);
//ip-> totlen = htons(20 + payload_len + options_len);
ip-> id= htons(0x1234);
ip-> flags_offs = 0;
ip-> ttl = 128;
ip-> proto = protocol;
ip-> checksum = 0;
ip-> srcip = *(unsigned int*)my_ip;
ip-> dstip = *(unsigned int*)target_ip;
ip-> options[0] = 0x7;
ip-> options[1] = 0x27;
ip-> options[2] = 0x04;
for(int i=3; i<39; i++){
    ip->options[i] = 0;
}
ip-> padding = 0;
ip -> checksum = htons(checksum((unsigned char *)ip, 20+options_len));
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
int i;
int j;
int size;
// local //  unsigned char target_ip[4]={ 139,162,197,70 };
unsigned char target_ip[4]={ 147,162,2,100};
unsigned char target_mac[6];
unsigned char buffer[1500];
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

forge_icmp( icmp);
forge_ip(ip, 8 + 20, options_len, 1, target_ip);

if( ((*(unsigned int*) target_ip) & (*(unsigned int*) netmask)) ==
        ((*(unsigned int*) my_ip) & (*(unsigned int*) netmask)) )
            resolve_ip(target_ip,target_mac);
else 
            resolve_ip(gateway,target_mac);

printf("Resolved MAC is...\n");
for(i=0;i<6;i++)
    printf("%.2X(%.3d) ",target_mac[i],target_mac[i]);
printf("\n");    
forge_ethernet(eth,target_mac, 0x0800);

printf("Echo Request over IP over Ethernet...\n");
            for(i=0;i<14+20+8+options_len+20;i++){
                if(i>=16 && (i-16)%4 == 0)printf("\n");
                if(i == 14)printf("\n");
                printf("%.2X(%.3d) ",buffer[i],buffer[i]);
                
            }
printf("\n");
structlen = sizeof(struct sockaddr_ll);
for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");

size = sendto(s, buffer, 14 + 20 + 8 + options_len + 20  , 0, (struct sockaddr *) &sll, structlen);

if (size == -1 ){
    perror("Sendto failed");
    return -1;
}

for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");

printf("Trying to receive...\n");
for(j=0;j<1000;j++){
    size = recvfrom(s, buffer, 1500 , 0, (struct sockaddr *) &sll, &structlen);
    if (size == -1 ){
         perror("Recvfrom failed");
         return -1;
    }
    if (eth->type == htons(0x0800)) {
        printf("RCV\n");
           if(ip->proto == 1){
               printf("ICMP HERE");
                if(icmp->type == 0 ){
                    printf("Echo reply received\n");
                        for(i=0;i<14+20+8+options_len+20;i++)
                            printf("%.2X(%.3d) ",buffer[i],buffer[i]);
                        printf("\n");
                        return 1;
                }
                if(icmp->type == 0xC){
                    printf("ERROR ON SOURCE DATA OPTION\n");
                }
            }
    }
}
}



int resolve_ip (unsigned char * target_ip, unsigned char * target_mac)
{
unsigned char buf[1500];
int i,size,j;
int structlen;
struct eth_frame * eth;
struct arp_packet * arp;

eth = (struct eth_frame *) buf;
arp = (struct arp_packet *) eth->payload;

forge_ethernet(eth,broadcast,0x0806);
forge_arp_request(arp,target_ip); 

printf("ARP requested prepared... \n");
for(i=0;i<14+sizeof(struct arp_packet);i++)
    printf("%.2X(%.3d) ",buf[i],buf[i]);


structlen = sizeof(struct sockaddr_ll);

for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");

size = sendto(s, buf, 14 + sizeof(struct arp_packet) , 0, (struct sockaddr *) &sll, structlen);

if (size == -1 ){
    perror("Recvfrom failed");
    return -1;
}

printf("%d bytes sent to the NIC\n",size);

for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");

for(j=0;j<1000;j++){
    size = recvfrom(s, buf, 1500 , 0, (struct sockaddr *) &sll, &structlen);
    if (size == -1 ){
         perror("Recvfrom failed");
         return -1;
    }
    if ( eth->type == htons(0x0806) ){
        if(arp->op == htons(0x0002) ) {
            printf("ARP response received (size = %d)... \n",size);
            for(i=0;i<size;i++)
                printf("%.2X(%.3d) ",buf[i],buf[i]);
            printf("\n");
            for(i=0;i<6;i++) 
                   target_mac[i]=arp->src_mac[i];
            return 1; 
        }
    }    

}

return 0;
}
