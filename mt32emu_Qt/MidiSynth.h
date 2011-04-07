#include "stdafx.h"

#ifndef MT32EMU_MIDISYNTH_H
#define MT32EMU_MIDISYNTH_H

namespace MT32Emu {

class MidiStream;
class SynthEventWin32;
class MidiInWin32;
class WaveOutWin32;

class MidiSynth {
private:
	unsigned int sampleRate;
	unsigned int len;
	unsigned int midiDevID;
	unsigned int latency;

//	midiSynth = this;
	MidiStream *midiStream;
	SynthEventWin32 *synthEvent;
	MidiInWin32 *midiIn;
	WaveOutWin32 *waveOut;
	//	WaveOutQt *waveOut;

	Bit16s *stream1;
	Bit16s *stream2;
	char *pathToROMfiles;

	bool pendingClose;
	DWORD bufferStartTS, bufferStartS;
	DWORD playCursor;

	Synth *synth;

public:

#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *mt32emuExtInt;
#endif

	MidiSynth();
	int Init();
	int Close();
	int Reset();
	void Render(Bit16s *bufpos);
	void PlaySysex(Bit8u *bufpos, DWORD len);
	void SetMasterVolume(UINT pMasterVolume);
	void SetParameters(UINT pSampleRate, UINT pmidiDevID, UINT platency);
	bool IsPendingClose();
	DWORD GetTimeStamp();
	void handleReport(MT32Emu::ReportType type, const void *reportData);
};

}
#endif
