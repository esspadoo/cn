#include <poll.h>
#include<stdio.h>
#include<signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#define TIMEOUT 3 
#define N 4

struct bufelem {
int id; //packet itself
int seq; //protocol data
} pktbuf[N];
int mytime;
int winstart;
int winsize;
int seq=0;

int pktsend(int id){
while( winsize == N ) pause();
struct bufelem * pkt;
pkt = pktbuf + (( winstart + winsize) % N);
pkt -> id = id;
pkt -> seq = seq;
printf(" Sending pkt id = %d seq=%d\n",pkt->id,pkt->seq);
seq = ( seq + 1 ) % (N+1);
winsize ++;
return 1;
}


unsigned char databuf[100];
int bufstart=0;

void myio (int signal) {
struct pollfd p[1];
int t=1;
int i;
int recv_ack;
printf("I/O handler called\n");
p[0].fd = 0;//stdin
p[0].events = POLLIN;
p[0].revents= 0;
if (-1 == poll(p,1,0)){perror("Poll failed\n");return ;}
if(p[0].revents==POLLIN) 
	 if ( scanf ("%d", &recv_ack))
		for(i=0;i<winsize;i++){
			if (pktbuf[(winstart + i)%N].seq == recv_ack){ 
				printf("Dequeueing %d packets\n", i+1);
				winstart = ( winstart + i + 1)%N;	
				winsize -= (i+1);
				mytime=0; // Reset Timeout
				break;
			}
		}
}

int mytime=0; 
void mytimer (int signal){
int i;
printf("Timer handler called\n");
if(mytime++ < TIMEOUT) return; // Timeout not expired

mytime=0;

for(i = 0 ; i < winsize ; i++){
		printf(" Resending pkt id = %d seq = %d\n",
        pktbuf[(winstart+i)%N].id,pktbuf[(winstart+i)%N].seq);
	} 
}


struct sigaction sa_timer, sa_io;

int main()
{
sigset_t mask;
int s=0; //standard input
int flags=0;
int t;
struct itimerval itime;
sa_timer.sa_handler=mytimer;
sa_io.sa_handler=myio;
if (-1 == sigaction(SIGIO, &sa_io,NULL)) {perror("SIGIO sigaction failed"); return 1;}
if (-1 == sigaction(SIGALRM, &sa_timer,NULL)) {perror("SIGARM sigaction failed"); return 1;}
if(-1 == fcntl(s, F_SETOWN, getpid())){perror("fcntl F_SETOWN failed"); return 1;}

flags=fcntl(s,F_GETFL);
if(flags == -1) {perror("fcntl F_GETFL failed"); return -1;}
if ( -1 == fcntl(s,F_SETFL, flags|O_ASYNC|O_NONBLOCK)){perror("fcntl F_SETFL failed"); return -1;}
itime.it_interval.tv_sec=1;
itime.it_interval.tv_usec=0;
itime.it_value.tv_sec=1;
itime.it_value.tv_usec=0;
if(-1 == setitimer(ITIMER_REAL,&itime,NULL)) {perror("setitimer failed"); return -1;}
if(-1 == sigemptyset(&mask)){perror("sigemptyset failed");return 1;}
if(-1 == sigaddset(&mask,SIGIO)){perror("sigaddset SIGIO failed");return 1;}
if(-1 == sigaddset(&mask,SIGALRM)){perror("sigaddset SIGALRM failed");return 1;}
if(-1 == sigprocmask(SIG_UNBLOCK,&mask,NULL)){perror("setprocmask failed");return 1;}
int id=0;
while(1){
 	while(pktsend(id)) id++;	
	}
}






