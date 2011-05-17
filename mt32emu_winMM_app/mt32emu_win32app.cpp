#include "stdafx.h"

using namespace MT32Emu;

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	static MidiSynth midiSynth;
	int wResult;

	midiSynth.Init();

	std::cin >> wResult;

	midiSynth.Close();

	ExitProcess(0);
}