#include "stdafx.h"

#include <ResamplerModel.h>
#include <SincResampler.h>
#include <IIR2xResampler.h>

using namespace MT32Emu;
using namespace SRCTools;

static const unsigned int CHANNEL_COUNT = 2;

class ArrayFloatSampleProvider : public FloatSampleProvider {
	FloatSample *inp;
	unsigned int inLength;

public:
	ArrayFloatSampleProvider(FloatSample inBuffer[], unsigned int inBufferLength) : inp(inBuffer), inLength(inBufferLength)
	{}

	void getOutputSamples(FloatSample *outBuffer, unsigned int size) {
		if (inLength == 0) {
			Synth::muteSampleBuffer(outBuffer, CHANNEL_COUNT * size);
			return;
		}
		const unsigned int framesToCopy = inLength < size ? inLength : size;
		const unsigned int samplesToCopy = CHANNEL_COUNT * framesToCopy;
		memcpy(outBuffer, inp, sizeof(FloatSample) * samplesToCopy);
		inp += samplesToCopy;
		inLength -= framesToCopy;
		size -= framesToCopy;
		if (size > 0) {
			outBuffer += samplesToCopy;
			Synth::muteSampleBuffer(outBuffer, CHANNEL_COUNT * size);
		}
	}
};

void readInputSamples(FloatSample in[], unsigned &inLength) {
	FloatSample *inp = in;
	while (!std::cin.eof() && inLength < MAX_SAMPLES_PER_RUN) {
		inLength++;
		std::cin >> *inp++;
		std::cin >> *inp++;
	}
}

void generateDelta(FloatSample in[], unsigned &inLength) {
	inLength++;
	in[0] = 1.0;
	in[1] = 0.0;
}

void generateStep(FloatSample in[], unsigned &inLength) {
	FloatSample *inp = in;
	const unsigned pulseLength = MAX_SAMPLES_PER_RUN >> 1;
	while (inLength < pulseLength) {
		inLength++;
		*inp++ = 1.0;
		*inp++ = 0.0;
	}
}

void generateSine(FloatSample in[], unsigned &inLength, double frequency) {
	FloatSample *inp = in;
	while (inLength < MAX_SAMPLES_PER_RUN) {
		inLength++;
		*inp++ = FloatSample(sin(inLength * frequency));
		*inp++ = 0.0;
	}
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cerr << "Usage: mt32emu_SampleRateConverter_tester <target_sample_rate>";
		return 1;
	}
	double sampleRate;
	if (sscanf_s(argv[1], "%lf", &sampleRate) != 1) {
		std::cerr << "Usage: mt32emu_SampleRateConverter_tester <target_sample_rate>";
		return 1;
	}
	FloatSample in[CHANNEL_COUNT * MAX_SAMPLES_PER_RUN];
	unsigned inLength = 0;
	readInputSamples(in, inLength);
//	generateDelta(in, inLength);
//	generateStep(in, inLength);
//	generateSine(in, inLength, 2.0 * 3.14159265358979323846 * sampleRate * 128.0 / 1024.0);
	ArrayFloatSampleProvider inSampleProvider(in, inLength);
	ResamplerStage &resamplerStage = *SincResampler::createSincResampler(1.0, sampleRate, 0.25, 0.5, 106, 256);
//	ResamplerStage &resamplerStage = *new IIR2xInterpolator(IIRResampler::FAST);
	FloatSampleProvider &src = ResamplerModel::createResamplerModel(inSampleProvider, resamplerStage);
	const unsigned outLength = MAX_SAMPLES_PER_RUN;
	FloatSample out[CHANNEL_COUNT * outLength];
	LARGE_INTEGER freq, startTime, endTime;
	QueryPerformanceFrequency(&freq);
	unsigned framesTotal = outLength;
	unsigned framesPerRun = framesTotal;
	FloatSample *outp = out;
	QueryPerformanceCounter(&startTime);
	while (0 < framesTotal) {
		src.getOutputSamples(outp, framesPerRun);
		outp += CHANNEL_COUNT * framesPerRun;
		framesTotal -= framesPerRun;
	}
	QueryPerformanceCounter(&endTime);
	double time = double(endTime.QuadPart - startTime.QuadPart) / freq.QuadPart;
	std::cerr << "Elapsed time: " << time * 1e3 << " msec" << std::endl;
	std::cout.precision(17);
	for (unsigned i = 0; i < CHANNEL_COUNT * outLength; i += CHANNEL_COUNT) {
		std::cout << out[i] << "\t" << out[i + 1] << std::endl;
	}
	return 0;
}
