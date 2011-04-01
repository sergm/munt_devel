#include "stdafx.h"

using namespace MT32Emu;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	MidiSynth *midiSynth;
	int wResult;

	midiSynth = GetMidiSynth();
	midiSynth->Init();

	std::cin >> wResult;

	midiSynth->Close();

	ExitProcess(0);
}