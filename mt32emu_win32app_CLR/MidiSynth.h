/* Copyright (C) 2011 Sergey V. Mikayev
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "stdafx.h"

#ifndef MT32EMU_MIDISYNTH_H
#define MT32EMU_MIDISYNTH_H

namespace MT32Emu {

class MidiSynth {
private:
	unsigned int sampleRate;
	unsigned int len;
	unsigned int midiDevID;
	unsigned int latency;
	bool reverbEnabled;
	DACInputMode emuDACInputMode;

	Bit16s *stream;
	char *pathToROMfiles;

	bool pendingClose;
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
	void Render();
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
