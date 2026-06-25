#include <sys/types.h>          /* See NOTES */
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdio.h>
#include <net/if.h>
#include <stdbool.h>
#include<string.h>
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

void forge_ip(struct ip_datagram * ip, unsigned short payload_len, char protocol, char * target_ip, unsigned char ttl)
{
ip-> ver_ihl = 0x45;
ip-> tos = 0;
ip-> totlen = htons(20 + payload_len);
ip-> id= htons(0x1234);
ip-> flags_offs = 0;
ip-> ttl = ttl;
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
int i;
int j;
int size;
// local //  unsigned char target_ip[4]={ 139,162,197,70 };
unsigned char target_ip[4]={ 194,244,10,164 };
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


 if( ((*(unsigned int*) target_ip) & (*(unsigned int*) netmask)) ==
        ((*(unsigned int*) my_ip) & (*(unsigned int*) netmask)) )
            resolve_ip(target_ip,target_mac);
 else 
            resolve_ip(gateway,target_mac);
 for(int t=1; t<40;t++){
  forge_icmp(icmp);
  forge_ip(ip, 8 + 20, 1, target_ip, t);


  //forge_ip(ip, 8+20, 1, target_ip, t);

  forge_ethernet(eth,target_mac, 0x0800);

  structlen = sizeof(struct sockaddr_ll);
  for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
  sll.sll_family=AF_PACKET;
  sll.sll_ifindex = if_nametoindex("eth0");

 unsigned char tmp_buf[20 + 8];
  for (int i = 0; i < 20 + 8; i++) {
    tmp_buf[i] = buffer[14 + i];
//    printf("[DEBUG] Read %.2X(%.3d) - Copy %.2X(%.3d)", buffer[14+i], buffer[14+i], tmp_buf[i], tmp_buf[i]);
  }


  size = sendto(s, buffer, 14 + 20 + 8 + 20  , 0, (struct sockaddr *) &sll, structlen);

  if (size == -1 ){
      perror("Sendto failed");
     return -1;
  }

  for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
  sll.sll_family=AF_PACKET;
  sll.sll_ifindex = if_nametoindex("eth0");

  for(j=0;j<50;j++){
    bool skip = false;
    size = recvfrom(s, buffer, 1500 , 0, (struct sockaddr *) &sll, &structlen);
    if (size == -1 ){
         perror("Recvfrom failed");
         return -1;
    }
    if (eth->type == htons(0x0800)) {
           if(ip->proto == 1){
                if(icmp->type == 0 ){
                  printf("Received an ICMP response, we check!");
                  if(icmp->id == htons(0xABCD)) {
                     printf("Echo reply received! TTL: %d\n", t);
                      return 1;
                  }
                }else if(icmp->type == 11){
                  unsigned char * cp = &buffer[14+20+8];
                  if(memcmp(cp,tmp_buf,28) == 0){
                    printf("Time exceeded in transit! TTL: %d\n",t);
                    break;
                  }
                }else{
                  printf("IPv4 ICMP unknown error! TTL: %d\n", t);
                } 
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
    //printf("%.2X(%.3d) ",buf[i],buf[i]);


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
            //printf("ARP response received (size = %d)... \n",size);
           // for(i=0;i<size;i++)
                //printf("%.2X(%.3d) ",buf[i],buf[i]);
//            printf("\n");
            for(i=0;i<6;i++) 
              target_mac[i]=arp->src_mac[i];
            return 1; 
        }
    }    

}

return 0;
}
