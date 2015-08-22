#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "render.h"
#include "sound.h"
#include "tile_map.h"

struct world;

enum entity_flag {
	EF_DESTROYED = 0x1
};

enum entity_type {
	ET_PLAYER
};

struct entity {
	entity(entity_type type);
	virtual ~entity();

	virtual void init();
	virtual void tick(int move_clipped);
	virtual void draw(draw_context* dc);

	world*	_world;
	u16		_flags;
	u16		_type;
	vec2	_pos;
	vec2	_old_pos;
	vec2	_vel;
	float	_rot;
	float	_radius;
	rgba	_colour;
};

struct world {
	random r;

	list<entity> entities;

	world();
};

entity* spawn_entity(world* w, entity* e);
void destroy_entity(entity* e);

void world_tick(world* w);
void world_draw(draw_context* dc, world* w);

#endif // GAME_H