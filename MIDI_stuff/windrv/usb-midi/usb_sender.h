#include "stdafx.h"

#include "lusb0_usb.h"

//#define MIDITECH

#ifdef MIDITECH

#define SYNTH_NAME "MIDITECH MIDILINK-mini\0";
#define SYNTH_NAME_W L"MIDITECH MIDILINK-mini\0";

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

#define SYNTH_NAME "USB2.0-MIDI\0";
#define SYNTH_NAME_W L"USB2.0-MIDI\0";

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

static struct usb_device *device = NULL; // the device
static usb_dev_handle *dev = NULL;    // the device handle

static bool sendShortMessage(DWORD message) {
	if ((message & 0x80) == 0) {
		printf("invalid message: %i\n", message);
		return false; // Invalid status byte
	}
	char tmp[4];
	tmp[0] = char((message & 0xF0) >> 4);
	tmp[1] = char(message);
	tmp[2] = char(message >> 8);
	tmp[3] = char(message >> 16);
	int ret = usb_bulk_write(dev, EP_OUT, tmp, sizeof(tmp), 5000);
	if (ret < 0) {
		printf("error writing:\n%s\n", usb_strerror());
		return false;
	}
	return true;
}

static bool sendSysex(char *sysex, UINT len) {
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
			return false;
		}
	}
	return true;
}

static void initUSB() {
	usb_init(); /* initialize the library */
	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */
}

static struct usb_device *findUSB() {
    struct usb_bus *bus;

    for (bus = usb_get_busses(); bus; bus = bus->next) {
        for (device = bus->devices; device; device = device->next) {
            if (device->descriptor.idVendor == MY_VID && device->descriptor.idProduct == MY_PID) {
                return device;
            }
        }
    }
    return NULL;
}

static int openUSB() {
	if ((device = findUSB()) == NULL) {
		printf("USB-MIDI device not found\n");
		return -1;
	}
	if ((dev = usb_open(device)) == NULL) {
		printf("error opening device: \n%s\n", usb_strerror());
		return 1;
	}

	if (usb_set_configuration(dev, MY_CONFIG) < 0) {
		printf("error setting config #%d: %s\n", MY_CONFIG, usb_strerror());
		usb_close(dev);
		return 2;
	}

	if (usb_claim_interface(dev, MY_INTF) < 0) {
		printf("error claiming interface #%d:\n%s\n", MY_INTF, usb_strerror());
		usb_close(dev);
		return 3;
	}
	return 0;
}

static void closeUSB() {
	usb_release_interface(dev, MY_INTF);
	usb_close(dev);
}
