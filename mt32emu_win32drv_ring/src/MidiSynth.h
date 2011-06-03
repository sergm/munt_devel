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
	unsigned int latency;
	char pathToROMfiles[256];
	bool resetEnabled;
	DACInputMode emuDACInputMode;
	float outputGain;
	float reverbOutputGain;
	bool reverbEnabled;
	bool reverbOverridden;
	Bit8u reverbMode;
	Bit8u reverbTime;
	Bit8u reverbLevel;

	Bit16s *stream;

	bool pendingClose;
	DWORD playCursor;

	Synth *synth;

	void LoadSettings();
	void ReloadSettings();
	void ApplySettings();

#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *mt32emuExtInt;
#endif

public:

	MidiSynth();
	int Init();
	int Close();
	int Reset();
	void StoreSettings(
		int newSampleRate,
		int newLatency,
		bool newReverbEnabled,
		bool newReverbOverridden,
		int newReverbMode,
		int newReverbTime,
		int newReverbLevel,
		int newOutputGain,
		int newReverbGain,
		int newDACInputMode);
	void Render();
	void PushMIDI(DWORD msg);
	void PlaySysex(Bit8u *bufpos, DWORD len);
	bool IsPendingClose();
	void handleReport(MT32Emu::ReportType type, const void *reportData);
};

}
#endif
