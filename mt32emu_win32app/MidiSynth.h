#include "stdafx.h"

#ifndef MT32EMU_MIDISYNTH_H
#define MT32EMU_MIDISYNTH_H

namespace MT32Emu {

class MidiSynth {
private:
	unsigned int latency;

public:
	unsigned int sampleRate;
	unsigned int len;
	unsigned int midiDevID;
	int masterVolume;

	Bit8u sysexbuf[4096];

	Bit16s *stream1;
	Bit16s *stream2;

	bool pendingClose;
	DWORD bufferStartTS, bufferStartS;
	DWORD playCursor;

	Synth *synth;

#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *mt32emuExtInt;
#endif

	MidiSynth();
	int Init();
	int Close();
	int Reset();
	void SetMasterVolume(UINT pMasterVolume);
	void SetParameters(UINT pSampleRate, UINT pmidiDevID, UINT platency);
	void Render(Bit16s *bufpos);
};

}
#endif
