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

texture g_tree2;
texture g_house;
texture g_field;
texture g_ground;
texture g_rock;
texture g_farmer;

array<vec2> g_rocks;
array<vec2> g_trees;
array<vec2> g_houses;
array<vec2> g_fields;

bool g_in_menu = true;

void game_init() {
	world* w = &g_world;

	g_dl_world.init(64 * 1024);
	g_dl_ui.init(64 * 1024);

	g_tree2 = load_texture("tree2");
	g_house = load_texture("house");
	g_field = load_texture("field");
	g_ground = load_texture("ground");
	g_rock = load_texture("rock");
	g_farmer = load_texture("farmer");

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
	g_sound_grind3 = sound_play(sfx::GRIND2, -20.0f, -1000.0f, SOUND_LOOP);

	spawn_entity(w, new player);

	{
		w->map.init(512, 512);

		raw_header* png = (raw_header*)find_asset(ASSET_PNG, "map");

		if (!png) panic("Missing map!");

		int width, height;
		u8* data = image_from_memory((u8*)&png[1], png->size, &width, &height);

		if (!data || (width != 512) || (height != 512)) panic("map wrong size/corrupt!");

		for(int y = 0; y < 512; y++) {
			for(int x = 0; x < 512; x++) {
				u32 c = ((u32*)data)[x + (y * 512)] & 0xFFFFFF;

				if (c == 0x000000)
					w->map.set_id(x, y, TILE_WALL);

				vec2 p(x * 5.0f, y * 5.0f);

				if (c == 0xFF0000) {
					g_rocks.push_back(p);
				}
				
				if (c == 0x00FF00) {
					g_trees.push_back(p);
				}

				if (c == 0xFFFF00) {
					g_houses.push_back(p);
				}

				if (c == 0x00FFFF) {
					g_fields.push_back(p);
				}

				if (c == 0x0000FF) {
					if (farmer* f = spawn_entity(w, new farmer)) {
						f->set_pos(p);
					}
				}
			}
		}

		free(data);
	}
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
	g_hide_mouse_cursor = !g_in_menu;

	draw_context dc(g_dl_world);
	draw_context dc_ui(g_dl_ui);

	if (g_in_menu) {
		dc_ui.rect({ -1000.0f, -1000.0f }, { 1000.0f + 640.0f, 1000.0f + 360.0f }, rgba(0.2f));

		float y = 40.0f;
		draw_string(dc_ui, vec2(320.0f, y), vec2(4.0f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.75f), "IN THE WOODS"); y += 28.0f;
		draw_string(dc_ui, vec2(320.0f, y), vec2(1.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.5f), "By Stephen Cakebread"); y += 7.0f * 8.0f;

		draw_string(dc_ui, vec2(320.0f, y), vec2(1.5f), TEXT_CENTRE | TEXT_VCENTRE, rgba(0.65f, 0.65f, 0.65f, 1.0f), "using an xbox 360 controller is highly recommended"); y += 5.0f * 8.0f;

		float x0 = 320.0f - 140.0f;
		float x1 = 320.0f + 140.0f;

		rgba c0(0.5f);
		rgba c(0.75f);

		draw_string(dc_ui, vec2(320.0f, y), vec2(1.5f), TEXT_CENTRE, c0, "Xbox 360 Gamepad"); y += 15.0f;
		draw_string(dc_ui, vec2(x0, y), vec2(1.5f), TEXT_LEFT, c, "Move");
		draw_string(dc_ui, vec2(x1, y), vec2(1.5f), TEXT_RIGHT, c, "Left Stick"); y += 15.0f;
		draw_string(dc_ui, vec2(x0, y), vec2(1.5f), TEXT_LEFT, c, "Action");
		draw_string(dc_ui, vec2(x1, y), vec2(1.5f), TEXT_RIGHT, c, "A"); y += 15.0f;

		y += 10.0f;

		draw_string(dc_ui, vec2(320.0f, y), vec2(1.5f), TEXT_CENTRE, c0, "Keyboard"); y += 15.0f;
		draw_string(dc_ui, vec2(x0, y), vec2(1.5f), TEXT_LEFT, c, "Move");
		draw_stringf(dc_ui, vec2(x1, y), vec2(1.5f), TEXT_RIGHT, c, "%c%c%c%c or \001\002\003\004", g_input_key_w, g_input_key_a, g_input_key_s, g_input_key_d); y += 15.0f;
		draw_string(dc_ui, vec2(x0, y), vec2(1.5f), TEXT_LEFT, c, "Action");
		draw_string(dc_ui, vec2(x1, y), vec2(1.5f), TEXT_RIGHT, c, "Space or Enter"); y += 15.0f + 5.0f;
		draw_string(dc_ui, vec2(x0, y), vec2(1.5f), TEXT_LEFT, c, "Fullscreen");
		draw_string(dc_ui, vec2(x1, y), vec2(1.5f), TEXT_RIGHT, c, "F11"); y += 15.0f;

		y += 4.0f * 8.0f;

		draw_string(dc_ui, vec2(320.0f, y), vec2(1.5f), TEXT_CENTRE, c, "Press Action to Start");
		
		if (g_input.start) {
			g_in_menu = false;
		}
	}
	else {
		world_tick(w);
		world_draw(&dc, w);
	}

	make_proj_view(w, -w->camera_pos, 90.0f, (float)view_size.x / (float)view_size.y, 360.0f, 1.0f, 1000.0f);

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, w->proj_view);
	g_dl_world.render();
	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, (float)view_size.x / (float)view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();
}