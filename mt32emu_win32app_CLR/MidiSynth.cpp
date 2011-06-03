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

namespace MT32Emu {

MidiSynth *midiSynth;

#define	RENDER_EVERY_MS 1				// provides minimum possible latency
#define	MIN_RENDER_SAMPLES 320	// render at least this number of samples
#define	SAFE_RENDER_SAMPLES 320	// render up to this safe point

class MidiStream {
private:
	static const unsigned int maxPos = 1024;
	unsigned int startpos;
	unsigned int endpos;
	DWORD stream[maxPos][2];
public:
	MidiStream() {
		startpos = 0;
		endpos = 0;
	}

	DWORD PutMessage(DWORD msg, DWORD timestamp) {
		unsigned int newEndpos = endpos;

		newEndpos++;
		if (newEndpos == maxPos) // check for buffer rolloff
			newEndpos = 0;
		if (startpos == newEndpos) // check for buffer full
			return -1;
		stream[endpos][0] = msg;	// ok to put data and update endpos
		stream[endpos][1] = timestamp;
		endpos = newEndpos;
		return 0;
	}

	DWORD GetMessage() {
		if (startpos == endpos) // check for buffer empty
			return -1;
		DWORD msg = stream[startpos][0];
		startpos++;
		if (startpos == maxPos) // check for buffer rolloff
			startpos = 0;
		return msg;
	}

	DWORD PeekMessageTime() {
		if (startpos == endpos) // check for buffer empty
			return -1;
		return stream[startpos][1];
	}

	DWORD PeekMessageTimeAt(unsigned int pos) {
		if (startpos == endpos) // check for buffer empty
			return -1;
		unsigned int peekPos = (startpos + pos) % maxPos;
		return stream[peekPos][1];
	}
} midiStream;

class SynthEventWin32 {
private:
	HANDLE hEvent;

public:
	int Init() {
		hEvent = CreateEvent(NULL, false, true, NULL);
		if (hEvent == NULL) {
			MessageBox(NULL, L"Can't create sync object", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 1;
		}
		return 0;
	}

	void Close() {
		CloseHandle(hEvent);
	}

	void Wait() {
		WaitForSingleObject(hEvent, INFINITE);
	}

	void Release() {
		SetEvent(hEvent);
	}
} synthEvent;

class MidiInWin32 {
private:
	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;
	Bit8u sysexbuf[4096];

static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	if (midiSynth->IsPendingClose())
		return;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		synthEvent.Wait();
		midiSynth->PlaySysex((Bit8u*)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);
		synthEvent.Release();
		std::cout << "Play SysEx message " << pMIDIhdr->dwBytesRecorded << " bytes\n";

		//	Add SysEx Buffer for reuse
		if (midiInAddBuffer(hMidiIn, pMIDIhdr, sizeof(MIDIHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to add SysEx Buffer", NULL, MB_OK | MB_ICONEXCLAMATION);
			return;
		}
		return;
	}
	if (wMsg != MIM_DATA)
		return;

#ifndef RENDER_EVERY_MS
	midiStream.PutMessage(dwParam1, midiSynth->GetTimeStamp());
#else
	midiStream.PutMessage(dwParam1, 0);
#endif
}

public:
	int Init(unsigned int midiDevID) {
		int wResult;

		//	Init midiIn port
		wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)MidiInProc, (DWORD_PTR)&midiSynth, CALLBACK_FUNCTION);
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
} midiIn;

class WaveOutWin32 {
private:
	HWAVEOUT	hWaveOut;
	WAVEHDR		WaveHdr;

public:
	int Init(Bit16s *stream, unsigned int len, unsigned int sampleRate) {
		int wResult;
		PCMWAVEFORMAT wFormat = {WAVE_FORMAT_PCM, 2, sampleRate, sampleRate * 4, 4, 16};

		//	Open waveout device
		wResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wFormat, NULL, (DWORD_PTR)&midiSynth, CALLBACK_NULL);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open waveform output device.", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 2;
		}

		//	Prepare header
		WaveHdr.dwBufferLength = 4 * len;
		WaveHdr.lpData = (LPSTR)stream;
		WaveHdr.dwFlags = WHDR_BEGINLOOP | WHDR_ENDLOOP;
		WaveHdr.dwLoops = -1;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 1", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}
		return 0;
	}

	int Close() {
		int wResult;

		wResult = waveOutReset(hWaveOut);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Reset WaveOut", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Unprepare Wave Header", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = waveOutClose(hWaveOut);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Close WaveOut", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}
		return 0;
	}

	int Start() {
		if (waveOutWrite(hWaveOut, &WaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		return 0;
	}

	int Pause() {
		if (waveOutPause(hWaveOut) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Pause wave playback", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 9;
		}
		return 0;
	}

	int Resume() {
		if (waveOutRestart(hWaveOut) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Pause wave playback", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 9;
		}
		return 0;
	}

	DWORD GetPos() {
		MMTIME mmTime;
		mmTime.wType = TIME_SAMPLES;

		if (waveOutGetPosition(hWaveOut, &mmTime, sizeof MMTIME) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to get current playback position", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 10;
		}
		return mmTime.u.sample;
	}
} waveOut;

int MT32_Report(void *userData, ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	midiSynth->handleReport(type, reportData);
#endif
	return 0;
}

void MidiSynth::handleReport(ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	mt32emuExtInt->handleReport(synth, type, reportData);
#endif
}

void render(void *) {
	midiSynth->Render();
}

void MidiSynth::Render() {
	DWORD msg, timeStamp;
	Bit16s *bufpos;
	DWORD buflen;

	while (!pendingClose) {
		bufpos = stream + 2 * playCursor;
		timeStamp = GetTimeStamp();
		if (timeStamp > playCursor) {
			buflen = timeStamp - playCursor;

#ifdef RENDER_EVERY_MS
			if (buflen < 64) {
				Sleep(1);
				continue;
			}
#else
			if (buflen < SAFE_RENDER_SAMPLES + MIN_RENDER_SAMPLES) {
				Sleep(DWORD((SAFE_RENDER_SAMPLES + MIN_RENDER_SAMPLES - buflen) * 0.028f));
				continue;
			} else {
				buflen -= SAFE_RENDER_SAMPLES;
			}
#endif

		} else {
			buflen = len - playCursor;
			if (timeStamp < SAFE_RENDER_SAMPLES) {
				Sleep(DWORD(SAFE_RENDER_SAMPLES * 0.028f));
			}
		}

		for(;;) {
			timeStamp = midiStream.PeekMessageTime();
			if (timeStamp == -1) {	// if midiStream is empty - exit
				break;
			}

#ifndef RENDER_EVERY_MS
			//	render samples from playCursor to current midiMessage timeStamp
			DWORD playlen = timeStamp - playCursor;
			if (playlen > buflen) {	// if midiMessage is too far - exit
				break;
			}
			if (playlen > 0) {		// if midiMessage with same timeStamp - skip rendering
				synthEvent.Wait();
				synth->render(bufpos, playlen);
				synthEvent.Release();
				playCursor += playlen;
				bufpos += 2 * playlen;
				buflen -= playlen;
			}
#endif

			// play midiMessage
			msg = midiStream.GetMessage();
			synthEvent.Wait();
			synth->playMsg(msg);
			synthEvent.Release();
		}

		//	render rest of samples
		synthEvent.Wait();
		synth->render(bufpos, buflen);
		synthEvent.Release();
		playCursor += buflen;
		if (playCursor >= len) {
			playCursor -= len;
		}
#if MT32EMU_USE_EXTINT == 1
		if (mt32emuExtInt != NULL) {
			mt32emuExtInt->doControlPanelComm(synth, 4 * len);
		}
#endif
	}
}

MidiSynth::MidiSynth() {
	midiSynth = this;
	sampleRate = 32000;
	latency = 50;
	len = UINT(sampleRate * latency / 1000.f);
	midiDevID = 0;
	reverbEnabled = true;
	emuDACInputMode = DACInputMode_GENERATION2;
	pathToROMfiles = "C:/WINDOWS/SYSTEM32/";
}

int MidiSynth::Init() {
	UINT wResult;

	stream = new Bit16s[2 * len];

	//	Init synth
	if (synthEvent.Init()) {
		return 1;
	}
	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	synth->setReverbEnabled(reverbEnabled);
	synth->setDACInputMode(emuDACInputMode);

	//	Init External Interface
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt == NULL) {
		mt32emuExtInt = new MT32Emu::ExternalInterface();
	}
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->start();
	}
#endif

	wResult = waveOut.Init(stream, len, sampleRate);
	if (wResult) return wResult;

	wResult = midiIn.Init(midiDevID);
	if (wResult) return wResult;

	//	Start playing streams
	synth->render(stream, len);

	pendingClose = false;

	wResult = waveOut.Start();
	if (wResult) return wResult;

	playCursor = 0;

	wResult = midiIn.Start();
	if (wResult) return wResult;
	
	_beginthread(&render, 16384, NULL);

	return 0;
}

void MidiSynth::SetMasterVolume(UINT masterVolume) {
	synth->setOutputGain(masterVolume / 100.0f);
	synth->setReverbOutputGain(masterVolume / 147.0f);
}

void MidiSynth::SetReverbEnabled(bool pReverbEnabled) {
	reverbEnabled = pReverbEnabled;
	synth->setReverbEnabled(reverbEnabled);
}

void MidiSynth::SetDACInputMode(DACInputMode pEmuDACInputMode) {
	emuDACInputMode = pEmuDACInputMode;
	synth->setDACInputMode(emuDACInputMode);
}

void MidiSynth::SetParameters(UINT pSampleRate, UINT pMidiDevID, UINT platency) {
	sampleRate = pSampleRate;
	latency = platency;
	len = UINT(latency * sampleRate / 1000.f);
	midiDevID = pMidiDevID;
}

int MidiSynth::Reset() {
	UINT wResult;

	wResult = waveOut.Pause();
	if (wResult) return wResult;

	synthEvent.Wait();
	synth->close();
	delete synth;

	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	synth->setReverbEnabled(reverbEnabled);
	synth->setDACInputMode(emuDACInputMode);
	synthEvent.Release();

	wResult = waveOut.Resume();
	if (wResult) return wResult;

	return wResult;
}

bool MidiSynth::IsPendingClose() {
	return pendingClose;
}

void MidiSynth::PlaySysex(Bit8u *bufpos, DWORD len) {
	synth->playSysex(bufpos, len);
}

DWORD MidiSynth::GetTimeStamp() {
	return waveOut.GetPos() % len;
}

int MidiSynth::Close() {
	UINT wResult;
	pendingClose = true;

	// Close External Interface
#if MT32EMU_USE_EXTINT == 1
	if(mt32emuExtInt != NULL) {
		mt32emuExtInt->stop();
		delete mt32emuExtInt;
		mt32emuExtInt = NULL;
	}
#endif

	waveOut.Pause();

	wResult = midiIn.Close();
	if (wResult) return wResult;

	wResult = waveOut.Close();
	if (wResult) return wResult;

	synthEvent.Wait();
	synth->close();

	// Cleanup memory
	delete synth;
	delete stream;

	synthEvent.Close();
	return 0;
}

}
