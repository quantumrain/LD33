#pragma once

#include "ut.h"
#include "vec_types.h"
#include "vec_ops.h"
#include "gpu.h"
#include "audio.h"
#include "std.h"
#include "asset.h"

// input

enum game_key {
	KEY_NONE,
	KEY_L_UP, KEY_L_DOWN, KEY_L_LEFT, KEY_L_RIGHT,
	KEY_R_UP, KEY_R_DOWN, KEY_R_LEFT, KEY_R_RIGHT,
	KEY_FIRE,
	KEY_ESCAPE,
	KEY_DEBUG_STATS,
	KEY_MAX
};

struct input {
	vec2i	mouse_pos;
	int		mouse_buttons;
	int		mouse_time;
	bool	mouse_active;

	vec2	pad_left;
	vec2	pad_right;
	int		pad_buttons;

	bool	start;
};

extern bool g_request_mouse_capture;
extern bool g_hide_mouse_cursor;
extern input g_input;
extern int g_input_key_w;
extern int g_input_key_a;
extern int g_input_key_s;
extern int g_input_key_d;

bool is_key_down(game_key k);
bool is_key_pressed(game_key k);

void input_init();
void input_update();
void input_lost_focus();
void input_mouse_move_event(vec2i pos);
void input_mouse_button_event(int button, bool down);
void input_key_event(int key, bool down);
