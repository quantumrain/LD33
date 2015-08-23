#pragma once

struct wav_header;

struct sound_id { u16 v; sound_id(u16 v_ = 0) : v(v_) { } operator u16() { return v; } };
struct voice_id { u16 v; voice_id(u16 v_ = 0) : v(v_) { } operator u16() { return v; } };

enum { SOUND_LOOP = 0x1 };

void audio_init();
void audio_shutdown();

sound_id audio_create_sound(wav_header* h);
voice_id audio_create_voice(int format);

void audio_play(voice_id vid, sound_id sid, float semitones, float decibels, int flags);
void audio_stop(voice_id sid);

int audio_format(sound_id sid);
bool audio_is_playing(voice_id vid);

void audio_set_volume(voice_id vid, float decibels);
void audio_set_frequency(voice_id vid, float semitones);