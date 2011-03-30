#include "stdafx.h"
#include "math.h"

const static int MAX_SAMPLES = 44100;
const static float SampleRate = 44100.f;
const static float WGAMP = 16384.f;
const static float PI = 3.141592654f;
const static float PI2 = 6.283185307f;
static _int16 carrier[MAX_SAMPLES];
static _int16 modulator[MAX_SAMPLES];

int waveform, filtval;
float pulsewidth, freq, cutoff, resonance_factor;

int generate_samples(_int16 samples[])
{
	float waveCntr = 0.f;
	float sample = 0.f;

	//	init deltas
	float resonanceLen = SampleRate / cutoff;
	float waveLen = SampleRate / freq;
	float cosineLen = (waveLen * (100 - filtval) + 2 * (filtval - 50)) / 100.f;
	float pulseLen = 0.5f;
	if (pulsewidth > 50) {
		pulseLen -= 0.3734694f * (pulsewidth - 50) / 50.0f;
	}
	pulseLen *= waveLen;
	float hLen = pulseLen - cosineLen;
	float lLen = waveLen - hLen - 2 * cosineLen;

	int t;
	for(t = 0; t < MAX_SAMPLES; t++) {
		// filtered square wave with 2 cosine waves on slopes

		// 1st cosine segment
		if (waveCntr < cosineLen) {
			sample = -cosf(PI * waveCntr / cosineLen);

			// add 2nd overlapped cosine segment
			if ((lLen < 0) && (waveCntr < -lLen))
//				sample += 1.f + cosf(PI * (waveLen + waveCntr - (cosineLen + hLen)) / cosineLen); // slightly optimized
				sample += 1.f + cosf(PI * (waveCntr + lLen - cosineLen) / cosineLen);
		} else 

		// high linear segment
		if (waveCntr < (cosineLen + hLen)) {
			sample = 1.f;
		} else 

		// 2nd cosine segment
		if (waveCntr < (2 * cosineLen + hLen)) {
			sample = cosf(PI * (waveCntr - (cosineLen + hLen)) / cosineLen);
		} else {

		// low linear segment
			sample = -1.f;
		}

		waveCntr++;
		if (waveCntr > waveLen)
			waveCntr -= waveLen;

		// sawtooth waves
		if ((waveform & 1) != 0) {
			sample *= cosf(6.283185307f * waveCntr / waveLen);
		}

		//	done
		samples[t] = _int16(sample * WGAMP);
	}

//	+ resonance
	waveCntr = 0;
	for(t = 0; t < MAX_SAMPLES; t++) {
		sample = samples[t];
		waveCntr++;
		float resonance_fade;
		resonance_fade = 8.f;
		float phase = 0.f;
		if (waveCntr < waveLen)
			phase += PI;
		sample = sample / (1.f + .2f * resonance_factor) +
			resonance_factor * 1023.f * sinf(phase + PI2 * waveCntr /
			resonanceLen) * resonanceLen / (resonanceLen + resonance_fade *
			waveCntr);
		samples[t] = _int16(sample);

		waveCntr++;
		if (waveCntr > waveLen)
			waveCntr -= waveLen;
	}
	return t;
}

int _tmain(int argc, _TCHAR* argv[])
{
	waveform = 01;
	pulsewidth = 100.f;
	freq = 45.f;
	filtval = 94;
	cutoff = expf((filtval - 50) * 0.110903549f) * freq;
	resonance_factor = 0.f;
	generate_samples(carrier);

	waveform = 0;
	pulsewidth = 0.f;
	freq = 5000.f;
	filtval = 60;
	cutoff = expf((filtval - 50) * 0.110903549f) * freq;
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
