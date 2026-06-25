//prova edit

#include <poll.h>
#include<stdio.h>
#include<signal.h>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>



unsigned char databuf[100];
int bufstart=0;

void myio (int signal) {
struct pollfd p[1];
int t=1;
printf("I/O handler called\n");
p[0].fd = 0;//stdin
p[0].events = POLLIN;
p[0].revents= 0;
if (-1 == poll(p,1,0)){perror("Poll failed\n");return ;}
if(p[0].revents==POLLIN) 
	for(bufstart;t;bufstart+=t){
		t=read(p[0].fd,databuf+bufstart,99-bufstart);
		if(t==-1)  { if (errno!=EAGAIN) perror("Read failed"); t=0;}
	}
}

void mytimer (int signal){
printf("Timer handler called\n");
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

while(1){
	if(bufstart) {
		databuf[bufstart]=0;
		printf("Received data: %s\n", databuf);
		bufstart=0;
		}
	}

}






