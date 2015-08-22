#include "pch.h"
#include "platform.h"
#include "base.h"

enum {
	MAX_SHADERS			= 32,
	MAX_BLEND_STATES	= 16,
	MAX_DEPTH_STATES	= 16,
	MAX_RASTER_STATES	= 16,
	MAX_VERTEX_BUFFERS	= 128,
	MAX_TEXTURES		= 128,
	MAX_SAMPLERS		= 16,
	MAX_CONSTANTS		= 16
};

enum {
	GPU_USED				= 0x1,
	GPU_DISCARD_ON_RESET	= 0x2,
};

template<typename T, size_t MAX> struct gpu_array {
	T	data[MAX];
	u8	flags[MAX];

	void set_flag(int i, int flag) { if (valid(i) && used(i)) flags[i] |= flag; }

	bool valid(int i)				{ return (i > 0) && (i < MAX); }
	bool used(int i)				{ return (flags[i] & GPU_USED) != 0; }
	bool discard_on_reset(int i)	{ return (flags[i] & GPU_DISCARD_ON_RESET) != 0; }

	T* operator[](int i) { return valid(i) && used(i) ? &data[i] : 0; }

	u16 alloc() {
		for(int i = 1; i < MAX; i++)
			if (!used(i)) { flags[i] = GPU_USED; return (u16)i; }
		return 0;
	}

	void free(u16 i) {
		assert(valid(i) && used(i));
		memset(&data[i], 0, sizeof(T));
		flags[i] = 0;
	}

	gpu_array() { memset(data, 0, sizeof(data)); }
};

DXGI_FORMAT to_dx(gpu_format format);
D3D10_BLEND to_dx(gpu_blend blend);
D3D10_BLEND_OP to_dx(gpu_blend_op op);
D3D10_CULL_MODE to_dx(gpu_cull mode);
D3D10_PRIMITIVE_TOPOLOGY to_dx(gpu_primitive primitive);

struct gpu_shader {
	ID3D10InputLayout*	layout;
	ID3D10VertexShader*	vertex;
	ID3D10PixelShader*	pixel;
};

struct gpu_blend_state  { ID3D10BlendState* p; };
struct gpu_depth_state  { ID3D10DepthStencilState* p; };
struct gpu_raster_state { ID3D10RasterizerState* p; };

struct gpu_vertex_buffer {
	ID3D10Buffer* p;
	UINT stride;
};

struct gpu_texture {
	ID3D10Texture2D* p;
	ID3D10ShaderResourceView* srv;
	ID3D10RenderTargetView* rtv;
	ID3D10DepthStencilView* dsv;
};

struct gpu_sampler { ID3D10SamplerState* p; };

struct gpu_context {
	gpu_array<gpu_shader, MAX_SHADERS>					shader;
	gpu_array<gpu_blend_state, MAX_BLEND_STATES>		blend_state;
	gpu_array<gpu_depth_state, MAX_DEPTH_STATES>		depth_state;
	gpu_array<gpu_raster_state, MAX_RASTER_STATES>		raster_state;
	gpu_array<gpu_vertex_buffer, MAX_VERTEX_BUFFERS>	vertex_buffer;
	gpu_array<gpu_texture, MAX_TEXTURES>				texture;
	gpu_array<gpu_sampler, MAX_SAMPLERS>				sampler;

	HWND				hwnd;
	IDXGISwapChain*		swap_chain;
	ID3D10Device*		device;

	::texture			backbuffer;
	pipeline			active_pipeline;

	ID3D10Buffer*		cb[MAX_CONSTANTS];
};

gpu_context g_gpu;

// swap chain helpers

static void gpu_swap_chain_desc(DXGI_SWAP_CHAIN_DESC* scd, HWND hwnd, vec2i view_size) {
	scd->BufferDesc.Width		= view_size.x;
	scd->BufferDesc.Height		= view_size.y;
	scd->BufferDesc.RefreshRate	= { 1, 60 };
	scd->BufferDesc.Format		= DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
	scd->SampleDesc				= { 1, 0 };
	scd->BufferUsage			= DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd->BufferCount			= 1;
	scd->OutputWindow			= hwnd;
	scd->Windowed				= TRUE;
	scd->SwapEffect				= DXGI_SWAP_EFFECT_DISCARD;
	scd->Flags					= 0;
}

static void acquire_backbuffer() {
	g_gpu.backbuffer = gpu_discard_on_reset(texture(g_gpu.texture.alloc()));

	if (!g_gpu.backbuffer)
		panic("acquire_backbuffer: out of space");

	gpu_texture* t = g_gpu.texture[g_gpu.backbuffer];

	if (FAILED(g_gpu.swap_chain->GetBuffer(0, __uuidof(ID3D10Texture2D), (void**)&t->p)))
		panic("acquire_backbuffer: GetBuffer failed");

	if (FAILED(g_gpu.device->CreateRenderTargetView(t->p, 0, &t->rtv)))
		panic("acquire_backbuffer: CreateRenderTargetView failed");
}

// init

void gpu_init(void* hwnd, vec2i view_size) {
	g_gpu.hwnd = (HWND)hwnd;

	HMODULE d3d = LoadLibrary(L"d3d10.dll");

	if (!d3d)
		panic("d3d10.dll not found - do you have directx10 installed?");

	auto d3d_create = (decltype(&D3D10CreateDeviceAndSwapChain))GetProcAddress(d3d, "D3D10CreateDeviceAndSwapChain");

	if (!d3d_create)
		panic("D3D10CreateDeviceAndSwapChain: not found in d3d10.dll");

	DXGI_SWAP_CHAIN_DESC scd = { };
	gpu_swap_chain_desc(&scd, g_gpu.hwnd, view_size);

	u32 flags = 0;

#ifdef _DEBUG
	flags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	if (FAILED(d3d_create(0, D3D10_DRIVER_TYPE_HARDWARE, 0, flags, D3D10_SDK_VERSION, &scd, &g_gpu.swap_chain, &g_gpu.device)))
		panic("D3D10CreateDeviceAndSwapChain: failed");

	IDXGIDevice1* dxgi_device;
	if (SUCCEEDED(g_gpu.swap_chain->GetDevice(__uuidof(IDXGIDevice1), (void**)&dxgi_device))) {
		dxgi_device->SetMaximumFrameLatency(1);
		dxgi_device->Release();
	}

	acquire_backbuffer();

	for(int i = 0; i < MAX_CONSTANTS; i++) {
		D3D10_BUFFER_DESC bd = { };

		bd.ByteWidth		= sizeof(mat44);
		bd.Usage			= D3D10_USAGE_DYNAMIC;
		bd.BindFlags		= D3D10_BIND_CONSTANT_BUFFER;
		bd.CPUAccessFlags	= D3D10_CPU_ACCESS_WRITE;
		bd.MiscFlags		= 0;

		if (FAILED(g_gpu.device->CreateBuffer(&bd, 0, &g_gpu.cb[i])))
			panic("gpu_create_vertex_buffer: CreateBuffer failed");
	}
}

void gpu_shutdown() {
	g_gpu.device->ClearState();

	for(int i = 1; i < MAX_SHADERS; i++)		gpu_destroy(shader(i));
	for(int i = 1; i < MAX_BLEND_STATES; i++)	gpu_destroy(blend_state(i));
	for(int i = 1; i < MAX_DEPTH_STATES; i++)	gpu_destroy(depth_state(i));
	for(int i = 1; i < MAX_RASTER_STATES; i++)	gpu_destroy(raster_state(i));
	for(int i = 1; i < MAX_VERTEX_BUFFERS; i++)	gpu_destroy(vertex_buffer(i));
	for(int i = 1; i < MAX_TEXTURES; i++)		gpu_destroy(texture(i));
	for(int i = 1; i < MAX_SAMPLERS; i++)		gpu_destroy(sampler(i));

	for(int i = 0; i < MAX_CONSTANTS; i++) {
		g_gpu.cb[i]->Release();
		g_gpu.cb[i] = 0;
	}

	g_gpu.swap_chain->Release();
	g_gpu.device->Release();

	g_gpu.swap_chain = 0;
	g_gpu.device     = 0;
	g_gpu.hwnd       = 0;
}

// reset

void gpu_reset(vec2i view_size) {
	if (!g_gpu.device)
		return;

	for(int i = 1; i < MAX_TEXTURES; i++) {
		if (g_gpu.texture.discard_on_reset(i))
			gpu_destroy(texture(i));
	}

	if (FAILED(g_gpu.swap_chain->ResizeBuffers(1, view_size.x, view_size.y, DXGI_FORMAT_UNKNOWN, 0))) {
		debug("gpu_reset: ResizeBuffers failed");
	}

	acquire_backbuffer();
}

// present

void gpu_present() {
	g_gpu.swap_chain->Present(1, 0);
}

// input_layout

void input_layout::element(int offset_, gpu_type type_, gpu_usage usage_, int usage_index_) {
	assert(count < MAX_COUNT);

	offset[count]		= offset_;
	type[count]			= type_;
	usage[count]		= usage_;
	usage_index[count]	= usage_index_;

	count++;
}

static void input_layout_to_dx(D3D10_INPUT_ELEMENT_DESC* ied, input_layout* il) {
	for(int i = 0; i < il->count; i++) {
		DXGI_FORMAT format = DXGI_FORMAT_R32_FLOAT;

		switch(il->type[i]) {
			case gpu_type::F32_1:	format = DXGI_FORMAT_R32_FLOAT;				break;
			case gpu_type::F32_2:	format = DXGI_FORMAT_R32G32_FLOAT;			break;
			case gpu_type::F32_3:	format = DXGI_FORMAT_R32G32B32_FLOAT;		break;
			case gpu_type::F32_4:	format = DXGI_FORMAT_R32G32B32A32_FLOAT;	break;
			case gpu_type::COLOUR:	format = DXGI_FORMAT_R8G8B8A8_UNORM;		break;
			case gpu_type::U8_4:	format = DXGI_FORMAT_R8G8B8A8_UINT;			break;
			case gpu_type::U8_4_N:	format = DXGI_FORMAT_R8G8B8A8_UNORM;		break;
			case gpu_type::I16_2:	format = DXGI_FORMAT_R16G16_SINT;			break;
			case gpu_type::I16_4:	format = DXGI_FORMAT_R16G16B16A16_SINT;		break;
			case gpu_type::I16_2_N:	format = DXGI_FORMAT_R16G16_SNORM;			break;
			case gpu_type::I16_4_N:	format = DXGI_FORMAT_R16G16B16A16_SNORM;	break;
			case gpu_type::U16_2_N:	format = DXGI_FORMAT_R16G16_UNORM;			break;
			case gpu_type::U16_4_N:	format = DXGI_FORMAT_R16G16B16A16_UNORM;	break;
			default: panic("unknown type"); break;
		}

		const char* semantic_name = "";
		int semantic_index = 0;

		switch(il->usage[i]) {
			case gpu_usage::POSITION:	semantic_name = "POSITION"; break;
			case gpu_usage::NORMAL:		semantic_name = "NORMAL";   break;
			case gpu_usage::UV:			semantic_name = "TEXCOORD"; break;
			case gpu_usage::COLOUR:		semantic_name = "COLOR";    break;
			default: panic("unknown semantic"); break;
		}

		D3D10_INPUT_ELEMENT_DESC* e = &ied[i];

		e->SemanticName			= semantic_name;
		e->SemanticIndex		= semantic_index;
		e->Format				= format;
		e->InputSlot			= 0;
		e->AlignedByteOffset	= il->offset[i];
		e->InputSlotClass		= D3D10_INPUT_PER_VERTEX_DATA;
		e->InstanceDataStepRate	= 0;
	}
}

// create

shader gpu_create_shader(input_layout* il, const u8* vs_code, int vs_size, const u8* ps_code, int ps_size) {
	int si = g_gpu.shader.alloc();

	if (!si)
		panic("gpu_create_shader: out of space");

	gpu_shader* s = g_gpu.shader[si];

	if (FAILED(g_gpu.device->CreateVertexShader(vs_code, vs_size, &s->vertex)))
		panic("gpu_create_shader: CreateVertexShader failed");

	if (FAILED(g_gpu.device->CreatePixelShader(ps_code, ps_size, &s->pixel)))
		panic("gpu_create_shader: CreateVertexShader failed");

	D3D10_INPUT_ELEMENT_DESC ied[input_layout::MAX_COUNT];
	input_layout_to_dx(ied, il);

	if (FAILED(g_gpu.device->CreateInputLayout(ied, il->count, vs_code, vs_size, &s->layout)))
		panic("gpu_create_shader: CreateInputLayout failed");

	return si;
}

blend_state gpu_create_blend_state(bool alpha_blend_enable, gpu_blend src, gpu_blend dst, gpu_blend_op op, gpu_blend src_a, gpu_blend dst_a, gpu_blend_op op_a) {
	int bsi = g_gpu.blend_state.alloc();

	if (!bsi)
		panic("gpu_create_blend_state: out of space");

	gpu_blend_state* bs = g_gpu.blend_state[bsi];

	D3D10_BLEND_DESC bd = { };

	bd.BlendEnable[0]			= alpha_blend_enable;
	bd.SrcBlend					= to_dx(src);
	bd.DestBlend				= to_dx(dst);
	bd.BlendOp					= to_dx(op);
	bd.SrcBlendAlpha			= to_dx(src_a);
	bd.DestBlendAlpha			= to_dx(dst_a);
	bd.BlendOpAlpha				= to_dx(op_a);
	bd.RenderTargetWriteMask[0]	= D3D10_COLOR_WRITE_ENABLE_ALL;

	if (FAILED(g_gpu.device->CreateBlendState(&bd, &bs->p)))
		panic("gpu_create_blend_state: CreateBlendState failed");

	return bsi;
}

depth_state gpu_create_depth_state(bool depth_enable, bool depth_write) {
	int dsi = g_gpu.depth_state.alloc();

	if (!dsi)
		panic("gpu_create_depth_state: out of space");

	gpu_depth_state* ds = g_gpu.depth_state[dsi];

	D3D10_DEPTH_STENCIL_DESC dd = { };

	dd.DepthEnable		= depth_enable;
	dd.DepthWriteMask	= depth_write ? D3D10_DEPTH_WRITE_MASK_ALL : D3D10_DEPTH_WRITE_MASK_ZERO;
	dd.DepthFunc		= D3D10_COMPARISON_LESS;

	if (FAILED(g_gpu.device->CreateDepthStencilState(&dd, &ds->p)))
		panic("gpu_create_depth_state: CreateDepthStencilState failed");

	return dsi;
}

raster_state gpu_create_raster_state(gpu_cull cull, bool fill_solid) {
	int rsi = g_gpu.raster_state.alloc();

	if (!rsi)
		panic("gpu_create_raster_state: out of space");

	gpu_raster_state* rs = g_gpu.raster_state[rsi];

	D3D10_RASTERIZER_DESC rd = { };

	rd.CullMode					= to_dx(cull);
	rd.FillMode					= fill_solid ? D3D10_FILL_SOLID : D3D10_FILL_WIREFRAME;
	rd.FrontCounterClockwise	= TRUE;
	rd.DepthClipEnable			= TRUE;

	if (FAILED(g_gpu.device->CreateRasterizerState(&rd, &rs->p)))
		panic("gpu_create_raster_state: CreateRasterizerState failed");

	return rsi;
}

pipeline gpu_create_pipeline(shader shader, blend_state blend, depth_state depth, raster_state raster, gpu_primitive primitive) {
	return pipeline { shader, blend, depth, raster, primitive };
}

vertex_buffer gpu_create_vertex_buffer(int vertex_stride, int num_vertices) {
	int vbi = g_gpu.vertex_buffer.alloc();

	if (!vbi)
		panic("gpu_create_vertex_buffer: out of space");

	gpu_vertex_buffer* vb = g_gpu.vertex_buffer[vbi];

	D3D10_BUFFER_DESC bd = { };

	bd.ByteWidth		= vertex_stride * num_vertices;
	bd.Usage			= D3D10_USAGE_DYNAMIC;
	bd.BindFlags		= D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags	= D3D10_CPU_ACCESS_WRITE;
	bd.MiscFlags		= 0;

	if (FAILED(g_gpu.device->CreateBuffer(&bd, 0, &vb->p)))
		panic("gpu_create_vertex_buffer: CreateBuffer failed");

	vb->stride = vertex_stride;

	return vbi;
}

texture gpu_create_texture(int width, int height, gpu_format fmt, u32 bind_flags, u8* initial_data) {
	int ti = g_gpu.texture.alloc();

	if (!ti)
		panic("gpu_create_texture: out of space");

	gpu_texture* t = g_gpu.texture[ti];

	D3D10_TEXTURE2D_DESC td = { };

	td.Width			= width;
	td.Height			= height;
	td.MipLevels		= 1;
	td.ArraySize		= 1;
	td.Format			= to_dx(fmt);
	td.SampleDesc		= { 1, 0 };
	td.Usage			= initial_data ? D3D10_USAGE_IMMUTABLE : D3D10_USAGE_DEFAULT;
	td.BindFlags		= 0;
	td.CPUAccessFlags	= 0;
	td.MiscFlags		= 0;

	if (bind_flags & gpu_bind::SHADER)        td.BindFlags |= D3D10_BIND_SHADER_RESOURCE;
	if (bind_flags & gpu_bind::RENDER_TARGET) td.BindFlags |= D3D10_BIND_RENDER_TARGET;
	if (bind_flags & gpu_bind::DEPTH_STENCIL) td.BindFlags |= D3D10_BIND_DEPTH_STENCIL;

	D3D10_SUBRESOURCE_DATA id = { };
	id.pSysMem			= initial_data;
	id.SysMemPitch		= width * 4;
	id.SysMemSlicePitch	= width * height * 4;

	assert((initial_data == 0) || (fmt == gpu_format::RGBA)); // only support rgba8 initial data for now

	if (FAILED(g_gpu.device->CreateTexture2D(&td, initial_data ? &id : 0, &t->p)))
		panic("gpu_create_texture: CreateTexture2D failed");

	if (bind_flags & gpu_bind::SHADER) {
		if (FAILED(g_gpu.device->CreateShaderResourceView(t->p, 0, &t->srv)))
			panic("gpu_create_texture: CreateShaderResourceView failed");
	}

	if (bind_flags & gpu_bind::RENDER_TARGET) {
		if (FAILED(g_gpu.device->CreateRenderTargetView(t->p, 0, &t->rtv)))
			panic("gpu_create_texture: CreateRenderTargetView failed");
	}

	if (bind_flags & gpu_bind::DEPTH_STENCIL) {
		if (FAILED(g_gpu.device->CreateDepthStencilView(t->p, 0, &t->dsv)))
			panic("gpu_create_texture: CreateDepthStencilView failed");
	}

	return ti;
}

sampler gpu_create_sampler(bool filter_linear, bool address_clamp) {
	int si = g_gpu.sampler.alloc();

	if (!si)
		panic("gpu_create_sampler: out of space");

	gpu_sampler* s = g_gpu.sampler[si];

	D3D10_SAMPLER_DESC sd = { };

	sd.Filter			= filter_linear ? D3D10_FILTER_MIN_MAG_MIP_LINEAR : D3D10_FILTER_MIN_MAG_MIP_POINT;
	sd.AddressU			= address_clamp ? D3D10_TEXTURE_ADDRESS_CLAMP : D3D10_TEXTURE_ADDRESS_WRAP;
	sd.AddressV			= address_clamp ? D3D10_TEXTURE_ADDRESS_CLAMP : D3D10_TEXTURE_ADDRESS_WRAP;
	sd.AddressW			= address_clamp ? D3D10_TEXTURE_ADDRESS_CLAMP : D3D10_TEXTURE_ADDRESS_WRAP;
	sd.ComparisonFunc = D3D10_COMPARISON_NEVER;

	if (FAILED(g_gpu.device->CreateSamplerState(&sd, &s->p)))
		panic("gpu_create_sampler: CreateSamplerState failed");

	return si;
}

// destroy

void gpu_destroy(shader si) {
	if (gpu_shader* s = g_gpu.shader[si]) {
		if (s->layout) s->layout->Release();
		if (s->vertex) s->vertex->Release();
		if (s->pixel ) s->pixel->Release();

		g_gpu.shader.free(si);
	}
}

void gpu_destroy(blend_state bsi) {
	if (gpu_blend_state* bs = g_gpu.blend_state[bsi]) {
		if (bs->p) bs->p->Release();

		g_gpu.blend_state.free(bsi);
	}
}

void gpu_destroy(depth_state dsi) {
	if (gpu_depth_state* ds = g_gpu.depth_state[dsi]) {
		if (ds->p) ds->p->Release();

		g_gpu.depth_state.free(dsi);
	}
}

void gpu_destroy(raster_state rsi) {
	if (gpu_raster_state* rs = g_gpu.raster_state[rsi]) {
		if (rs->p) rs->p->Release();

		g_gpu.raster_state.free(rsi);
	}
}

void gpu_destroy(vertex_buffer vbi) {
	if (gpu_vertex_buffer* vb = g_gpu.vertex_buffer[vbi]) {
		if (vb->p) vb->p->Release();

		g_gpu.vertex_buffer.free(vbi);
	}
}

void gpu_destroy(texture ti) {
	if (gpu_texture* t = g_gpu.texture[ti]) {
		if (t->p)   t->p->Release();
		if (t->srv) t->srv->Release();
		if (t->rtv) t->rtv->Release();
		if (t->dsv) t->dsv->Release();

		g_gpu.texture.free(ti);
	}
}

void gpu_destroy(sampler si) {
	if (gpu_sampler* s = g_gpu.sampler[si]) {
		if (s->p) s->p->Release();

		g_gpu.sampler.free(si);
	}
}

// discard on reset

texture gpu_discard_on_reset(texture ti) {
	g_gpu.texture.set_flag(ti, GPU_DISCARD_ON_RESET);
	return ti;
}

// map

void* gpu_map(vertex_buffer vbi) {
	gpu_vertex_buffer* vb = g_gpu.vertex_buffer[vbi];

	if (!vb || !vb->p)
		return 0;

	void* data = 0;

	if (FAILED(vb->p->Map(D3D10_MAP_WRITE_DISCARD, 0, &data)))
		return 0;

	return data;
}

void gpu_unmap(vertex_buffer vbi) {
	gpu_vertex_buffer* vb = g_gpu.vertex_buffer[vbi];

	if (vb && vb->p)
		vb->p->Unmap();
}

// render target

void gpu_set_render_target(texture ti, texture dsi) {
	ID3D10RenderTargetView* rtv = 0;
	ID3D10DepthStencilView* dsv = 0;

	if (gpu_texture* t = g_gpu.texture[ti])
		rtv = t->rtv;

	if (gpu_texture* ds = g_gpu.texture[dsi])
		dsv = ds->dsv;

	g_gpu.device->OMSetRenderTargets(rtv ? 1 : 0, rtv ? &rtv : 0, dsv);
}

texture gpu_backbuffer() {
	return g_gpu.backbuffer;
}

// control

void gpu_set_viewport(vec2i pos, vec2i size, vec2 depth) {
	D3D10_VIEWPORT vp;

	vp.TopLeftX	= pos.x;
	vp.TopLeftY	= pos.y;
	vp.Width	= size.x;
	vp.Height	= size.y;
	vp.MinDepth	= depth.x;
	vp.MaxDepth	= depth.y;

	g_gpu.device->RSSetViewports(1, &vp);
}

void gpu_set_pipeline(pipeline pl) {
	g_gpu.active_pipeline = pl;
}

void gpu_set_vertex_buffer(vertex_buffer vbi) {
	ID3D10Buffer*	p		= 0;
	UINT			stride	= 0;
	UINT			offset	= 0;

	if (gpu_vertex_buffer* vb = g_gpu.vertex_buffer[vbi]) {
		p		= vb->p;
		stride	= vb->stride;
	}

	g_gpu.device->IASetVertexBuffers(0, 1, &p, &stride, &offset);
}

void gpu_set_textures(texture_group tg) {
	ID3D10ShaderResourceView* srvs[GPU_TEXTURE_GROUP_SIZE] = { };

	for(int i = 0; i < GPU_TEXTURE_GROUP_SIZE; i++) {
		if (gpu_texture* t = g_gpu.texture[tg.texture[i]])
			srvs[i] = t->srv;
	}

	g_gpu.device->VSSetShaderResources(0, GPU_TEXTURE_GROUP_SIZE, srvs);
	g_gpu.device->PSSetShaderResources(0, GPU_TEXTURE_GROUP_SIZE, srvs);
}

void gpu_set_samplers(sampler_group sg) {
	ID3D10SamplerState* samplers[GPU_SAMPLER_GROUP_SIZE] = { };

	for(int i = 0; i < GPU_SAMPLER_GROUP_SIZE; i++) {
		if (gpu_sampler* s = g_gpu.sampler[sg.sampler[i]])
			samplers[i] = s->p;
	}

	g_gpu.device->VSSetSamplers(0, GPU_SAMPLER_GROUP_SIZE, samplers);
	g_gpu.device->PSSetSamplers(0, GPU_SAMPLER_GROUP_SIZE, samplers);
}

// constants

void gpu_set_const(int slot, vec4 m) {
	if ((slot < 0) || (slot >= MAX_CONSTANTS))
		return;

	ID3D10Buffer* b = g_gpu.cb[slot];

	void* data = 0;

	if (SUCCEEDED(b->Map(D3D10_MAP_WRITE_DISCARD, 0, &data))) {
		*(mat44*)data = mat44(m, m, m, m);
		b->Unmap();
	}

	g_gpu.device->VSSetConstantBuffers(0, 1, &b);
	g_gpu.device->PSSetConstantBuffers(0, 1, &b);
}

void gpu_set_const(int slot, mat44 m) {
	if ((slot < 0) || (slot >= MAX_CONSTANTS))
		return;

	ID3D10Buffer* b = g_gpu.cb[slot];

	if (b) {
		void* data = 0;

		if (SUCCEEDED(b->Map(D3D10_MAP_WRITE_DISCARD, 0, &data))) {
			*(mat44*)data = m;
			b->Unmap();
		}
	}

	g_gpu.device->VSSetConstantBuffers(0, 1, &b);
	g_gpu.device->PSSetConstantBuffers(0, 1, &b);
}

// drawing

void gpu_clear(texture ti, const rgba& c) {
	if (gpu_texture* t = g_gpu.texture[ti]) {
		if (t->rtv) g_gpu.device->ClearRenderTargetView(t->rtv, (float*)&c);
		if (t->dsv) g_gpu.device->ClearDepthStencilView(t->dsv, D3D10_CLEAR_DEPTH | D3D10_CLEAR_STENCIL, c.r, 0);
	}
}

void gpu_draw(int count, int base) {
	if (count <= 0)
		return;

	ID3D10InputLayout*       layout = 0;
	ID3D10VertexShader*      vertex = 0;
	ID3D10PixelShader*       pixel  = 0;
	ID3D10BlendState*        blend  = 0;
	ID3D10DepthStencilState* depth  = 0;
	ID3D10RasterizerState*   raster = 0;

	if (gpu_shader* s = g_gpu.shader[g_gpu.active_pipeline.shader]) {
		layout = s->layout;
		vertex = s->vertex;
		pixel  = s->pixel;
	}

	if (gpu_blend_state*  bs = g_gpu.blend_state [g_gpu.active_pipeline.blend ]) blend  = bs->p;
	if (gpu_depth_state*  ds = g_gpu.depth_state [g_gpu.active_pipeline.depth ]) depth  = ds->p;
	if (gpu_raster_state* rs = g_gpu.raster_state[g_gpu.active_pipeline.raster]) raster = rs->p;

	// todo: remember old state, diff while setting; also for textures/samplers?

	g_gpu.device->IASetPrimitiveTopology(to_dx(g_gpu.active_pipeline.primitive));
	g_gpu.device->IASetInputLayout(layout);
	g_gpu.device->VSSetShader(vertex);
	g_gpu.device->PSSetShader(pixel);
	g_gpu.device->OMSetBlendState(blend, (f32*)&rgba(1.0f), 0xFFFFFFFF);
	g_gpu.device->OMSetDepthStencilState(depth, 0);
	g_gpu.device->RSSetState(raster);

	g_gpu.device->Draw(count, base);
}

// constant mapping helpers

static DXGI_FORMAT to_dx(gpu_format format) {
	switch(format) {
		case gpu_format::RGBA:		return DXGI_FORMAT_R8G8B8A8_UNORM;
		case gpu_format::RGBA_SRGB:	return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		case gpu_format::D24S8:		return DXGI_FORMAT_D24_UNORM_S8_UINT;
		default: panic("to_dx: unknown gpu_format (%i)", format);
	}
}

static D3D10_BLEND to_dx(gpu_blend blend) {
	switch(blend) {
		case gpu_blend::ZERO:			return D3D10_BLEND_ZERO;
		case gpu_blend::ONE:			return D3D10_BLEND_ONE;
		case gpu_blend::SRC_COLOUR:		return D3D10_BLEND_SRC_COLOR;
		case gpu_blend::INV_SRC_COLOUR:	return D3D10_BLEND_INV_SRC_COLOR;
		case gpu_blend::DST_COLOUR:		return D3D10_BLEND_DEST_COLOR;
		case gpu_blend::INV_DST_COLOUR:	return D3D10_BLEND_INV_DEST_COLOR;
		case gpu_blend::SRC_ALPHA:		return D3D10_BLEND_SRC_ALPHA;
		case gpu_blend::INV_SRC_ALPHA:	return D3D10_BLEND_INV_SRC_ALPHA;
		case gpu_blend::DST_ALPHA:		return D3D10_BLEND_DEST_ALPHA;
		case gpu_blend::INV_DST_ALPHA:	return D3D10_BLEND_INV_DEST_ALPHA;
		default: panic("to_dx: unknown gpu_blend (%i)", blend);
	}
}

static D3D10_BLEND_OP to_dx(gpu_blend_op op) {
	switch(op) {
		case gpu_blend_op::ADD:				return D3D10_BLEND_OP_ADD;
		case gpu_blend_op::SUBTRACT:		return D3D10_BLEND_OP_SUBTRACT;
		case gpu_blend_op::REV_SUBTRACT:	return D3D10_BLEND_OP_REV_SUBTRACT;
		case gpu_blend_op::MIN:				return D3D10_BLEND_OP_MIN;
		case gpu_blend_op::MAX:				return D3D10_BLEND_OP_MAX;
		default: panic("to_dx: unknown gpu_blend_op (%i)", op);
	}
}

static D3D10_CULL_MODE to_dx(gpu_cull mode) {
	switch(mode) {
		case gpu_cull::NONE:	return D3D10_CULL_NONE;
		case gpu_cull::FRONT:	return D3D10_CULL_FRONT;
		case gpu_cull::BACK:	return D3D10_CULL_BACK;
		default: panic("to_dx: unknown gpu_cull (%i)", mode);
	}
}

static D3D10_PRIMITIVE_TOPOLOGY to_dx(gpu_primitive primitive) {
	switch(primitive) {
		case gpu_primitive::LINE_LIST:		return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
		case gpu_primitive::LINE_STRIP:		return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
		case gpu_primitive::TRIANGLE_LIST:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		case gpu_primitive::TRIANGLE_STRIP:	return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
		default: panic("to_dx: unknown gpu_primitive (%i)", primitive);
	}
}