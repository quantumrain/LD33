#pragma once

enum class sfx {
	DIT,
	RAIN,
	WIND,
	GRIND,
	GRIND2,
	_MAX
};

void init_sound();
void define_sound(sfx s, const char* name, int max_voices, int max_cooldown);
void finalise_sound();

void sound_update();

voice_id sound_play(sfx s, float semitones = 0.0f, float decibels = 0.0f, int flags = 0);
void sound_stop(sfx s);