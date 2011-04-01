#include "stdafx.h"
#include "math.h"

const static int MAX_SAMPLES = 44100;
const static float SampleRate = 44100.f;
const static float WGAMP = 15668.f;
const static float PI = 3.141592654f;
const static float PI2 = 6.283185307f;
const static float RESAMPFACTOR = 512.0f;
const static float RESAMPMAX = 1.75f;
const static float RESAMPFADE = 0.1f;
static _int16 carrier[MAX_SAMPLES];
static _int16 modulator[MAX_SAMPLES];

int waveform, cutoff, pulseWidth, resonance;
float freq;

int generate_samples(_int16 samples[])
{
	float wavePos = 0.f;
	float sample = 0.f;
	float resAmp = powf(RESAMPFACTOR, -(1.0f - resonance / 30.0f));

	// Init internal values
	int pulseWidthVal = int(2.56f * pulseWidth);
	int cutoffVal = int(2.56f * cutoff);

	// Wave lenght in samples
	float waveLen = SampleRate / freq;

	// Anti-aliasing feature
	if (waveLen < 4.0f) {
		waveLen = 4.0f;
	}

	// Init wavePos
	float cosineLen = 0.5f * waveLen;
	if (cutoffVal > 128) {
		float ft = (cutoffVal - 128) / 127.0f;
		cosineLen *= expf(-2.162823f * ft);
	}

	// Anti-aliasing feature
	if (cosineLen < 2.0f) {
		cosineLen = 2.0f;
		resAmp = 0.0f;
	}

	// Start playing at the middle of 1st cosine segment
	wavePos = 0.5f * cosineLen;

	// Ratio of negative segment to waveLen
	float pulseLen = 0.5f;
	if (pulseWidthVal > 128) {
		// Formula determined from sample analysis.
		float pt = 0.5f / 127.0f * (pulseWidthVal - 128);
		pulseLen += (1.239f - pt) * pt;
	}
	pulseLen *= waveLen;

	float lLen = pulseLen - cosineLen;

	// Ignore pulsewidths too high for given freq
	if (lLen < 0.0f) {
		lLen = 0.0f;
	}

	// Ignore pulsewidths too high for given freq and cutoff
	float hLen = waveLen - lLen - 2 * cosineLen;
	if (hLen < 0.0f) {
		hLen = 0.0f;
	}

	// Correct resAmp for cutoff in range 50..60
	if (cutoffVal < 158) {
		resAmp *= (1.0f - (158 - cutoffVal) / 30.0f);
	}

	int t;
	for(t = 0; t < MAX_SAMPLES; t++) {
		// filtered square wave with 2 cosine waves on slopes

		// 1st cosine segment
		if (wavePos < cosineLen) {
			sample = -cosf(PI * wavePos / cosineLen);
		} else

		// high linear segment
		if (wavePos < (cosineLen + hLen)) {
			sample = 1.f;
		} else

		// 2nd cosine segment
		if (wavePos < (2 * cosineLen + hLen)) {
			sample = cosf(PI * (wavePos - (cosineLen + hLen)) / cosineLen);
		} else {

		// low linear segment
			sample = -1.f;
		}

		if (cutoffVal < 128) {

			// Attenuate samples below cutoff 50 another way
			// Found by sample analysis
			sample *= expf(0.693147181f * -0.048820569f * (128 - cutoffVal));
		} else {

			// Add resonance sine. Effective for cutoff > 50 only
			float resSample = 1.0f;
			float resAmpFade = 0.0f;

			// wavePos relative to the middle of cosine segment
			float relWavePos = wavePos - 0.5f * cosineLen;

			// Always positive
			if (relWavePos < 0.0f) {
				relWavePos += waveLen;
			}

			// negative segments
			if (!(relWavePos < (cosineLen + hLen))) {
				resSample = -resSample;
				relWavePos -= cosineLen + hLen;
			}

			// wavePos relative to the middle of cosine segment
			// Negative for first half of cosine segment
			float relWavePosC = wavePos - 0.5f * cosineLen;

			if (!(wavePos < (cosineLen + hLen))) {
				relWavePosC -= cosineLen + hLen;
			}

			// Resonance sine WG
			resSample *= sinf(PI * relWavePos / cosineLen);

			// Resonance sine amp
			resAmpFade = RESAMPMAX - RESAMPFADE * (relWavePos / cosineLen);

			// Fading to zero while in first half of cosine segment to avoid breaks in the wave
			if (relWavePosC < 0.0f) {
				resAmpFade *= -relWavePosC / (0.5f * cosineLen);
			}

			sample += resSample * resAmp * resAmpFade;
		}

		// sawtooth waves
		if ((waveform & 1) != 0) {
			sample *= cosf(PI2 * wavePos / waveLen);
		}

		wavePos++;
		if (wavePos > waveLen)
			wavePos -= waveLen;

		// TVA emulation just for testings
		sample *= (1.0f - 0.5f * resonance / 30.0f);

		//	done
		samples[t] = _int16(sample * WGAMP);
	}
	return t;
}

int _tmain(int argc, _TCHAR* argv[])
{
	waveform = 0;
	freq = 45.f;
	pulseWidth = 0;
	cutoff = 65;
	resonance = 30;
	generate_samples(carrier);

	waveform = 0;
	freq = 5000.f;
	pulseWidth = 100;
	cutoff = 50;
	resonance = 0;
	generate_samples(modulator);

	for(int t = 0; t < MAX_SAMPLES; t++) {
//		carrier[t] = carrier[t] * modulator[t] >> 14;
	}

	HFILE h = _lopen("C:/DOWNLOADS/1.wav", OF_READWRITE);
	_llseek(h, 44, FILE_BEGIN);
	_lwrite(h, (LPCCH)carrier, MAX_SAMPLES << 1);
	_lclose(h);
}
