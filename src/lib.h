#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

#include "config.c"

typedef struct node node;
typedef struct tag tag;

struct node {
    xcb_window_t window;
    struct node *next;
    struct node *prev;
};

struct tag {
    node *head;
    node *current;
};

#define XCBFLUSH xcb_flush(conn) 


xcb_connection_t *conn;
xcb_screen_t     *screen;
xcb_window_t 	 root_window;
char             *flow_pipe;
int16_t 		 pipe_num;
node *head, *current;
tag *current_tag, *tagset[TAGS];
short screen_w, screen_h, master_size;
int default_screen;

bool active;

#include "window.c"

