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

world::world() : player() { }

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
		e->_pos += e->_vel * (1.0f / 60.0f);
	}

	e->tick(clipped);
}

void entity_render(draw_context* dc, entity* e) {
	e->draw(&dc->copy());
}

void world_tick(world* w) {

	// do player + player_bodies here

	// ents

	for_all(w->entities, [](entity* e) { entity_update(e); });

	// camera

	vec2 camera_target;
	vec2 camera_dir;

	if (player* p = w->player) {
		if (p->_body.not_empty()) {
			vec2 avg;

			for(auto& b : p->_body) {
				avg += b->_pos;
			}

			avg /= (float)p->_body.size();

			camera_target = lerp(avg, p->_pos, 1.25f);

#if 0
			if (length_sq(p->_pos - avg) > 0.01f)
				camera_dir = normalise(p->_pos - avg);

			float r = 0.0f;
			
			if (length_sq(camera_target - w->camera_pos) > 0.01f)
				r = dot(normalise(camera_target - w->camera_pos), camera_dir);

			static float rate;

			rate += (clamp(r, 0.0f, 1.0f) - rate) * ((r < rate) ? 0.2f : 0.025f);

			w->camera_pos += project_onto((camera_target - w->camera_pos), camera_dir) * lerp(0.05f, 0.3f, square(rate));
			w->camera_pos += cancel((camera_target - w->camera_pos), camera_dir) * lerp(0.05f, 0.1f, square(rate));
#else
			static float f;
			float v = clamp(length(p->_vel), 0.0f, 100.0f) / 100.0f;
			f += (v - f) * 0.5f;
			w->camera_pos += (camera_target - w->camera_pos) * 0.05f * f;
#endif
		}
	}


	// prune

	if (w->player && (w->player->_flags & EF_DESTROYED))
		w->player = 0;

	prune(w->entities);
}

void world_draw(draw_context* dc, world* w) {
	static texture tree2 = load_texture("tree2");
	static texture ground = load_texture("ground");

	random r(100);

	draw_context dcc = dc->copy();
	
	static float t_wind;
	t_wind += 0.025f;
	float f_wind = cosf(t_wind) * 0.01f;

	dcc.set(ground);
	for(int x = -10; x < 10; x++) {
		for(int y = -10; y < 10; y++) {
			vec2 c(x * 150.0f, y * 150.0f);
			for(int i = 0; i < 5; i++) {
				vec2 p = c + r.range(vec2(50.0f));
				float s = r.range(120.0f, 200.0f);
				float rot = r.range(PI);

				draw_tex_tile(dcc, p, s, rot, rgba(r.range(0.3f, 0.5f), 1.0f), ground, 0);
			}
		}
	}


	for_all(w->entities, [dc](entity* e) { entity_render(dc, e); });

	dcc.set(tree2);

	for(int x = -10; x < 10; x+=2) {
		for(int y = -10; y < 10; y+=2) {
			vec2 c(x * 150.0f, y * 150.0f);

			for(int i = 0; i < 1; i++) {
				//dcc.rect(p - s, p + s, 0.0f, 1.0f, rgba(0.25f, 1.0f));

				vec2 p = c + r.range(vec2(256.0f));
				float s = r.range(200.0f, 400.0f);
				float rot = r.range(PI);
				float wind = r.range(0.5f, 1.0f) * f_wind;
		
				draw_tex_tile(dcc, p, s, rot + wind, rgba(0.25f, 1.0f), tree2, 0);
			}
		}
	}

}