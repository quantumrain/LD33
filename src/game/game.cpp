#include "pch.h"
#include "game.h"

/*

	Herding game?

		You are a monster
		Herd children/whatever into your nest for your young to eat!
		Easy general puzzles around that?
		- Not that interesting though, and herding games always suck

*/

void stupid_thing(draw_context& dc) {
	static vec2 p;
	static vec2 v;
	static float t;

	p += v * DT;
	v += g_input.pad_left * 64.0f;
	v *= 0.8f;
	
	float l = length(v) * DT * 0.1f;
	int spr = 0;

	if (l > 0.001f) {
		t += l;

		int tt = ((int)t) & 3;

		if (tt == 0) spr = 0;
		else if (tt == 1) spr = 1;
		else if (tt == 2) spr = 0;
		else if (tt == 3) spr = 2;
	}
	else
		t = 0.0f;
	
	{
		draw_context dcc = dc.copy().set(g_sheet);
		draw_tile(dcc, p, 16.0f, rgba(), spr, 0);
	}
}

const wchar_t* g_win_name = L"LD33 - You are the Monster";

world g_world;

draw_list g_dl_world;
draw_list g_dl_ui;

void game_init() {
	g_dl_world.init(64 * 1024);
	g_dl_ui.init(64 * 1024);

	init_sound();
	define_sound(sfx::DIT, "dit", 2, 2);
	finalise_sound();

	spawn_entity(&g_world, new player);
}

void game_frame(vec2i view_size) {
	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = false;

	draw_context dc(g_dl_world);
	draw_context dc_ui(g_dl_ui);

	world_tick(&g_world);
	world_draw(&dc, &g_world);

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, top_down_proj_view(vec2(), 90.0f, (float)view_size.x / (float)view_size.y, 360.0f, 1.0f, 1000.0f));
	g_dl_world.render();
	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, (float)view_size.x / (float)view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();
}