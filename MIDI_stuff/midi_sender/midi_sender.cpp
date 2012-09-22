#include "stdafx.h"

static const int SYSEX_SIZE = 120;
static const int SYSEX_SRV_LEN = 10;
static LARGE_INTEGER freq;

static void checkError(MMRESULT res) {
	if (res != MMSYSERR_NOERROR) {
		ExitProcess(res);
	}
}

static void sendSysex(HMIDIOUT hmo, LPSTR sysex, UINT len) {
	MIDIHDR mh;
	LARGE_INTEGER time;
	_int64 startTime;

	int sendDelay = 1 + len / 3;
	mh.lpData = sysex;
	mh.dwBufferLength = len;
	mh.dwFlags = 0;
	checkError(midiOutPrepareHeader(hmo, &mh, sizeof(MIDIHDR)));
	checkError(midiOutLongMsg(hmo, &mh, sizeof(MIDIHDR)));
	QueryPerformanceCounter(&time);
	startTime = time.QuadPart;
	for(;;) {
		QueryPerformanceCounter(&time);
		int msDelay = int(1e3 * (time.QuadPart - startTime) / freq.QuadPart);
		if (sendDelay <= msDelay) break;
		Sleep(1 + sendDelay - msDelay);
	}
	checkError(midiOutUnprepareHeader(hmo, &mh, sizeof(MIDIHDR)));
}

static long getAddress(LPSTR sysex) {
	return (sysex[5] << 14) | (sysex[6] << 7) | sysex[7];
}

static void setAddress(LPSTR sysex, long addr) {
	sysex[7] = char(addr & 0x7F);
	sysex[6] = char((addr >> 7) & 0x7F);
	sysex[5] = char((addr >> 14) & 0x7F);
}

static void calcChecksum(LPSTR sysex, UINT len) {
	long sum = 0;
	for (UINT i = 5; i < len - 2; i++) {
		sum +=  sysex[i];
	}
	sysex[len - 2] = char(-sum & 0x7F);
}

static void sendLongSysex(HMIDIOUT hmo, LPSTR sysex, UINT len) {
	if (len <= SYSEX_SIZE) {
		sendSysex(hmo, sysex, len);
	} else {
		char sysexPart[SYSEX_SIZE];
		memcpy(sysexPart, sysex, 5);
		long sysexAddress = getAddress(sysex);
		char *sysexData = &sysex[8];
		UINT dataLen = len - SYSEX_SRV_LEN;
		while (dataLen > 0) {
			UINT partDataLen = dataLen > SYSEX_SIZE - SYSEX_SRV_LEN ? SYSEX_SIZE - SYSEX_SRV_LEN : dataLen;
			setAddress(sysexPart, sysexAddress);
			memcpy(&sysexPart[8], sysexData, partDataLen);
			calcChecksum(sysexPart, partDataLen + SYSEX_SRV_LEN);
			sysexPart[partDataLen + SYSEX_SRV_LEN - 1] = '\xF7';
			sendSysex(hmo, sysexPart, partDataLen + SYSEX_SRV_LEN);
			sysexAddress += partDataLen;
			sysexData += partDataLen;
			dataLen -= partDataLen;
		}
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	QueryPerformanceFrequency(&freq);

	FILE *file;
	file = fopen("init.syx", "rb");
	if (file == NULL) {
		std::cout << "File cannot be opened\n";
		return 1;
	}
	fseek(file, 0, SEEK_END);
	long fileSize = ftell(file);
	char *fileBuffer = new char[fileSize];
	fseek(file, 0, SEEK_SET);
	fread(fileBuffer, sizeof(char), fileSize, file);
	fclose(file);

	HMIDIOUT hmo;
	UINT devID = 0;
	checkError(midiOutOpen(&hmo, devID, NULL, NULL, CALLBACK_NULL));
	long sysexBeginPos = -1;
	for (long pos = 0; pos < fileSize; pos++) {
		if (fileBuffer[pos] == '\xF0') {
			sysexBeginPos = pos;
		}
		else if (sysexBeginPos != -1 && fileBuffer[pos] == '\xF7') {
			sendLongSysex(hmo, &fileBuffer[sysexBeginPos], pos - sysexBeginPos + 1);
			sysexBeginPos = -1;
		}
	}
	ExitProcess(midiOutClose(hmo));
}
