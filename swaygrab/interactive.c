#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>
#include <poll.h>
#include <xkbcommon/xkbcommon.h>
#include <xkbcommon/xkbcommon-names.h>
#include "client/window.h"
#include "client/registry.h"
#include "client/cairo.h"
#include "log.h"
#include "swaygrab/interactive.h"
#include "swaygrab/json.h"
#include "wlc/wlc.h"

static struct wlc_geometry *grab_geometry;
static struct registry *registry;
static struct window *window;
static int start_x = -1, start_y = -1;
static bool done = false;

static void wl_dispatch_events() {
	wl_display_flush(registry->display);

	if (wl_display_dispatch(registry->display) == -1) {
		sway_abort("Failed to run wl_display_dispatch");
	}
}

static void notify_key(enum wl_keyboard_key_state state, xkb_keysym_t sym, uint32_t code, uint32_t codepoint) {
	if (state == WL_KEYBOARD_KEY_STATE_PRESSED) {
		switch (sym) {
		case XKB_KEY_Escape: // fallthrough
		case XKB_KEY_c: // fallthrough
		case XKB_KEY_C:
			if (sym == XKB_KEY_Escape || xkb_state_mod_name_is_active(registry->input->xkb.state,
					XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE) > 0
			) {
				sway_abort("Swaygrab cancelled.");
			}
		}
	}
}

static void mouse_button_notify(struct window *window, int x, int y, uint32_t button, uint32_t state_w) {
	static uint32_t first_button;

	// printf("Swaygrab! Mouse button %d clicked at %d %d %d\n", button, x, y, state_w);

	if (done) {
		// in case the user is click-happy, just throw it away
		return;
	}

	if (start_x == -1 && start_y == -1 && state_w == 1) {
		// pressing a button for the first time
		first_button = button;
		start_x = x;
		start_y = y;
	} else if ((button == first_button && state_w == 0 && ((start_x != x) || (start_y != y))) // clicked, moved, released button
		|| (state_w == 1 && ((start_x != x) || (start_y != y))) // clicked, released, moved, clicked again
	) {
		grab_geometry->origin.x = (start_x < x) ? start_x : x;
		grab_geometry->origin.y = (start_y < y) ? start_y : y;
		grab_geometry->size.w = ((start_x > x) ? start_x : x) - grab_geometry->origin.x;
		grab_geometry->size.h = ((start_y > y) ? start_y : y) - grab_geometry->origin.y;
		done = true;
	}
}

static void swaygrab_setup_window(int index) {
	registry = registry_poll();

	if (!registry->swaylock) {
		sway_abort("Swaygrab interactive mode requires the compositor to support the Swaylock extension.");
	}

	sway_log(L_DEBUG, "output index is %d", index);

	struct output_state *output = registry->outputs->items[index];
	window = window_setup(registry, output->width, output->height, output->scale, true);

	if (!window) {
		sway_abort("Failed to create surfaces.");
	}

	registry->input->notify = notify_key;
	window->pointer_input.notify_button = mouse_button_notify;

	cairo_identity_matrix(window->cairo);
	cairo_set_source_u32(window->cairo, 0x4c789933);
	cairo_set_operator(window->cairo, CAIRO_OPERATOR_SOURCE);
	window_render(window);

	window_make_shell(window);
	wl_shell_surface_set_fullscreen(window->shell_surface, 3, 0, output->output);
	lock_set_lock_surface(registry->swaylock, output->output, window->surface);
}

static void interactive_render() {
	cairo_identity_matrix(window->cairo);
	cairo_set_source_u32(window->cairo, 0x4c789933);
	cairo_set_operator(window->cairo, CAIRO_OPERATOR_SOURCE);
	cairo_paint(window->cairo);

	// cairo_paint(window->cairo);
	if (start_x > -1 && start_y > -1) {
		cairo_set_source_u32(window->cairo, 0x000000);
		cairo_rectangle(window->cairo, start_x, start_y, window->pointer_input.last_x - start_x, window->pointer_input.last_y - start_y);
		cairo_fill(window->cairo);
		cairo_stroke(window->cairo);
	}

	window_render(window);
}

struct wlc_geometry *get_geo_interactively(const char *output) {
	grab_geometry = malloc(sizeof(struct wlc_geometry));
	swaygrab_setup_window(get_output_index(output));

	while (wl_display_dispatch(registry->display) != -1) {
		wl_dispatch_events();

		interactive_render();
		if (done) {
			break;
		}
	}

	window_teardown(window);
	registry_teardown(registry);
	// printf("grab geometry x %d y %d w %d h %d\n", grab_geometry->origin.x, grab_geometry->origin.y, grab_geometry->size.w, grab_geometry->size.h);
	return grab_geometry;
}
