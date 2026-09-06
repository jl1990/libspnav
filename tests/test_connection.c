/* Exercise actual connection cleanup without a running daemon or hardware. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <errno.h>
static int peer = -1, config_fd = -1, fail_socket, fail_connect = 1;
static int test_socket(int domain, int type, int protocol)
{
 int pair[2]; (void)domain; (void)type; (void)protocol;
 if(fail_socket) { errno = EMFILE; return -1; }
 assert(socketpair(AF_UNIX, SOCK_STREAM, 0, pair) == 0);
 peer = pair[1];
 if(!fail_connect) { int reply=0x7faa5501; assert(write(peer,&reply,sizeof reply)==sizeof reply); }
 return pair[0];
}
static int test_connect(int fd, const struct sockaddr *addr, socklen_t len)
{ (void)fd; (void)addr; (void)len; if(fail_connect) {errno=ECONNREFUSED;return -1;} return 0; }
static FILE *test_fopen(const char *name, const char *mode)
{
 FILE *fp; (void)mode; assert(!strcmp(name,"/etc/spnavrc"));
 fp=tmpfile(); assert(fp); config_fd=fileno(fp);
 fputs("# connection regression\nsocket = /tmp/spnav-test.sock\n",fp); rewind(fp); return fp;
}
#define socket test_socket
#define connect test_connect
#define fopen test_fopen
#include "../src/spnav.c"
#undef socket
#undef connect
#undef fopen
static void config_closed(void)
{ errno=0;assert(fcntl(config_fd,F_GETFD)==-1 && errno==EBADF); }
int main(void)
{
 int i, saved_stdin;
 unsetenv("SPNAV_SOCKET");
 for(i=0;i<20;i++) {
  assert(spnav_open()==-1); config_closed();
  assert(ev_queue==NULL && sock==-1); close(peer);peer=-1;
 }
 fail_socket=1; assert(spnav_open()==-1 && ev_queue==NULL);fail_socket=0;
 /* A valid socket may occupy fd 0. Closing must still release it. */
 saved_stdin=dup(0);close(0);fail_connect=0;
 /* test_socket pre-fills a protocol reply for the successful handshake. */
 assert(spnav_open()==0 && sock==0);config_closed();
 assert(spnav_close()==0 && sock==-1 && ev_queue==NULL && ev_queue_tail==NULL);
 assert(fcntl(0,F_GETFD)==-1);close(peer);
 if(saved_stdin>=0) {assert(dup2(saved_stdin,0)==0);close(saved_stdin);}
 puts("Connection cleanup tests passed");return 0;
}
