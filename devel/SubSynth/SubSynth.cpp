// SubSynth.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "math.h"

const static int MAX_SAMPLES = 44100;
const static float SampleRate = 44100.f;
static _int16 carrier[MAX_SAMPLES];
static _int16 modulator[MAX_SAMPLES];

int waveform;
float pulsewidth, freq, cutoff, resonance_factor;

int generate_samples(_int16 samples[])
{
//	init filters
	float filtval = 6.283185307f * cutoff / SampleRate;
			filtval = filtval / (1 + filtval);
	float hpfval = 1.f / (1.f + 6.283185307f * cutoff / SampleRate);
	float SynthWave = 0.f;
	float SynthPrev = 0.f;
	float hpfSynthWavePrev = 0.f;
	float hpfSynthPrev = 0.f;

	float SynthPulseCounter = 0.f;
	float sample = 0.f;
	float sample_amp = .5f;
	float sample_bias = 0.f;
	float sample_max = -32768.f;
	float sample_min = 32768.f;

//	init deltas
	float resonanceDelta = SampleRate / cutoff;
	float SynthDelta = SampleRate / freq;
	float SynthPulseDelta = SynthDelta * (1.f - pulsewidth * .0093f);
	if (waveform == 0) SynthPulseDelta *= .5f;

	int t;
	for(t = 0; t < MAX_SAMPLES; t++) {
		if (waveform != 0) {
			//Sawtooth samples
			if ((SynthPulseDelta - SynthPulseCounter) >= 1.f) {
				SynthWave = 65536.f * SynthPulseCounter / SynthPulseDelta;
			} else if ((SynthPulseDelta - SynthPulseCounter) > 0.f) {
				SynthWave = 65536.f * (SynthPulseDelta - SynthPulseCounter);
			} else {
				SynthWave = 0.f;
			}
		} else {
			//Square wave.
			if ((SynthPulseDelta - SynthPulseCounter) >= 1.f) {
				SynthWave = 0.f;
			} else if ((SynthPulseDelta - SynthPulseCounter) > 0.f) {
				SynthWave = 65536.f * (1.f + SynthPulseCounter - SynthPulseDelta);
			} else if
				((SynthDelta - SynthPulseCounter) >= 1.f) {
				SynthWave = 65535.f;
			} else {
				SynthWave = 65536.f * (SynthDelta - SynthPulseCounter);
			}
		}
		SynthWave -= 32768.f;

		SynthPulseCounter++;
		if (SynthPulseCounter > SynthDelta) {
			SynthPulseCounter -= SynthDelta;
//			sample_bias = .5f * (sample_max + sample_min);
//			sample_amp = 65536.f / (sample_max - sample_min);
//			sample_max = 0.f;
//			sample_min = 65536.f;
		}

//		no filter
//		sample = SynthWave;

//		square * sinusoid
//		sample *= cosf(6.283185307f * SynthPulseCounter / SynthDelta);

//		sinusoid
//		sample = 32767.f * sinf(6.283185307f * SynthPulseCounter / SynthDelta);

//		LPF
		sample = SynthPrev + ((SynthWave - SynthPrev) * filtval);
		SynthPrev = sample;

//		normalization
//		if (sample_min > sample) sample_min = sample;
//		if (sample_max < sample) sample_max = sample;
//		sample -= sample_bias;
		sample *= sample_amp;
//		if (sample > 32767.f) sample = 32767.f;
//		if (sample < -32768.f) sample = -32768.f;

//		done
		samples[t] = _int16(sample);
	}

//	Reversed LPF
	SynthPrev = 0;
	for(t = MAX_SAMPLES - 1; t >= 0; t--) {
		SynthWave = samples[t];
		sample = SynthPrev + ((SynthWave - SynthPrev) * filtval);
		SynthPrev = sample;
		samples[t] = _int16(sample);
	}

//	2nd LPF
	SynthPrev = 0;
	for(t = 0; t < MAX_SAMPLES; t++) {
		SynthWave = samples[t];
		sample = SynthPrev + ((SynthWave - SynthPrev) * filtval);
		SynthPrev = sample;
		samples[t] = _int16(sample);
	}

//	2nd reversed LPF
	SynthPrev = 0;
	for(t = MAX_SAMPLES - 1; t >= 0; t--) {
		SynthWave = samples[t];
		sample = SynthPrev + ((SynthWave - SynthPrev) * filtval);
		SynthPrev = sample;
		samples[t] = _int16(sample);
	}

//	+ resonance
	SynthPulseCounter = 0;
	for(t = 0; t < MAX_SAMPLES; t++) {
		sample = samples[t];
		SynthPulseCounter++;
		float resonance_fade;
//		if (cutoff / freq == 1.f)
//			resonance_fade = 1.f;
//		else
			resonance_fade = 8.f;
		float phase = 0.f;
//		if (SynthPulseCounter < SynthPulseDelta)
			phase += 3.141593f;
		sample = sample / (1.f + .2f * resonance_factor) +
			resonance_factor * 1023.f * sinf(phase + 6.283185307f * SynthPulseCounter /
			resonanceDelta) * resonanceDelta / (resonanceDelta + resonance_fade *
			SynthPulseCounter);
		samples[t] = _int16(sample);

		SynthPulseCounter++;
		if (SynthPulseCounter > SynthDelta)
			SynthPulseCounter -= SynthDelta;
	}
	return t;
}

int _tmain(int argc, _TCHAR* argv[])
{
	waveform = 0;
	pulsewidth = 0.f;
	freq = 460.f;
	cutoff = exp((45 - 50) * 0.110903549) * freq;
	resonance_factor = 0.f;
	generate_samples(carrier);

	waveform = 0;
	pulsewidth = 0.f;
	freq = 5000.f;
	cutoff = exp((60 - 50) * 0.110903549) * freq;
	resonance_factor = 0.f;
	generate_samples(modulator);

	for(int t = 0; t < MAX_SAMPLES; t++) {
//		carrier[t] = carrier[t] * modulator[t] >> 14;
	}

	HFILE h = _lopen("C:/DOWNLOADS/1.wav", OF_READWRITE);
	_llseek(h, 44, FILE_BEGIN);
	_lwrite(h, (LPCCH)carrier, MAX_SAMPLES << 1);
	_lclose(h);
}
