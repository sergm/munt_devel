#include "stdafx.h"

using namespace MT32Emu;

int main() {
	AReverbModel model(0);
	model.open(32000);
	model.setParameters(5, 3);
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
