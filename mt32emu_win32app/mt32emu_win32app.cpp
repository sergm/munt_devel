// mt32emu_win32app.cpp : main project file.

#include "stdafx.h"
#include "MidiIn.h"

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
int main() {
	std::cout << "Available MIDI in devices:" << std::endl;
	UINT midiInDeviceCount = midiInGetNumDevs();
	for (UINT deviceIx = 0; deviceIx < midiInDeviceCount; deviceIx++) {
		MIDIINCAPS caps = {};
		MMRESULT res = midiInGetDevCaps(deviceIx, &caps, sizeof(caps));
		if (res == MMSYSERR_NOERROR) {
			std::wcout << deviceIx << ": " << caps.szPname << std::endl;
		}
	}
	std::cout << std::endl << "Enter input MIDI device ID: ";
	unsigned int midiDevID = 0;
	std::cin >> midiDevID;

	MT32Emu::MidiSynth &midiSynth = MT32Emu::MidiSynth::getInstance();
	MT32Emu::MidiInWin32 midiIn;

	DWORD res = midiSynth.Init();
	if (res) {
		char str[80];
		wsprintfA(str, "Error %d during initialization", res);
		MessageBoxA(NULL, str, NULL, MB_OK | MB_ICONEXCLAMATION);
		return res;
	}
	midiIn.Init(&midiSynth, midiDevID);
	midiIn.Start();
	std::cin >> res;
	midiIn.Close();
	midiSynth.Close();

	return 0;
}
