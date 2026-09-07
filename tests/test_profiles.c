#include "../src/spnav.c"
#include <assert.h>
#include <sys/wait.h>
static void fullread(int fd, void *data, int n)
{
    char *p = data;
    int got;
    while (n) {
        got = read(fd, p, n);
        assert(got > 0);
        p += got;
        n -= got;
    }
}
int main(void)
{
    int fd[2], status;
    pid_t child;
    struct spnav_profile_set *s = calloc(1, sizeof *s);
    assert(s && socketpair(AF_UNIX, SOCK_STREAM, 0, fd) == 0);
    child = fork();
    assert(child >= 0);
    if (!child) {
        struct reqresp rr;
        int off = 0, n;
        close(fd[0]);
        s->version = 1;
        s->revision = 7;
        s->count = 1;
        strcpy(s->profiles[0].name, "Default");
        for (;;) {
            fullread(fd[1], &rr, sizeof rr);
            switch (rr.type & 0xffff) {
            case REQ_PROFILE_BEGIN: {
                struct reqresp event={0};event.type=UEV_RAWBUTTON;event.data[0]=5;event.data[1]=1;
                assert(write(fd[1],&event,sizeof event)==sizeof event);

                off = 0;
                rr.data[0] = sizeof *s;
                break;
            }
            case REQ_PROFILE_READ:
                off = rr.data[0];
                assert(off >= 0 && off < (int)sizeof *s);
                n = sizeof *s - off;
                if (n > 24)
                    n = 24;
                memcpy(rr.data, (char *)s + off, n);
                break;
            case REQ_PROFILE_WRITE:
                assert(rr.data[6] == off);
                n = sizeof *s - off;
                if (n > 24)
                    n = 24;
                memcpy((char *)s + off, rr.data, n);
                off += n;
                break;
            case REQ_PROFILE_APPLY:
                assert(off == sizeof *s && s->profiles[0].sensitivity == 2500);
                rr.data[6] = -2;
                assert(write(fd[1], &rr, sizeof rr) == sizeof rr);
                close(fd[1]);
                _exit(0);
            default:
                assert(0);
            }
            rr.data[6] = 0;
            assert(write(fd[1], &rr, sizeof rr) == sizeof rr);
        }
    }
    close(fd[1]);
    ev_queue=ev_queue_tail=calloc(1,sizeof *ev_queue);
    sock = fd[0];
    proto = 1;
    assert(spnav_profiles_read(s) == 0 && s->revision == 7 && s->count == 1 &&
           !strcmp(s->profiles[0].name, "Default"));
    spnav_event event;assert(spnav_poll_event(&event)==SPNAV_EVENT_RAWBUTTON && event.button.bnum==5);
    s->profiles[0].sensitivity = 2500;
    assert(spnav_profiles_apply(s) == -2);
    spnav_close();
    free(s);
    assert(waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0);
    puts("Library snapshot framing and stale-revision propagation passed");
    return 0;
}
