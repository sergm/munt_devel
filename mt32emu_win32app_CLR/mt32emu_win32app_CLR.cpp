// mt32emu_win32app_CLR.cpp : main project file.

#include "stdafx.h"
#include "Form1.h"

using namespace mt32emu_win32app_CLR;

[STAThreadAttribute]

#if MT32EMU_USE_EXTINT == 1
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
#else
int main(array<System::String ^> ^args)
#endif
{
	// Enabling Windows XP visual effects before any controls are created
	Application::EnableVisualStyles();
	Application::SetCompatibleTextRenderingDefault(false); 

	InitMT32emu();

	// Create the main window and run it
	Application::Run(gcnew Form1());

	CloseMT32emu();

	return 0;
}
