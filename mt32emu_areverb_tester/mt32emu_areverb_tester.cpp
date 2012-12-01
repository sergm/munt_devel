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
	if (mode < 0 || mode > 2) {
		std::cout << "Wrong value for mode, only modes 0..2 are allowed";
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
	AReverbModel model((ReverbMode)mode);
	model.open(32000);
	model.setParameters(time, level);
	do {
		Bit16s inl = 0;
		Bit16s inr = 0;
		if (!std::cin.eof()) {
			std::cin >> inl >> inr;
		}
		inl = 0;
		float inLeft = inl / 8192.0f;
		float inRight = inr / 8192.0f;
		float outLeft, outRight;
		model.process(&inLeft, &inRight, &outLeft, &outRight, 1);
		Bit16s outl = Bit16s(outLeft * 8192.0f);
		Bit16s outr = Bit16s(outRight * 8192.0f);
		std::cout << outl << "; " << outr << std::endl;
	} while (model.isActive());
	return 0;
}
