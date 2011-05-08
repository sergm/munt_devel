#include "stdafx.h"

namespace MT32Emu {

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
};

class MidiInWin32 {
private:
	MidiSynth *midiSynth;
	MidiStream *midiStream;

	HMIDIIN hMidiIn;
	MIDIHDR MidiInHdr;
	Bit8u sysexbuf[4096];

static void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR /* dwParam2 */) {
	MidiInWin32 *inst;

	inst = (MidiInWin32*)dwInstance;

	if (inst->midiSynth->IsPendingClose())
		return;

	LPMIDIHDR pMIDIhdr = (LPMIDIHDR)dwParam1;
	if (wMsg == MIM_LONGDATA) {
		inst->midiSynth->PlaySysex((Bit8u*)pMIDIhdr->lpData, pMIDIhdr->dwBytesRecorded);
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
	inst->midiStream->PutMessage(dwParam1, inst->midiSynth->GetTimeStamp());
}

public:
	int Init(MidiSynth *pMidiSynth, MidiStream *pMidiStream, unsigned int midiDevID) {
		int wResult;

		midiSynth = pMidiSynth;
		midiStream = pMidiStream;

		//	Init midiIn port
		wResult = midiInOpen(&hMidiIn, midiDevID, (DWORD_PTR)MidiInProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
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

class WaveOutPa {
private:
PaStream *stream;

public:
	static int Render(const void *, void *output, unsigned long frameCount,
		const PaStreamCallbackTimeInfo *, PaStreamCallbackFlags, void *userData) {

		MidiSynth *midiSynth = (MidiSynth *)userData;
		if (midiSynth->IsPendingClose()) {
			return paComplete;
		}
		midiSynth->Render((Bit16s*)output, frameCount);
		return paContinue;
	}

	int Init(MidiSynth *midiSynth, unsigned int sampleRate) {
		Pa_Initialize();
		PaStreamParameters p = {Pa_GetDefaultOutputDevice(), 2, paInt16, 0.05, NULL};
		return Pa_OpenStream(&stream, NULL, &p, sampleRate, paFramesPerBufferUnspecified,
			paNoFlag, &WaveOutPa::Render, midiSynth);
	}

	int Close() {
		Pa_CloseStream(stream);
		return 0;
	}

	int Start() {
		Pa_StartStream(stream);
		return 0;
	}

	int Pause() {
		Pa_StopStream(stream);
		return 0;
	}

	int Resume() {
		return Start();
	}
};


#if MT32EMU_USE_EXTINT == 1
int MT32_Report(void * /* userData */, ReportType type, const void *reportData) {
//	mt32emuExtInt->handleReport(type, reportData);
	return 0;
}
void MidiSynth::handleReport(ReportType type, const void *reportData) {
	mt32emuExtInt->handleReport(synth, type, reportData);
#else
int MT32_Report(void *, ReportType, const void *) {
	return 0;
}
void MidiSynth::handleReport(ReportType, const void *) {
#endif
}

void MidiSynth::Render(Bit16s *startpos, qint64 len) {
	DWORD msg, timeStamp;
	int buflen = len;
	int playlen;
	Bit16s *bufpos = startpos;

	// Set timestamp of buffer start
	bufferStartS = playCursor;
	bufferStartTS = clock() - latency;

	for(;;) {
		timeStamp = midiStream->PeekMessageTime();
		if (timeStamp == -1)	// if midiStream is empty - exit
			break;

		//	render samples from playCursor to current midiMessage timeStamp
		playlen = int(timeStamp - playCursor);
		if (playlen > buflen)	// if midiMessage is too far - exit
			break;
		if (playlen < 0) {
			std::cout << "Play MIDI message at neg timeStamp " << timeStamp << ", playCursor " << playCursor << " samples\n";
		}
		if (playlen > 0) {		// if midiMessage with same timeStamp - skip rendering
			mutex->lock();
			synth->render(bufpos, playlen);
			mutex->unlock();
			playCursor += playlen;
			bufpos += 2 * playlen;
			buflen -= playlen;
		}

		// play midiMessage
		msg = midiStream->GetMessage();
		mutex->lock();
		synth->playMsg(msg);
		mutex->unlock();
	}

	//	render rest of samples
	mutex->lock();
	synth->render(bufpos, buflen);
	mutex->unlock();

	playCursor += buflen;

#if MT32EMU_USE_EXTINT == 1
	if (mt32emuExtInt != NULL) {
		mt32emuExtInt->doControlPanelComm(synth, 4 * len);
	}
#endif
}

MidiSynth::MidiSynth() {
	sampleRate = 32000;
	latency = 50;
	midiDevID = 0;
	reverbEnabled = true;
	emuDACInputMode = DACInputMode_GENERATION2;
	pathToROMfiles = "C:/WINDOWS/SYSTEM32/";

	mutex = new QMutex;
	waveOut = new WaveOutPa;
	midiStream = new MidiStream;
	midiIn = new MidiInWin32;
	synth = new Synth();
}

int MidiSynth::Init() {
	QMessageBox msgBox;
	UINT wResult;

	//	Init synth
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		msgBox.setText("Can't open Synth");
		msgBox.exec();
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

	wResult = waveOut->Init(this, sampleRate);
	if (wResult) {
		msgBox.setText("Can't open WaveOut");
		msgBox.exec();
		return wResult;
	}

	wResult = midiIn->Init(this, midiStream, midiDevID);
	if (wResult) {
		msgBox.setText("Can't open MidiIn");
		msgBox.exec();
		return wResult;
	}

	pendingClose = false;

	playCursor = 0;
	bufferStartS = 0;
	bufferStartTS = clock() - latency;

	wResult = waveOut->Start();
	if (wResult) {
		msgBox.setText("Can't start WaveOut");
		msgBox.exec();
		return wResult;
	}

	wResult = midiIn->Start();
	if (wResult) {
		msgBox.setText("Can't start MidiIn");
		msgBox.exec();
		return wResult;
	}
	
	return 0;
}

void MidiSynth::SetMasterVolume(UINT masterVolume) {
	Bit8u sysex[] = {0x10, 0x00, 0x16, 0x01};

	sysex[3] = (Bit8u)masterVolume;
	mutex->lock();
	synth->writeSysex(16, sysex, 4);
	mutex->unlock();
}

void MidiSynth::SetReverbEnabled(bool pReverbEnabled) {
	reverbEnabled = pReverbEnabled;
	mutex->lock();
	synth->setReverbEnabled(reverbEnabled);
	mutex->unlock();
}

void MidiSynth::SetDACInputMode(DACInputMode pEmuDACInputMode) {
	emuDACInputMode = pEmuDACInputMode;
	mutex->lock();
	synth->setDACInputMode(emuDACInputMode);
	mutex->unlock();
}

void MidiSynth::SetParameters(UINT pSampleRate, UINT pMidiDevID, UINT platency) {
	sampleRate = pSampleRate;
	latency = platency;
	midiDevID = pMidiDevID;
}

int MidiSynth::Reset() {
	QMessageBox msgBox;
	UINT wResult;

	wResult = waveOut->Pause();
	if (wResult) return wResult;

	mutex->lock();
	synth->close();
	delete synth;

	synth = new Synth();
	SynthProperties synthProp = {sampleRate, true, true, 0, 0, 0, pathToROMfiles,
		NULL, MT32_Report, NULL, NULL, NULL};
	if (!synth->open(synthProp)) {
		msgBox.setText("Can't open Synth");
		msgBox.exec();
		return 1;
	}
	synth->setReverbEnabled(reverbEnabled);
	synth->setDACInputMode(emuDACInputMode);
	mutex->unlock();

	wResult = waveOut->Resume();
	if (wResult) return wResult;

	return wResult;
}

bool MidiSynth::IsPendingClose() {
	return pendingClose;
}

void MidiSynth::PlaySysex(Bit8u *bufpos, DWORD len) {
	mutex->lock();
	synth->playSysex(bufpos, len);
	mutex->unlock();
}

DWORD MidiSynth::GetTimeStamp() {
	return bufferStartS + DWORD(sampleRate / 1000.f * (clock() - bufferStartTS));
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

	mutex->lock();

	wResult = midiIn->Close();
	if (wResult) return wResult;

	wResult = waveOut->Close();
	if (wResult) return wResult;

	synth->close();

	return 0;
}

MidiSynth::~MidiSynth() {
	delete synth;
	delete midiIn;
	delete midiStream;
	delete waveOut;
	delete mutex;
}

}