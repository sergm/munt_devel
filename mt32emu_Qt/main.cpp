#include "stdafx.h"
#include <QtGui>
#include "ui_mt32emu_Qt.h"

using namespace MT32Emu;

int main(int argv, char **args)
{
	MidiSynth *midiSynth;

    QApplication app(argv, args);
    app.setApplicationName("Roland MT-32 Emulator");

	midiSynth = new MidiSynth;
	midiSynth->Init();

	QDialog mainWindow;
	Ui::MainWindow ui_MainWindow;
    ui_MainWindow.setupUi(&mainWindow);

	mainWindow.exec();

	midiSynth->Close();
	delete midiSynth;

    return 0;
}
