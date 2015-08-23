#include "pch.h"
#include "platform.h"
#include "base.h"

struct audio_sound {
	u8* data;
	u32 size;
	u16 format;
	u16 samples;
};

struct audio_voice {
	IXAudio2SourceVoice* p;
	u16 format;
	u16 last_samples;

	audio_voice() : p(), format(), last_samples() { }
	~audio_voice() { if (p) p->DestroyVoice(); }
};

struct audio {
	IXAudio2*				device;
	IXAudio2MasteringVoice*	master_voice;

	array<audio_sound> sound;
	array<audio_voice> voice;
};

audio g_audio;

// sound sys

void audio_init() {
	CoInitializeEx(0, COINIT_MULTITHREADED);

	HMODULE xaudio2 = LoadLibrary(L"xaudio2_8.dll"); // DLLs before 8 were DirectSound apparently

	if (!xaudio2) {
		debug("xaudio2.dll not found - audio will be disabled");
		return;
	}

	auto xaudio2_create = (decltype(&XAudio2Create))GetProcAddress(xaudio2, "XAudio2Create");

	if (!xaudio2_create) {
		debug("xaudio2.dll - XAudio2Create not found in dll?");
		return;
	}

	if (FAILED(xaudio2_create(&g_audio.device, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
		debug("XAudioCreate failed");
		return;
	}

	if (FAILED(g_audio.device->CreateMasteringVoice(&g_audio.master_voice))) {
		debug("CreateMasterVoice failed");
		g_audio.device->Release();
		g_audio.device = 0;
		return;
	}
}

void audio_shutdown()  {
	if (!g_audio.device)
		return;

	g_audio.voice.free();
	g_audio.sound.free();

	g_audio.master_voice->DestroyVoice();
	g_audio.device->Release();
}

sound_id audio_create_sound(wav_header* h) {
	if (!g_audio.device)
		return 0;

	audio_sound* s = g_audio.sound.push_back();

	if (!s)
		return 0;

	s->data    = (u8*)&h[1];
	s->size    = h->size;
	s->samples = h->samples;
	s->format  = h->format;

	return g_audio.sound.size();
}

voice_id audio_create_voice(int format) {
	if (!g_audio.device)
		return 0;

	if ((format < 0) || (format >= WAV_FORMAT_COUNT))
		return 0;

	audio_voice* v = g_audio.voice.push_back();

	WAVEFORMATEX wfx = { };

	wfx.wFormatTag		= WAVE_FORMAT_PCM;
	wfx.nSamplesPerSec	= 44100;
	wfx.nChannels		= ((format == WAV_FORMAT_STEREO_8) || (format == WAV_FORMAT_STEREO_16)) ? 2 : 1;
	wfx.wBitsPerSample	= ((format == WAV_FORMAT_MONO_16) || (format == WAV_FORMAT_STEREO_16)) ? 16 : 8;
	wfx.nBlockAlign		= wfx.nChannels * (((format == WAV_FORMAT_MONO_16) || (format == WAV_FORMAT_STEREO_16)) ? 2 : 1);
	wfx.nAvgBytesPerSec	= wfx.nSamplesPerSec * wfx.nBlockAlign;

	v->p			= 0;
	v->format		= format;
	v->last_samples	= 44100;

	if (FAILED(g_audio.device->CreateSourceVoice(&v->p, &wfx, 0, XAUDIO2_MAX_FREQ_RATIO))) {
		debug("failed to create voice for format %i", format);
		return 0;
	}

	return g_audio.voice.size();
}

void audio_play(voice_id vid, sound_id sid, float semitones, float decibels, int flags) {
	if (!g_audio.device)
		return;

	if ((sid <= 0) || (sid > g_audio.sound.size()))
		return;

	if ((vid <= 0) || (vid > g_audio.voice.size()))
		return;

	audio_sound* s = &g_audio.sound[sid - 1];
	audio_voice* v = &g_audio.voice[vid - 1];

	if (s->format != v->format) {
		debug("audio_play: trying to use an incompatible sound with a voice");
		return;
	}

	if (!v->p)
		return;

	XAUDIO2_BUFFER xab = { };

	xab.AudioBytes	= s->size;
	xab.pAudioData	= s->data;
	xab.Flags		= XAUDIO2_END_OF_STREAM;
	xab.LoopCount	= (flags & SOUND_LOOP) ? XAUDIO2_LOOP_INFINITE : 0;

	v->p->Stop();
	v->p->FlushSourceBuffers();

	if (v->last_samples != s->samples) {
		if (FAILED(v->p->SetSourceSampleRate(s->samples))) {
			debug("audio_play: SetSourceSampleRate failed"); // todo: can't call this until all buffers are flushed, so might need to defer starting sound
		}
		else {
			v->last_samples = s->samples;
		}
	}

	v->p->SetFrequencyRatio(powf(2.0f, semitones / 12.0f));
	v->p->SetVolume(powf(10.0f, decibels / 20.0f));
	v->p->SubmitSourceBuffer(&xab);
	v->p->Start();
}

void audio_stop(voice_id vid) {
	if (!g_audio.device)
		return;

	if ((vid <= 0) || (vid > g_audio.voice.size()))
		return;

	audio_voice* v = &g_audio.voice[vid - 1];

	if (!v->p)
		return;

	v->p->Stop();
	v->p->FlushSourceBuffers();
}

int audio_format(sound_id sid) {
	if (!g_audio.device)
		return 0;

	if ((sid <= 0) || (sid > g_audio.sound.size()))
		return 0;

	audio_sound* s = &g_audio.sound[sid - 1];

	return s->format;
}

bool audio_is_playing(voice_id vid) {
	if (!g_audio.device)
		return false;

	if ((vid <= 0) || (vid > g_audio.voice.size()))
		return false;

	audio_voice* v = &g_audio.voice[vid - 1];

	XAUDIO2_VOICE_STATE s;
	v->p->GetState(&s, XAUDIO2_VOICE_NOSAMPLESPLAYED);

	return s.BuffersQueued != 0;
}

void audio_set_volume(voice_id vid, float decibels) {
	if (!g_audio.device)
		return;

	if ((vid <= 0) || (vid > g_audio.voice.size()))
		return;

	audio_voice* v = &g_audio.voice[vid - 1];

	v->p->SetVolume(powf(10.0f, decibels / 20.0f));
}

void audio_set_frequency(voice_id vid, float semitones) {
	if (!g_audio.device)
		return;

	if ((vid <= 0) || (vid > g_audio.voice.size()))
		return;

	audio_voice* v = &g_audio.voice[vid - 1];

	v->p->SetFrequencyRatio(powf(2.0f, semitones / 12.0f));
}