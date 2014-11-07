// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
#pragma once

// MinGW doesn't define UNICODE by default
#ifndef UNICODE
#define UNICODE
#endif

#include <windows.h>
#include <process.h>
#include <iostream>
#include <mt32emu.h>

#include "MidiSynth.h"
