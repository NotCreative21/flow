#include "lib.h"
#include <stdlib.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>

void quit() {
	active = false;
	for (int8_t i = 0; i < TAGS; i++)
		free(tagset[i]);

    close(pipe_num); 
	unlink(flow_pipe);
    xcb_disconnect(conn);

    exit(0);
}

// Handle events
void map_request(xcb_map_request_event_t *ev) {
    create_window(ev->window);

    xcb_map_window(conn, ev->window);
}

void destroy_notify(xcb_destroy_notify_event_t *ev) {
    destroy_window(ev->window);
}


void setup(void) {
	screen = xcb_setup_roots_iterator(xcb_get_setup(conn)).data;

	root_window = screen->root;

	xcb_grab_server(conn);

	{
		xcb_void_cookie_t cookie;
		const uint32_t value = XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT;
		
		cookie = xcb_change_window_attributes_checked(conn, root_window, XCB_CW_EVENT_MASK, &value);

		if (xcb_request_check(conn, cookie)) {
			printf("another wm is already running!\n");
			xcb_disconnect(conn);
			quit();
		}
	}

	xcb_ungrab_server(conn);

	xcb_grab_button(conn, 0, root_window, 
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE , XCB_GRAB_MODE_ASYNC,
			XCB_GRAB_MODE_ASYNC, root_window, XCB_NONE, 1, XCB_MOD_MASK_1);
	xcb_grab_button(conn, 0, root_window, 
			XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE , XCB_GRAB_MODE_ASYNC,
			XCB_GRAB_MODE_ASYNC, root_window, XCB_NONE, 3, XCB_MOD_MASK_1);
	xcb_grab_button (conn, 0, root_window, XCB_EVENT_MASK_BUTTON_PRESS, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root_window,
                   XCB_NONE, 1, XCB_NONE);


	flow_pipe = PIPE_FILE;
	mkfifo(flow_pipe, O_RDONLY | O_NONBLOCK);

	screen_w = screen->width_in_pixels;
    screen_h = screen->height_in_pixels;

	head = NULL;
	current = NULL;

    for (int8_t i = 0; i < TAGS; ++i) {
        tagset[i] = (tag*)calloc(1,sizeof(tag));
        tagset[i]->head = NULL;
        tagset[i]->current = NULL;
    }
    current_tag = tagset[0];

	active = true;

    uint32_t masks[1] = {
        XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY | // destroy notify
        XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT |
		XCB_EVENT_MASK_KEY_RELEASE |
		XCB_EXPOSE
	}; // map request
    xcb_change_window_attributes(conn, screen->root, XCB_CW_EVENT_MASK, masks);

    xcb_flush(conn);
}

void loop(void) {
	uint32_t len;
	xcb_window_t focused_window, window;
	xcb_get_geometry_reply_t *geometry;
	xcb_generic_event_t *ev;
	focused_window = root_window;
	uint32_t attr[5];
	while (active) {
		ev = xcb_wait_for_event(conn);
		switch (ev->response_type & ~0x80) {
			case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t *e;

					e = (xcb_button_press_event_t *) ev;

					printf ("Button press = %d, Modifier = %d\n", e->detail, e->state);

					window = e->child;

					xcb_configure_window (conn, window, XCB_CONFIG_WINDOW_STACK_MODE, attr);
					geometry = xcb_get_geometry_reply (conn, xcb_get_geometry (conn, window), NULL);
					printf ("e->response_type = %d, e->sequence = %d, e->detail = %d, e->state = %d\n", e->response_type, e->sequence,
							e->detail, e->state);
					if (e->detail == 1) {
					  if (e->state == XCB_MOD_MASK_1) {
						attr[4] = 1;
						xcb_warp_pointer (conn, XCB_NONE, window, 0, 0, 0, 0, 1, 1);
						xcb_grab_pointer (conn, 0, root_window,
										  XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
										  XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root_window, XCB_NONE, XCB_CURRENT_TIME);
					} else {
						if (focused_window != window) {
						printf ("focusing window %d\n", window);
						focused_window = window;
						xcb_ungrab_button (conn, 1, root_window, XCB_NONE);
						xcb_grab_button (conn, 0, window, XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE, XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root_window, XCB_NONE, XCB_BUTTON_INDEX_ANY, XCB_MOD_MASK_ANY);
						xcb_set_input_focus (conn, XCB_INPUT_FOCUS_POINTER_ROOT, window, XCB_CURRENT_TIME);
						}
					  }
					} else {
					  attr[4] = 3;
					  xcb_warp_pointer (conn, XCB_NONE, window, 0, 0, 0, 0, geometry->width, geometry->height);
					  xcb_grab_pointer (conn, 0, root_window,
										XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_POINTER_MOTION_HINT,
										XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC, root_window, XCB_NONE, XCB_CURRENT_TIME);
					}
				} break;
			case XCB_MOTION_NOTIFY: 
				{
					xcb_query_pointer_reply_t *pointer;

					pointer = xcb_query_pointer_reply (conn, xcb_query_pointer (conn, root_window), 0);
					geometry = xcb_get_geometry_reply (conn, xcb_get_geometry (conn, window), NULL);
					if (attr[4] == 1) {     /* move */
						if (pointer->root_x > screen->width_in_pixels - 5) {
							attr[0] = screen->width_in_pixels / 2;
							attr[1] = 0;
							attr[2] = screen->width_in_pixels / 2;
							attr[3] = screen->height_in_pixels;
							xcb_configure_window (conn, window,
												  XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y | XCB_CONFIG_WINDOW_WIDTH |
												  XCB_CONFIG_WINDOW_HEIGHT, attr);

						} else {
							attr[0] = (pointer->root_x + geometry->width > screen->width_in_pixels)
							? (screen->width_in_pixels - geometry->width) : pointer->root_x;
							attr[1] = (pointer->root_y + geometry->height > screen->height_in_pixels)
							? (screen->height_in_pixels - geometry->height) : pointer->root_y;
							xcb_configure_window (conn, window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, attr);
						}
					} else if (attr[4] == 3) {      /* resize */
						attr[0] = pointer->root_x - geometry->x;
						attr[1] = pointer->root_y - geometry->y;
						xcb_configure_window (conn, window, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, attr);
					}
					XCBFLUSH;
				} break;
			case XCB_BUTTON_RELEASE:
				{
					xcb_ungrab_pointer(conn, XCB_CURRENT_TIME);
					XCBFLUSH;
				} break;
			case XCB_EXPOSE:
				{
					printf("expose\n");
					XCBFLUSH;
				} break;

			case XCB_MAP_REQUEST:
				{
					xcb_map_request_event_t* map = (xcb_map_request_event_t*)ev;
					xcb_map_window(conn, map->window);
					printf("map req\n");
					XCBFLUSH;
				}
			case XCB_DESTROY_NOTIFY: 
				{
					printf("destroy notify\n");
					XCBFLUSH;
				} break;
			case XCB_EVENT_MASK_KEY_RELEASE:
				{
					printf("keypress\n");
					XCBFLUSH;
				} break;
		}
	}
	char buf[255];

	if ((len = read(pipe_num, buf, sizeof(buf)))) {
		buf[len - 1] = '\0';
		// todo handle command
		printf("cmd: %s", buf);
		XCBFLUSH;
	}

	free(ev);

	// sleep for 500 ns
	struct timespec wait = { 
		0, 
		500L 
	};

	if (xcb_connection_has_error(conn)) {
		printf("connect failure!\n");
		quit();
	}

	nanosleep(&wait, NULL);
		
}

int main(void) {
	default_screen = atoi(getenv("DISPLAY"));
	conn = xcb_connect(NULL, &default_screen);
	if (xcb_connection_has_error(conn) > 0) {
		printf("failed to connect to display %i, is another wm running?\n", 
				default_screen);
		return 1;
	}

	printf("starting flow on display %i\n", default_screen);

	setup();

	signal(SIGINT, quit);

	loop();

	return 0;
}
