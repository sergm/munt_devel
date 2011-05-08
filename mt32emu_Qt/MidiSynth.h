#include "stdafx.h"

#ifndef MT32EMU_MIDISYNTH_H
#define MT32EMU_MIDISYNTH_H

namespace MT32Emu {

class MidiStream;
class SynthEventWin32;
class MidiInWin32;
class WaveOutPa;

class MidiSynth {
private:
	unsigned int sampleRate;
	unsigned int midiDevID;
	unsigned int latency;
	bool reverbEnabled;
	DACInputMode emuDACInputMode;

	QMutex *mutex;
	MidiStream *midiStream;
	MidiInWin32 *midiIn;
	WaveOutPa *waveOut;

	char *pathToROMfiles;

	bool pendingClose;
	double StartTime;
	DWORD playCursor;

	Synth *synth;

public:

#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *mt32emuExtInt;
#endif

	MidiSynth();
	virtual ~MidiSynth();
	int Init();
	int Close();
	int Reset();
	void Render(Bit16s *bufpos, qint64);
	void PlaySysex(Bit8u *bufpos, DWORD len);
	void SetMasterVolume(UINT pMasterVolume);
	void SetReverbEnabled(bool pReverbEnabled);
	void SetDACInputMode(DACInputMode pEmuDACInputMode);
	void SetParameters(UINT pSampleRate, UINT pmidiDevID, UINT platency);
	bool IsPendingClose();
	DWORD GetTimeStamp();
	void handleReport(MT32Emu::ReportType type, const void *reportData);
};

}
#endif
