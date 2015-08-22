#include "pch.h"
#include "game.h"

player_body::player_body(player* head) : entity(ET_PLAYER_BODY), _head(head) {
}

void player_body::init() {
}

void player_body::tick(int move_clipped) {
}

void player_body::draw(draw_context* dc) {
	dc->shape(vec2(), 32, _radius, 0.0f, rgba());
}