#include "pch.h"
#include "base.h"
#include "render.h"

shader g_shader_blur_x;
shader g_shader_blur_y;
shader g_shader_combine;
shader g_shader_reduce;
shader g_shader_sprite;

blend_state g_blend_default;
blend_state g_blend_pre_alpha;

depth_state g_depth_default;

raster_state g_raster_default;

pipeline g_pipeline_sprite;
pipeline g_pipeline_bloom_reduce;
pipeline g_pipeline_bloom_blur_x;
pipeline g_pipeline_bloom_blur_y;
pipeline g_pipeline_combine;

sampler g_sampler_point_clamp;
sampler g_sampler_linear_clamp;



void init_render_state() {
	input_layout il_xyz;
	il_xyz.element(offsetof(v_xyz_uv_rgba, x), gpu_type::F32_3, gpu_usage::POSITION, 0);
	il_xyz.element(offsetof(v_xyz_uv_rgba, u), gpu_type::F32_2, gpu_usage::UV, 0);
	il_xyz.element(offsetof(v_xyz_uv_rgba, r), gpu_type::F32_4, gpu_usage::COLOUR, 0);

	input_layout il_xyzw;
	il_xyzw.element(offsetof(v_xyzw_uv, x), gpu_type::F32_4, gpu_usage::POSITION, 0);
	il_xyzw.element(offsetof(v_xyzw_uv, u), gpu_type::F32_2, gpu_usage::UV, 0);

	g_shader_blur_x		= load_shader(&il_xyzw, "fs_blur_x");
	g_shader_blur_y		= load_shader(&il_xyzw, "fs_blur_y");
	g_shader_combine	= load_shader(&il_xyzw, "fs_combine");;
	g_shader_reduce		= load_shader(&il_xyzw, "fs_reduce");
	g_shader_sprite		= load_shader(&il_xyz, "uv_c");

	g_blend_default		= gpu_create_blend_state(false, gpu_blend::ONE, gpu_blend::ZERO, gpu_blend_op::ADD, gpu_blend::ONE, gpu_blend::ZERO, gpu_blend_op::ADD);
	g_blend_pre_alpha	= gpu_create_blend_state(true, gpu_blend::ONE, gpu_blend::INV_SRC_ALPHA, gpu_blend_op::ADD, gpu_blend::ONE, gpu_blend::INV_SRC_ALPHA, gpu_blend_op::ADD);

	g_depth_default		= gpu_create_depth_state(false, false);

	g_raster_default	= gpu_create_raster_state(gpu_cull::NONE, true);

	g_pipeline_sprite       = gpu_create_pipeline(g_shader_sprite,  g_blend_pre_alpha, g_depth_default, g_raster_default, gpu_primitive::TRIANGLE_LIST);
	g_pipeline_bloom_reduce = gpu_create_pipeline(g_shader_reduce,  g_blend_default,   g_depth_default, g_raster_default, gpu_primitive::TRIANGLE_LIST);
	g_pipeline_bloom_blur_x = gpu_create_pipeline(g_shader_blur_x,  g_blend_default,   g_depth_default, g_raster_default, gpu_primitive::TRIANGLE_LIST);
	g_pipeline_bloom_blur_y = gpu_create_pipeline(g_shader_blur_y,  g_blend_default,   g_depth_default, g_raster_default, gpu_primitive::TRIANGLE_LIST);
	g_pipeline_combine      = gpu_create_pipeline(g_shader_combine, g_blend_default,   g_depth_default, g_raster_default, gpu_primitive::TRIANGLE_LIST);

	g_sampler_point_clamp	= gpu_create_sampler(false, true);
	g_sampler_linear_clamp	= gpu_create_sampler(true, true);
}