#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "render.h"
#include "sound.h"
#include "tile_map.h"

#define DT (1.0f / 60.0f)

struct world;
struct entity;
struct player;
struct player_body;

enum entity_flag {
	EF_DESTROYED = 0x1
};

enum entity_type {
	ET_PLAYER,
	ET_PLAYER_BODY
};

struct world {
	random r;

	list<entity> entities;

	world();
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

struct player : entity {
	player();

	virtual void init();
	virtual void tick(int move_clipped);
	virtual void draw(draw_context* dc);

	array<player_body*> _body;
};

struct player_body : entity {
	player_body(player* head);

	virtual void init();
	virtual void tick(int move_clipped);
	virtual void draw(draw_context* dc);

	player* _head;
};

entity* spawn_entity(world* w, entity* e);
void destroy_entity(entity* e);

template<typename T> T* spawn_entity(world* w, T* e) { return (T*)spawn_entity(w, static_cast<entity*>(e)); }

void world_tick(world* w);
void world_draw(draw_context* dc, world* w);

#endif // GAME_H