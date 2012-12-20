// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

// TODO: reference additional headers your program requires here
#include <windows.h>
#include <iostream>
#include <mt32emu.h>
#if MT32EMU_USE_REVERBMODEL == 1
#include <AReverbModel.h>
#elif MT32EMU_USE_REVERBMODEL == 2
#include <BReverbModel.h>
#endif
