#include "stdafx.h"

namespace MT32Emu {

static MidiSynth midiSynth;

MidiSynth* GetMidiSynth() {
	return &midiSynth;
}

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

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (midiSynth.pendingClose)
		return;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		WaitForSingleObject(midiSynth.sysexEvent, INFINITE);
		midiSynth.synth->playSysex((Bit8u*)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);
		SetEvent(midiSynth.sysexEvent);
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
	midiStream.PutMessage(dwParam1, midiSynth.bufferStartS + DWORD(midiSynth.sampleRate / 1000.f * (GetTickCount() - midiSynth.bufferStartTS)));
}

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg != WOM_DONE)
		return;

	if (midiSynth.pendingClose)
		return;

	midiSynth.bufferStartS = midiSynth.playCursor + midiSynth.len;
	midiSynth.bufferStartTS = GetTickCount();
	LPWAVEHDR pWaveHdr = LPWAVEHDR(dwParam1);

	midiSynth.Render((Bit16s*)pWaveHdr->lpData);

	// Apply master volume
	for (Bit16s *p = (Bit16s*)pWaveHdr->lpData; p < (Bit16s*)pWaveHdr->lpData + 2 * midiSynth.len; p++) {
		int newSample = (*p * midiSynth.masterVolume) >> 8;

		if (newSample > 32767)
			newSample = 32767;

		if (newSample < -32768)
			newSample = -32768;

		*p = newSample;
	}

	if (waveOutWrite(hWaveOut, pWaveHdr, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
	}
}

int MT32_Report(void *userData, MT32Emu::ReportType type, const void *reportData) {
#if MT32EMU_USE_EXTINT == 1
	midiSynth.mt32emuExtInt->handleReport(midiSynth.synth, type, reportData);
#endif
	return 0;
}

class MidiInWin32 {
private:
	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;

public:
	int Init() {
		int wResult;

		//	Init midiIn port
		wResult = midiInOpen(&hMidiIn, midiSynth.midiDevID, (DWORD_PTR)MidiInProc, (DWORD_PTR)&midiSynth, CALLBACK_FUNCTION);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open midi input device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 5;
		}

		//	Prepare SysEx midiIn buffer
		MidiInHdr.lpData = (LPSTR)midiSynth.sysexbuf;
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
} midiInWin32;

class WaveOutWin32 {
private:
	HWAVEOUT	hWaveOut;
	WAVEHDR		WaveHdr1;
	WAVEHDR		WaveHdr2;

public:
	int Init() {
		int wResult;
		PCMWAVEFORMAT wFormat = {WAVE_FORMAT_PCM, 2, midiSynth.sampleRate, midiSynth.sampleRate * 4, 4, 16};

		//	Open waveout device
		wResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wFormat, (DWORD_PTR)waveOutProc, (DWORD_PTR)&midiSynth, CALLBACK_FUNCTION);
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to open waveform output device.", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 2;
		}

		//	Prepare 2 Headers
		WaveHdr1.lpData = (LPSTR)midiSynth.stream1;
		WaveHdr1.dwBufferLength = 4 * midiSynth.len;
		WaveHdr1.dwFlags = 0L;
		WaveHdr1.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr1, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 1", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 3;
		}

		WaveHdr2.lpData = (LPSTR)midiSynth.stream2;
		WaveHdr2.dwBufferLength = 4 * midiSynth.len;
		WaveHdr2.dwFlags = 0L;
		WaveHdr2.dwLoops = 0L;
		wResult = waveOutPrepareHeader(hWaveOut, &WaveHdr2, sizeof(WAVEHDR));
		if (wResult != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to Prepare Header 2", NULL, MB_OK | MB_ICONEXCLAMATION);
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

	int Write() {
		if (waveOutWrite(hWaveOut, &WaveHdr1, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
			MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
			return 4;
		}
		if (waveOutWrite(hWaveOut, &WaveHdr2, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
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
} waveOutWin32;

void MidiSynth::Render(Bit16s *bufpos) {
	DWORD msg, timeStamp;
	int buflen = len;
	int playlen;

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
			WaitForSingleObject(sysexEvent, INFINITE);
			synth->render(bufpos, playlen);
			SetEvent(sysexEvent);
			playCursor += playlen;
			bufpos += 2 * playlen;
			buflen -= playlen;
		}

		// play midiMessage
		msg = midiStream.GetMessage();
		WaitForSingleObject(sysexEvent, INFINITE);
		synth->playMsg(msg);
		SetEvent(sysexEvent);
//		std::cout << "Play MIDI message " << msg << " at " << timeStamp / 1000.f << " ms\n";
	}

	//	render rest of samples
	WaitForSingleObject(sysexEvent, INFINITE);
	synth->render(bufpos, buflen);
	SetEvent(sysexEvent);
	playCursor += buflen;
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->doControlPanelComm(synth, 4 * len);
	}
#endif
}

MidiSynth::MidiSynth() {
	sampleRate = 32000;
	latency = 150;
	len = UINT(sampleRate * latency / 2000.f);
	midiDevID = 0;
	masterVolume = 256;
}

int MidiSynth::Init(void) {
	UINT wResult;

	stream1 = new Bit16s[2 * len];
	stream2 = new Bit16s[2 * len];

	//	Init synth
	sysexEvent = CreateEvent(NULL, false, true, NULL);
	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, "C:\\WINDOWS\\SYSTEM32\\",
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}

	//	Init External Interface
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt == NULL) {
		mt32emuExtInt = new MT32Emu::ExternalInterface();
	}
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->start();
	}
#endif

	wResult = waveOutWin32.Init();
	if (wResult) return wResult;

	wResult = midiInWin32.Init();
	if (wResult) return wResult;

	//	Start playing 2 streams
	synth->render(stream1, len);
	synth->render(stream2, len);

	pendingClose = false;

	wResult = waveOutWin32.Write();
	if (wResult) return wResult;

	playCursor = 0;
	bufferStartS = len;
	bufferStartTS = GetTickCount();

	wResult = midiInWin32.Start();
	if (wResult) return wResult;
	
	return 0;
}

void MidiSynth::SetMasterVolume(UINT pMasterVolume) {
	masterVolume = pMasterVolume;
}

void MidiSynth::SetParameters(UINT pSampleRate, UINT pMidiDevID, UINT platency) {
	sampleRate = pSampleRate;
	latency = platency;
	len = UINT(latency * sampleRate / 2000.f);
	midiDevID = pMidiDevID;
}

int MidiSynth::Reset() {
	UINT wResult;

	wResult = waveOutWin32.Pause();
	if (wResult) return wResult;

	WaitForSingleObject(sysexEvent, INFINITE);
	synth->close();
	delete synth;

	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, "C:\\WINDOWS\\SYSTEM32\\",
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	SetEvent(sysexEvent);

	wResult = waveOutWin32.Resume();
	if (wResult) return wResult;

	return wResult;
}

int MidiSynth::Close() {
	UINT wResult;
	pendingClose = true;

//	Close External Interface
#if MT32EMU_USE_EXTINT == 1
	if(mt32emuExtInt != NULL) {
		mt32emuExtInt->stop();
		delete mt32emuExtInt;
		mt32emuExtInt = NULL;
	}
#endif

	wResult = midiInWin32.Close();
	if (wResult) return wResult;

	wResult = waveOutWin32.Close();
	if (wResult) return wResult;

	synth->close();

	// Cleanup memory
	delete synth;
	delete stream1;
	delete stream2;

	CloseHandle(sysexEvent);
	return 0;
}

}