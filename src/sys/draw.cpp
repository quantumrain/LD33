#include "pch.h"
#include "base.h"
#include "render.h"

// draw_buffer

draw_buffer::draw_buffer() : _vb(0), _vertices(0), _num_vertices(0), _max_vertices(0), _vertex_stride(0) { }

draw_buffer::~draw_buffer() {
	destroy();
}

bool draw_buffer::create(int vertex_stride, int max_vertices) {
	destroy();

	if ((max_vertices <= 0) || (vertex_stride <= 0))
		return false;

	_num_vertices = 0;
	_max_vertices = max_vertices;
	_vertex_stride = vertex_stride;

	int size_in_bytes = vertex_stride * max_vertices;

	if (!(_vertices = new u8[size_in_bytes]) || !(_vb = gpu_create_vertex_buffer(vertex_stride, max_vertices))) {
		destroy();
		return false;
	}

	return true;
}

void draw_buffer::destroy() {
	gpu_destroy(_vb);

	delete [] _vertices;

	_vb = 0;
	_vertices = 0;
	_num_vertices = 0;
	_max_vertices = 0;
	_vertex_stride = 0;
}

void draw_buffer::clear() {
	_num_vertices = 0;
}

u8* draw_buffer::alloc(int count) {
	if ((_num_vertices + count) > _max_vertices)
		return 0;

	u8* p = _vertices + _num_vertices * _vertex_stride;

	_num_vertices += count;

	return p;
}

void draw_buffer::flush() {
	if (_num_vertices <= 0)
		return;

	if (void* p = gpu_map(_vb)) {
		memcpy(p, _vertices, _num_vertices * _vertex_stride);
		gpu_unmap(_vb);
	}
}

// draw_list

bool draw_list::init(int max_tris) {
	return verts.create(sizeof(v_xyz_uv_rgba), max_tris * 3);
}

void draw_list::destroy() {
	verts.destroy();
}

void draw_list::reset() {
	verts.clear();
	batches.clear();
}

v_xyz_uv_rgba* draw_list::alloc(texture tex, int count) {
	batch* b = 0;

	if (batches.empty() || ((b = &batches.back())->tex != tex)) {
		if ((b = batches.push_back({ tex, 0 })) == 0)
			return 0;
	}

	v_xyz_uv_rgba* v = (v_xyz_uv_rgba*)verts.alloc(count);

	if (v)
		b->count += count;

	return v;
}

void draw_list::render() {
	if (batches.empty())
		return;

	verts.flush();

	gpu_set_vertex_buffer(verts._vb);
	gpu_set_samplers({ g_sampler_point_clamp });

	int base = 0;

	for(auto& c : batches) {
		gpu_set_textures({ c.tex });
		gpu_draw(c.count, base);
		base += c.count;
	}
}

// draw_context

void draw_context::vert(float x, float y, float u, float v, float r, float g, float b, float a) const {
	v_xyz_uv_rgba* vp = _dl.alloc(_texture, 1);

	if (!vp)
		return;

	r = square(r * _colour.r); // todo: terrible approximation of srgb...
	g = square(g * _colour.g);
	b = square(b * _colour.b);
	a = a * _colour.a;

	vec3 p = _transform * vec3(x, y, 0.0f);

	*vp = { p.x, p.y, p.z, u, v, r, g, b, a };
}

void draw_context::vert(float x, float y, float r, float g, float b, float a) const {
	vert(x, y, 0.0f, 0.0f, r, g, b, a);
}

void draw_context::vert(vec2 p, vec2 uv, rgba c) const {
	vert(p.x, p.y, uv.x, uv.y, c.r, c.g, c.b, c.a);
}

void draw_context::vert(vec2 p, rgba c) const {
	vert(p.x, p.y, 0.0f, 0.0f, c.r, c.g, c.b, c.a);
}

void draw_context::line(vec2 p0, vec2 p1, float w, rgba c0, rgba c1) const {
	vec2 n = perp(normalise(p1 - p0)) * w;

	vec2 a = p0 - n;
	vec2 b = p0 + n;
	vec2 c = p1 - n;
	vec2 d = p1 + n;

	tri(a, c, b, c0, c1, c0);
	tri(b, c, d, c0, c1, c1);
}

void draw_context::line(vec2 p0, vec2 p1, float w, rgba c) const {
	line(p0, p1, w, c, c);
}

void draw_context::line_with_cap(vec2 p0, vec2 p1, float w, rgba c0, rgba c1) const {
	vec2 pn = normalise(p1 - p0) * w;
	vec2 n = perp(pn);

	vec2 a = p0 - n;
	vec2 b = p0 + n;
	vec2 c = p1 - n;
	vec2 d = p1 + n;

	quad(a, b, c, d,  c0, c0, c1, c1);

	tri(b, a, p0 - pn, c0);
	tri(c, d, p1 + pn, c1);
}

void draw_context::tri(vec2 p0, vec2 p1, vec2 p2, rgba c) const {
	vert(p0, c);
	vert(p1, c);
	vert(p2, c);
}

void draw_context::tri(vec2 p0, vec2 p1, vec2 p2, rgba c0, rgba c1, rgba c2) const {
	vert(p0, c0);
	vert(p1, c1);
	vert(p2, c2);
}

void draw_context::tri(vec2 p0, vec2 p1, vec2 p2, vec2 uv0, vec2 uv1, vec2 uv2, rgba c0, rgba c1, rgba c2) const {
	vert(p0, uv0, c0);
	vert(p1, uv1, c1);
	vert(p2, uv2, c2);
}

void draw_context::quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c) const {
	vert(p0, c);
	vert(p2, c);
	vert(p1, c);

	vert(p1, c);
	vert(p2, c);
	vert(p3, c);
}

void draw_context::quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c0, rgba c1, rgba c2, rgba c3) const {
	vert(p0, c0);
	vert(p2, c2);
	vert(p1, c1);

	vert(p1, c1);
	vert(p2, c2);
	vert(p3, c3);
}

void draw_context::quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3, rgba c0, rgba c1, rgba c2, rgba c3) const {
	vert(p0, uv0, c0);
	vert(p2, uv2, c2);
	vert(p1, uv1, c1);

	vert(p1, uv1, c1);
	vert(p2, uv2, c2);
	vert(p3, uv3, c3);
}

void draw_context::rect(vec2 p0, vec2 p1, rgba c) const {
	vert(p0.x, p0.y, c.r, c.g, c.b, c.a);
	vert(p0.x, p1.y, c.r, c.g, c.b, c.a);
	vert(p1.x, p0.y, c.r, c.g, c.b, c.a);

	vert(p1.x, p0.y, c.r, c.g, c.b, c.a);
	vert(p0.x, p1.y, c.r, c.g, c.b, c.a);
	vert(p1.x, p1.y, c.r, c.g, c.b, c.a);
}

void draw_context::rect(vec2 p0, vec2 p1, rgba c0, rgba c1, rgba c2, rgba c3) const {
	vert(p0.x, p0.y, c0.r, c0.g, c0.b, c0.a);
	vert(p0.x, p1.y, c2.r, c2.g, c2.b, c2.a);
	vert(p1.x, p0.y, c1.r, c1.g, c1.b, c1.a);

	vert(p1.x, p0.y, c1.r, c1.g, c1.b, c1.a);
	vert(p0.x, p1.y, c2.r, c2.g, c2.b, c2.a);
	vert(p1.x, p1.y, c3.r, c3.g, c3.b, c3.a);
}

void draw_context::rect(vec2 p0, vec2 p1, vec2 uv0, vec2 uv1, rgba c0, rgba c1, rgba c2, rgba c3) const {
	vert(p0.x, p0.y, uv0.x, uv0.y, c0.r, c0.g, c0.b, c0.a);
	vert(p0.x, p1.y, uv0.x, uv1.y, c2.r, c2.g, c2.b, c2.a);
	vert(p1.x, p0.y, uv1.x, uv0.y, c1.r, c1.g, c1.b, c1.a);

	vert(p1.x, p0.y, uv1.x, uv0.y, c1.r, c1.g, c1.b, c1.a);
	vert(p0.x, p1.y, uv0.x, uv1.y, c2.r, c2.g, c2.b, c2.a);
	vert(p1.x, p1.y, uv1.x, uv1.y, c3.r, c3.g, c3.b, c3.a);
}

void draw_context::rect(vec2 p0, vec2 p1, vec2 uv0, vec2 uv1, rgba c) const {
	vert(p0.x, p0.y, uv0.x, uv0.y, c.r, c.g, c.b, c.a);
	vert(p0.x, p1.y, uv0.x, uv1.y, c.r, c.g, c.b, c.a);
	vert(p1.x, p0.y, uv1.x, uv0.y, c.r, c.g, c.b, c.a);

	vert(p1.x, p0.y, uv1.x, uv0.y, c.r, c.g, c.b, c.a);
	vert(p0.x, p1.y, uv0.x, uv1.y, c.r, c.g, c.b, c.a);
	vert(p1.x, p1.y, uv1.x, uv1.y, c.r, c.g, c.b, c.a);
}

void draw_context::fill(int count, const vec2* p, rgba c) const {
	for(int i = 2; i < count; i++) tri(p[0], p[i - 1], p[i], c);
}

void draw_context::outline(int count, const vec2* p, float line_width, rgba c) const {
	const int MAX_COUNT = 32;

	vec2 n[MAX_COUNT];
	vec2 b[MAX_COUNT];

	count = min(count, MAX_COUNT);

	// todo: caps on corners > 90 degrees

	for(int i = 0, j = count - 1; i < count; j = i++) n[i] = normalise(p[i] - p[j]);
	for(int i = 0, j = count - 1; i < count; j = i++) b[j] = bevel_offset_from_normals(n[i], n[j]) * line_width;
	for(int i = 0, j = count - 1; i < count; j = i++) quad(p[j] - b[j], p[i] - b[i], p[j] + b[j], p[i] + b[i], c);
}

void draw_context::shape(vec2 p, int count, float radius, float rot, rgba c) const {
	float f  = TAU / count;
	vec2  n1 = rotation(rot - (count - 1) * f);

	for(int i = 0; i < count; i++) {
		vec2 n0 = rotation(rot - i * f);
		vert(p, c);
		vert(p + n0 * radius, c);
		vert(p + n1 * radius, c);
		n1 = n0;
	}
}

void draw_context::shape_outline(vec2 p, int count, float radius, float rot, float w, rgba c) const {
	float f  = TAU / count;
	float r0 = radius - w;
	float r1 = radius + w;
	vec2  n1 = rotation(rot - (count - 1) * f);

	for(int i = 0; i < count; i++) {
		vec2 n0 = rotation(rot - i * f);
		quad(p + n1 * r0, p + n0 * r0, p + n1 * r1, p + n0 * r1, c);
		n1 = n0;
	}
}

// helpers

void draw_tile(const draw_context& dc, vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c0, rgba c1, rgba c2, rgba c3, int tile_num, int flags) {
	int tx = tile_num & 15;
	int ty = tile_num >> 4;

	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;

	const float a = 1.0f / (256.0f * 16.0f);
	const float b = 1.0f - a;

	if ((flags & 1) == 0) {
		uv0 = vec2((tx + a) / 16.0f, (ty + a) / 16.0f);
		uv1 = vec2((tx + b) / 16.0f, (ty + a) / 16.0f);
		uv2 = vec2((tx + a) / 16.0f, (ty + b) / 16.0f);
		uv3 = vec2((tx + b) / 16.0f, (ty + b) / 16.0f);
	} else {
		uv0 = vec2((tx + a) / 16.0f, (ty + b) / 16.0f);
		uv1 = vec2((tx + a) / 16.0f, (ty + a) / 16.0f);
		uv2 = vec2((tx + b) / 16.0f, (ty + b) / 16.0f);
		uv3 = vec2((tx + b) / 16.0f, (ty + a) / 16.0f);
	}

	if (flags & 2) {
		swap(uv0, uv2);
		swap(uv1, uv3);
		swap(uv0, uv1);
		swap(uv2, uv3);
	}

	if (flags & DT_FLIP_X) {
		swap(uv0, uv1);
		swap(uv2, uv3);
	}

	if (flags & DT_FLIP_Y) {
		swap(uv0, uv2);
		swap(uv1, uv3);
	}

	if (flags & DT_ALT_TRI) {
		dc.vert(p0, uv0, c0);
		dc.vert(p3, uv3, c3);
		dc.vert(p1, uv1, c1);

		dc.vert(p0, uv0, c0);
		dc.vert(p2, uv2, c2);
		dc.vert(p3, uv3, c3);
	}
	else {
		dc.vert(p0, uv0, c0);
		dc.vert(p2, uv2, c2);
		dc.vert(p1, uv1, c1);

		dc.vert(p1, uv1, c1);
		dc.vert(p2, uv2, c2);
		dc.vert(p3, uv3, c3);
	}
}

void draw_tile(const draw_context& dc, vec2 c, float s, float rot, rgba col, int tile_num, int flags) {
	vec2 cx(cosf(rot) * s * 0.5f, sinf(rot) * s * 0.5f);
	vec2 cy(perp(cx));
	draw_tile(dc, c - cx - cy, c + cx - cy, c - cx + cy, c + cx + cy, col, col, col, col, tile_num, flags);
}

void draw_tile(const draw_context& dc, vec2 c, float s, rgba col, int tile_num, int flags) {
	vec2 cx(s, 0.0f);
	vec2 cy(0.0f, s);
	draw_tile(dc, c - cx - cy, c + cx - cy, c - cx + cy, c + cx + cy, col, col, col, col, tile_num, flags);
}

void draw_tile(const draw_context& dc, vec2 p0, vec2 p1, rgba col, int tile_num, int flags) {
	draw_tile(dc, p0, vec2(p1.x, p0.y), vec2(p0.x, p1.y), p1, col, col, col, col, tile_num, flags);
}

void draw_tile(const draw_context& dc, vec2 p0, vec2 p1, rgba c0, rgba c1, rgba c2, rgba c3, int tile_num, int flags) {
	draw_tile(dc, p0, vec2(p1.x, p0.y), vec2(p0.x, p1.y), p1, c0, c1, c2, c3, tile_num, flags);
}

void draw_tex_tile(const draw_context& dc, vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c0, rgba c1, rgba c2, rgba c3, texture t, int flags) {
	vec2 uv0;
	vec2 uv1;
	vec2 uv2;
	vec2 uv3;

	float a = 0.0f;
	float b = 1.0f;

	if ((flags & 1) == 0) {
		
		uv0 = vec2(a, a);
		uv1 = vec2(b, a);
		uv2 = vec2(a, b);
		uv3 = vec2(b, b);
	} else {
		uv0 = vec2(a, b);
		uv1 = vec2(a, a);
		uv2 = vec2(b, b);
		uv3 = vec2(b, a);
	}

	if (flags & 2) {
		swap(uv0, uv2);
		swap(uv1, uv3);
		swap(uv0, uv1);
		swap(uv2, uv3);
	}

	if (flags & DT_FLIP_X) {
		swap(uv0, uv1);
		swap(uv2, uv3);
	}

	if (flags & DT_FLIP_Y) {
		swap(uv0, uv2);
		swap(uv1, uv3);
	}

	if (flags & DT_ALT_TRI) {
		dc.vert(p0, uv0, c0);
		dc.vert(p3, uv3, c3);
		dc.vert(p1, uv1, c1);

		dc.vert(p0, uv0, c0);
		dc.vert(p2, uv2, c2);
		dc.vert(p3, uv3, c3);
	}
	else {
		dc.vert(p0, uv0, c0);
		dc.vert(p2, uv2, c2);
		dc.vert(p1, uv1, c1);

		dc.vert(p1, uv1, c1);
		dc.vert(p2, uv2, c2);
		dc.vert(p3, uv3, c3);
	}
}

void draw_tex_tile(const draw_context& dc, vec2 c, float s, float rot, rgba col, texture t, int flags) {
	vec2 cx(cosf(rot) * s * 0.5f, sinf(rot) * s * 0.5f);
	vec2 cy(perp(cx));
	draw_tex_tile(dc, c - cx - cy, c + cx - cy, c - cx + cy, c + cx + cy, col, col, col, col, t, flags);
}

void draw_tex_tile(const draw_context& dc, vec2 c, float s, rgba col, texture t, int flags) {
	vec2 cx(s, 0.0f);
	vec2 cy(0.0f, s);
	draw_tex_tile(dc, c - cx - cy, c + cx - cy, c - cx + cy, c + cx + cy, col, col, col, col, t, flags);
}

void draw_tex_tile(const draw_context& dc, vec2 p0, vec2 p1, rgba col, texture t, int flags) {
	draw_tex_tile(dc, p0, vec2(p1.x, p0.y), vec2(p0.x, p1.y), p1, col, col, col, col, t, flags);
}

void draw_tex_tile(const draw_context& dc, vec2 p0, vec2 p1, rgba c0, rgba c1, rgba c2, rgba c3, texture t, int flags) {
	draw_tex_tile(dc, p0, vec2(p1.x, p0.y), vec2(p0.x, p1.y), p1, c0, c1, c2, c3, t, flags);
}


// fullscreen quad

vertex_buffer g_vb_fullscreen_quad;

void init_fullscreen_quad() {
	g_vb_fullscreen_quad = gpu_create_vertex_buffer(sizeof(v_xyzw_uv), 3);

	if (v_xyzw_uv* v = (v_xyzw_uv*)gpu_map(g_vb_fullscreen_quad)) {
		v[0] = { -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, +1.0f };
		v[1] = { -1.0f, +3.0f, 0.0f, 1.0f, 0.0f, -1.0f };
		v[2] = { +3.0f, -1.0f, 0.0f, 1.0f, 2.0f, +1.0f };
		gpu_unmap(g_vb_fullscreen_quad);
	}
}

void draw_fullscreen_quad(pipeline pl) {
	gpu_set_pipeline(pl);
	gpu_set_vertex_buffer(g_vb_fullscreen_quad);
	gpu_draw(3, 0);
}