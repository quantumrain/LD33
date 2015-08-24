#include "pch.h"
#include "game.h"

looper::looper() : entity(ET_LOOPER), _set_home(), _t() {
	set_radius(20.0f);
}

void looper::init() {
}

void looper::tick(int move_clipped) {
	if (!_set_home) {
		_set_home = true;
		_home = _pos;
		_t = _pos.x * 0.005f;

		entity* best = 0;
		float best_dist = 0.0f;

		for(auto& e : _world->entities) {
			if (e->_type != ET_LOOPER)
				continue;

			if (e == this)
				continue;

			float d = length_sq(_pos - e->_pos);

			if (!best || (d < best_dist)) {
				best = e;
				best_dist = d;
			}
		}

		_dir = perp(normalise(best->_pos - _pos));
	}

	_t += 0.1f;

	vec2 target = _home + (_dir * cosf(_t) * 160.0f); // search for nearest friend and use them as the angle

	_vel += (target - _pos);

	_rot += 0.025f;
	_vel *= 0.7f;

	interact_enemy_and_player(this, rgba(0.2f, 1.2f, 0.2f, 1.0f));

	avoid_enemy(_world, this);
}

void looper::draw(draw_context* dc) {
	dc->translate(_pos.x, _pos.y);
	dc->rotate_z(_rot);

	float f = 0.2f + fabsf(cosf(_rot * 3.75f));

	dc->shape(vec2(), 4, _radius * 0.75f, _rot, rgba(0.2f, 0.8f * f, 0.2f, 1.0f));
	dc->shape(vec2(), 4, _radius * 0.25f, _rot, rgba(0.2f, 0.8f * (1.0f - f), 0.2f, 1.0f));

	dc->shape_outline(vec2(), 4, _radius * 0.35f, _rot, 4.0f, rgba(0.0f, 1.0f));
	dc->shape_outline(vec2(), 4, _radius * 0.35f, _rot + TAU * 0.85f, 4.0f, rgba(0.0f, 1.0f));

	dc->shape_outline(vec2(), 4, _radius, _rot, 4.0f, rgba(0.0f, 1.0f));
	dc->shape_outline(vec2(), 4, _radius, _rot + TAU / 8.0f, 4.0f, rgba(0.0f, 1.0f));
}