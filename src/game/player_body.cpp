#include "pch.h"
#include "game.h"

player_body::player_body(player* head) : entity(ET_PLAYER_BODY), _head(head), _render_radius(1.0f), _index(0) {
}

void player_body::init() {
}

void player_body::tick(int move_clipped) {
	_vel *= 0.8f;
}

void player_body::draw(draw_context* dc) {
	//entity::draw(dc);
}