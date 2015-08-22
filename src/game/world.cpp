#include "pch.h"
#include "game.h"

template<typename C, typename F> void for_all(C& c, F&& f) {
	for(int i = 0; i < c.size(); i++) {
		if (!(c[i]->_flags & EF_DESTROYED)) f(c[i]);
	}
}

template<typename C> void prune(C& c) {
	for(int i = 0; i < c.size(); ) {
		if (c[i]->_flags & EF_DESTROYED) c.swap_remove(i); else i++;
	}
}

// world

world::world() { }

// entity

entity::entity(entity_type type) : _world(), _flags(), _type(type), _rot(0.0f), _radius(8.0f) { }
entity::~entity() { }
void entity::init() { }
void entity::tick(int move_clipped) { }

void entity::draw(draw_context* dc) {
	dc->translate(_pos.x, _pos.y);
	dc->rotate_z(_rot);
	dc->shape_outline(vec2(), 16, _radius, _rot, 0.5f, rgba());
}

// etc

entity* spawn_entity(world* w, entity* e) {
	if (e) {
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

	e->_old_pos = e->_pos;
	e->_pos += e->_vel * (1.0f / 60.0f);

	e->tick(clipped);
}

void entity_render(draw_context* dc, entity* e) {
	e->draw(&dc->copy());
}

void world_tick(world* w) {
	for_all(w->entities, [](entity* e) { entity_update(e); });
	prune(w->entities);
}

void world_draw(draw_context* dc, world* w) {
	for_all(w->entities, [dc](entity* e) { entity_render(dc, e); });
}