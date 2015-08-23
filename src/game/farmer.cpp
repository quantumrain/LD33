#include "pch.h"
#include "game.h"

void avoid_farmer(world* w, entity* self) {
	for(auto e : w->entities) {
		if (e == self)
			continue;

		if (e->_flags & EF_DESTROYED)
			continue;

		if (e->_type != ET_PLAYER_BODY)
			continue;

		vec2	d		= self->_pos - e->_pos;
		float	min_l	= (self->_radius * 0.5f) + e->_radius;

		if (length_sq(d) > square(min_l))
			continue;

		float l = length(d);

		if (l < 0.0001f)
			d = vec2(1.0f, 0.0f);
		else
			d /= l;

		float force_self = 32.0f;
		self->_vel += d * force_self;
	}
}

farmer::farmer() : entity(ET_FARMER), _fleeing(), _set_home() {
	set_radius(20.0f);
}

void farmer::init() {
}

void farmer::tick(int move_clipped) {
	if (!_set_home) {
		_set_home = true;
		_home = _pos;
	}

	if (player* pl = _world->player) {
		vec2 delta = _pos - pl->_pos;
		float dist = length(delta);

		if (dist < 100.0f) {
			_fleeing = 100;
			_waiting = 180;
		}

		if ((dist > 0.1f) && (_fleeing > 0)) {
			if (dist < 200.0f)
				_fleeing = 100;

			_fleeing--;
			_vel += normalise(delta) * 20.0f;
		}
		else {
			if (_waiting > 0) {
				_waiting--;
			}
		}
	}

	if (_waiting <= 0) {
		vec2 delta = _home - _pos;
		float dist = length(delta);

		if (dist > 10.0f) {
			_vel += normalise(delta) * 20.0f;
		}
	}

	if (length_sq(_vel) > 0.1f)
		_rot = rotation_of(_vel);

	_vel *= 0.8f;

	avoid_farmer(_world, this);
}

void farmer::draw(draw_context* dc) {
//	entity::draw(&dc->copy());

	dc->set(g_farmer);
	draw_tex_tile(*dc, _pos, 48.0f, _rot + PI * 0.5f, rgba(1.0f, 1.0f), 0);
}