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
}

void recurse(draw_context dcc, int i, float t) {
	if (i < 0)
		return;

	draw_context dcw = dcc.copy();

	float m = lerp(10.0f, 30.0f, ((i - 1) / 7.0f)) * lerp(1.0f, 0.15f, square((i - 1) / 7.0f));
	float r = cosf(t + ((i - 1) / 7.0f) * TAU) * 0.5f;

	dcc.push(rotate_z(r));
	dcc.push(translate(vec3(-m, 0.0f, 0.0f)));

	recurse(dcc, i - 1, t);

	float s = lerp(10.0f, 30.0f, (i / 7.0f)) * lerp(1.0f, 0.15f, square(i / 7.0f));
	float h = s * 0.5f;
	if (i == 7) s *= 1.5f; // accidentally after h, but looks nicer for it

	//dcw.push(rotate_z(cosf(t * 3.0f) * 0.1f));

#if 1
	// green 0xFFA3CE27
	// purple 0xFF5026CC
	// dark purple 0xFF3C1D99

	draw_context dcs = dcw.copy().push(translate(vec3(s * 0.25f, 0.0f, 0.0f)));

	dcs.copy().push(rotate_z(deg_to_rad(-55.0f))).fill({ vec2(-0.10f, 0.0f) * s, vec2( 0.00f,-2.0f) * s, vec2( 0.10f, 0.0f) * s }, rgba(0xFF6430FF));
	dcs.copy().push(rotate_z(deg_to_rad( 55.0f))).fill({ vec2(-0.10f, 0.0f) * s, vec2( 0.00f, 2.0f) * s, vec2( 0.10f, 0.0f) * s }, rgba(0xFF6430FF));
	dcs.copy().push(rotate_z(deg_to_rad(-25.0f))).fill({ vec2(-0.10f, 0.0f) * s, vec2( 0.00f,-1.5f) * s, vec2( 0.10f, 0.0f) * s }, rgba(0xFF6430FF));
	dcs.copy().push(rotate_z(deg_to_rad( 25.0f))).fill({ vec2(-0.10f, 0.0f) * s, vec2( 0.00f, 1.5f) * s, vec2( 0.10f, 0.0f) * s }, rgba(0xFF6430FF));

	if (i == 0) dcw.shape(vec2(-h, 0.0f), 3, s * 0.5f, PI, rgba(0xFFA3CE27));
	dcw.shape(vec2(h, 0.0f), 3, s, 0.0f, rgba(0xFFA3CE27));
	dcw.shape(vec2(0.0f, 0.0f), 4, s * 0.5f, 0.0f, rgba(0xFF6430FF));
	dcw.shape(vec2(0.0f, 0.0f), 4, s * 0.25f, 0.0f, rgba(0xFFA3CE27));
#else
	if (i == 0) {
		dcw.fill({
				vec2( 0.500f, 0.866f) * h + vec2(-h, 0.0f),
				vec2(-1.000f, 0.000f) * h + vec2(-h, 0.0f),
				vec2(-0.000f, 0.000f) * h + vec2(-h, 0.0f),
				vec2( 0.500f, 0.500f) * h + vec2(-h, 0.0f)
			}, rgba(0xFFA3CE27));

		dcw.fill({
				vec2( 0.500f, -0.866f) * h + vec2(-h, 0.0f),
				vec2(-1.000f, -0.000f) * h + vec2(-h, 0.0f),
				vec2(-0.000f, -0.000f) * h + vec2(-h, 0.0f),
				vec2( 0.500f, -0.500f) * h + vec2(-h, 0.0f)
			}, rgba(0xFFA3CE27));
	}

	if (i == 7) {
		dcw.fill({
				vec2(-0.500f, 0.866f) * s + vec2(h, 0.0f),
				vec2( 1.000f, 0.000f) * s + vec2(h, 0.0f),
				vec2( 0.000f, 0.000f) * s + vec2(h, 0.0f),
				vec2(-0.500f, 0.500f) * s + vec2(h, 0.0f)
			}, rgba(0xFFA3CE27));

		dcw.fill({
				vec2(-0.500f, -0.866f) * s + vec2(h, 0.0f),
				vec2( 1.000f, -0.000f) * s + vec2(h, 0.0f),
				vec2( 0.000f, -0.000f) * s + vec2(h, 0.0f),
				vec2(-0.500f, -0.500f) * s + vec2(h, 0.0f)
			}, rgba(0xFFA3CE27));
	}
	else {
		dcw.fill({
				vec2(-0.500f, 0.866f) * s + vec2(h, 0.0f),
				vec2( 0.366f, 0.366f) * s + vec2(h, 0.0f),
				vec2( 0.000f, 0.000f) * s + vec2(h, 0.0f),
				vec2(-0.500f, 0.500f) * s + vec2(h, 0.0f)
			}, rgba(0xFFA3CE27));

		dcw.fill({
				vec2(-0.500f, -0.866f) * s + vec2(h, 0.0f),
				vec2( 0.366f, -0.366f) * s + vec2(h, 0.0f),
				vec2( 0.000f, -0.000f) * s + vec2(h, 0.0f),
				vec2(-0.500f, -0.500f) * s + vec2(h, 0.0f)
			}, rgba(0xFFA3CE27));
	}

	// red 0xFFE52B60
	// green 0xFFA3CE27
	// dark green 0xFF7A991D
	// purple 0xFF5026CC
	dcw.shape(vec2(0.0f, 0.0f), 4, s * 0.25f, 0.0f, rgba(0xFF7A991D));
#endif
}

void game_frame(vec2i view_size) {
	g_dl_world.reset();
	g_dl_ui.reset();

	g_request_mouse_capture = false;

	draw_context dc(g_dl_world);
	draw_context dc_ui(g_dl_ui);

	float static t;
	t += 0.03f;

	{
		draw_context dcc = dc.copy();

		recurse(dcc, 7, t);
	}

	gpu_set_pipeline(g_pipeline_sprite);
	gpu_set_const(0, top_down_proj_view(vec2(), 90.0f, (float)view_size.x / (float)view_size.y, 360.0f, 1.0f, 1000.0f));
	g_dl_world.render();
	gpu_set_const(0, fit_ui_proj_view(640.0f, 360.0f, (float)view_size.x / (float)view_size.y, -64.0f, 64.0f));
	g_dl_ui.render();
}