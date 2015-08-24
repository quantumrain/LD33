#include "pch.h"
#include "game.h"

random g_rand;

struct particle {
	vec2 pos;
	vec2 vel;
	float damp;
	float size0;
	float size1;
	float rot;
	float rot_v;
	rgba c0;
	rgba c1;
	int time;
	int lifetime;
};

struct psys {
	array<particle> particles;
};

psys g_particles;

void psys_init(int max_particles) {
	g_particles.particles.reserve(max_particles);
}

void psys_update() {
	psys* sys = &g_particles;

	for(int i = 0; i < sys->particles.size(); i++) {
		particle* p = &sys->particles[i];

		if (p->time++ >= p->lifetime) {
			sys->particles.swap_remove(i);
			i--;
			continue;
		}

		p->pos += p->vel * DT;
		p->vel *= p->damp;
	}
}

void psys_render(draw_context* dc) {
	psys* sys = &g_particles;

	dc->set(g_texture_white);

	for(auto& p : sys->particles) {
		float f = p.time / (float)p.lifetime;
		float s = lerp(p.size0, p.size1, f);

		draw_context dcc = dc->copy().translate(p.pos.x, p.pos.y).rotate_z(p.rot);
		dcc.rect(-s, s, lerp(p.c0, p.c1, f));
	}
}

void psys_spawn(vec2 pos, vec2 vel, float damp, float size0, float size1, float rot_v, rgba c0, rgba c1, int lifetime) {
	psys* sys = &g_particles;

	if (sys->particles.size() >= sys->particles.capacity())
		return;

	particle* p = sys->particles.push_back(particle());

	p->pos = pos;
	p->vel = vel;
	p->damp = damp;
	p->size0 = size0;
	p->size1 = size1;
	p->rot = g_rand.range(PI);
	p->rot_v = rot_v;
	p->c0 = c0;
	p->c1 = c1;
	p->time = 0;
	p->lifetime = lifetime;
}

// effects

void fx_explosion(vec2 pos, float strength, int count, rgba c, float p_size, int lifetime) {
	for(int i = 0; i < count; i++) {
		vec2 v = rotation(g_rand.range(PI)) * square(0.5f + g_rand.rand(1.0f)) * strength;
		psys_spawn(pos, v, 0.8f, g_rand.range(p_size - (p_size / 4.0f), p_size + (p_size / 4.0f)), 1.0f, g_rand.range(1.0f), c, c, g_rand.range(lifetime - (lifetime / 4), lifetime + (lifetime / 4)));
	}
}