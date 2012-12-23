#include "stdafx.h"

using namespace MT32Emu;

int getIntArg(const char * const arg) {
	int value;
	return sscanf(arg, "%i", &value) == 1 ? value : -1;
}

int main(int argc, char *argv[]) {
	if (argc != 4) {
		std::cout << "Usage: mt32emu_areverb_tester <mode> <time> <delay>";
		return 1;
	}
	int mode = getIntArg(argv[1]);
	if (mode < 0 || mode > 3) {
		std::cout << "Wrong value for mode, only modes 0..3 are allowed";
		return 2;
	}
	int time = getIntArg(argv[2]);
	if (time < 0 || time > 7) {
		std::cout << "Wrong value for time, must be in range 0..7";
		return 2;
	}
	int level = getIntArg(argv[3]);
	if (level < 0 || level > 7) {
		std::cout << "Wrong value for level, must be in range 0..7";
		return 2;
	}
#if MT32EMU_USE_REVERBMODEL == 1
	AReverbModel model((ReverbMode)mode);
#elif MT32EMU_USE_REVERBMODEL == 2
	BReverbModel model((ReverbMode)mode);
#endif
	model.open(32000);
	model.setParameters(time, level);
	// This simulates warmup time for allpass buffers to fill with noise
	DWORD warmupDelay = 10000 + GetTickCount() & 32767;
	for (DWORD i = 0; i < warmupDelay; i++) {
		float inLeft = 0.0f;
		float inRight = 0.0f;
		float outLeft, outRight;
		model.process(&inLeft, &inRight, &outLeft, &outRight, 1);
	}
	int i = 0;
	do {
		Bit16s inl = 0;
		Bit16s inr = 0;
		if (!std::cin.eof()) {
			std::cin >> inl >> inr;
		}
		float inLeft = inl / 8192.0f;
		float inRight = inr / 8192.0f;
		float outLeft, outRight;
		model.process(&inLeft, &inRight, &outLeft, &outRight, 1);
		Bit16s outl = Bit16s(outLeft * 8192.0f);
		Bit16s outr = Bit16s(outRight * 8192.0f);
		std::cout << outl << "; " << outr << std::endl;
	} while (i++ < 65535 && model.isActive());
	return 0;
}
