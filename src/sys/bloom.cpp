#include "pch.h"
#include "base.h"
#include "render.h"

void bloom_fx::init_view(vec2i size) {
	for(int i = 0; i < MAX_LEVELS; i++) {
		bloom_level* bl = levels + i;

		size /= 2;

		bl->size = size;
		bl->reduce = gpu_discard_on_reset(gpu_create_texture(size.x, size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
		bl->blur_x = gpu_discard_on_reset(gpu_create_texture(size.x, size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
		bl->blur_y = gpu_discard_on_reset(gpu_create_texture(size.x, size.y, gpu_format::RGBA_SRGB, gpu_bind::SHADER | gpu_bind::RENDER_TARGET, 0));
	}
}

void bloom_fx::render(texture source) {
	gpu_set_samplers({ g_sampler_linear_clamp });

	for(int i = 0; i < MAX_LEVELS; i++) {
		bloom_level* bl = levels + i;

		gpu_set_const(0, vec4(1.0f / to_vec2(bl->size), 0.0f, 0.0f));
		gpu_set_viewport(vec2i(), bl->size, vec2(0.0f, 1.0f));

		gpu_set_render_target(bl->reduce, 0);
		gpu_set_textures({ (i == 0) ? source : levels[i - 1].blur_y });
		draw_fullscreen_quad(g_pipeline_bloom_reduce);

		gpu_set_render_target(bl->blur_x, 0);
		gpu_set_textures({ bl->reduce });
		draw_fullscreen_quad(g_pipeline_bloom_blur_x);

		gpu_set_render_target(bl->blur_y, 0);
		gpu_set_textures({ bl->blur_x });
		draw_fullscreen_quad(g_pipeline_bloom_blur_y);
	}
}

void bloom_fx::set_textures(texture_group* tg, int first_slot) {
	for(int i = 0; i < MAX_LEVELS; i++)
		tg->texture[first_slot + i] = levels[i].blur_y;
}