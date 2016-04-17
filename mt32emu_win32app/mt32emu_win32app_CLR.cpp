// mt32emu_win32app_CLR.cpp : main project file.

#include "stdafx.h"
#include "MidiIn.h"
#include "Form1.h"

using namespace mt32emu_win32app_CLR;

[STAThreadAttribute]

//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
int main()
{
	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	DWORD res = midiSynth.Init();
	if (res) {
		char str[80];
		wsprintfA(str, "Error %d during initialization", res);
		MessageBoxA(NULL, str, NULL, MB_OK | MB_ICONEXCLAMATION);
		return res;
	}
	midiIn.Init(&midiSynth, midiDevID);
	midiIn.Start();

	//ShowWindow(GetConsoleWindow(), SW_HIDE);

	// Create the main window and run it
	Application::Run(gcnew Form1());

	midiIn.Close();
	midiSynth.Close();

	return 0;
}
