/* Copyright (C) 2003, 2004, 2005 Dean Beeler, Jerome Fisher
 * Copyright (C) 2011-2017 Dean Beeler, Jerome Fisher, Sergey V. Mikayev
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

/* Exclude rarely-used stuff from Windows headers */
#define WIN32_LEAN_AND_MEAN

/* Windows Header Files: */
#include <windows.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <mmddk.h>

#define MAX_DRIVERS 2

/* Per driver */
#define MAX_CLIENTS 8

/* extensible MID mapping */
#define MM_UNMAPPED 0xffff

/* Private message posted to the synth application to initiate safe MIDI data transfer. */
#define WM_APP_DRV_HAS_DATA WM_APP + 1

/* Private message sent by the synth application to retrieve the MIDI data available. */
#define DRVM_USER_WANTS_DATA DRVM_USER

#define RING_BUFFER_SIZE 128
#define RING_BUFFER_IX_MASK RING_BUFFER_SIZE - 1

/* Defined in WIN32 API only. */
typedef struct tagCOPYDATASTRUCT {
	DWORD dwData;
	DWORD cbData;
	LPVOID lpData;
} COPYDATASTRUCT;

enum MIDIEventDataType {
	MIDI_SHORT_DATA,
	MIDI_LONG_DATA
};

static struct Driver {
	BOOL open;
	WORD clientCount;
	HDRVR hdrvr;
	struct Client {
		BOOL allocated;
		DWORD instance;
		UINT flags;
		DWORD callback;
		DWORD synthInstance;
	} clients[MAX_CLIENTS];
} drivers[MAX_DRIVERS];

static struct MIDIEvent {
	BYTE driverIx;
	BYTE clientIx;
	WORD midiDataType;
	DWORD midiData;
	DWORD timestampLow;
	DWORD timestampHigh;
} ringBuffer[RING_BUFFER_SIZE];

static DWORD tickCounter;
static DWORD tickCounterWraps;

static HWND hwnd = NULL;
static WORD driverCount;

static UINT ringBufferStart = 0;
static UINT ringBufferEnd = 0;

static BOOL checkWindow() {
	if (hwnd == NULL) {
		hwnd = FindWindow("mt32emu_class", NULL);
		ringBufferStart = 0;
		ringBufferEnd = 0;
	}
	return hwnd == NULL;
}

static void updateTickCounter() {
	DWORD currentTime = timeGetTime();
	if (currentTime < tickCounter) tickCounterWraps++;
	tickCounter = currentTime;
}

static DWORD modGetCaps(DWORD capsPtr, DWORD capsSize) {
	static const char synthName[] = "MT-32 Synth Emulator";
	static MIDIOUTCAPS myCaps = { 0 };

	if (!myCaps.wMid) {
		myCaps.wMid = MM_UNMAPPED;
		myCaps.wPid = MM_MPU401_MIDIOUT;
		myCaps.vDriverVersion = 0x0090;
		memcpy(&myCaps.szPname, synthName, sizeof(synthName));
		myCaps.wTechnology = MOD_MIDIPORT;
		myCaps.wVoices = 0;
		myCaps.wNotes = 0;
		myCaps.wChannelMask = 0xffff;
		myCaps.dwSupport = 0;
	}

	memcpy((LPVOID)capsPtr, &myCaps, min(sizeof(myCaps), capsSize));
	return MMSYSERR_NOERROR;
}

static void DoCallback(WORD driverNum, DWORD clientNum, UINT msg, DWORD param1, DWORD param2) {
	struct Client * const client = &drivers[driverNum].clients[clientNum];
	DriverCallback(client->callback, client->flags, drivers[driverNum].hdrvr, msg, client->instance, param1, param2);
}

static DWORD openDriver(struct Driver *driver, UINT uDeviceID, UINT uMsg, LPLONG dwUserPtr, DWORD dwParam1, DWORD dwParam2) {
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

	{
		LPMIDIOPENDESC desc = (LPMIDIOPENDESC)dwParam1;
		driver->clients[clientNum].allocated = TRUE;
		driver->clients[clientNum].flags = HIWORD(dwParam2);
		driver->clients[clientNum].callback = desc->dwCallback;
		driver->clients[clientNum].instance = desc->dwInstance;
	}

	*dwUserPtr = clientNum;
	driver->clientCount++;
	DoCallback(uDeviceID, clientNum, MOM_OPEN, NULL, NULL);
	return MMSYSERR_NOERROR;
}

static DWORD closeDriver(struct Driver *driver, UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	if (!driver->clients[dwUser].allocated) {
		return MMSYSERR_INVALPARAM;
	}
	driver->clients[dwUser].allocated = FALSE;
	driver->clientCount--;
	DoCallback(uDeviceID, dwUser, MOM_CLOSE, NULL, NULL);
	return MMSYSERR_NOERROR;
}

static BOOL pushEvent(WORD driverIx, WORD clientIx, WORD dataType, DWORD data) {
	const UINT newRingBufferEnd = (ringBufferEnd + 1) & RING_BUFFER_IX_MASK;
	BOOL result;
	updateTickCounter();
	if (ringBufferStart != newRingBufferEnd) {
		ringBuffer[ringBufferEnd].driverIx = (BYTE)driverIx;
		ringBuffer[ringBufferEnd].clientIx = (BYTE)clientIx;
		ringBuffer[ringBufferEnd].midiDataType = dataType;
		ringBuffer[ringBufferEnd].midiData = data;
		ringBuffer[ringBufferEnd].timestampLow = tickCounter;
		ringBuffer[ringBufferEnd].timestampHigh = tickCounterWraps;
		ringBufferEnd = newRingBufferEnd;
		result = TRUE;
	} else {
		/* Buffer overflow. */
		result = FALSE;
	}

	if (!PostMessage(hwnd, WM_APP_DRV_HAS_DATA, NULL, NULL)) {
		/* Synth app failed to find our session or was terminated. Better try to reconnect. */
		hwnd = NULL;
	}
	return result;
}

LRESULT FAR PASCAL _loadds DriverProc(DWORD dwDriverID, HDRVR hdrvr, WPARAM wMessage, LPARAM dwParam1, LPARAM dwParam2) {
	switch(wMessage) {
	case DRV_LOAD:
		memset(drivers, 0, sizeof(drivers));
		driverCount = 0;
		return DRV_OK;

	case DRV_ENABLE:
		return DRV_OK;

	case DRV_OPEN: {
		WORD driverNum;
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
		drivers[driverNum].open = TRUE;
		drivers[driverNum].clientCount = 0;
		drivers[driverNum].hdrvr = hdrvr;
		driverCount++;
		return DRV_OK;
	}

	case DRV_INSTALL:
		return DRV_OK;

	case DRV_QUERYCONFIGURE:
		return 0;

	case DRV_CONFIGURE:
		return DRVCNF_OK;

	case DRV_CLOSE: {
		WORD i;
		for (i = 0; i < MAX_DRIVERS; i++) {
			if (drivers[i].open && drivers[i].hdrvr == hdrvr) {
				drivers[i].open = FALSE;
				driverCount--;
				return DRV_OK;
			}
		}
		return DRV_CANCEL;
	}

	case DRV_DISABLE:
		return DRV_OK;

	case DRV_FREE:
		return DRV_OK;

	case DRV_REMOVE:
		return DRV_OK;

	case DRVM_USER_WANTS_DATA:
		/* First, try to restore connection if lost, then ensure this is the right connection. */
		if (checkWindow() || hwnd != (HWND)dwParam1) return DRV_CANCEL;

		while (ringBufferStart != ringBufferEnd) {
			const struct MIDIEvent * const currentEvent = &ringBuffer[ringBufferStart];
			const WORD currentDriverIx = currentEvent->driverIx;
			const DWORD currentClientIx = currentEvent->clientIx;
			const DWORD synthInstance = drivers[currentDriverIx].clients[currentClientIx].synthInstance;

			if (currentEvent->midiDataType == MIDI_SHORT_DATA) {
				const double nanoTime = (currentEvent->timestampHigh * 4294967296.0 + currentEvent->timestampLow) * 1e6;
				/* 0, short MIDI message indicator, timestamp, data */
				DWORD msg[] = { 0, 0, (DWORD)nanoTime, (DWORD)(nanoTime / 4294967296.0), currentEvent->midiData };
				const COPYDATASTRUCT cds = { synthInstance, sizeof(msg), msg };
				const LRESULT res = SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cds);
				if (res != 1 && (checkWindow() || hwnd != (HWND)dwParam1)) return DRV_CANCEL;
			} else {
				const LPMIDIHDR midiHdr = (LPMIDIHDR)currentEvent->midiData;
				const COPYDATASTRUCT cds = { synthInstance, midiHdr->dwBufferLength, midiHdr->lpData };
				const LRESULT res = SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cds);
				if (res != 1 && (checkWindow() || hwnd != (HWND)dwParam1)) return DRV_CANCEL;
				midiHdr->dwFlags |= MHDR_DONE;
				midiHdr->dwFlags &= ~MHDR_INQUEUE;
				DoCallback(currentDriverIx, currentClientIx, MOM_DONE, currentEvent->midiData, NULL);
			}
			ringBufferStart = (ringBufferStart + 1) & RING_BUFFER_IX_MASK;
		}
		return DRV_OK;

	}
	return DefDriverProc(dwDriverID, hdrvr, wMessage, dwParam1, dwParam2);
}

LRESULT FAR PASCAL _loadds modMessage(UINT uDeviceID, UINT uMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2) {
	struct Driver * const driver = &drivers[uDeviceID];
	switch (uMsg) {
	case MODM_OPEN: {
		DWORD result;
		if (checkWindow()) return MMSYSERR_NOTENABLED;
		result = openDriver(driver, uDeviceID, uMsg, (LPLONG)dwUser, dwParam1, dwParam2);
		if (result != MMSYSERR_NOERROR) return result;
		updateTickCounter();

		{
			static const char MIDI_SESSION_NAME_FORMAT[] = "Windows MME Session #%i";

			const double nanoTime = (tickCounterWraps * 4294967296.0 + tickCounter) * 1e6;
			/* 0, handshake indicator, version, timestamp, MIDI session name. */
			DWORD msg[12] = { 0, (DWORD)-1, 1, (DWORD)nanoTime, (DWORD)(nanoTime / 4294967296.0) };
			const COPYDATASTRUCT cds = { 0, sizeof(msg), msg };
			DWORD clientIx = *(LPLONG)dwUser;

			wsprintf((LPSTR)&msg[5], MIDI_SESSION_NAME_FORMAT, clientIx);
			driver->clients[clientIx].synthInstance = (DWORD)SendMessage(hwnd, WM_COPYDATA, NULL, (LPARAM)&cds);
			return result;
		}
	}

	case MODM_CLOSE:
		if (driver->clients[dwUser].allocated == FALSE) return MMSYSERR_NOTENABLED;
		checkWindow();

		{
			/* End of session message */
			const DWORD res = (DWORD)SendMessage(hwnd, WM_APP, (WPARAM)driver->clients[dwUser].synthInstance, NULL);
			/* Synth app failed to clean up properly or was terminated. Better try to reconnect. */
			if (res != 1) hwnd = NULL;
		}

		return closeDriver(driver, uDeviceID, uMsg, dwUser, dwParam1, dwParam2);

	case MODM_RESET:
		return MMSYSERR_NOERROR;

	case MODM_GETNUMDEVS:
		return 1;

	case MODM_GETDEVCAPS:
		return modGetCaps(dwParam1, dwParam2);

	case MODM_DATA: {
		static BOOL reentranceLock = FALSE;
		BOOL result;
		if (driver->clients[dwUser].allocated == FALSE) return MMSYSERR_NOTENABLED;
		if (reentranceLock || hwnd == NULL) return MIDIERR_NOTREADY;
		reentranceLock = TRUE;
		result = pushEvent(uDeviceID, (WORD)dwUser, MIDI_SHORT_DATA, dwParam1);
		reentranceLock = FALSE;
		return result ? MMSYSERR_NOERROR : MIDIERR_NOTREADY;
	}

	case MODM_LONGDATA: {
		static BOOL reentranceLock = FALSE;
		const LPMIDIHDR midiHdr = (LPMIDIHDR)dwParam1;
		if (driver->clients[dwUser].allocated == FALSE) return MMSYSERR_NOTENABLED;
		if ((midiHdr->dwFlags & MHDR_PREPARED) == 0) return MIDIERR_UNPREPARED;
		if (reentranceLock || hwnd == NULL) return MIDIERR_NOTREADY;
		reentranceLock = TRUE;
		if (!pushEvent(uDeviceID, (WORD)dwUser, MIDI_LONG_DATA, dwParam1)) {
			reentranceLock = FALSE;
			return MIDIERR_NOTREADY;
		}
		midiHdr->dwFlags &= ~MHDR_DONE;
		midiHdr->dwFlags |= MHDR_INQUEUE;
		reentranceLock = FALSE;
		return MMSYSERR_NOERROR;
	}

	default:
		return MMSYSERR_NOTSUPPORTED;
	}
}
