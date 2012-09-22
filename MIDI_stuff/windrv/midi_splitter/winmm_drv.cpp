/* Copyright (C) 2003, 2004, 2005 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

#define MAX_DRIVERS 1
#define MAX_CLIENTS 3 // Per driver

static const char midiPortName[] = "Buggy USB-MIDI device";
static const int SYSEX_SIZE = 120;
static const int SYSEX_SRV_LEN = 10;

static int driverCount;
static HMIDIOUT hmo = NULL;
static LARGE_INTEGER freq;

struct Driver {
	bool open;
	int clientCount;
	HDRVR hdrvr;
	struct Client {
		bool allocated;
		DWORD instance;
		DWORD flags;
		DWORD_PTR callback;
		DWORD synth_instance;
	} clients[MAX_CLIENTS];
} drivers[MAX_DRIVERS];

STDAPI_(LONG) DriverProc(DWORD dwDriverID, HDRVR hdrvr, WORD wMessage, DWORD dwParam1, DWORD dwParam2) {
	switch(wMessage) {
	case DRV_LOAD:
		memset(drivers, 0, sizeof(drivers));
		driverCount = 0;
		QueryPerformanceFrequency(&freq);
		return DRV_OK;
	case DRV_ENABLE:
		return DRV_OK;
	case DRV_OPEN:
		int driverNum;
		if (driverCount == MAX_DRIVERS) {
			return 0;
		} else {
			for (driverNum = 0; driverNum < MAX_DRIVERS; driverNum++) {
				if (!drivers[driverNum].open) {
					break;
				}
				if (driverNum == MAX_DRIVERS) {
					return 0;
				}
			}
		}
		drivers[driverNum].open = true;
		drivers[driverNum].clientCount = 0;
		drivers[driverNum].hdrvr = hdrvr;
		driverCount++;
		return DRV_OK;
	case DRV_INSTALL:
	case DRV_PNPINSTALL:
		return DRV_OK;
	case DRV_QUERYCONFIGURE:
		return 0;
	case DRV_CONFIGURE:
		return DRVCNF_OK;
	case DRV_CLOSE:
		for (int i = 0; i < MAX_DRIVERS; i++) {
			if (drivers[i].open && drivers[i].hdrvr == hdrvr) {
				drivers[i].open = false;
				--driverCount;
				return DRV_OK;
			}
		}
		return DRV_CANCEL;
	case DRV_DISABLE:
		return DRV_OK;
	case DRV_FREE:
		return DRV_OK;
	case DRV_REMOVE:
		return DRV_OK;
	}
	return DRV_OK;
}


HRESULT modGetCaps(PVOID capsPtr, DWORD capsSize) {
	MIDIOUTCAPSA * myCapsA;
	MIDIOUTCAPSW * myCapsW;
	MIDIOUTCAPS2A * myCaps2A;
	MIDIOUTCAPS2W * myCaps2W;

	CHAR synthName[] = "MIDI Splitter\0";
	WCHAR synthNameW[] = L"MIDI Splitter\0";

	switch (capsSize) {
	case (sizeof(MIDIOUTCAPSA)):
		myCapsA = (MIDIOUTCAPSA *)capsPtr;
		myCapsA->wMid = MM_UNMAPPED;
		myCapsA->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCapsA->szPname, synthName, sizeof(synthName));
		myCapsA->wTechnology = MOD_MIDIPORT;
		myCapsA->vDriverVersion = 0x0090;
		myCapsA->wVoices = 0;
		myCapsA->wNotes = 0;
		myCapsA->wChannelMask = 0xffff;
		myCapsA->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPSW)):
		myCapsW = (MIDIOUTCAPSW *)capsPtr;
		myCapsW->wMid = MM_UNMAPPED;
		myCapsW->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCapsW->szPname, synthNameW, sizeof(synthNameW));
		myCapsW->wTechnology = MOD_MIDIPORT;
		myCapsW->vDriverVersion = 0x0090;
		myCapsW->wVoices = 0;
		myCapsW->wNotes = 0;
		myCapsW->wChannelMask = 0xffff;
		myCapsW->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPS2A)):
		myCaps2A = (MIDIOUTCAPS2A *)capsPtr;
		myCaps2A->wMid = MM_UNMAPPED;
		myCaps2A->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCaps2A->szPname, synthName, sizeof(synthName));
		myCaps2A->wTechnology = MOD_MIDIPORT;
		myCaps2A->vDriverVersion = 0x0090;
		myCaps2A->wVoices = 0;
		myCaps2A->wNotes = 0;
		myCaps2A->wChannelMask = 0xffff;
		myCaps2A->dwSupport = 0;
		return MMSYSERR_NOERROR;

	case (sizeof(MIDIOUTCAPS2W)):
		myCaps2W = (MIDIOUTCAPS2W *)capsPtr;
		myCaps2W->wMid = MM_UNMAPPED;
		myCaps2W->wPid = MM_MPU401_MIDIOUT;
		memcpy(myCaps2W->szPname, synthNameW, sizeof(synthNameW));
		myCaps2W->wTechnology = MOD_MIDIPORT;
		myCaps2W->vDriverVersion = 0x0090;
		myCaps2W->wVoices = 0;
		myCaps2W->wNotes = 0;
		myCaps2W->wChannelMask = 0xffff;
		myCaps2W->dwSupport = 0;
		return MMSYSERR_NOERROR;

	default:
		return MMSYSERR_ERROR;
	}
}

void DoCallback(int driverNum, int clientNum, DWORD msg, DWORD param1, DWORD param2) {
	Driver::Client *client = &drivers[driverNum].clients[clientNum];
	DriverCallback(client->callback, client->flags, drivers[driverNum].hdrvr, msg, client->instance, param1, param2);
}

LONG OpenDriver(Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	int clientNum;
	if (driver->clientCount == 0) {
		clientNum = 0;
	} else if (driver->clientCount == MAX_CLIENTS) {
		return MMSYSERR_ALLOCATED;
	} else {
		int i;
		for (i = 0; i < MAX_CLIENTS; i++) {
			if (!driver->clients[i].allocated) {
				break;
			}
		}
		if (i == MAX_CLIENTS) {
			return MMSYSERR_ALLOCATED;
		}
		clientNum = i;
	}
	MIDIOPENDESC *desc = (MIDIOPENDESC *)dwParam1;
	driver->clients[clientNum].allocated = true;
	driver->clients[clientNum].flags = HIWORD(dwParam2);
	driver->clients[clientNum].callback = desc->dwCallback;
	driver->clients[clientNum].instance = desc->dwInstance;
	*(LONG *)dwUser = clientNum;
	driver->clientCount++;
	DoCallback(uDeviceID, clientNum, MOM_OPEN, NULL, NULL);
	return MMSYSERR_NOERROR;
}

LONG CloseDriver(Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	if (!driver->clients[dwUser].allocated) {
		return MMSYSERR_INVALPARAM;
	}
	driver->clients[dwUser].allocated = false;
	driver->clientCount--;
	DoCallback(uDeviceID, dwUser, MOM_CLOSE, NULL, NULL);
	return MMSYSERR_NOERROR;
}

UINT GetMidiId() {
	MIDIOUTCAPSA midiOutCaps;
	UINT numOfDevices = midiOutGetNumDevs();
	UINT midiId = MIDI_MAPPER;

	for (UINT_PTR i = 0; i < numOfDevices; i++) {
		midiOutGetDevCapsA(i, &midiOutCaps, sizeof midiOutCaps);
		if (strcmp(midiPortName, midiOutCaps.szPname) == 0) {
			midiId = i;
			break;
		}
	}
	return midiId;
}

static MMRESULT sendSysex(HMIDIOUT hmo, LPSTR sysex, UINT len) {
	MMRESULT res;
	MIDIHDR mh;
	LARGE_INTEGER time;
	_int64 startTime;

	int sendDelay = 1 + len / 3;
	mh.lpData = sysex;
	mh.dwBufferLength = len;
	mh.dwFlags = 0;
	res = midiOutPrepareHeader(hmo, &mh, sizeof(MIDIHDR));
	if (res != MMSYSERR_NOERROR) return res;
	res = midiOutLongMsg(hmo, &mh, sizeof(MIDIHDR));
	if (res == MMSYSERR_NOERROR) {
		QueryPerformanceCounter(&time);
		startTime = time.QuadPart;
		for(;;) {
			QueryPerformanceCounter(&time);
			int msDelay = int(1000.0 * double(time.QuadPart - startTime) / freq.QuadPart);
			if (sendDelay <= msDelay) break;
			Sleep(1 + sendDelay - msDelay);
		}
	}
	return midiOutUnprepareHeader(hmo, &mh, sizeof(MIDIHDR));
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

static MMRESULT sendLongSysex(HMIDIOUT hmo, LPSTR sysex, UINT len) {
	if (len <= SYSEX_SIZE) {
		return sendSysex(hmo, sysex, len);
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
			MMRESULT res = sendSysex(hmo, sysexPart, partDataLen + SYSEX_SRV_LEN);
			if (res != MMSYSERR_NOERROR) return res;
			sysexAddress += partDataLen;
			sysexData += partDataLen;
			dataLen -= partDataLen;
		}
		return MMSYSERR_NOERROR;
	}
}

STDAPI_(LONG) modMessage(UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	Driver *driver = &drivers[uDeviceID];
	MIDIHDR *pMidiHdr;
	MMRESULT res;
	switch (uMsg) {
	case MODM_OPEN:
		if (driver->clientCount == 0) {
			res = midiOutOpen(&hmo, GetMidiId(), NULL, NULL, CALLBACK_NULL);
			if (res != MMSYSERR_NOERROR) return res;
		}
		return OpenDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MODM_CLOSE:
		if (driver->clients[dwUser].allocated == false) {
			return MMSYSERR_ERROR;
		}
		if (driver->clientCount == 0) return MMSYSERR_INVALPARAM;
		if (driver->clientCount == 1) midiOutClose(hmo);
		return CloseDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MODM_PREPARE:
		if (driver->clients[dwUser].allocated == false) {
			return MMSYSERR_ERROR;
		}
		((MIDIHDR *)dwParam1)->dwFlags |= MHDR_PREPARED;
		return MMSYSERR_NOERROR;

	case MODM_UNPREPARE:
		if (driver->clients[dwUser].allocated == false) {
			return MMSYSERR_ERROR;
		}
		((MIDIHDR *)dwParam1)->dwFlags &= ~MHDR_PREPARED;
		return MMSYSERR_NOERROR;

	case MODM_GETDEVCAPS:
		return modGetCaps((PVOID)dwParam1, dwParam2);

	case MODM_DATA:
		if (driver->clients[dwUser].allocated == false) {
			return MMSYSERR_ERROR;
		}
		return midiOutShortMsg(hmo, dwParam1);

	case MODM_LONGDATA:
		if (driver->clients[dwUser].allocated == false) {
			return MMSYSERR_ERROR;
		}
		pMidiHdr = (MIDIHDR *)dwParam1;
		res = sendLongSysex(hmo, pMidiHdr->lpData, pMidiHdr->dwBufferLength);
		pMidiHdr->dwFlags |= MHDR_DONE;
		pMidiHdr->dwFlags &= ~MHDR_INQUEUE;
		DoCallback(uDeviceID, dwUser, MOM_DONE, dwParam1, dwParam2);
		if (res != MMSYSERR_NOERROR) return res;
		return MMSYSERR_NOERROR;

	case MODM_GETNUMDEVS:
		return 0x1;

	default:
		return MMSYSERR_NOERROR;
		break;
	}
}
