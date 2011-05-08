FORMS	    = mt32emu_Qt.ui
INCLUDEPATH += ../mt32emu/src
LIBS	    += SDL_Net.lib mt32emu.lib winmm.lib portaudio_x86.lib
HEADERS	    = MidiSynth.h stdafx.h
SOURCES     = main.cpp MidiSynth.cpp

QT           += multimedia
