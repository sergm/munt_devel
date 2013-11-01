#include "stdafx.h"

#include "BossEmu.h"

using namespace MT32Emu;

static const int SAMPLE_COUNT = 65535;

int main(int argc, char *argv[]) {
	if (argc != 3) {
		std::cout << "Usage: mt32emu_breverb_comparator <ROM filename> <test data filename>\n";
		return 1;
	}

	FILE *romFile = fopen(argv[1], "rb");
	if (romFile == NULL) {
		std::cout << "Can't open ROM file " << argv[1] << "\n";
		return 2;
	}

	unsigned char rom[32768];
	int romLength = fread(rom, 1, 32768, romFile);
	fclose(romFile);

	FILE *dataFile = fopen(argv[2], "rb");
	if (dataFile == NULL) {
		std::cout << "Can't open test data file " << argv[2] << "\n";
		return 2;
	}

	Sample inLeft[SAMPLE_COUNT];
	Sample inRight[SAMPLE_COUNT];

	memset(inLeft, 0, sizeof(short) * SAMPLE_COUNT);
	memset(inRight, 0, sizeof(short) * SAMPLE_COUNT);

	int dataLength = fread(inRight, 1, SAMPLE_COUNT, dataFile);
	fclose(dataFile);

	std::cout << "Loaded " << dataLength << " bytes of test data\n";

	Sample bReverbOutLeft[SAMPLE_COUNT];
	Sample bReverbOutRight[SAMPLE_COUNT];

	Sample bossEmuOutLeft[SAMPLE_COUNT];
	Sample bossEmuOutRight[SAMPLE_COUNT];

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);

	for (int mode = 0; mode < 4; ++mode) {
		for (int time = 0; time < 8; ++time) {
			for (int level = 0; level < 8; ++level) {
				std::cout << "Testing mode=" << mode << ", time=" << time << ", level=" << level << "... ";

				BReverbModel model((ReverbMode)mode);
				model.open();
				model.setParameters(time, level);

				BossEmu emuModel(rom, romLength);
				emuModel.setParameters(mode, time, level);

				LARGE_INTEGER startTime1, endTime1;

				QueryPerformanceCounter(&startTime1);
				model.process(inLeft, inRight, bReverbOutLeft, bReverbOutRight, SAMPLE_COUNT);
				QueryPerformanceCounter(&endTime1);

				LARGE_INTEGER startTime2, endTime2;

				QueryPerformanceCounter(&startTime2);
				emuModel.process(inLeft, inRight, bossEmuOutLeft, bossEmuOutRight, SAMPLE_COUNT);
				QueryPerformanceCounter(&endTime2);

				bool failed = false;
				for (int i = 0; i < SAMPLE_COUNT; ++i) {
					if (bReverbOutLeft[i] != bossEmuOutLeft[i] || bReverbOutRight[i] != bossEmuOutRight[i]) {
						failed = true;
						break;
					}
				}
				if (failed) {
					std::cout << "FAILED\n";
				} else {
					std::cout << "finished OK\n";
				}
				double time1 = double(endTime1.QuadPart - startTime1.QuadPart) / freq.QuadPart;
				double time2 = double(endTime2.QuadPart - startTime2.QuadPart) / freq.QuadPart;
				std::cout << "Elapsed time: " << time1 << " sec / " << time2 << " sec\n\n";
			}
		}
	}

	return 0;
}
