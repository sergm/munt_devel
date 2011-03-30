// SubSynth.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

const static int MAX_SAMPLES = 44100;

class WTSynth {
private:
	const static int SAMPLES_PER_ROW = 1024;
	const static int MAX_CUTOFF = 100;
	const static int WGAMP = 16384.f;

	float SampleRate;
	_int16 waveTable[MAX_CUTOFF][SAMPLES_PER_ROW];
	int waveform;
	float pulsewidth, freq, cutoff, resonance_factor;

	void init_WaveTable();
	void ring_modulation(_int16 samples[]);

public:
	WTSynth();
	void setSynth(int waveform, float freq, float pulsewidth, float cutoff, float resonance_factor);
	void generate_samples(_int16 samples[]);
	void store_samples(_int16 *samples);
} wtSynth;

WTSynth::WTSynth() {
	SampleRate = MAX_SAMPLES;
}

void WTSynth::setSynth(int wf, float f, float pw, float co, float rf) {
	waveform = wf;
	freq = f;
	pulsewidth = pw;
	cutoff = co;
	resonance_factor = rf;
	init_WaveTable();
}

void WTSynth::init_WaveTable() {
	float sample = 0.f;
	float pw = .25f;

	for(int t = 0; t < SAMPLES_PER_ROW; t++) {
		if ((t < pw * SAMPLES_PER_ROW) || (t > (1.f - pw) * SAMPLES_PER_ROW)) {
			sample = -WGAMP;
		} else {
			sample = WGAMP;
		}
		waveTable[0][t] = _int16(sample);
	}

// now filter the wave
	float SynthPrev;
	float filtval = .015f;

	//	2 stages of filtering in both directions, i.e. 4 stages total
	for(int stage = 2; stage > 0; stage--) {
	//	Forward LPF
		SynthPrev = -WGAMP;
		for(int t = 0; t < SAMPLES_PER_ROW; t++) {
			sample = waveTable[0][t];
			sample = SynthPrev + ((sample - SynthPrev) * filtval);
			SynthPrev = sample;
			waveTable[0][t] = _int16(sample);
		}

	//	Reversed LPF
		SynthPrev = -WGAMP;
		for(int t = SAMPLES_PER_ROW - 1; t >= 0; t--) {
			sample = waveTable[0][t];
			sample = SynthPrev + ((sample - SynthPrev) * filtval);
			SynthPrev = sample;
			waveTable[0][t] = _int16(sample);
		}
	}
}

void WTSynth::generate_samples(_int16 samples[]) {
	float delta = freq * SAMPLES_PER_ROW / SampleRate;
	float cnt = 0;
	for(int t = 0; t < MAX_SAMPLES; t++) {
		float sample;
		sample = waveTable[0][(int)cnt];
		if (cnt + 1 < SAMPLES_PER_ROW) {
			sample += (waveTable[0][(int)cnt + 1] - sample) * (cnt - (int)cnt);
		}
		samples[t] = (_int16)sample;
		cnt += delta;
		if (SAMPLES_PER_ROW <= cnt) {
			cnt -= SAMPLES_PER_ROW;
		}
	}
}

void WTSynth::store_samples(_int16 *samples) {
	HFILE h = _lopen("C:/DOWNLOADS/1.wav", OF_READWRITE);
	_llseek(h, 44, FILE_BEGIN);
	_lwrite(h, (LPCCH)samples, MAX_SAMPLES << 1);
	_lclose(h);
}

int _tmain(int argc, _TCHAR* argv[])
{
	static _int16 carrier[MAX_SAMPLES];
	static _int16 modulator[MAX_SAMPLES];

	WTSynth wtSynth;
	wtSynth.setSynth(0, 43500, 0, 0, 0);
	wtSynth.generate_samples(carrier);
	wtSynth.store_samples(carrier);
}
