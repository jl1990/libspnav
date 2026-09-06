#include <assert.h>
#include <sys/wait.h>
#include "../src/spnav.c"
static void exchange(int type, int value, int status, int operation, int expected)
{
	int fds[2], child_status;
	pid_t pid;
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
	pid = fork(); assert(pid >= 0);
	if(!pid) {
		struct reqresp rr;
		close(fds[0]);
		assert(read(fds[1], &rr, sizeof rr) == sizeof rr);
		assert((rr.type & 0xffff) == type);
		if(operation == 1 || operation == 3 || operation == 5 || operation == 7) assert(rr.data[0] == value);
		if(operation == 2 && !status) usleep(600000); /* legitimate hardware latency */
		rr.data[0] = value; rr.data[6] = status;
		assert(write(fds[1], &rr, sizeof rr) == sizeof rr);
		close(fds[1]); _exit(0);
	}
	close(fds[1]); sock = fds[0]; proto = 1;
	assert((operation == 1 ? spnav_cfg_set_lcd(value) : operation == 2 ? spnav_lcd_refresh() : operation == 3 ? spnav_cfg_set_lcd_idle(value) : operation == 4 ? spnav_cfg_get_lcd_idle() : operation == 5 ? spnav_cfg_set_lcd_brightness(value) : operation == 6 ? spnav_cfg_get_lcd_brightness() : operation == 7 ? spnav_cfg_set_led_idle(value) : operation == 8 ? spnav_cfg_get_led_idle() : spnav_cfg_get_lcd()) == expected);
	close(sock); sock = -1;
	assert(waitpid(pid, &child_status, 0) == pid && WIFEXITED(child_status) && !WEXITSTATUS(child_status));
}
static void focus_exchange(const char *id)
{
	int fds[2], status, len = strlen(id);
	pid_t pid;
	assert(socketpair(AF_UNIX, SOCK_STREAM, 0, fds) == 0);
	pid = fork(); assert(pid >= 0);
	if(!pid) {
		int offset = 0;
		close(fds[0]);
		do {
			struct reqresp rr;
			int count = len - offset;
			assert(read(fds[1], &rr, sizeof rr) == sizeof rr);
			assert((rr.type & 0xffff) == REQ_SET_FOCUS);
			assert(rr.data[6] == ((len - offset) | (offset ? REQSTR_CONT_BIT : 0)));
			if(count > 24) count = 24;
			assert(!memcmp(rr.data, id + offset, count));
			rr.data[6] = 0;
			assert(write(fds[1], &rr, sizeof rr) == sizeof rr);
			offset += count;
		} while(offset < len);
		close(fds[1]); _exit(0);
	}
	close(fds[1]); sock = fds[0]; proto = 1;
	assert(spnav_set_focus(id) == 0);
	close(sock); sock = -1;
	assert(waitpid(pid, &status, 0) == pid && WIFEXITED(status) && !WEXITSTATUS(status));
}
int main(void)
{
	assert(spnav_cfg_set_lcd(-1) == -1);
	assert(spnav_cfg_set_lcd(4) == -1);
	exchange(REQ_SCFG_LCD, SPNAV_LCD_ENABLED | SPNAV_LCD_PROFILE, 0, 1, 0);
	exchange(REQ_GCFG_LCD, SPNAV_LCD_PROFILE, 0, 0, SPNAV_LCD_PROFILE);
	exchange(REQ_GCFG_LCD, 0, -1, 0, -1); /* old or unsupported daemon */
	exchange(REQ_LCD_REFRESH, 0, -1, 2, -1);
	exchange(REQ_LCD_REFRESH, 0, 0, 2, 0);
	assert(spnav_cfg_set_lcd_idle(-1) == -1);
	assert(spnav_cfg_set_lcd_idle(86401) == -1);
	exchange(REQ_SCFG_LCD_IDLE, 120, 0, 3, 0);
	exchange(REQ_GCFG_LCD_IDLE, 120, 0, 4, 120);
	exchange(REQ_GCFG_LCD_IDLE, 0, -1, 4, -1);
	exchange(REQ_SCFG_LCD_BRIGHTNESS, 65, 0, 5, 0);
	exchange(REQ_GCFG_LCD_BRIGHTNESS, 65, 0, 6, 65);
	assert(spnav_cfg_set_lcd_brightness(101) == -1);
	exchange(REQ_SCFG_LED_IDLE, 120, 0, 7, 0);
	exchange(REQ_GCFG_LED_IDLE, 120, 0, 8, 120);
	exchange(REQ_GCFG_LED_IDLE, 0, -1, 8, -1);
	assert(spnav_cfg_set_led_idle(-1) == -1 && spnav_cfg_set_led_idle(86401) == -1);
	assert(spnav_set_focus(NULL) == -1 && spnav_set_focus("bad\napp") == -1);
	focus_exchange(""); focus_exchange("blender");
	focus_exchange("org.blender.Blender.desktop");
	puts("Library LCD, LED and focus protocol tests passed");
	return 0;
}
