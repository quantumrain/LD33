#include "pch.h"
#include "base.h"
#include "render.h"
#include "sound.h"

#define USE_AA 0

#if USE_AA
#define AA_VIEW_SCALE 2
#else
#define AA_VIEW_SCALE 1
#endif

void game_frame(vec2i view_size);
void init_fullscreen_quad();
void init_font();
void init_debug_draw();
void render_debug_draw(vec2i view_size);

random g_render_rand(1);

texture g_aa_reduce_target;

texture g_draw_blur_x;
texture g_draw_blur_y;

texture g_draw_target;
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
	g_draw_target = gpu_discard_on_reset(gpu_create_texture(view_size.x * AA_VIEW_SCALE, view_size.y * AA_VIEW_SCALE, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));

#if USE_AA
	g_aa_reduce_target = gpu_discard_on_reset(gpu_create_texture(view_size.x, view_size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
#else
	g_aa_reduce_target = g_draw_target;
#endif

	g_draw_blur_x = gpu_discard_on_reset(gpu_create_texture(view_size.x, view_size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
	g_draw_blur_y = gpu_discard_on_reset(gpu_create_texture(view_size.x, view_size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
	
	g_bloom_fx.init_view(view_size);
}

void frame_render(vec2i view_size) {
	sound_update();

	// clear

	gpu_clear(g_draw_target, rgba(0.035f));

	// game + bloom

	gpu_set_render_target(g_draw_target, 0);
	gpu_set_viewport(vec2i(0, 0), view_size * AA_VIEW_SCALE, vec2(0.0f, 1.0f));

	game_frame(view_size);
	
#if USE_AA
	{
		gpu_set_samplers({ g_sampler_linear_clamp });
		gpu_set_viewport(vec2i(), view_size, vec2(0.0f, 1.0f));
		gpu_set_render_target(g_aa_reduce_target, 0);
		gpu_set_textures({ g_draw_target });
		draw_fullscreen_quad(g_pipeline_bloom_reduce);
	}
#endif

	g_bloom_fx.render(g_aa_reduce_target);

	// blur the draw buffer

	{
		gpu_set_samplers({ g_sampler_linear_clamp });

		gpu_set_const(0, vec4(1.0f / 750.0f, 0.0f, 0.0f));
		gpu_set_viewport(vec2i(), view_size, vec2(0.0f, 1.0f));

		gpu_set_render_target(g_draw_blur_x, 0);
		gpu_set_textures({ g_bloom_fx.levels[0].reduce });
		draw_fullscreen_quad(g_pipeline_bloom_blur_x);

		gpu_set_render_target(g_draw_blur_y, 0);
		gpu_set_textures({ g_draw_blur_x });
		draw_fullscreen_quad(g_pipeline_bloom_blur_y);
	}

	// combine
	
	gpu_set_render_target(gpu_backbuffer(), 0);
	gpu_set_viewport(vec2i(0, 0), view_size, vec2(0.0f, 1.0f));

	gpu_set_samplers({ g_sampler_point_clamp, g_sampler_linear_clamp });

	texture_group tg { g_draw_blur_y };
	g_bloom_fx.set_textures(&tg, 1);
	gpu_set_textures(tg);

	gpu_set_const(0, g_render_rand.range(vec4(500.0f), vec4(1500.0f)));

	draw_fullscreen_quad(g_pipeline_combine);

	// debug

	render_debug_draw(view_size);

	gpu_set_textures({});
}