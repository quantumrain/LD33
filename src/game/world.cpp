#include "pch.h"
#include "game.h"

extern array<vec2> g_rocks;
extern array<vec2> g_trees;
extern array<vec2> g_houses;
extern array<vec2> g_fields;

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

world::world() : player() { }

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

	random r(100);

	draw_context dcc = dc->copy();
	
	static float t_wind;
	t_wind += 0.025f;
	float f_wind = cosf(t_wind) * 0.01f;

	dcc.rect({ 20.0f * 5.0f }, { (512.0f - 20.0f) * 5.0f }, rgba(0.2f));

	dcc.set(g_ground);
	for(int x = 1; x < 17; x++) {
		for(int y = 1; y < 17; y++) {
			vec2 c(x * 150.0f, y * 150.0f);
			for(int i = 0; i < 5; i++) {
				vec2 p = c + r.range(vec2(50.0f));
				float s = r.range(120.0f, 200.0f);
				float rot = r.range(PI);

				draw_tex_tile(dcc, p, s, rot, rgba(r.range(0.3f, 0.5f), 1.0f), 0);
			}
		}
	}

	dcc.set(g_field);

	for(auto& c : g_fields) {
		draw_tex_tile(dcc, c, 400.0f, 0.0f, rgba(0.25f, 1.0f), 0);
	}

	for_all(w->entities, [dc](entity* e) { entity_render(dc, e); });

	dcc.set(g_rock);

	random rr(100);
	for(auto& rock : g_rocks) {
		draw_tex_tile(dcc, rock, 190.0f, rr.range(PI), rgba(1.0f), 0);
	}

	dcc.set(g_house);

	for(auto& c : g_houses) {
		draw_tex_tile(dcc, c, 256.0f, r.range(0.1f), rgba(0.25f, 1.0f), 0);
	}

	dcc.set(g_tree2);

	for(auto& c : g_trees) {
		float s = 300.0f;
		float rot = r.range(PI);
		float wind = r.range(0.5f, 1.0f) * f_wind;
		
		draw_tex_tile(dcc, c, s, rot + wind, rgba(0.25f, 1.0f), 0);
	}

	dcc.set(g_texture_white);
	dcc.tri(vec2(0.0f, 0.0f), vec2(500, 0.0f), vec2(0.0f, 500.0f), rgba(0.0f, 1.0f), rgba(0.0f, 0.0f), rgba(0.0f, 0.0f));
	dcc.tri(vec2(0.0f, 0.0f), vec2(600, 0.0f), vec2(0.0f, 600.0f), rgba(0.0f, 1.0f), rgba(0.0f, 0.0f), rgba(0.0f, 0.0f));
	dcc.tri(vec2(0.0f, 0.0f), vec2(1000, 0.0f), vec2(0.0f, 1000.0f), rgba(0.0f, 1.0f), rgba(0.0f, 0.0f), rgba(0.0f, 0.0f));

	static bool do_map;

	if (is_key_pressed(KEY_DEBUG_STATS))
		do_map = !do_map;

	if (do_map) {
		if (player* pl = w->player) {
			vec2i ix((int)(pl->_pos.x / 5.0f), (int)(pl->_pos.y / 5.0f));

			int x0 = clamp(ix.x - 60, 0, w->map._w - 1);
			int x1 = clamp(ix.x + 60, 0, w->map._w - 1);
			int y0 = clamp(ix.y - 40, 0, w->map._h - 1);
			int y1 = clamp(ix.y + 40, 0, w->map._h - 1);

			for(int y = y0; y < y1; y++) {
				for(int x = x0; x < x1; x++) {
					vec2 p(x, y);

					if (w->map.is_slide_solid(x, y)) {
						dc->rect(p * 5.0f, (p + 1.0f) * 5.0f, rgba(colours::FUCHSIA) * 0.25f);
					}
				}
			}
		}
	}

}