#include "pch.h"
#include "base.h"
#include "render.h"
#include "sound.h"

void game_frame(vec2i view_size);
void init_fullscreen_quad();
void init_font();
void init_debug_draw();
void render_debug_draw(vec2i view_size);

random g_render_rand(1);

texture g_draw_target;
texture g_depth_target;
texture g_texture_white;
texture g_sheet;

bloom_fx g_bloom_fx;

void frame_init() {
	if (!load_assets("assets.dat"))
		panic("failed to load assets.dat");

	init_render_state();
	init_fullscreen_quad();
	init_font();
	init_debug_draw();

	u32 white = 0xFFFFFFFF;

	g_texture_white = gpu_create_texture(1, 1, gpu_format::RGBA, gpu_bind::SHADER, (u8*)&white);
	g_sheet         = load_texture("sheet");
}

void frame_init_view(vec2i view_size) {
	g_draw_target  = gpu_discard_on_reset(gpu_create_texture(view_size.x, view_size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
	g_depth_target = gpu_discard_on_reset(gpu_create_texture(view_size.x, view_size.y, gpu_format::D24S8, gpu_bind::DEPTH_STENCIL, 0));

	g_bloom_fx.init_view(view_size);
}

void frame_render(vec2i view_size) {
	sound_update();

	// clear

	gpu_clear(g_draw_target, rgba(0.0f));
	gpu_clear(g_depth_target, rgba(1.0f));

	// game + bloom

	gpu_set_render_target(g_draw_target, g_depth_target);
	gpu_set_viewport(vec2i(0, 0), view_size, vec2(0.0f, 1.0f));

	game_frame(view_size);
	g_bloom_fx.render(g_draw_target);

	// combine

	gpu_set_render_target(gpu_backbuffer(), 0);
	gpu_set_viewport(vec2i(0, 0), view_size, vec2(0.0f, 1.0f));

	gpu_set_samplers({ g_sampler_point_clamp, g_sampler_linear_clamp });

	texture_group tg { g_draw_target };
	g_bloom_fx.set_textures(&tg, 1);
	gpu_set_textures(tg);

	gpu_set_const(0, g_render_rand.range(vec4(500.0f), vec4(1500.0f)));

	draw_fullscreen_quad(g_pipeline_combine);

	// debug

	render_debug_draw(view_size);

	gpu_set_textures({});
}