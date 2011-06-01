// mt32emu_win32app_CLR.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"

using namespace mt32emu_win32app_CLR;

[STAThreadAttribute]

#if MT32EMU_USE_EXTINT == 1
//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
int wmain()
#else
//int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
int main(array<System::String ^> ^args)
#endif
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
	ShowWindow(GetConsoleWindow(), SW_HIDE);

	// Create the main window and run it
	Application::Run(gcnew Form1());

	midiSynth.Close();

	return 0;
}
