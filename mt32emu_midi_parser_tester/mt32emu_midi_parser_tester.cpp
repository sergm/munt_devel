#include "stdafx.h"

using namespace MT32Emu;

Bit8u stream1[] = {0x00};
Bit8u stream2[] = {0x00};

void fail(char msg[] = NULL) {
	if (msg != NULL) std::cout << msg << std::endl;
	_getch();
	exit(1);
}

int main() {
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
	class : public MidiStreamParser {
	protected:
		// User-supplied method. Invoked when a complete short MIDI message is parsed in the input MIDI stream.
		virtual void handleShortMessage(const Bit32u message) {
			printf("Got ShortMessage: %06x\n", message);
		}

		// User-supplied method. Invoked when a complete well-formed System Exclusive MIDI message is parsed in the input MIDI stream.
		virtual void handleSysex(const Bit8u stream[], const Bit32u len) {
			std::cout << "Got SysEx:\n";
			for (unsigned int i = 0; i < len; i++) printf("%02x ", stream[i]);
			std::cout << std::endl;
		}

		// User-supplied method. Invoked when a System Realtime MIDI message is parsed in the input MIDI stream.
		virtual void handleSytemRealtimeMessage(const Bit8u realtime) {
			printf("Got SytemRealtimeMessage: %02x\n", realtime);
		}

		// User-supplied method. Invoked when an error occurs during processing the input MIDI stream.
		virtual void printDebug(const char *debugMessage) {
			std::cout << debugMessage << std::endl;
		}
	} parser;
	Bit32u len = sizeof(stream1);
	std::cout << "Sent " << len << " bytes\n";
	parser.parseStream(stream1, len);
	len = sizeof(stream2);
	std::cout << "Sent " << len << " bytes\n";
	parser.parseStream(stream2, len);
	_getch();
	return 0;
}
