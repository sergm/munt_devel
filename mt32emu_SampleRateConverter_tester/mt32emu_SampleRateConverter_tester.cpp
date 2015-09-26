#include "stdafx.h"

#include "SampleRateConverter.h"

using namespace MT32Emu;

Bit32u Synth::getStereoOutputSampleRate() {
	return SAMPLE_RATE;
}

Bit32u Synth::getStereoOutputSampleRate(AnalogOutputMode mode) {
	static const unsigned int SAMPLE_RATES[] = { SAMPLE_RATE, SAMPLE_RATE, SAMPLE_RATE * 3 / 2, SAMPLE_RATE * 3 };

	return SAMPLE_RATES[mode];
}

template <class S>
void render(S *inBuffer, Bit32u inLength) {
	while (inLength-- > 0) {
		if (std::cin.eof()) {
			*inBuffer++ = 0;
			*inBuffer++ = 0;
		} else {
			std::cin >> *inBuffer++;
			std::cin >> *inBuffer++;
		}
	}
}

void Synth::render(Bit16s *inBuffer, Bit32u inLength) {
	::render(inBuffer, inLength);
}

void Synth::render(float *inBuffer, Bit32u inLength) {
	::render(inBuffer, inLength);
}

int main(int argc, char *argv[]) {
	if (argc != 2) {
		std::cout << "Usage: mt32emu_SampleRateConverter_tester <target_sample_rate>";
		return 1;
	}
	float sampleRate;
	if (sscanf_s(argv[1], "%f", &sampleRate) != 1) {
		std::cout << "Usage: mt32emu_SampleRateConverter_tester <target_sample_rate>";
		return 1;
	}
	Synth synth;
	SampleRateConverter &src = *SampleRateConverter::createSampleRateConverter(&synth, sampleRate, SampleRateConverter::SRC_FASTEST);
	Bit16s out[2 * MAX_SAMPLES_PER_RUN];
	LARGE_INTEGER freq, startTime, endTime;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&startTime);
	int framesTotal = 30;
	int framesPerRun = 1;
	for (int i = 0; i < framesTotal; i += framesPerRun)
		src.getOutputSamples(out + 2 * i, framesPerRun);
	QueryPerformanceCounter(&endTime);
	double time = double(endTime.QuadPart - startTime.QuadPart) / freq.QuadPart;
	std::cerr << "Elapsed time: " << time * 1e3 << " msec" << std::endl;
	std::cout.precision(17);
	for (int i = 0; i < 2 * framesTotal; i += 2) {
		std::cout << out[i] << "\t" << out[i + 1] << std::endl;
	}
	return 0;
}
