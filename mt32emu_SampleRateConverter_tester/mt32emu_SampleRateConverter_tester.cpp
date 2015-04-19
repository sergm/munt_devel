#include "stdafx.h"

#include "SampleRateConverter.h"

using namespace MT32Emu;

Bit32u Synth::getStereoOutputSampleRate() {
	return SAMPLE_RATE;
}

void Synth::render(Sample *inBuffer, Bit32u inLength) {
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
	Sample out[2 * MAX_SAMPLES_PER_RUN];
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
