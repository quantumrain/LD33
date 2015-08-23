#include "pch.h"
#include "game.h"

entity::entity(entity_type type) : _world(), _flags(), _type(type), _rot(0.0f), _radius(8.0f) { }
entity::~entity() { }
void entity::init() { }
void entity::tick(int move_clipped) { }

void entity::draw(draw_context* dc) {
	dc->translate(_pos.x, _pos.y);
	dc->rotate_z(_rot);
	dc->shape_outline(vec2(), 16, _radius, _rot, 0.5f, rgba());
}

void entity::set_pos(vec2 p) {
	_pos = p;
	_bb.min = p - _radius;
	_bb.max = p + _radius;
}

void entity::set_radius(float r) {
	_radius = r;
	_bb.min = _pos - r;
	_bb.max = _pos + r;
}

// etc

entity* spawn_entity(world* w, entity* e) {
	if (e) {
		if (e->_type == ET_PLAYER)
			w->player = (player*)e;

		e->_world = w;
		w->entities.push_back(e);
		e->init();
	}

	return e;
}

void destroy_entity(entity* e) {
	if (e) {
		e->_flags |= EF_DESTROYED;
	}
}

void entity_update(entity* e) {
	if (e->_flags & EF_DESTROYED)
		return;

	int clipped = 0;

	if (!(e->_flags & EF_NO_PHYSICS)) {
		e->_old_pos = e->_pos;
		clipped = move_entity(e, e->_pos + e->_vel * (1.0f / 60.0f));
		//e->_pos = e->_pos + e->_vel * (1.0f / 60.0f);
	}

	e->tick(clipped);
}

void entity_render(draw_context* dc, entity* e) {
	e->draw(&dc->copy());
}

int move_entity(entity* e, vec2 new_pos) {
	int clipped = 0;

	e->_bb = e->_world->map.slide(e->_bb, new_pos - e->_pos, &clipped);
	e->_pos = (e->_bb.min + e->_bb.max) * 0.5f;

	return clipped;
}