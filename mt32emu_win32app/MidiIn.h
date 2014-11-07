/* Copyright (C) 2011, 2014 Sergey V. Mikayev
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

namespace MT32Emu {

class MidiInWin32 {
private:
	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;
	Bit8u sysexbuf[4096];

	static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
		MidiSynth &midiSynth = MidiSynth::getInstance();

		LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
		if (wMsg == MIM_LONGDATA) {
			if (pMIDIhdr->dwBytesRecorded == 0) {
				// 0 length means returning the buffer to the application when closing
				return;
			}
			midiSynth.PlaySysex((Bit8u*)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);
			//std::cout << "Play SysEx message " << pMIDIhdr->dwBytesRecorded << " bytes\n";

			//	Add SysEx Buffer for reuse
			if (midiInAddBuffer(hMidiIn, pMIDIhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
				MessageBox(NULL, L"Failed to add SysEx Buffer", NULL, MB_OK | MB_ICONEXCLAMATION);
				return;
			}
			return;
		}
		if (wMsg != MIM_DATA) return;
		midiSynth.PlayMIDI(dwParam1);
	}

public:
	int Init(MidiSynth *midiSynth, unsigned int midiDevID) {
		int wResult;

		//	Init midiIn port
		wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)MidiInProc, (DWORD_PTR)midiSynth, CALLBACK_FUNCTION);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open midi input device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 5;
		}

		//	Prepare SysEx midiIn buffer
		MidiInHdr.lpData = (LPSTR)sysexbuf;
		MidiInHdr.dwBufferLength = 4096;
		MidiInHdr.dwFlags = 0L;
		wResult = midiInPrepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare midi buffer Header", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 6;
		}

		//	Add SysEx Buffer
		wResult = midiInAddBuffer(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to add SysEx Buffer", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 7;
		}
		return 0;
	}

	int Close() {
		int wResult;

		wResult = midiInReset(hMidiIn);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Reset MidiIn port", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = midiInUnprepareHeader(hMidiIn, &MidiInHdr, sizeof(MIDIHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Unprepare Midi Header", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = midiInClose(hMidiIn);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Close MidiIn port", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		return 0;
	}

	int Start() {
		return midiInStart(hMidiIn);
	}
};

}