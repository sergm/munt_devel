#include "stdafx.h"

using namespace MT32Emu;

int main(int argc, char *argv[]) {
	Analog *analog = Analog::createAnalogModel();
	int i = 0;
	do {
		Sample inLeft = 0;
		if (analog->getDACStreamsLength(1) > 0) std::cin >> inLeft;
		Sample silence = 0;
		Sample outStream[2];
		Sample *outP = outStream;
		analog->process(&outP, &inLeft, &silence, &silence, &silence, &silence, &silence, 1);
		printf("%.16e\n", outStream[0]);
	} while (!std::cin.eof() && (i++ < 65535));
	return 0;
}
