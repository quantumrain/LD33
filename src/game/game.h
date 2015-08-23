#ifndef GAME_H
#define GAME_H

#include "base.h"
#include "render.h"
#include "sound.h"
#include "tile_map.h"

#define DT (1.0f / 60.0f)

enum tile_type {
	TILE_EMPTY,
	TILE_WALL
};

struct world;
struct entity;
struct player;
struct player_body;

enum entity_flag {
	EF_DESTROYED = 0x1,
	EF_NO_PHYSICS = 0x2
};

enum entity_type {
	ET_PLAYER,
	ET_PLAYER_BODY,
	ET_FARMER
};

struct world {
	random r;

	player* player;

	list<entity> entities;

	mat44 proj, view, proj_view;

	tile_map map;

	world();

	vec2 camera_pos;
};

struct entity {
	entity(entity_type type);
	virtual ~entity();

	virtual void init();
	virtual void tick(int move_clipped);
	virtual void draw(draw_context* dc);

	void set_pos(vec2 p);
	void set_radius(float r);

	world*	_world;
	u16		_flags;
	u16		_type;
	aabb2	_bb;
	vec2	_pos;
	vec2	_old_pos;
	vec2	_vel;
	float	_rot;
	float	_radius;
	rgba	_colour;
};

struct player : entity {
	player();

	void new_body_segment();

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
	float _render_radius;
	int _index;
};

struct farmer : entity {
	farmer();

	virtual void init();
	virtual void tick(int move_clipped);
	virtual void draw(draw_context* dc);

	int _fleeing;
	int _waiting;
	vec2 _home;
	bool _set_home;
};

entity* spawn_entity(world* w, entity* e);
void destroy_entity(entity* e);

void entity_update(entity* e);
void entity_render(draw_context* dc, entity* e);

int move_entity(entity* e, vec2 new_pos);

template<typename T> T* spawn_entity(world* w, T* e) { return (T*)spawn_entity(w, static_cast<entity*>(e)); }

void world_tick(world* w);
void world_draw(draw_context* dc, world* w);

extern texture g_tree2;
extern texture g_ground;
extern texture g_rock;
extern texture g_house;
extern texture g_field;
extern texture g_farmer;

#endif // GAME_H