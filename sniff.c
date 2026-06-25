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

unsigned char rxbuf[1500];

int main () 
{
struct sockaddr_ll sll;
int i,s,size;
int structlen;
s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));

if ( s == -1 ) {
    perror("Socket Failed");
    return -1;
}

structlen = sizeof(struct sockaddr_ll);
for(i=0;i<structlen; i++) 
    ((char *)&sll)[i]=0;
sll.sll_family=AF_PACKET;
sll.sll_ifindex = if_nametoindex("eth0");


size = recvfrom(s, rxbuf, 1500 , 0, (struct sockaddr *) &sll, &structlen);

if (size == -1 ){
    perror("Recvfrom failed");
    return -1;
}

for(i=0;i<size;i++)
    printf("%.2X(%.3d) ",rxbuf[i],rxbuf[i]);

}


