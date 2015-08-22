#include "pch.h"
#include "game.h"

const wchar_t* g_win_name = L"LD33 - Base Code";

world g_world;

draw_list g_dl_world;
draw_list g_dl_ui;

void game_init() {
	g_dl_world.init(64 * 1024);
	g_dl_ui.init(64 * 1024);

	init_sound();
	define_sound(sfx::DIT, "dit", 2, 2);
	finalise_sound();
}

void game_frame(vec2i view_size) {
	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = false;

	if (g_input.start)
		sound_play(sfx::DIT);

	{
		draw_context dcc = dc.copy().set(g_sheet);
		draw_tile(dcc, vec2(-100, 0), vec2(-100 + 16, 16), rgba(), 0, 0);
	}

	draw_string(dc, vec2(), vec2(2.0f), 0, rgba(colours::YELLOW), "Hello world!");

	dc.shape_outline(vec2(-50.0f, 75.0f), 5, 30.0f, deg_to_rad(45.0f), 2.0f, colours::GREEN);

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, top_down_proj_view(vec2(), 90.0f, (float)view_size.x / (float)view_size.y, 360.0f, 1.0f, 1000.0f));
	g_dl_world.render();
	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, (float)view_size.x / (float)view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();
}