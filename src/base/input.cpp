#include "pch.h"
#include "platform.h"
#include "base.h"

vec2 remap_stick(int ix, int iy);

input g_input;
bool g_input_key[KEY_MAX];
bool g_input_down[KEY_MAX];
bool g_input_last[KEY_MAX];

int g_input_key_w;
int g_input_key_a;
int g_input_key_s;
int g_input_key_d;

vec2i g_input_old_mouse_pos;

bool is_key_down(game_key k)	{ return g_input_down[k]; }
bool is_key_pressed(game_key k)	{ return g_input_down[k] && !g_input_last[k]; }

decltype(&XInputEnable)		xinput_enable;
decltype(&XInputGetState)	xinput_get_state;

void input_init() {
	g_input_key_w = MapVirtualKey(0x11, MAPVK_VSC_TO_VK);
	g_input_key_a = MapVirtualKey(0x1E, MAPVK_VSC_TO_VK);
	g_input_key_s = MapVirtualKey(0x1F, MAPVK_VSC_TO_VK);
	g_input_key_d = MapVirtualKey(0x20, MAPVK_VSC_TO_VK);

	HMODULE xinput = LoadLibrary(L"xinput1_4.dll");
	if (!xinput) xinput = LoadLibrary(L"xinput1_3.dll");
	if (!xinput) xinput = LoadLibrary(L"xinput9_1_0.dll");

	if (xinput) {
		xinput_enable		= (decltype(&XInputEnable))GetProcAddress(xinput, "XInputEnable");
		xinput_get_state	= (decltype(&XInputGetState))GetProcAddress(xinput, "XInputGetState");
	}

	if (xinput_enable)
		xinput_enable(TRUE);
}

void input_update() {
	for(int i = 0; i < KEY_MAX; i++) {
		g_input_last[i] = g_input_down[i];
		g_input_down[i] = g_input_key[i];
	}

	if (xinput_get_state) {
		static int stick_check_time;

		if (--stick_check_time <= 0) {
			XINPUT_STATE state;

			if (xinput_get_state(0, &state) == ERROR_SUCCESS) {
				g_input.pad_left	= remap_stick(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY);
				g_input.pad_right	= remap_stick(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY);
				g_input.pad_buttons	= ((state.Gamepad.wButtons & XINPUT_GAMEPAD_A) != 0) || ((state.Gamepad.wButtons & XINPUT_GAMEPAD_START) != 0);
				stick_check_time	= 0;
			}
			else {
				g_input.pad_left	= vec2();
				g_input.pad_right	= vec2();
				g_input.pad_buttons	= false;
				stick_check_time	= 60;
			}

#ifdef _DEBUG
			if (state.Gamepad.bRightTrigger > 30) Sleep(100);
#endif
		}
	}

	if (g_input_old_mouse_pos == g_input.mouse_pos) {
		if (++g_input.mouse_time > 60) {
			if ((length_sq(g_input.pad_right) > 0.2) || is_key_down(KEY_R_LEFT) || is_key_down(KEY_R_RIGHT) || is_key_down(KEY_R_UP) || is_key_down(KEY_R_DOWN))
				g_input.mouse_active = false;
		}
		else
			g_input.mouse_active = true;
	}
	else {
		g_input.mouse_time		= 0;
		g_input.mouse_active	= true;
		g_input_old_mouse_pos	= g_input.mouse_pos;
	}

	static bool last_start;

	bool new_start	= is_key_down(KEY_FIRE) || (g_input.mouse_buttons & 1) || (g_input.pad_buttons & 1);
	g_input.start	= !last_start && new_start;
	last_start		= new_start;
}

void input_lost_focus() {
	memset(g_input_key, 0, sizeof(g_input_key));
	g_input.mouse_buttons = 0;
}

void input_mouse_move_event(vec2i pos) {
	g_input.mouse_pos = pos;
}

void input_mouse_button_event(int button, bool down) {
	if (down)
		g_input.mouse_buttons |= 1 << button;
	else
		g_input.mouse_buttons &= ~(1 << button);
}

void input_key_event(int key, bool down) {
	game_key kp = KEY_NONE;

	switch(key) {
		case VK_UP:		kp = KEY_R_UP;			break;
		case VK_DOWN:	kp = KEY_R_DOWN;		break;
		case VK_LEFT:	kp = KEY_R_LEFT;		break;
		case VK_RIGHT:	kp = KEY_R_RIGHT;		break;
		case VK_RETURN:	kp = KEY_FIRE;			break;
		case VK_SPACE:	kp = KEY_FIRE;			break;
		case 27:		kp = KEY_ESCAPE;		break;
		case VK_F1:		kp = KEY_DEBUG_STATS;	break;

		default:
			if (key == g_input_key_w) kp = KEY_L_UP;
			if (key == g_input_key_s) kp = KEY_L_DOWN;
			if (key == g_input_key_a) kp = KEY_L_LEFT;
			if (key == g_input_key_d) kp = KEY_L_RIGHT;
		break;
	}

	if (kp != KEY_NONE)
		g_input_key[kp] = down;
}

vec2 remap_stick(int ix, int iy) {
	vec2 d(ix / 32768.0f, iy / -32768.0f);
	float i = length(d);

	if (i < 0.2f)
		return vec2();

	float j = clamp((i - 0.2f) / 0.7f, 0.0f, 1.0f);

	return d * (square(j) / i);
}