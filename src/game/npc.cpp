#include "pch.h"
#include "game.h"

static float g_snake_wriggle;

float wiggle(float t, int seg);

npc::npc() : entity(ET_NPC) {
	set_radius(12.0f);
	set_pos(vec2(200.0f, 200.0f));
}

void npc::new_body_segment() {
	player_body** new_body = _body.push_back(spawn_entity(_world, new player_body));
	(*new_body)->set_pos(_pos + _world->r.range(vec2(1.0f)));
}

void npc::init() {
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

void npc::tick(int move_clipped) {
	if (player* pl = _world->player) {
		if (length_sq(pl->_pos - _pos) < square(120.0f)) {
			extern bool g_win_game;
			g_win_game = true;
		}
	}

	vec2 move_delta;

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
	
	bool fire_held = false;

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

	static int relax;

	if (relax < 10) {
		relax++;
		attack_dir = vec2(0.0f, 1.0f);
		is_attacking = true;

		for(int seg = 2; seg < _body.size(); seg++) {
			player_body* b = _body[seg];
			float f = (seg / (float)(_body.size() - 1));

			f *= lerp(1.0f, 0.0f, f);

			b->_vel -= rotation(b->_rot - wiggle(g_snake_wriggle, seg) * 1.25f) * 10.0f * f;
		}

		if (relax == 6)
			_vel += attack_dir * 600.0f;

		//player_body* front = _body.front();
		//move_entity(this, front->_pos + attack_dir * front->_render_radius);
	}
	else
		is_attacking = false;

	/*if (is_attacking) {
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
	}*/

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

			target = b->_pos;
		}

		move_sum += length(_vel) * 0.1f;

		if (length_sq(move_delta) > 0.01f) {
			if (length_sq(_pos - front->_pos) > 0.001f) {
				vec2 dir = normalise(_pos - front->_pos);
				vec2 actual_target = front->_pos + dir * front->_render_radius;
				_vel += (actual_target - _pos);
			}
		}
	}
}

void draw_body(draw_context& dc, player_body* b, bool head, bool tail, float wibble_spine, float radius);

void npc::draw(draw_context* dc) {
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