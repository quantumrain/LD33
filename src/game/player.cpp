#include "pch.h"
#include "game.h"

player::player() : entity(ET_PLAYER) {
	_radius = 1.0f;
}

void player::init() {
}

void player::tick(int move_clipped) {
	const int MAX_LENGTH = 8;

	static float t;
	t += 0.3f * length(g_input.pad_left);
	float f = 0.5f + cosf(t) * 0.5f;

	_vel *= 0.8f;
	_vel += g_input.pad_left * 48.0f * f;

	if (_body.size() < MAX_LENGTH) {
		player_body** new_body = _body.push_back(spawn_entity(_world, new player_body(this)));

		(*new_body)->_pos = _pos;
	}

	vec2 target = _pos;

	for(int seg = 0; seg < _body.size(); seg++) {
		player_body* b = _body[seg];

		//float f = 6.0f + (cosf(t + (seg * PI)) * 4.0f);
		float ff	= (1.0f + cosf(t + (seg * PI))) * 0.5f;
		float f		= lerp(0.5f, 2.0f, ff);

		if (length_sq(target - b->_pos) < 0.001f)
			continue;

		vec2 dir = normalise(target - b->_pos);
		vec2 actual_target = target - dir * b->_radius * f;

		b->_pos = lerp(b->_pos, actual_target, 0.5f);
		//b->_vel += normalise(actual_target - b->_pos) * f * 32.0f;
		//b->_vel *= 0.5f;

		float seg_f = seg / (MAX_LENGTH - 1.0f);

		b->_radius = lerp(10.0f, 30.0f, seg_f) * lerp(1.0f, 0.15f, square(seg_f)) * 0.75f;

		target = b->_pos;
	}

	if (_body.not_empty()) {
		player_body* b = _body.front();

		if (length_sq(g_input.pad_left) > 0.01f) {
			if (length_sq(_pos - b->_pos) > 0.001f) {
				vec2 dir = normalise(_pos - b->_pos);
				vec2 actual_target = b->_pos + dir * b->_radius * 2.0f;
				_vel += (actual_target - _pos);
			}
		}
	}
}

void player::draw(draw_context* dc) {
	dc->rect(-vec2(_radius), vec2(_radius), colours::RED); 
}