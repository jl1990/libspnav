#include <assert.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>
static int interrupt_read;
static ssize_t test_read(int fd, void *buf, size_t size)
{ if(interrupt_read) {interrupt_read=0;errno=EINTR;return -1;} return read(fd,buf,size); }
#define read test_read
#include "../src/spnav.c"
#undef read
static long millis(void)
{ struct timeval tv;gettimeofday(&tv,0);return tv.tv_sec*1000L+tv.tv_usec/1000; }
int main(void)
{
 int fds[2], status; pid_t child; char guarded[4]={'L',0,0,'R'}; long start;
 alarm(3); /* a closed socket must not spin forever */
 assert(socketpair(AF_UNIX,SOCK_STREAM,0,fds)==0);
 sock=fds[0];proto=1;close(fds[1]);
 assert(spnav_cfg_get_lcd()==-1);close(sock);sock=-1;
 alarm(0);
 assert(socketpair(AF_UNIX,SOCK_STREAM,0,fds)==0);sock=fds[0];
 assert(write(fds[1],"OK",2)==2);interrupt_read=1;
 assert(wait_resp(guarded+1,2,100)==0);
 assert(!memcmp(guarded,"LOKR",4));close(sock);close(fds[1]);sock=-1;
 assert(socketpair(AF_UNIX,SOCK_STREAM,0,fds)==0);sock=fds[0];
 child=fork();assert(child>=0);
 if(!child) {close(fds[0]);assert(write(fds[1],"X",1)==1);usleep(500000);close(fds[1]);_exit(0);}
 close(fds[1]);start=millis();
 assert(wait_resp(guarded+1,2,50)==-1);
 assert(millis()-start<300);close(sock);sock=-1;
 assert(waitpid(child,&status,0)==child && WIFEXITED(status) && !WEXITSTATUS(status));
 puts("Request disconnect, interruption and deadline tests passed");return 0;
}
