#include "stdafx.h"

using namespace MT32Emu;

static unsigned int SampleRate = 32000;
static unsigned int latency = 150;
static unsigned int len = UINT(SampleRate * latency / 2000.f);
static unsigned int midiDevID = 0;
static int masterVolume = 256;

DWORD BufferStartTS, BufferStartS;
DWORD playCursor;
HANDLE SysexEvent;
Synth *synth;
Bit16s *stream1;
Bit16s *stream2;

HMIDIIN hMidiIn;
MIDIHDR MidiInHdr;

HWAVEOUT	hWaveOut;
WAVEHDR		WaveHdr1;
WAVEHDR		WaveHdr2;
bool		pendingClose;

#if MT32EMU_USE_EXTINT == 1
	MT32Emu::ExternalInterface *mt32emuExtInt;
#endif

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
		stream[endpos][1] = BufferStartS + DWORD(SampleRate / 1000.f * (GetTickCount() - BufferStartTS));
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
};

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (pendingClose)
		return;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		WaitForSingleObject(SysexEvent, INFINITE);
		synth->playSysex((Bit8u*)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);
		SetEvent(SysexEvent);
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
	MidiStream *midiStream = (MidiStream*) dwInstance;
	midiStream->PutMessage(dwParam1, dwParam2);
}

void Render(MidiStream *midiStream, Bit16s *bufpos) {
	DWORD msg, timeStamp;
	int buflen = len;
	int playlen;

	for(;;) {
		timeStamp = midiStream->PeekMessageTime();
		if (timeStamp == -1)	// if midiStream is empty - exit
			break;

		//	render samples from playCursor to current midiMessage timeStamp
		playlen = int(timeStamp - playCursor);
		if (playlen > buflen)	// if midiMessage is too far - exit
			break;
		if (playlen < 0) std::cout << "Play MIDI message at neg timeStamp " << timeStamp << ", playCursor " << playCursor << " samples\n";
		if (playlen > 0) {		// if midiMessage with same timeStamp - skip rendering
			WaitForSingleObject(SysexEvent, INFINITE);
			synth->render(bufpos, playlen);
			SetEvent(SysexEvent);
			playCursor += playlen;
			bufpos += 2 * playlen;
			buflen -= playlen;
		}

		// play midiMessage
		msg = midiStream->GetMessage();
		WaitForSingleObject(SysexEvent, INFINITE);
		synth->playMsg(msg);
		SetEvent(SysexEvent);
//		std::cout << "Play MIDI message " << msg << " at " << timeStamp / 1000.f << " ms\n";
	}

	//	render rest of samples
	WaitForSingleObject(SysexEvent, INFINITE);
	synth->render(bufpos, buflen);
	SetEvent(SysexEvent);
	playCursor += buflen;
#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->doControlPanelComm(synth, 4 * len);
	}
#endif
}

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg != WOM_DONE)
		return;

	if (pendingClose)
		return;

	BufferStartS = playCursor + len;
	BufferStartTS = GetTickCount();
	MidiStream *midiStream = (MidiStream*) dwInstance;
	LPWAVEHDR pWaveHdr = LPWAVEHDR(dwParam1);

	Render(midiStream, (Bit16s*)pWaveHdr->lpData);

	// Apply master volume
	for (Bit16s *p = (Bit16s*)pWaveHdr->lpData; p < (Bit16s*)pWaveHdr->lpData + 2 * len; p++) {
		int newSample = (*p * masterVolume) >> 8;

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
	mt32emuExtInt->handleReport(synth, type, reportData);
#endif
	return 0;
}

int InitMT32emu() {
	static MidiStream midiStream;
	static Bit8u sysexbuf[4096];

	UINT wResult;
	PCMWAVEFORMAT wFormat = {WAVE_FORMAT_PCM, 2, SampleRate, SampleRate * 4, 4, 16};

	stream1 = new Bit16s[2 * len];
	stream2 = new Bit16s[2 * len];

//	Init synth
	SysexEvent = CreateEvent(NULL, false, true, NULL);
	synth = new Synth();
	SynthProperties synthProp = {SampleRate, true, true, 0, 0, 0, "C:\\WINDOWS\\SYSTEM32\\",
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

//	Open waveout device
	wResult = waveOutOpen(&hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&wFormat, (DWORD_PTR)waveOutProc,
		(DWORD_PTR)&midiStream, CALLBACK_FUNCTION);
	if (wResult != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to open waveform output device.", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 2;
	}

//	Prepare 2 Headers
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

//	Init midiIn port
	wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)MidiInProc, (DWORD_PTR)&midiStream, CALLBACK_FUNCTION);
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

//	Start playing 2 streams
	synth->render(stream1, len);
	synth->render(stream2, len);

	pendingClose = false;

	if (waveOutWrite(hWaveOut, &WaveHdr1, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 4;
	}
	if (waveOutWrite(hWaveOut, &WaveHdr2, sizeof(WAVEHDR)) != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to write block to device", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 4;
	}

//	Start recording MIDI stream
	playCursor = 0;
	BufferStartS = len;
	BufferStartTS = GetTickCount();
	wResult = midiInStart(hMidiIn);
	return wResult;
}

void SetMT32emuMasterVolume(UINT pMasterVolume) {
	masterVolume = pMasterVolume;
}

void SetMT32emu(UINT pSampleRate, UINT pmidiDevID, UINT platency) {
	SampleRate = pSampleRate;
	latency = platency;
	len = UINT(latency * SampleRate / 2000.f);
	midiDevID = pmidiDevID;
}

int ResetMT32emu() {
	UINT wResult;

//	Pause Playback
	wResult = waveOutPause(hWaveOut);
	if (wResult != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to Pause wave playback", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 9;
	}

//	Close synth
	WaitForSingleObject(SysexEvent, INFINITE);
	synth->close();
	delete synth;

//	Init synth
	synth = new Synth();
	SynthProperties synthProp = {SampleRate, true, true, 0, 0, 0, "C:\\WINDOWS\\SYSTEM32\\",
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		MessageBox(NULL, L"Can't open Synth", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 1;
	}
	SetEvent(SysexEvent);

//	Resume Playback
	wResult = waveOutRestart(hWaveOut);
	if (wResult != MMSYSERR_NOERROR) {
		MessageBox(NULL, L"Failed to Pause wave playback", NULL, MB_OK | MB_ICONEXCLAMATION);
		return 9;
	}
	return wResult;
}

int CloseMT32emu() {
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

//	Close synth
	synth->close();
	delete synth;
	delete stream1;
	delete stream2;

	CloseHandle(SysexEvent);
	return 0;
}