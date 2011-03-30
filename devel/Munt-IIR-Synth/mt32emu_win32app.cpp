#include "stdafx.h"

using namespace MT32Emu;

static const unsigned int SAMPLE_RATE = 44100;

Synth *synth;
float amp = 1.f;
float freq = 1000;
int pulsewidth = 0;
int waveform = 0;
int resonance = 0;
int filtval = 128 - 16;

void generateSamples() {
Bit16s Buf[SAMPLE_RATE];
float SynthPulseCounter = 0;
int sampleNum;
float history[32] = {0};

	for(sampleNum = 0; sampleNum < SAMPLE_RATE; sampleNum++) {
		float sample = 0;
// Render synthesised waveform
		float SynthDelta = SAMPLE_RATE / freq;
		float SynthPulseDelta = SynthDelta * (1.f - .0093f * pulsewidth);
		if ((waveform & 1) != 0) {
			//Sawtooth samples
			if ((SynthPulseDelta - SynthPulseCounter) >= 1.f) {
				sample = float(2 * WGAMP) * SynthPulseCounter / SynthPulseDelta - float(WGAMP);
			} else if ((SynthPulseDelta - SynthPulseCounter) > 0.f) {
				sample = float(2 * WGAMP) * (SynthPulseDelta - SynthPulseCounter) - float(WGAMP);
			} else {
				sample = -float(WGAMP);
			}
		} else {
			//Square wave.
			SynthPulseDelta *= .5f;
			if ((SynthPulseDelta - SynthPulseCounter) >= 1.f) {
				sample = -float(WGAMP);
			} else if ((SynthPulseDelta - SynthPulseCounter) > 0.f) {
				sample = float(2 * WGAMP) * (1.f + SynthPulseCounter - SynthPulseDelta) - float(WGAMP);
			} else if ((SynthDelta - SynthPulseCounter) >= 1.f) {
				sample = float(WGAMP);
			} else {
				sample = float(2 * WGAMP) * (SynthDelta - SynthPulseCounter) - float(WGAMP);
			}
		}
		SynthPulseCounter++;
		if (SynthPulseCounter > SynthDelta) {
			SynthPulseCounter -= SynthDelta;
		}

		float freqsum = synth->tables.tCutoffFreq[filtval] * freq;
		if(freqsum >= (FILTERGRAN - 500.0))
			freqsum = (FILTERGRAN - 500.0f);
		sample = (((synth->iirFilter)((sample), &history[0], synth->tables.filtCoeff[(Bit32s)freqsum][resonance])));

		// This amplifies signal fading near the cutoff point
		if (filtval < 128 + 0) {
			sample *= .125f * (128 + 8 - filtval);
		}

		Buf[sampleNum] = (Bit16s)(amp * sample);
	}
	HFILE h = _lopen("C:/DOWNLOADS/1.wav", OF_READWRITE);
	_llseek(h, 44, FILE_BEGIN);
	_lwrite(h, (LPCCH)Buf, sampleNum << 1);
	_lclose(h);
}

int main(int argc, char *argv[]) {

//	init synth
	synth = new Synth();
	SynthProperties synthProp = {SAMPLE_RATE, true, true, 0, 0, 0, "C:\\WINDOWS\\SYSTEM32\\",
		NULL, NULL, NULL, NULL, NULL};
	if (!synth->open(synthProp))
		return 1;

	generateSamples();
}