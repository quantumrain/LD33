#include "pch.h"
#include "game.h"

static float g_snake_wriggle;

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
	set_radius(12.0f);
	set_pos(vec2(200.0f, 200.0f));
}

void player::new_body_segment() {
	player_body** new_body = _body.push_back(spawn_entity(_world, new player_body(this)));
	(*new_body)->set_pos(_pos + _world->r.range(vec2(1.0f)));
}

void player::init() {
	const int MAX_LENGTH = 16;

	for(int i = 0; i < MAX_LENGTH; i++)
		new_body_segment();

	for(int seg = 0; seg < _body.size(); seg++) {
		player_body* b = _body[seg];

		b->_index = seg;
		
		float seg_f = 1.0f - (seg / (MAX_LENGTH - 1.0f));
		b->_render_radius = lerp(5.0f, 50.0f, square(seg_f)) * lerp(1.0f, 0.15f, square(seg_f));

		if (seg == 0)
			b->_render_radius *= 1.3f;

		b->set_radius(b->_render_radius * 0.33f);
	}
}

void player::tick(int move_clipped) {
	// input

	vec2 pad_left = g_input.pad_left;

	if (is_key_down(KEY_L_LEFT)) pad_left.x -= 1.0f;
	if (is_key_down(KEY_L_RIGHT)) pad_left.x += 1.0f;
	if (is_key_down(KEY_L_DOWN)) pad_left.y += 1.0f;
	if (is_key_down(KEY_L_UP)) pad_left.y -= 1.0f;

	if (is_key_down(KEY_R_LEFT)) pad_left.x -= 1.0f;
	if (is_key_down(KEY_R_RIGHT)) pad_left.x += 1.0f;
	if (is_key_down(KEY_R_DOWN)) pad_left.y += 1.0f;
	if (is_key_down(KEY_R_UP)) pad_left.y -= 1.0f;

	if (length_sq(pad_left) > 1.0f)
		pad_left = normalise(pad_left);

	vec2 move_delta;
	move_delta = (vec2)(_world->view.x * pad_left.x) - (vec2)(_world->view.y * pad_left.y);

	float move_length = length(move_delta);

	if (move_length > 0.0001f) {
		move_delta *= square(move_length) / move_length;
		move_length = square(move_length);
		_rot = rotation_of(move_delta);
	}

	//

	static bool is_attacking;
	static bool attack_latch = true;
	static int attack_frame;
	static int attack_cooldown;
	static vec2 attack_dir;
	static float attack_scale_flip;
	
	bool fire_held = (g_input.pad_buttons & 1) || is_key_down(KEY_FIRE);

	if (attack_cooldown > 0)
		attack_cooldown--;

	if (fire_held) {
		if (!is_attacking && !attack_latch && (attack_cooldown <= 0)) {
			is_attacking = true;
			attack_latch = true;
			attack_frame = 0;
			attack_cooldown = 30;
			attack_dir = rotation(_body.front()->_rot);
			attack_scale_flip = !attack_scale_flip;
		}
	}
	else
		attack_latch = false;

	float input_scale_override = attack_scale_flip ? -4.0f : 4.0f;

	if (is_attacking) {
		move_delta *= 0.0f;
		attack_frame++;

		if (attack_frame < 10) {
			_vel -= attack_dir * 50.0f;

			if (_body.size() > 1) {
				for(int seg = 2; seg < _body.size(); seg++) {
					player_body* b = _body[seg];
					float f = (seg / (float)(_body.size() - 1));

					f *= lerp(1.0f, 0.0f, f);

					b->_vel -= rotation(b->_rot - wiggle(g_snake_wriggle, seg) * 1.25f) * 250.0f * f;
				}

				player_body* front = _body.front();
				move_entity(this, front->_pos + attack_dir * front->_render_radius);
			}
		}
		else if (attack_frame < 15) {
			_vel += attack_dir * 400.0f;
		}
		else if (attack_frame < 25) {
			//_vel -= attack_dir * 50.0f;
			input_scale_override = lerp(3.0f, 0.0f, (attack_frame - 15) / 10.0f);
		}
		else
			is_attacking = false;
	}

	g_snake_wriggle += 0.2f * move_length;
	float move_wriggle_scale = (0.65f + cosf(g_snake_wriggle) * 0.35f);
	_vel *= 0.8f;
	_vel += move_delta * 64.0f * move_wriggle_scale; // slightly different to standard wiggle

	if (_body.not_empty()) {
		player_body* front = _body.front();
		vec2 target = _pos;

		float input_shake_scale = sqrtf(move_length);

		static float anyway;
		anyway += 0.01f;

		float move_sum = 0.0f;

		for(int seg = 0; seg < _body.size(); seg++) {
			player_body* b = _body[seg];

			b->_old_pos = b->_pos;
			b->_pos += b->_vel * (1.0f / 60.0f);
			b->_vel *= 0.8f;

			float ff	= wiggle(g_snake_wriggle, seg);
			float f		= lerp(0.75f, 1.25f, ff);

			if (length_sq(target - b->_pos) < 0.001f)
				continue;

			vec2 dir = normalise(target - b->_pos);

			if (is_attacking && (seg == 0))
				dir = attack_dir;

			float shake_scale	= (seg == 0) ? 0.05f : 0.1f;
			float shake_offset	= (seg == 0) ? (PI / 2) : 0.1f;

			//shake_scale *= max(input_shake_scale, square((seg + 1.0f) / MAX_LENGTH) * 0.1f); // TODO: FIX IN MORNING, need to fix collision so coiling when stationary will work
			if (is_attacking)
				input_shake_scale = input_scale_override;

			shake_scale *= input_shake_scale;

			dir = rotation(rotation_of(dir) + cosf(-g_snake_wriggle + anyway + seg * TAU / 8.0f + shake_offset) * shake_scale);

			vec2 actual_target = target - dir * b->_render_radius * f;

			b->_pos = lerp(b->_pos, actual_target, 0.9f);
			b->_rot = rotation_of(dir);

			move_sum += length(b->_pos - b->_old_pos);

			avoid_player(_world, b);

			target = b->_pos;
		}

		move_sum += length(_vel) * 0.1f;

		static float grind_lim = 100.0f;

		if (is_attacking)
			grind_lim += (5.0f - grind_lim) * 0.5f;
		else
			grind_lim += (100.0f - grind_lim) * 0.2f;

		float grind_vol		= -15.0f - clamp(powf(150.0f, 5.0f) / (powf(move_sum, 5.0f) + 0.1f), grind_lim, 1000.0f);
		float grind2_vol	= -5.0f - clamp(3000000.0f / (square(square(move_sum)) + 0.1f), 10.0f, 30.0f);
		float grind3_vol	= -5.0f - clamp(powf(150.0f, 5.0f) / (powf(move_sum, 5.0f) + 0.1f), 10.0f, 1000.0f);

		float grind_wriggle_scale = clamp(0.5f + cosf(g_snake_wriggle) * 0.65f, 0.0f, 1.0f);
		grind2_vol -= (clamp((move_sum - 40.0f) / 50.0f, 0.0f, 1.0f) * grind_wriggle_scale) * 30.0f;

		extern voice_id g_sound_grind;
		extern voice_id g_sound_grind2;
		extern voice_id g_sound_grind3;
		audio_set_volume(g_sound_grind, grind_vol);
		audio_set_volume(g_sound_grind2, grind2_vol);
		audio_set_volume(g_sound_grind3, grind3_vol);
		audio_set_frequency(g_sound_grind, (square(move_sum / 100.0f)) - 8.0f);

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

		float ff	= wiggle(g_snake_wriggle, seg);
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

	rgba spine_colour(0xFF000000);//0xFF6430FF);
	rgba body_colour(0xFF000000);//FFA3CE27);
	rgba markings_colour(0xFF323232);//0xFF6430FF);

	draw_context dc_spine = dc.copy().translate(radius * spine_pos, 0.0f);

	dc_spine.copy().rotate_z(deg_to_rad(-55.0f) * (spine_sweep0 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f, -2.0f), vec2(0.10f, 0.0f) }, rgba(spine_colour));
	dc_spine.copy().rotate_z(deg_to_rad( 55.0f) * (spine_sweep0 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f,  2.0f), vec2(0.10f, 0.0f) }, rgba(spine_colour));
	dc_spine.copy().rotate_z(deg_to_rad(-25.0f) * (spine_sweep1 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f, -1.5f), vec2(0.10f, 0.0f) }, rgba(spine_colour));
	dc_spine.copy().rotate_z(deg_to_rad( 25.0f) * (spine_sweep1 + wibble_spine)).scale(spine_scale).fill({ vec2(-0.10f, 0.0f), vec2( 0.00f,  1.5f), vec2(0.10f, 0.0f) }, rgba(spine_colour));

	if (tail) {
		draw_context dc_tail = dc.copy().translate(-radius * 0.5f, 0.0f).scale(radius * 0.5f); // todo: want this to rotate, so maybe make it a final body piece?

		dc_tail.fill({ vec2(0.0f, -0.2f), vec2(-4.0f, 0.0f), vec2(0.0f, 0.2f) }, rgba(spine_colour));
		dc_tail.fill({ vec2(1.0f, -1.0f), vec2(-1.5f, 0.0f), vec2(1.0f, 1.0f) }, rgba(body_colour));
	}

	if (head) {
		dc.shape(vec2(radius * 0.25f, 0.0f), 3, radius, 0.0f, rgba(body_colour));	
	}

	dc.shape(vec2(radius * 0.5f, 0.0f), 3, radius, 0.0f, rgba(body_colour));
	dc.shape(vec2(0.0f, 0.0f), 4, radius * 0.5f, 0.0f, rgba(markings_colour));
	dc.shape(vec2(0.0f, 0.0f), 4, radius * 0.25f, 0.0f, rgba(body_colour));
}