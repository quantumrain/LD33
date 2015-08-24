#include "pch.h"
#include "game.h"

farmer::farmer() : entity(ET_FARMER), _set_home(), _t() {
	set_radius(20.0f);
}

void farmer::init() {
	_t = _world->r.range(PI);
}

void farmer::tick(int move_clipped) {
	if (!_set_home) {
		_set_home = true;
		_home = _pos;
	}

	vec2 delta = _home - _pos;
	float dist = length(delta);

	if (dist > 10.0f)
		_vel += normalise(delta) * 20.0f;

	_t += 0.1f;
	_rot += cosf(_t) * 0.05f;
	_vel *= 0.7f;

	interact_enemy_and_player(this, rgba(1.2f, 0.2f, 0.2f, 1.0f));

	avoid_enemy(_world, this);
}

void farmer::draw(draw_context* dc) {
	dc->translate(_pos.x, _pos.y);
	dc->rotate_z(_rot);

	float f = 0.2f + fabsf(cosf(_t));

	dc->shape(vec2(), 4, _radius * 0.75f, _rot, rgba(0.8f * f, 0.2f, 0.2f, 1.0f));
	dc->shape(vec2(), 4, _radius * 0.25f, _rot, rgba(0.8f * (1.0f - f), 0.2f, 0.2f, 1.0f));

	dc->shape_outline(vec2(), 4, _radius * 0.35f, _rot, 4.0f, rgba(0.0f, 1.0f));
	dc->shape_outline(vec2(), 4, _radius * 0.35f, _rot + TAU * 0.85f, 4.0f, rgba(0.0f, 1.0f));

	dc->shape_outline(vec2(), 4, _radius, _rot, 4.0f, rgba(0.0f, 1.0f));
	dc->shape_outline(vec2(), 4, _radius, _rot + TAU / 8.0f, 4.0f, rgba(0.0f, 1.0f));
}