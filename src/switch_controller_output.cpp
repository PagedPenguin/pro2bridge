/************************************************************************
MIT License

Nintendo Switch Controller Output Implementation
Global instances for switch_tinyusb library

*************************************************************************/

#include "switch_controller_output.h"

// Global USB HID device and Switch gamepad instances
Adafruit_USBD_HID G_usb_hid;
NSGamepad SwitchGamepad(&G_usb_hid);
