#include <sys/types.h>
#include <sys/socket.h>
#include <linux/if_packet.h>
#include <net/ethernet.h> /* The L2 Protocol */
#include <errno.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <net/if.h>




/*
struct sockaddr_ll {
        -----> unsigned short sll_family;   
               unsigned short sll_protocol; 
        ---->  int            sll_ifindex;  
               unsigned short sll_hatype;   
               unsigned char  sll_pkttype;  
               unsigned char  sll_halen;    
               unsigned char  sll_addr[8];  
           };
           Arrow field for our purpose, other field for dgram packet (change dgran and sockraw in the socket creation)
*/

int main()
{
  unsigned char rxbuf[1500];
  struct sockaddr_ll sll;
  int i, s, pck_size, structlen;
  
  s = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL)); 
  if ( s == -1){perror("Socket Failed");return -1;}


  structlen = sizeof(struct sockaddr_ll);
  for (i=0; i<structlen; i++)
    ((char*) &sll)[i]=0; //cleaning the structure (all bytes) that can be dirty 
    // instead of char we can increase the sizelike (int 32bit) and  improve the performance
  
  
  sll.sll_family = AF_PACKET; // 
  // use if_nametoindex man 3 to map name to interface ixd
  sll.sll_ifindex = if_nametoindex("eth0");
  
  //with one call now we receive only 1 packet from the net
  pck_size = recvfrom(s, rxbuf, 1500, 0, (struct sockaddr *) &sll, &structlen);
  if (pck_size == -1){perror("Recvfrom failed"); return -1;}

  for(i=0;i<pck_size;i++){
    printf("%.2X(%.3d) ", rxbuf[i], rxbuf[i]);
  }


}
