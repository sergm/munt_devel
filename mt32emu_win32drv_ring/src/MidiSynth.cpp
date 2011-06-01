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

#define	RENDER_EVERY_MS 1 // provides minimum possible latency
#define	MIN_RENDER_SAMPLES 320 // render at least this number of samples

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

void printDebug(void *userData, const char *fmt, va_list list) {
}

int MT32_Report(void *userData, ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	midiSynth->handleReport(type, reportData);
#endif
	switch(type) {
	case MT32Emu::ReportType_errorControlROM:
		std::cout << "MT32: Couldn't find Control ROM file\n";
		break;
	case MT32Emu::ReportType_errorPCMROM:
		std::cout << "MT32: Couldn't open PCM ROM file\n";
		break;
	case MT32Emu::ReportType_lcdMessage:
		std::cout << "MT32: LCD-Message: " << (char *)reportData << "\n";
		break;
	default:
		//LOG(LOG_ALL,LOG_NORMAL)("MT32: Report %d",type);
		break;
	}
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
	DWORD playlen;
	Bit16s *bufpos;
	DWORD buflen;

	while (!pendingClose) {
		bufpos = stream + 2 * playCursor;
		timeStamp = waveOut.GetPos() % len;
		if (timeStamp > playCursor) {
			buflen = timeStamp - playCursor;

#ifdef RENDER_EVERY_MS
			if (buflen < 64) {
				Sleep(1);
				continue;
			}
#else
			if (buflen < MIN_RENDER_SAMPLES) {
				Sleep(DWORD((MIN_RENDER_SAMPLES - buflen) * 0.028f));
				continue;
			}
#endif

		} else {
			buflen = len - playCursor;
		}

		for(;;) {
			timeStamp = midiStream.PeekMessageTime();
			if (timeStamp == -1) {	// if midiStream is empty - exit
				break;
			}

			//	render samples from playCursor to current midiMessage timeStamp
			playlen = timeStamp - playCursor;
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
}

int LoadIntValue(char *key, int nDefault) {
	return GetPrivateProfileIntA("mt32emu", key, nDefault, "mt32emu.ini");
}

DWORD LoadStringValue(char *key, char *nDefault, char *destination, DWORD nSize) {
	return GetPrivateProfileStringA("mt32emu", key, nDefault, destination, nSize, "mt32emu.ini");
}

void MidiSynth::LoadSettings() {
	sampleRate = LoadIntValue("SampleRate", 32000);
	latency = LoadIntValue("Latency", 50);
	len = UINT(sampleRate * latency / 1000.f);
	ReloadSettings();
}

void MidiSynth::ReloadSettings() {
	resetEnabled = true;
	if (LoadIntValue("ResetEnabled", 1) == 0) {
		resetEnabled = false;
	}

	reverbEnabled = true;
	if (LoadIntValue("ReverbEnabled", 1) == 0) {
		reverbEnabled = false;
	}

	reverbOverridden = true;
	if (LoadIntValue("ReverbOverridden", 0) == 0) {
		reverbOverridden = false;
	}

	reverbMode = LoadIntValue("ReverbMode", 0);
	reverbTime = LoadIntValue("ReverbTime", 5);
	reverbLevel = LoadIntValue("ReverbLevel", 3);

	outputGain = (float)LoadIntValue("OutputGain", 100);
	if (outputGain < 0.0f) {
		outputGain = -outputGain;
	}
	if (outputGain > 1000.0f) {
		outputGain = 1000.0f;
	}

	reverbOutputGain = (float)LoadIntValue("ReverbOutputGain", 100);
	if (reverbOutputGain < 0.0f) {
		reverbOutputGain = -reverbOutputGain;
	}
	if (reverbOutputGain > 1000.0f) {
		reverbOutputGain = 1000.0f;
	}

	emuDACInputMode = (DACInputMode)LoadIntValue("DACInputMode", DACInputMode_GENERATION2);

	DWORD s = LoadStringValue("PathToROMFiles", "C:/WINDOWS/SYSTEM32/", pathToROMfiles, 254);
	pathToROMfiles[s] = '/';
	pathToROMfiles[s + 1] = 0;
}

void MidiSynth::ApplySettings() {
	synth->setReverbEnabled(reverbEnabled);
	synth->setDACInputMode(emuDACInputMode);
	synth->setOutputGain(outputGain / 100.0f);
	synth->setReverbOutputGain(reverbOutputGain / 147.0f);
	if (reverbOverridden) {
		Bit8u sysex[] = {0x10, 0x00, 0x01, reverbMode, reverbTime, reverbLevel};
		synth->setReverbOverridden(false);
		synth->writeSysex(16, sysex, 6);
		synth->setReverbOverridden(true);
	}
}

int MidiSynth::Init() {
	UINT wResult;

	LoadSettings();

	stream = new Bit16s[2 * len];

	//	Init synth
	if (synthEvent.Init()) {
		return 1;
	}
	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, printDebug, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	ApplySettings();

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

	//	Start playing stream
	synth->render(stream, len);

	pendingClose = false;

	wResult = waveOut.Start();
	if (wResult) return wResult;

	playCursor = 0;
	_beginthread(render, 16384, NULL);
	return 0;
}

void MidiSynth::SetMasterVolume(UINT masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, 0x01};

	sysex[3] = (Bit8u)masterVolume;
	synthEvent.Wait();
	synth->writeSysex(16, sysex, 4);
	synthEvent.Release();
}

int MidiSynth::Reset() {
	UINT wResult;

	ReloadSettings();
	if (!resetEnabled) {
		ApplySettings();
		return 0;
	}

	wResult = waveOut.Pause();
	if (wResult) return wResult;

	synthEvent.Wait();
	synth->close();
	delete synth;

	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, printDebug, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	ApplySettings();
	synthEvent.Release();

	wResult = waveOut.Resume();
	if (wResult) return wResult;

	return wResult;
}

bool MidiSynth::IsPendingClose() {
	return pendingClose;
}

void MidiSynth::PushMIDI(DWORD msg) {
	midiStream.PutMessage(msg, waveOut.GetPos() % len);
}

void MidiSynth::PlaySysex(Bit8u *bufpos, DWORD len) {
	synthEvent.Wait();
	synth->playSysex(bufpos, len);
	synthEvent.Release();
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

	synthEvent.Wait();
	wResult = waveOut.Pause();
	if (wResult) return wResult;

	wResult = waveOut.Close();
	if (wResult) return wResult;

	synth->close();

	// Cleanup memory
	delete synth;
	delete stream;

	synthEvent.Close();
	return 0;
}

}
