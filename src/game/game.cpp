#include "pch.h"
#include "game.h"

/*

	Herding game?

		You are a monster
		Herd children/whatever into your nest for your young to eat!
		Easy general puzzles around that?
		- Not that interesting though, and herding games always suck

*/

const wchar_t* g_win_name = L"LD33 - You are the Monster";

world g_world;

draw_list g_dl_world;
draw_list g_dl_ui;

voice_id g_sound_grind;
voice_id g_sound_grind2;
voice_id g_sound_grind3;

void game_init() {
	g_dl_world.init(64 * 1024);
	g_dl_ui.init(64 * 1024);

	init_sound();
	define_sound(sfx::DIT, "dit", 2, 2);
	define_sound(sfx::RAIN, "rain", 2, 0);
	define_sound(sfx::WIND, "wind", 2, 0);
	define_sound(sfx::GRIND, "grind3", 1, 0);
	define_sound(sfx::GRIND2, "grind4", 2, 0);
	finalise_sound();

	//sound_play(sfx::RAIN, -64.0f, -20.0f, SOUND_LOOP);
	//sound_play(sfx::RAIN, -56.0f, -18.0f, SOUND_LOOP);
	sound_play(sfx::WIND, 0.0f, 2.0f, SOUND_LOOP);
	sound_play(sfx::WIND, -10.0f, 2.0f, SOUND_LOOP);
	g_sound_grind = sound_play(sfx::GRIND, 0.0f, -1000.0f, SOUND_LOOP);
	g_sound_grind2 = sound_play(sfx::GRIND2, -10.0f, -1000.0f, SOUND_LOOP);
	g_sound_grind3 = sound_play(sfx::GRIND2, 10.0f, -1000.0f, SOUND_LOOP);

	spawn_entity(&g_world, new player);
}

void make_proj_view(world* w, vec2 centre, float fov, float aspect, float virtual_height, float z_near, float z_far) {
	float camera_height	= virtual_height / (tanf(deg_to_rad(fov) * 0.5f) * 2.0f);
	w->proj			= perspective(deg_to_rad(fov), aspect, z_near, z_far);
	w->view			= (mat44)camera_look_at(vec3(centre, camera_height), vec3(centre, 0.0f), vec3(0.0f, -1.0f, 0));
	w->proj_view	= w->proj * w->view;
}

void game_frame(vec2i view_size) {
	world* w = &g_world;

	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = false;

	draw_context dc(g_dl_world);
	draw_context dc_ui(g_dl_ui);

	world_tick(w);
	world_draw(&dc, w);

	make_proj_view(w, -w->camera_pos, 90.0f, (float)view_size.x / (float)view_size.y, 360.0f, 1.0f, 1000.0f);

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, w->proj_view);
	g_dl_world.render();
	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, (float)view_size.x / (float)view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();
}