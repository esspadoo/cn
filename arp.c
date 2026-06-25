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
unsigned short *p = (unsigned short) b; 
unsigned short tot=0;
unsigned short prev=0;
for ( i = 0; i< len/2; i++){
    tot += ntohs(p[i]);
    if(prev>tot) tot++;
    prev=tot;
    }
return (0xFFFF - tot);
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
unsigned char target_ip[4]={ 139,162,197,70 };
unsigned char target_mac[6];
s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

if ( s == -1 ) {
    perror("Socket Failed");
    return -1;
}

resolve_ip(target_ip,target_mac);
printf("Resolved MAC is...\n");
            for(i=0;i<6;i++)
                printf("%.2X(%.3d) ",target_mac[i],target_mac[i]);
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

