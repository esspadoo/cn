#include <sys/types.h>          /* See NOTES */
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* the L2 protocols */
#include <stdio.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


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

unsigned short get_src_port(){
    return (unsigned short)(rand() % (65000 - 15000 +1) + 15000);
}
unsigned int get_seq_n(){
    return (unsigned int)((rand() + 1));
}


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


struct tcp_segment{
unsigned short src_port;
unsigned short dst_port;
unsigned int seq_n;
unsigned int  ack_n;
unsigned char off_res;
unsigned char flags;
unsigned short win;
unsigned short checksum;
unsigned short urg;
unsigned int opt_pad;
unsigned char payload;

};

struct tcp_checksum{
unsigned char src_ip[4];
unsigned char dst_ip[4];
unsigned char zero;
unsigned char proto;
unsigned short tcp_len;
struct tcp_segment tcp;
};


void forge_tcp(struct tcp_segment * tcp, unsigned char * target, unsigned short src_port, unsigned int seq_n){
tcp->src_port = htons(src_port);
tcp->dst_port = (htons(80));
tcp->seq_n = htonl(seq_n);
tcp->ack_n = 0;
tcp->off_res = 5 << 4; //0x50
tcp->flags = 0x2;
tcp->win = 0xFFFF;
tcp->checksum = 0;
tcp->urg = 0;
tcp->opt_pad = 0;
tcp->payload = 0;

struct tcp_checksum csum;
memset(&csum, 0, sizeof(csum));
   for(int i=0; i<4;i++){
       csum.src_ip[i] = my_ip[i];
       csum.dst_ip[i] = target[i];
   }
   csum.zero = 0;
   csum.proto = 6;
   csum.tcp_len = htons(sizeof(struct tcp_segment));
   memcpy(&csum.tcp, tcp, sizeof(struct tcp_segment));


tcp->checksum = htons(checksum((unsigned char *)&csum, sizeof(csum)));
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
srand(time(NULL));
int i;
int j;
int size;
// local //  unsigned char target_ip[4]={ 139,162,197,70 };
unsigned char target_ip[4]={ 142,251,29,139};
unsigned char target_mac[6];
unsigned char buffer[1500];
unsigned char tmp_buf[100];

struct eth_frame * eth;
struct ip_datagram * ip;
struct tcp_segment * tcp;


int structlen;
s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
if ( s == -1 ) {
    perror("Socket Failed");
    return -1;
}


eth = (struct eth_frame *) buffer;
ip  = (struct ip_datagram *) eth->payload;
tcp = (struct tcp_segment *) ip->payload;


unsigned short sp = get_src_port();
unsigned int sn = get_seq_n();
printf("SOURCE PORT: %d\n", sp);
printf("SEQ NUMBER: %d\n", sn);


forge_tcp(tcp, target_ip, sp, sn);
forge_ip(ip, sizeof(struct tcp_segment), 6, target_ip);

//Resolve MAC here
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


printf("TCP SYN over IP over ETHERNET...\n");
for(i=0;i<14+24+24;i++){
    printf("%.2X(%.3d) ",buffer[i],buffer[i]);
}

printf("AAAAAAAAAAAAAAAA\n");
structlen = sizeof(struct sockaddr_ll);
for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");


size = sendto(s, buffer, 14 + 20 + sizeof(struct tcp_segment) , 0, (struct sockaddr *) &sll, structlen);

if (size == -1 ){
    perror("Sendto failed");
    return -1;
}

for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");

for(j=0;j<1000;j++){
    size = recvfrom(s, buffer, 1500 , 0, (struct sockaddr *) &sll, &structlen);
    if (size == -1 ){
         perror("Recvfrom failed");
         return -1;
    }
    
    if (eth->type == htons(0x0800)) {
           if(ip->proto == 1){
                /*
                if(icmp->type == 0 ){
                    printf("Echo reply received\n");
                        for(i=0;i<14+20+8+20;i++)
                            printf("%.2X(%.3d) ",buffer[i],buffer[i]);
                        printf("\n");
                        return 1;
                }*/
               printf("ICMP");
            }
           
           if(ip->proto == 6){
                if(ntohs(tcp->src_port) == 80 ){
                        printf("SRC PORT OK\n");
                        
                    if(ntohs(tcp->dst_port) == sp){
                            printf("DEST PORT OK\n");

                            if((tcp->flags & 0x12) == 0x12){
                                printf("SYN-ACK OK\n");
                                
                                if(ntohl(tcp->ack_n) == sn + 1){
                                    printf("ACK OK\n");
                                   
                                   for(i=0;i<14+20+sizeof(struct tcp_segment);i++){
                                        printf("%.2X(%.3d) ",buffer[i],buffer[i]);
                                   }
                                   printf("\n");
                                   return 0;
                                }
                            }
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
