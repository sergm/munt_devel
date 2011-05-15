#include "stdafx.h"

namespace MT32Emu {

MidiSynth *midiSynth;

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
	midiStream.PutMessage(dwParam1, midiSynth->GetTimeStamp());
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
	WAVEHDR		WaveHdr1;
	WAVEHDR		WaveHdr2;
	WAVEHDR		WaveHdr3;
	WAVEHDR		WaveHdr4;

static void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2) {
	if (uMsg != WOM_DONE)
		return;

	if (midiSynth->IsPendingClose())
		return;

	LPWAVEHDR pWaveHdr = LPWAVEHDR(dwParam1);
	midiSynth->Render((Bit16s*)pWaveHdr->lpData);

	if (waveOutWrite(hWaveOut, pWaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
	}
}

public:
	int Init(Bit16s *stream1, Bit16s *stream2, Bit16s *stream3, Bit16s *stream4, unsigned int len, unsigned int sampleRate) {
		int wResult;
		PCMWAVEFORMAT wFormat = {WAVE_FORMAT_PCM, 2, sampleRate, sampleRate * 4, 4, 16};

		//	Open waveout device
		wResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wFormat, (DWORD_PTR)waveOutProc, (DWORD_PTR)&midiSynth, CALLBACK_FUNCTION);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open waveform output device.", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 2;
		}

		//	Prepare 4 Headers
		WaveHdr1.lpData = (LPSTR)stream1;
		WaveHdr1.dwBufferLength = 4 * len;
		WaveHdr1.dwFlags = 0L;
		WaveHdr1.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr1, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 1", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr2.lpData = (LPSTR)stream2;
		WaveHdr2.dwBufferLength = 4 * len;
		WaveHdr2.dwFlags = 0L;
		WaveHdr2.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr2, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 2", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr3.lpData = (LPSTR)stream3;
		WaveHdr3.dwBufferLength = 4 * len;
		WaveHdr3.dwFlags = 0L;
		WaveHdr3.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr3, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 3", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr4.lpData = (LPSTR)stream4;
		WaveHdr4.dwBufferLength = 4 * len;
		WaveHdr4.dwFlags = 0L;
		WaveHdr4.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr4, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 4", NULL, MB_OK | MB_ICONEXCLAMATION);
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

		wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr1, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Unprepare Wave Header", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 8;
		}

		wResult = waveOutUnprepareHeader(hWaveOut, &WaveHdr2, sizeof(WAVEHDR));
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
		if (waveOutWrite(hWaveOut, &WaveHdr1, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr2, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr3, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr4, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
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

	int GetPos() {
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

void MidiSynth::Render(Bit16s *startpos) {
	DWORD msg, timeStamp;
	int buflen = len;
	int playlen;
	Bit16s *bufpos = startpos;

	for(;;) {
		timeStamp = midiStream.PeekMessageTime();
		if (timeStamp == -1)	// if midiStream is empty - exit
			break;

		//	render samples from playCursor to current midiMessage timeStamp
		playlen = int(timeStamp - playCursor);
		if (playlen > buflen)	// if midiMessage is too far - exit
			break;
//		if (playlen < 0) std::cout << "Play MIDI message at neg timeStamp " << timeStamp << ", playCursor " << playCursor << " samples\n";
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
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->doControlPanelComm(synth, 4 * len);
	}
#endif
}

MidiSynth::MidiSynth() {
	midiSynth = this;
	sampleRate = 32000;
	latency = 75;
	len = UINT(sampleRate * latency / 4000.f);
	midiDevID = 0;
	reverbEnabled = true;
	emuDACInputMode = DACInputMode_GENERATION2;
	pathToROMfiles = "C:/WINDOWS/SYSTEM32/";
}

int MidiSynth::Init() {
	UINT wResult;

	stream1 = new Bit16s[2 * len];
	stream2 = new Bit16s[2 * len];
	stream3 = new Bit16s[2 * len];
	stream4 = new Bit16s[2 * len];

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

	wResult = waveOut.Init(stream1, stream2, stream3, stream4, len, sampleRate);
	if (wResult) return wResult;

	wResult = midiIn.Init(midiDevID);
	if (wResult) return wResult;

	//	Start playing 2 streams
	synth->render(stream1, len);
	synth->render(stream2, len);
	synth->render(stream3, len);
	synth->render(stream4, len);

	pendingClose = false;

	wResult = waveOut.Start();
	if (wResult) return wResult;

	playCursor = 0;

	wResult = midiIn.Start();
	if (wResult) return wResult;
	
	return 0;
}

void MidiSynth::SetMasterVolume(UINT masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, 0x01};

	sysex[3] = (Bit8u)masterVolume;
	synthEvent.Wait();
	synth->writeSysex(16, sysex, 4);
	synthEvent.Release();
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
	len = UINT(latency * sampleRate / 4000.f);
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
	return waveOut.GetPos();
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

	wResult = midiIn.Close();
	if (wResult) return wResult;

	wResult = waveOut.Close();
	if (wResult) return wResult;

	synth->close();

	// Cleanup memory
	delete synth;
	delete stream1;
	delete stream2;

	synthEvent.Close();
	return 0;
}

}