#include "pch.h"
#include "game.h"

static float t;

float wiggle(float t, int seg) {
	return (1.0f + cosf(t + (seg * TAU / 4.0f))) * 0.5f;
}

void avoid_player(world* w, entity* self) {
	for(auto e : w->entities) {
		if (e == self)
			continue;

		if (e->_flags & EF_DESTROYED)
			continue;

		if (e->_type != ET_PLAYER_BODY)
			continue;

		vec2	d		= self->_pos - e->_pos;
		float	min_l	= self->_radius + e->_radius;

		if (length_sq(d) > square(min_l))
			continue;

		if ((self->_type == ET_PLAYER) && (((player_body*)e)->_index == 0))
			continue;

		float l = length(d);

		if (l < 0.0001f)
			d = vec2(1.0f, 0.0f);
		else
			d /= l;

		float force_self	= 16.0f;
		float force_e		= 16.0f;

		self->_vel += d * force_self;
		e->_vel -= d * force_e;
	}
}

player::player() : entity(ET_PLAYER) {
	_radius = 12.0f;
}

void player::init() {
}

void player::tick(int move_clipped) {
	const int MAX_LENGTH = 16;

	vec2 move_delta = g_input.pad_left;
	float move_length = length(move_delta);

	if (move_length > 0.0001f) {
		move_delta *= square(move_length) / move_length;
		move_length = square(move_length);
	}

	t += 0.2f * move_length;

	_vel *= 0.8f;
	_vel += move_delta * 64.0f * (0.65f + cosf(t) * 0.35f); // slightly different to standard wiggle

	if (_body.size() < MAX_LENGTH) {
		player_body** new_body = _body.push_back(spawn_entity(_world, new player_body(this)));

		(*new_body)->_pos = _pos;
	}

	for(int seg = 0; seg < _body.size(); seg++) {
		player_body* b = _body[seg];

		b->_index = seg;

		float seg_f = 1.0f - (seg / (MAX_LENGTH - 1.0f));
		b->_render_radius = lerp(5.0f, 50.0f, square(seg_f)) * lerp(1.0f, 0.15f, square(seg_f));

		if (seg == 0)
			b->_render_radius *= 1.5f;

		b->_radius = b->_render_radius * 0.33f;
	}

	if (_body.not_empty()) {
		player_body* front = _body.front();
		vec2 target = _pos;

		float input_shake_scale = sqrtf(move_length);

		static float anyway;
		anyway += 0.01f;

		for(int seg = 0; seg < _body.size(); seg++) {
			player_body* b = _body[seg];

			float ff	= wiggle(t, seg);
			float f		= lerp(0.75f, 1.25f, ff);

			if (length_sq(target - b->_pos) < 0.001f)
				continue;

			vec2 dir = normalise(target - b->_pos);

			float shake_scale	= (seg == 0) ? 0.05f : 0.1f;
			float shake_offset	= (seg == 0) ? (PI / 2) : 0.1f;

			//shake_scale *= max(input_shake_scale, square((seg + 1.0f) / MAX_LENGTH) * 0.1f); // TODO: FIX IN MORNING, need to fix collision so coiling when stationary will work
			shake_scale *= input_shake_scale;

			dir = rotation(rotation_of(dir) + cosf(-t + anyway + seg * TAU / 8.0f + shake_offset) * shake_scale);

			vec2 actual_target = target - dir * b->_render_radius * f;

			b->_pos = lerp(b->_pos, actual_target, 0.9f);
			b->_rot = rotation_of(dir);

			avoid_player(_world, b);

			target = b->_pos;
		}

		if (length_sq(move_delta) > 0.01f) {
			if (length_sq(_pos - front->_pos) > 0.001f) {
				vec2 dir = normalise(_pos - front->_pos);
				vec2 actual_target = front->_pos + dir * front->_render_radius;
				_vel += (actual_target - _pos);
			}
		}
	}

	avoid_player(_world, this);
}

void draw_body(draw_context& dc, player_body* b, bool head, bool tail, float wibble_spine, float radius);

void player::draw(draw_context* dc) {
	//entity::draw(&dc->copy());

	static float wibble_body;
	static float wibble_spine;

	wibble_body += 0.015f * 4.0f;
	wibble_spine += 0.025f * 2.0f;

	for(int seg = _body.size() - 1; seg >= 0; seg--) {
		player_body* b = _body[seg];

		bool head = seg == 0;
		bool tail = seg == (_body.size() - 1);

		float rot_wibble_body	= cosf(wibble_body + seg) * 0.05f * 3.0f;
		float rot_wibble_spine	= cosf(wibble_spine + seg * 1.25f) * 0.05f * 1.5f;

		if (head)
			rot_wibble_body = 0.0f;

		float ff	= wiggle(t, seg);
		float f		= lerp(0.75f, 1.25f, 1.0f - square(1.0f - ff));
		float f2	= lerp(1.15f, 0.85f, square(ff));
		if (head) { f=1.0f; f2 = 1.0f; }

		draw_context dcc = dc->copy().translate(b->_pos.x, b->_pos.y).rotate_z(b->_rot + rot_wibble_body);
		dcc.push(scale(vec3(f, f2, 1.0f)));
		draw_body(dcc, b, head, tail, rot_wibble_spine, b->_render_radius);
	}
}

void draw_body(draw_context& dc, player_body* b, bool head, bool tail, float wibble_spine, float radius) {
	dc.translate(-radius * 0.25f, 0.0);

	float spine_scale	= radius;
	float spine_sweep0	= 1.0f;
	float spine_sweep1	= 1.0f;
	float spine_pos		= 0.25f;

	if (head) {
		spine_scale		*= 2.0f;
		spine_sweep0	*= 1.93f;
		spine_sweep1	*= 2.16f;
		spine_pos		*= 2.0f;
	}

	draw_context dc_spine = dc.copy().translate(radius * spine_pos, 0.0f);

	dc_spine.copy().rotate_z(deg_to_rad(-55.0f) * (spine_sweep0 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f, -2.0f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));
	dc_spine.copy().rotate_z(deg_to_rad( 55.0f) * (spine_sweep0 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f,  2.0f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));
	dc_spine.copy().rotate_z(deg_to_rad(-25.0f) * (spine_sweep1 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f, -1.5f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));
	dc_spine.copy().rotate_z(deg_to_rad( 25.0f) * (spine_sweep1 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f,  1.5f), vec2(0.10f, 0.0f) }, rgba(0xFF6430FF));

	if (tail) {
		draw_context dc_tail = dc.copy().translate(-radius * 0.5f, 0.0f).scale(radius * 0.5f); // todo: want this to rotate, so maybe make it a final body piece?

		dc_tail.fill({ vec2(0.0f, -0.2f), vec2(-4.0f, 0.0f), vec2(0.0f, 0.2f) }, rgba(0xFF6430FF));
		dc_tail.fill({ vec2(1.0f, -1.0f), vec2(-1.5f, 0.0f), vec2(1.0f, 1.0f) }, rgba(0xFFA3CE27));
	}

	if (head) {
		dc.shape(vec2(radius * 0.25f, 0.0f), 3, radius, 0.0f, rgba(0xFFA3CE27));	
	}

	dc.shape(vec2(radius * 0.5f, 0.0f), 3, radius, 0.0f, rgba(0xFFA3CE27));
	dc.shape(vec2(0.0f, 0.0f), 4, radius * 0.5f, 0.0f, rgba(0xFF6430FF));
	dc.shape(vec2(0.0f, 0.0f), 4, radius * 0.25f, 0.0f, rgba(0xFFA3CE27));
}