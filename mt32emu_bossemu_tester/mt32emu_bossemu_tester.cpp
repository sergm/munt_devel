#include "stdafx.h"

#include "BossEmu.h"

static int getIntArg(const char * const arg) {
	int value;
	return sscanf(arg, "%i", &value) == 1 ? value : -1;
}

int main(int argc, char *argv[]) {
	if (argc != 5) {
		std::cout << "Usage: mt32emu_bossemu_tester <ROM filename> <mode> <time> <level>";
		return 1;
	}

	FILE *romFile = fopen(argv[1], "rb");
	if (romFile == NULL) {
		std::cout << "Can't open ROM file " << argv[1];
		return 2;
	}
	unsigned char rom[32768];
	int romLength = fread(rom, 1, 32768, romFile);

	int mode = getIntArg(argv[2]);
	if (mode < 0 || mode > 3) {
		std::cout << "Wrong value for mode, only modes 0..3 are allowed";
		return 2;
	}

	int time = getIntArg(argv[3]);
	if (time < 0 || time > 7) {
		std::cout << "Wrong value for time, must be in range 0..7";
		return 2;
	}

	int level = getIntArg(argv[4]);
	if (level < 0 || level > 7) {
		std::cout << "Wrong value for level, must be in range 0..7";
		return 2;
	}

	BossEmu model(rom, romLength);
	model.setParameters(mode, time, level);

	short inLeft = 0;
	short inRight = 0;
	short outLeft, outRight;

	// This simulates warmup time for allpass buffers to fill with noise
	DWORD warmupDelay = 10000 + GetTickCount() & 32767;
	for (DWORD i = 0; i < warmupDelay; i++) {
		model.process(&inLeft, &inRight, &outLeft, &outRight, 1);
	}

	int i = 0;
	do {
		if (std::cin.eof()) {
			inLeft = 0;
			inRight = 0;
		} else {
			std::cin >> inLeft >> inRight;
		}
		model.process(&inLeft, &inRight, &outLeft, &outRight, 1);
		std::cout << outLeft << "; " << outRight << std::endl;
	} while (i++ < 65535 && model.isActive());
	return 0;
}
