#include "pch.h"
#include "game.h"

player_body::player_body() : entity(ET_PLAYER_BODY), _render_radius(1.0f), _index(0) {
	_flags |= EF_NO_PHYSICS;
}

void player_body::init() {
}

void player_body::tick(int move_clipped) {
}

void player_body::draw(draw_context* dc) {
	//entity::draw(dc);
}