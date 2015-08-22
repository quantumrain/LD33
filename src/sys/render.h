#pragma once

extern pipeline g_pipeline_sprite;
extern pipeline g_pipeline_bloom_reduce;
extern pipeline g_pipeline_bloom_blur_x;
extern pipeline g_pipeline_bloom_blur_y;
extern pipeline g_pipeline_combine;

extern sampler g_sampler_point_clamp;
extern sampler g_sampler_linear_clamp;

extern texture g_texture_white;
extern texture g_sheet;

struct v_xyz_uv_rgba {
	float x, y, z;
	float u, v;
	float r, g, b, a;
};

struct v_xyz_n_uv_rgba {
	float x, y, z;
	float nx, ny, nz;
	float u, v;
	float r, g, b, a;
};

struct v_xyzw_uv {
	float x, y, z, w;
	float u, v;
};

struct draw_buffer {
	draw_buffer();
	~draw_buffer();

	bool create(int vertex_stride, int max_vertices);
	void destroy();

	void clear();
	u8* alloc(int count);
	void flush();

	vertex_buffer _vb;

	u8* _vertices;

	int _num_vertices;
	int _max_vertices;
	int _vertex_stride;
};

struct draw_list {
	struct batch { texture tex; int count; };

	draw_buffer verts;
	array<batch> batches;

	bool init(int max_tris);
	void destroy();
	void reset();

	v_xyz_uv_rgba* alloc(texture tex, int count);
	void render();
};

struct draw_context {
	draw_context(draw_list& dl) : _dl(dl), _texture(g_texture_white) { }

	draw_context copy() const { return *this; }

	draw_context& set(texture t)          { _texture   = t; return *this;}
	draw_context& set(const rgba& c)    { _colour    = c; return *this;}
	draw_context& set(const transform& t) { _transform = t; return *this;}

	draw_context& push(const rgba& c)     { _colour    = _colour * c;    return *this; }
	draw_context& push(const transform& t)  { _transform = _transform * t; return *this; }

	draw_context& translate(float x, float y) { return push(::translate(vec3(x, y, 0.0f))); }
	draw_context& rotate_z(float x) { return push(::rotate_z(x)); }
	draw_context& scale(float x, float y) { return push(::scale(vec3(x, y, 1.0f))); }

	void vert(float x, float y, float u, float v, float r, float g, float b, float a) const;
	void vert(float x, float y, float r, float g, float b, float a) const;
	void vert(vec2 p, vec2 uv, rgba c) const;
	void vert(vec2 p, rgba c) const;

	void line(vec2 p0, vec2 p1, float w, rgba c0, rgba c1) const;
	void line(vec2 p0, vec2 p1, float w, rgba c) const;

	void line_with_cap(vec2 p0, vec2 p1, float w, rgba c0, rgba c1) const;

	void tri(vec2 p0, vec2 p1, vec2 p2, rgba c) const;
	void tri(vec2 p0, vec2 p1, vec2 p2, rgba c0, rgba c1, rgba c2) const;
	void tri(vec2 p0, vec2 p1, vec2 p2, vec2 uv0, vec2 uv1, vec2 uv2, rgba c0, rgba c1, rgba c2) const;

	void quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c) const;
	void quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c0, rgba c1, rgba c2, rgba c3) const;
	void quad(vec2 p0, vec2 p1, vec2 p2, vec2 p3, vec2 uv0, vec2 uv1, vec2 uv2, vec2 uv3, rgba c0, rgba c1, rgba c2, rgba c3) const;

	void rect(vec2 p0, vec2 p1, rgba c) const;
	void rect(vec2 p0, vec2 p1, rgba c0, rgba c1, rgba c2, rgba c3) const;
	void rect(vec2 p0, vec2 p1, vec2 uv0, vec2 uv1, rgba c0, rgba c1, rgba c2, rgba c3) const;
	void rect(vec2 p0, vec2 p1, vec2 uv0, vec2 uv1, rgba c) const;

	void fill(int count, const vec2* p, rgba c) const;
	void outline(int count, const vec2* p, float line_width, rgba c) const;

	void fill(std::initializer_list<vec2> l, rgba c) const { return fill(l.size(), l.begin(), c); }

	void shape(vec2 p, int vcount, float radius, float rot, rgba c) const;
	void shape_outline(vec2 p, int vcount, float radius, float rot, float w, rgba c) const;

	draw_list&	_dl;
	texture		_texture;
	rgba		_colour;
	transform	_transform;
};

enum draw_tile_flags {
	DT_ROT_0 = 0,
	DT_ROT_90 = 1,
	DT_ROT_180 = 2,
	DT_ROT_270 = 3,
	DT_FLIP_X = 4,
	DT_FLIP_Y = 8,
	DT_ALT_TRI = 16
};

void draw_tile(const draw_context& dc, vec2 p0, vec2 p1, vec2 p2, vec2 p3, rgba c0, rgba c1, rgba c2, rgba c3, int tile_num, int flags);
void draw_tile(const draw_context& dc, vec2 c, float s, float rot, rgba col, int tile_num, int flags);
void draw_tile(const draw_context& dc, vec2 c, float s, rgba col, int tile_num, int flags);
void draw_tile(const draw_context& dc, vec2 p0, vec2 p1, rgba col, int tile_num, int flags);
void draw_tile(const draw_context& dc, vec2 p0, vec2 p1, rgba c0, rgba c1, rgba c2, rgba c3, int tile_num, int flags);

void draw_fullscreen_quad(pipeline pl);

enum font_flags {
	TEXT_LEFT	= 1,
	TEXT_RIGHT	= 2,
	TEXT_CENTRE	= 3,
	TEXT_TOP	= 4,
	TEXT_BOTTOM	= 8,
	TEXT_VCENTRE= 12
};

void draw_string(const draw_context& dc, vec2 pos, vec2 scale, int flags, rgba col, const char* txt);
void draw_stringf(const draw_context& dc, vec2 pos, vec2 scale, int flags, rgba col, const char* fmt, ...);

struct bloom_level {
	vec2i size;
	texture reduce;
	texture blur_x;
	texture blur_y;
};

struct bloom_fx {
	static const int MAX_LEVELS = 5;
	bloom_level levels[MAX_LEVELS];

	void init_view(vec2i size);
	void render(texture source);
	void set_textures(texture_group* tg, int first_slot);
};

void debug_text(vec2 p, rgba c, const char* fmt, ...);
void debug_log(rgba c, const char* fmt, ...);

void init_render_state();

// assets

struct font {
	texture texture;
	u8 glyph_width[128];
};

struct find_asset_data {
	int file;
	u32 bank;
	u32 entry;

	find_asset_data() : file(), bank(), entry() { }
};

bool load_assets(const char* path);
void* find_asset(u32 code, const char* name);
void* find_next_asset(find_asset_data* fad, u32 code, const char** name);

texture load_texture(const char* name);
shader load_shader(input_layout* il, const char* name);
font load_font(const char* name);