#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>

int tmp;
int main()
{
struct sockaddr_in addr;
	int i=0,s,t;
char request[5000],response[1000000];
//unsigned char targetip[4] = { 142,250,178,4};
unsigned char targetip[4] = { 127,0,0,1};
s =  socket(AF_INET, SOCK_STREAM, 0);
if ( s == -1 ){
	tmp=errno;
	perror("Socket failed");
	printf("i=%d errno=%d\n",i,tmp);
	return 1;
	}
addr.sin_family = AF_INET;
addr.sin_port = htons(8883);
addr.sin_addr.s_addr = *(unsigned int*)targetip; // <indirizzo ip del server 216.58.213.100 >

if ( -1 == connect(s,(struct sockaddr *)&addr, sizeof(struct sockaddr_in))){
	perror("Connect failed"); 
	return 1;
	}
printf("%d\n",s);
sprintf(request,"GET /test.html HTTP/1.1 \r\n\r\n");
if ( -1 == write(s,request,strlen(request))){perror("write failed"); return 1;}
while((t=read(s,response,999999))>0){
for(i=0;i<t;i++) printf("%c",response[i]);
	}
	if ( t == -1) { perror("Read failed"); return 1;}
}
