#include "stdafx.h"

using namespace MT32Emu;

Bit8u stream[] = {0};
Bit32u len = sizeof(stream);

void fail(char msg[] = NULL) {
	if (msg != NULL) std::cout << msg << std::endl;
	_getch();
	exit(1);
}

int main(int argc, char *argv[]) {
	Synth &synth = *new Synth();
	FileStream controlROMFile;
	FileStream pcmROMFile;
	bool ok = controlROMFile.open("CM32L_CONTROL.ROM");
	if (!ok) fail("Invalid controlROMFile");
	ok = pcmROMFile.open("CM32L_PCM.ROM");
	if (!ok) fail("Invalid pcmROMFile");
	const ROMImage *controlROMImage = ROMImage::makeROMImage(&controlROMFile);
	const ROMImage *pcmROMImage = ROMImage::makeROMImage(&pcmROMFile);
	ok = synth.open(*controlROMImage, *pcmROMImage, AnalogOutputMode_ACCURATE);
	if (!ok) fail("Failed to open synth");
	Bit32u parsedLength = synth.playRawMidiStream(stream, len);
	std::cout << "Sent=" << len << ", parsed=" << parsedLength << std::endl;
	_getch();
	return 0;
}
