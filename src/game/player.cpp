#include "pch.h"
#include "game.h"

player::player() : entity(ET_PLAYER) {
	_radius = 1.0f;
}

void player::init() {
}

static float t;

void player::tick(int move_clipped) {
	const int MAX_LENGTH = 16;
	
	t += 0.3f * length(g_input.pad_left);
	float f = 0.5f + cosf(t) * 0.5f;

	_vel *= 0.8f;
	_vel += g_input.pad_left * 48.0f * f;

	if (_body.size() < MAX_LENGTH) {
		player_body** new_body = _body.push_back(spawn_entity(_world, new player_body(this)));

		(*new_body)->_pos = _pos;
	}

	for(int seg = 0; seg < _body.size(); seg++) {
		player_body* b = _body[seg];
		float seg_f = 1.0f - (seg / (MAX_LENGTH - 1.0f));
		b->_radius = lerp(5.0f, 50.0f, square(seg_f)) * lerp(1.0f, 0.15f, square(seg_f));

		if (seg == 0)
			b->_radius *= 1.5f;
	}

	float max_head_dist = 2.0f;

	vec2 target = _pos;
	float max_dist = max_head_dist;

	for(int seg = 0; seg < _body.size(); seg++) {
		player_body* b = _body[seg];

		//float f = 6.0f + (cosf(t + (seg * PI)) * 4.0f);
		float ff	= (1.0f + cosf(t + (seg * PI))) * 0.5f;
		float f		= lerp(0.75f, 1.25f, ff);

		if (length_sq(target - b->_pos) < 0.001f)
			continue;

		vec2 dir = normalise(target - b->_pos);
		vec2 actual_target = target - dir * b->_radius * f;

		b->_pos = lerp(b->_pos, actual_target, 0.9f);
		b->_rot = rotation_of(dir);

		target = b->_pos;
		max_dist = b->_radius;
	}

	if (_body.not_empty()) {
		player_body* b = _body.front();

		if (length_sq(g_input.pad_left) > 0.01f) {
			if (length_sq(_pos - b->_pos) > 0.001f) {
				vec2 dir = normalise(_pos - b->_pos);
				vec2 actual_target = b->_pos + dir * b->_radius * max_head_dist;
				_vel += (actual_target - _pos);
			}
		}
	}
}

void draw_body(draw_context& dc, player_body* b, bool head, bool tail, float wibble_spine) {
	float s = b->_radius;
	float h = b->_radius * 0.5f;

	dc.translate(-s * 0.25f, 0.0);

	float spine_scale = s;
	float spine_sweep = 1.0f;

	if (head) {
		h /= 1.5f;
		spine_scale *= 2.0f;
		spine_sweep *= 2.0f;
	}

	draw_context dc_spine = dc.copy().translate(s * 0.25f, 0.0f);

	dc_spine.copy().rotate_z(deg_to_rad(-55.0f) * (spine_sweep + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f, -2.0f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));
	dc_spine.copy().rotate_z(deg_to_rad( 55.0f) * (spine_sweep + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f,  2.0f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));
	dc_spine.copy().rotate_z(deg_to_rad(-25.0f) * (spine_sweep + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f, -1.5f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));
	dc_spine.copy().rotate_z(deg_to_rad( 25.0f) * (spine_sweep + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f,  1.5f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));

	if (tail) {
		draw_context dc_tail = dc.copy().translate(-h, 0.0f).scale(s * 0.5f);

		// todo: want this to rotate, so maybe make it a final body piece?

		dc_tail.fill({ vec2(0.0f, -0.2f), vec2(-4.0f, 0.0f), vec2(0.0f, 0.2f) }, rgba(0xFF6430FF));

		float w = 1.0f;
		float t0 = 1.0f;
		float t1 = -1.5f;
		dc_tail.fill({ vec2(t0, -w), vec2(t1, 0.0f), vec2(t0, w) }, rgba(0xFFA3CE27));

		//dc_tail.fill({ vec2(1.0f, -0.75f), vec2(-1.5f, 0.0f), vec2(1.0f, 0.75f) }, rgba(0xFFA3CE27));


		//dc_tail.shape(vec2(0.0f, 0.0f), 3, 1.0f, PI, rgba(0xFFA3CE27));
	}

	dc.shape(vec2(h, 0.0f), 3, s, 0.0f, rgba(0xFFA3CE27));
	dc.shape(vec2(0.0f, 0.0f), 4, s * 0.5f, 0.0f, rgba(0xFF6430FF));
	dc.shape(vec2(0.0f, 0.0f), 4, s * 0.25f, 0.0f, rgba(0xFFA3CE27));
}

void player::draw(draw_context* dc) {
	static float wibble_body;
	static float wibble_spine;

	wibble_body += 0.015f;
	wibble_spine += 0.025f;

	for(int seg = _body.size() - 1; seg >= 0; seg--) {
		player_body* b = _body[seg];

		bool head = seg == 0;
		bool tail = seg == (_body.size() - 1);

		float rot_wibble_body	= cosf(wibble_body + seg) * 0.05f;
		float rot_wibble_spine	= cosf(wibble_spine + seg * 1.25f) * 0.05f;

		if (head)
			rot_wibble_body = 0.0f;

		float ff	= (1.0f + cosf(t + (seg * PI))) * 0.5f;
		float f		= lerp(0.75f, 1.25f, ff);
		if (head) f=1.0f;

		draw_context dcc = dc->copy().translate(b->_pos.x, b->_pos.y).rotate_z(b->_rot + rot_wibble_body);
		dcc.push(scale(vec3(f, 1.0f, 1.0f)));
		draw_body(dcc, b, head, tail, rot_wibble_spine);
	}
}