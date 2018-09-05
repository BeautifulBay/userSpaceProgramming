#include <stdio.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <semaphore.h>
#include <string.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <sys/epoll.h>
#include <sys/types.h>

/* 8x16 dots for ascii */
#include "font_8x16.h"

/* key-value */
#define USE_ARRAY
#ifdef USE_ARRAY
#include "key_value_to_ascii2.h"
#else
#include "key_value_to_ascii.h"
#endif

/* for ioctl to get input capabilities */
#define BITS_PER_LONG (sizeof(unsigned long) * 8)
#define BITS_TO_LONGS(x) (((x) + BITS_PER_LONG - 1) / BITS_PER_LONG)
#define test_bit(bit, array) \
    ((array)[(bit)/BITS_PER_LONG] & (1 << ((bit) % BITS_PER_LONG)))

/* subject */
struct framebuffer_data {
#define FRAMEBUFF_FD   "/dev/fb0"
#define INPUT_DIR      "/dev/input"
#define EPOLL_SIZE_MAX 16
#define READ_DATA      1024
	int fb_fd;
	int epoll_fd;
	struct fb_var_screeninfo var;
	struct fb_fix_screeninfo fix;
	unsigned int line_width;
	unsigned int screen_size;
	unsigned int *framebuffer;
	unsigned int cur_x;
	unsigned int cur_y;
	unsigned int X_0;
	unsigned int Y_0;
	unsigned int X_M;
	unsigned int Y_M;
	sem_t display_sem;
	pthread_mutex_t buffer_mutex;
	struct input_event ev[4];
	char read_data[READ_DATA];
	int key_down_index;
	int pthread_exit;
	int shift_down;
	int cursor_sync;
	int (*init) (struct framebuffer_data*);
	int (*run) (struct framebuffer_data*);
	int (*clear) (struct framebuffer_data*);
	int (*exit) (struct framebuffer_data*);
};

/* init subject */
static int init_framebuffer(struct framebuffer_data *test)
{
	test->fb_fd = open(FRAMEBUFF_FD, O_RDWR);
	if (test->fb_fd < 0) {
		printf("open fb0 error!\n");
		return test->fb_fd;
	}

	if (ioctl(test->fb_fd, FBIOGET_VSCREENINFO, &test->var) < 0) {
		printf("ioctl get VSCREENINFO error!\n");
		goto err;
	}
	
	test->line_width  = test->var.xres * test->var.bits_per_pixel / 8;
	test->screen_size = test->var.yres * test->line_width;
	test->X_0 = test->var.xres / 2;
	test->Y_0 = test->var.yres / 2;
	test->X_M = test->var.xres;
	test->Y_M = test->var.yres;
	test->cur_x = test->X_0;
	test->cur_y = test->Y_0;
	test->shift_down     = 0;
	test->key_down_index = 0;
	test->pthread_exit   = 0;
	test->cursor_sync    = 0;

	test->framebuffer = (unsigned int *)mmap(NULL, test->screen_size, PROT_READ | PROT_WRITE, MAP_SHARED, test->fb_fd, 0);
	if (test->framebuffer == (unsigned int*) -1) {
		printf("mmap error!\n");
		goto err;
	}

	sem_init(&test->display_sem, 0, 0);
	pthread_mutex_init(&test->buffer_mutex, NULL);

	return 0;
err:
	close(test->fb_fd);
	return -1;
}

/* key-value */
static int decode_key_value_to_ascii(struct framebuffer_data *test, int code) 
{
	int i = 0;
#ifdef USE_ARRAY
	return test->shift_down ?  key_ascii2[code] : key_ascii[code];
#else
	int count = sizeof(key_ascii) / sizeof(key_ascii[0]);
	for (i = 0; i < count; i++) {
		if (key_ascii[i].key == code )
			return key_ascii[i].ascii;
	}
	return 0;
#endif
}

/* draw dot */
static void draw_pixel(struct framebuffer_data *test, unsigned int x, unsigned int y, unsigned int color)
{
	unsigned char *color8 = (unsigned char *)test->framebuffer + y * test->line_width + x * test->var.bits_per_pixel / 8;
	unsigned short *color16 = (unsigned short *)color8;
	unsigned int *color32 = (unsigned int *)color8;
	unsigned int red, blue, green;

	switch(test->var.bits_per_pixel)
	{
		case 8:
			*color8 = color;
			break;
		case 16:
			red = (color >> 16) & 0xFF;
			green = (color >> 8) & 0xFF;
			blue = (color >> 0) & 0xFF;
			*color16 = (red >> 3) << 11 | (blue >> 2) << 5 | (green >> 3) << 0;
			break;
		case 32:
			*color32 = color;
			break;
		default:
			printf("%d is not supported!\n", test->var.bits_per_pixel);
			break;
	}
}

/* bound X and Y */
static void bound_X_Y(struct framebuffer_data *test)
{
	if (test->cur_x >= test->X_M) {
		test->cur_x = test->X_0;
		test->cur_y += 16;
	} else if (test->cur_x < test->X_0) {
		test->cur_x = test->X_M - 8;
		test->cur_y -= 16;
	}
	if (test->cur_y >= test->Y_M || test->cur_y < test->Y_0) {
		printf("Out of range, so reset (0, 0)!\n");
		test->cur_x = test->X_0;
		test->cur_y = test->Y_0;
	}
}

/* draw char */
static void draw_char(struct framebuffer_data *test, char ascii)
{
	unsigned char *dots = (unsigned char *)&fontdata_8x16[ascii*16];
	unsigned char dot;
	int i, j;
	for (i = 0; i < 16; i++) {
		dot = dots[i];
		for (j = 0; j < 8; j++)
		if (dot & (1 << (7 - j)))
			draw_pixel(test, test->cur_x + j, test->cur_y + i, 0xff00ff);
		else
			draw_pixel(test, test->cur_x + j, test->cur_y + i, 0x0);
	}
	if (ascii != 0) {
		test->cur_x += 8;
		bound_X_Y(test);
	}
}

/* display char */
static void display_char(struct framebuffer_data *test)
{
	int ascii = -1;
	if (test->ev[test->key_down_index].type == EV_KEY) {
		if (test->ev[test->key_down_index].code == KEY_BACKSPACE)
		{
			test->cur_x -= 8;
			bound_X_Y(test);
			draw_char(test, 0);
		} else {
			ascii = decode_key_value_to_ascii(test, test->ev[test->key_down_index].code);
			if (ascii == 0) {
				printf("Cannot decode ev.code = %d\n", test->ev[test->key_down_index].code);
			} else {
#ifdef USE_ARRAY
				//printf("code = %d, shift_down = %d, ascii = %d\n", test->ev[test->key_down_index].code, test->shift_down, ascii);
				draw_char(test, ascii);
#else
				if (ascii >= 'A' && ascii <= 'Z') {
					if (test->shift_down == 1)
						draw_char(test, ascii);
					else
						draw_char(test, ascii + 32);
				} else {
					draw_char(test, ascii);
				}
#endif
			}
		}
	}
}

/* get dot color */
static void get_pixel(struct framebuffer_data *test, unsigned int x, unsigned int y, unsigned int *color)
{
	unsigned char *color8 = (unsigned char *)test->framebuffer + y * test->line_width + x * test->var.bits_per_pixel / 8;
	unsigned short *color16 = (unsigned short *)color8;
	unsigned int *color32 = (unsigned int *)color8;
	unsigned int red, blue, green;

	switch(test->var.bits_per_pixel)
	{
		case 8:
			*color = *color8;
			break;
		case 16:
			red   = ((*color16 >> 11) << 3) & 0xFF;
			blue  = ((*color16 << 5)  << 2) & 0xFF;
			green = ((*color16 >> 0)  << 3) & 0xFF;
			*color = (red << 16)  | (green << 8) | (blue << 0);
			break;
		case 32:
			*color = *color32;
			break;
		default:
			printf("%d is not supported!\n", test->var.bits_per_pixel);
			break;
	}
}

/* get cursor data */
static void get_cursor_data(struct framebuffer_data *test, unsigned int data1[16][8], unsigned int data2[16][8])
{
	unsigned int color = 0;
	int i, j;
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 8; j++) {
			get_pixel(test, test->cur_x + j, test->cur_y + i, &color);
			data1[i][j] = color;
			data2[i][j] = ~color;
		}
	}
}

/* draw cursor */
static void draw_cursor(struct framebuffer_data *test, int last_x, int last_y, unsigned int color[16][8])
{
	int i, j;
	for (i = 0; i < 16; i++) {
		for (j = 0; j < 8; j++) {
			draw_pixel(test, last_x + j, last_y + i, color[i][j]);
		}
	}
}

/* draw cursor thread */
static void *cursor_display_thread(void *msg)
{
	struct framebuffer_data *test = (struct framebuffer_data *)msg;
	int last_x = 0;
	int last_y = 0;
	unsigned int cursor_data[16][8] = { 0 };
	unsigned int cursor_data_reverse[16][8] = { 0 };
	while (1) {
		if (test->pthread_exit == 1)
			break;
		pthread_mutex_lock(&test->buffer_mutex);
		if (last_x != test->cur_x || last_y != test->cur_y) {
			last_x = test->cur_x;
			last_y = test->cur_y;
			get_cursor_data(test, cursor_data, cursor_data_reverse);
		}
		if (test->cursor_sync == 1) {
			test->cursor_sync = 0;
			draw_cursor(test, last_x, last_y, cursor_data);
		} else {
			test->cursor_sync = 1;
			draw_cursor(test, last_x, last_y, cursor_data_reverse);
		}
		pthread_mutex_unlock(&test->buffer_mutex);
		if (test->cursor_sync == 1) {
			usleep(100 * 1000);
		} else {
			usleep(500 * 1000);
		}
	}
}

/* display thread */
static void *display_thread(void *msg)
{
	struct framebuffer_data *test = (struct framebuffer_data *)msg;
	pthread_t cursor_id;

	if(pthread_create(&cursor_id, NULL, cursor_display_thread, (void *)test) != 0) {
		printf("pthread create cursor_display_thread error!\n");
	} else {
		while (1) {
			sem_wait(&test->display_sem);
			if (test->pthread_exit == 1)
				break;
			while(test->cursor_sync);
			pthread_mutex_lock(&test->buffer_mutex);
			display_char(test);
			pthread_mutex_unlock(&test->buffer_mutex);
		}
		pthread_join(cursor_id, NULL);
	}
}

/* init epoll */
static init_input(struct framebuffer_data *test)
{
	DIR *input_dir;
	struct dirent *event_n;
	int event_fd;
	char name[64] = { 0 };

	test->epoll_fd = epoll_create(EPOLL_SIZE_MAX);
	if (test->epoll_fd == -1) {
		printf("epoll create error!\n");
		return;
	}
	input_dir = opendir(INPUT_DIR);
	if (input_dir != 0) {
		while ((event_n = readdir(input_dir))) {
			unsigned long ev_bits[BITS_TO_LONGS(EV_MAX)];
			if (strncmp(event_n->d_name, "event", 5)) continue;
			event_fd = openat(dirfd(input_dir), event_n->d_name, O_RDONLY);
			if (event_fd < 0) continue;
			if (ioctl(event_fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
				close(event_fd);
				continue;
			}
			if (ioctl(event_fd, EVIOCGNAME(sizeof(name)), name) < 0) {
				strcpy(name, "unknown");
			}
			if (test_bit(EV_REL, ev_bits)) {
				close(event_fd);
				continue;
			}
			if (test_bit(EV_KEY, ev_bits)) {
				struct epoll_event ev = {
					.events = EPOLLIN | EPOLLET,
					.data.fd = event_fd,
				};
				epoll_ctl(test->epoll_fd, EPOLL_CTL_ADD, event_fd, &ev);
				//printf("event_n->d_name = %s, name = %s, event_fd = %d\n", event_n->d_name, name, event_fd);
			}
		}
	}
}

/* input thread */
static void *input_thread(void *msg)
{
	int i = 0;
	struct epoll_event polledevents[EPOLL_SIZE_MAX];
	int npolledevents;
	struct framebuffer_data *test = (struct framebuffer_data *)msg;

	init_input(test);	
	while (1) {
		if (test->pthread_exit == 1) {
			break;
		}
		npolledevents = epoll_wait(test->epoll_fd, polledevents, EPOLL_SIZE_MAX, -1);
		if (npolledevents <= 0) continue;
		for (i = 0; i < npolledevents; i++) {
			if (polledevents[i].events & EPOLLIN) {
				int nread = 0;
				nread = read(polledevents[i].data.fd, test->read_data, READ_DATA);
				if (nread <= 0)
					printf("read %d error!\n", polledevents[i].data.fd);
				else {
					int j = 0;
					while(test->cursor_sync);
					pthread_mutex_lock(&test->buffer_mutex);
					while(nread / sizeof(test->ev[0])) {
						memcpy(&test->ev[j], (struct input_event *)&test->read_data[j*sizeof(test->ev[0])], sizeof(test->ev[0]));
						nread -= sizeof(test->ev[0]);
						test->ev[j].value = !!test->ev[j].value;
						if (test->ev[j].type == EV_KEY && test->ev[j].value == 1) {
							//printf("\nev.type = %d, ev.code = %d, ev.value = %d, j = %d\n", test->ev[j].type, test->ev[j].code, test->ev[j].value, j);
							switch (test->ev[j].code) {
							case KEY_TAB:
								test->cur_x += 4*8;
								bound_X_Y(test);
								break;
							case KEY_F5:
								test->clear(test);
								break;
							case KEY_LEFTSHIFT:
							case KEY_RIGHTSHIFT:
								test->shift_down = 1;
								break;
							case KEY_ENTER:
								test->cur_x = test->X_0;
								test->cur_y += 16;
								bound_X_Y(test);
								break;
							case KEY_SPACE:
								test->cur_x += 8;
								bound_X_Y(test);
								break;
							case KEY_ESC:
								test->pthread_exit = 1;
							default:
								test->key_down_index = j;
								sem_post(&test->display_sem);
							}
						} else if (test->ev[j].type == EV_KEY && test->ev[j].value == 0) {
							if (test->ev[j].code == KEY_LEFTSHIFT || test->ev[j].code == KEY_RIGHTSHIFT)
								test->shift_down = 0;
						}
						if (++j >= 4) {
							//printf("index is out of range! j = %d\n", j);
							break;
						}
					}
					pthread_mutex_unlock(&test->buffer_mutex);
				}
			}
		}
	}
}

/* run framebuffer */
static int run_framebuffer(struct framebuffer_data *test)
{
	int ret = -1;
	pthread_t display_id;
	pthread_t input_id;

	ret = pthread_create(&display_id, NULL, display_thread, (void *)test);
	if (ret != 0) {
		printf("pthread_create display_thread error!\n");
	} else {
		ret = pthread_create(&input_id, NULL, input_thread, (void *)test);
		if (ret != 0) {
			printf("pthread_create input_thread error!\n");
		} else {
			pthread_join(input_id, NULL);
		}
		pthread_join(display_id, NULL);
	}
}

/* clear framebuffer */
static int clear_framebuffer(struct framebuffer_data *test)
{
	test->cur_x = test->X_0;
	test->cur_y = test->Y_0;
	system("clear");
	memset(test->framebuffer, 0, test->screen_size);
}

/* exit */
static int exit_framebuffer(struct framebuffer_data *test)
{
	munmap(test->framebuffer, test->screen_size);
	sem_destroy(&test->display_sem);
	pthread_mutex_destroy(&test->buffer_mutex);
	close(test->fb_fd);
}

/* init framebuffer functions */
static int init_framebuffer_func(struct framebuffer_data *test)
{
	test->init  = init_framebuffer;
	test->run   = run_framebuffer;
	test->clear = clear_framebuffer;
	test->exit  = exit_framebuffer;
}

/* main */
int main(int argc, char **argv)
{
	struct framebuffer_data test;
	
	init_framebuffer_func(&test);

	if (test.init(&test) == -1) {
		printf("init error!\n");
	} else {
		test.clear(&test);
		test.run(&test);
		test.exit(&test);
	}
	return 0;
}
