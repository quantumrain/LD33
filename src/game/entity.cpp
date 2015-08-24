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

player* find_nearest_player(world* w, vec2 p) {
	return w->player;
}

bool within(entity* a, entity* b, float d) {
	return a && b && (length_sq(a->_pos - b->_pos) < square(d));
}

player* overlaps_player(entity* e) {
	if (!e)
		return 0;

	player* p = find_nearest_player(e->_world, e->_pos);

	if (!p)
		return 0;

	float d = e->_radius * 0.95f + p->_radius * 0.95f;

	if (within(p, e, d))
		return p;

	return 0;
}

void interact_enemy_and_player(entity* e, rgba c) {
	if (player* pl = overlaps_player(e)) {
		if (pl->_dangerous) {
			fx_explosion(e->_pos, 200.0f, 40, rgba(0.0f, 1.0f), 9.0f, 20);
			fx_explosion(e->_pos, 300.0f, 10, c, 3.0f, 20);
			fx_explosion(e->_pos, 300.0f, 50, rgba(0.0f, 1.0f), 6.0f, 30);
			fx_explosion(e->_pos, 600.0f, 20, rgba(0.0f, 1.0f), 3.0f, 37);

			sound_play(sfx::UNIT_EXPLODE, 0.0f, -15.0f);

			vec2 d = normalise(pl->_pos - e->_pos);
			pl->_vel *= 0.5f;
			pl->_vel += d * 100.0f;
			destroy_entity(e);
		}
	}
}

void avoid_enemy(world* w, entity* self) {
	for(auto e : w->entities) {
		if (e == self)
			continue;

		if (e->_flags & EF_DESTROYED)
			continue;

		if ((e->_type != ET_PLAYER_BODY) && (e->_type != ET_PLAYER))
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

		float force_self	= 32.0f;
		float force_e		= 128.0f;

		self->_vel += d * force_self;

		if (e->_type == ET_PLAYER_BODY)
			force_e *= 0.5f;

		e->_vel -= d * force_e;
	}
}