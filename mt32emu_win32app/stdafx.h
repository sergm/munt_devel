// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#define _X86_

#include "targetver.h"

#include <stdio.h>
#include <tchar.h>
#include <windows.h>
#include <iostream>

// TODO: reference additional headers your program requires here

#include "mt32emu.h"

#if MT32EMU_USE_EXTINT == 1
#include "externalInterface.h"
#endif

#include "MidiSynth.h"
