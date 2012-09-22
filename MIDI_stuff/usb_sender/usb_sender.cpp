#include "stdafx.h"

//#define MIDITECH

#ifdef MIDITECH

// Device vendor and product id for MIDITECH MIDILINK-mini
#define MY_VID 0x1ACC
#define MY_PID 0x1A00

// Device configuration and interface id.
#define MY_CONFIG 1
#define MY_INTF 1

// Device endpoint(s)
#define EP_IN  0x81
#define EP_OUT 0x01

#else

// Device vendor and product id for USB2.0-MIDI
#define MY_VID 0x1A86
#define MY_PID 0x752D

// Device configuration and interface id.
#define MY_CONFIG 1
#define MY_INTF 1

// Device endpoint(s)
#define EP_IN  0x82
#define EP_OUT 0x02

#endif

static LARGE_INTEGER freq;

static usb_dev_handle *open_dev(void)
{
    struct usb_bus *bus;
    struct usb_device *dev;

    for (bus = usb_get_busses(); bus; bus = bus->next)
    {
        for (dev = bus->devices; dev; dev = dev->next)
        {
            if (dev->descriptor.idVendor == MY_VID
                    && dev->descriptor.idProduct == MY_PID)
            {
                return usb_open(dev);
            }
        }
    }
    return NULL;
}

static void sendMessage(usb_dev_handle *dev, ULONG message) {
	if ((message & 0x80) == 0) return; // Invalid status byte
	char tmp[4];
	tmp[0] = char(message >> 4);
	tmp[1] = char(message >> 8);
	tmp[2] = char(message >> 16);
	tmp[3] = char(message >> 24);
	int ret = usb_bulk_write(dev, EP_OUT, tmp, sizeof(tmp), 5000);
	if (ret < 0) {
		printf("error writing:\n%s\n", usb_strerror());
	}
}

static void sendSysex(usb_dev_handle *dev, char *sysex, UINT len) {
	char tmp[4];
	while (len > 0) {
		UINT tmpLen = len;
		if (len > 3) {
			tmp[0] = 4;
			tmpLen = 3;
		} else if (len == 3) tmp[0] = 7;
		else if (len == 2) tmp[0] = 6;
		else tmp[0] = 5;
		memcpy(&tmp[1], sysex, tmpLen);
		sysex += tmpLen;
		len -= tmpLen;
		int ret = usb_bulk_write(dev, EP_OUT, tmp, sizeof(tmp), 5000);
		if (ret < 0) {
			printf("error writing:\n%s\n", usb_strerror());
		}
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    usb_dev_handle *dev = NULL; /* the device handle */

	QueryPerformanceFrequency(&freq);

	usb_init(); /* initialize the library */
	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */

	if (!(dev = open_dev())) {
		printf("error opening device: \n%s\n", usb_strerror());
		return 0;
	} else {
		printf("success: device %04X:%04X opened\n", MY_VID, MY_PID);
	}

	if (usb_set_configuration(dev, MY_CONFIG) < 0) {
		printf("error setting config #%d: %s\n", MY_CONFIG, usb_strerror());
		usb_close(dev);
		return 1;
	}

	if (usb_claim_interface(dev, MY_INTF) < 0) {
		printf("error claiming interface #%d:\n%s\n", MY_INTF, usb_strerror());
		usb_close(dev);
		return 2;
	}

	char b[266];
	for (int i = 1; i < 265; i++) b[i] = (char)(i & 0x7F);
	b[0] = '\xF0';
	b[265] = '\xF7';
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	LONGLONG startTime = time.QuadPart;
	for (int i = 0; i < 2; i++) {
		sendSysex(dev, b, sizeof(b));
	}
	QueryPerformanceCounter(&time);
	printf("time elapsed: %f ms\n", 1e3 * double(time.QuadPart - startTime) / freq.QuadPart);

	usb_release_interface(dev, MY_INTF);
	usb_close(dev);
	printf("Done.\n");

	char c;
	fread(&c, 1, 1, stdin);
	ExitProcess(0);
}
