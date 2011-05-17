#pragma once

#include <windows.h>
#include <iostream>
#include <process.h>
#include "mt32emu.h"

#if MT32EMU_USE_EXTINT == 1
#include "externalInterface.h"
#endif

#include "MidiSynth.h"
