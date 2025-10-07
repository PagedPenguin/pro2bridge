/************************************************************************
MIT License

Copyright (c) 2021 touchgadgetdev@gmail.com
Modified for HID report parsing

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*************************************************************************/

#pragma once
#include <Arduino.h>
#include "switch_tinyusb.h"  // Use definitions from switch_tinyusb library

// Button names lookup table
static const char* NS_BUTTON_NAMES[] = {
  "Y",
  "B",
  "A",
  "X",
  "L",
  "R",
  "ZL",
  "ZR",
  "Minus",
  "Plus",
  "L-Stick",
  "R-Stick",
  "Home",
  "Capture",
  "Reserved1",
  "Reserved2"
};

// D-Pad direction names
static const char* NS_DPAD_NAMES[] = {
  "Up",
  "Up-Right",
  "Right",
  "Down-Right",
  "Down",
  "Down-Left",
  "Left",
  "Up-Left",
  "Center (8)",
  "Center (9)",
  "Center (10)",
  "Center (11)",
  "Center (12)",
  "Center (13)",
  "Center (14)",
  "Center"
};

// Parse and print Nintendo Switch gamepad report
inline void parseNSGamepadReport(const uint8_t* report, uint16_t len) {
  if (len < sizeof(HID_NSGamepadReport_Data_t)) {
    Serial.println("  Invalid report size");
    return;
  }
  
  HID_NSGamepadReport_Data_t* gamepad = (HID_NSGamepadReport_Data_t*)report;
  
  // Print pressed buttons
  bool any_button = false;
  Serial.print("  Buttons: ");
  for (uint8_t i = 0; i < 16; i++) {
    if (gamepad->buttons & (1 << i)) {
      if (any_button) Serial.print(", ");
      Serial.print(NS_BUTTON_NAMES[i]);
      any_button = true;
    }
  }
  if (!any_button) Serial.print("None");
  Serial.println();
  
  // Print D-Pad
  Serial.print("  D-Pad: ");
  if (gamepad->dPad <= 15) {
    Serial.println(NS_DPAD_NAMES[gamepad->dPad]);
  } else {
    Serial.printf("Unknown (%d)\n", gamepad->dPad);
  }
  
  // Print analog sticks (centered at 0x80 = 128)
  Serial.printf("  Left Stick:  X=%3d Y=%3d\n", gamepad->leftXAxis, gamepad->leftYAxis);
  Serial.printf("  Right Stick: X=%3d Y=%3d\n", gamepad->rightXAxis, gamepad->rightYAxis);
}

// Generic button parser for standard gamepads (first 2 bytes = buttons)
inline void parseGenericGamepadButtons(const uint8_t* report, uint16_t len) {
  if (len < 2) return;
  
  uint16_t buttons = report[0] | (report[1] << 8);
  
  Serial.print("  Buttons: ");
  bool any_button = false;
  for (uint8_t i = 0; i < 16; i++) {
    if (buttons & (1 << i)) {
      if (any_button) Serial.print(", ");
      Serial.printf("Btn%d", i + 1);
      any_button = true;
    }
  }
  if (!any_button) Serial.print("None");
  Serial.println();
}

// Parse Nintendo Switch Pro 2 Controller report (Report ID 0x05)
inline void parseSwitchPro2Report(const uint8_t* report, uint16_t len) {
  if (len < 16) return;
  
  // Switch Pro 2 format (Report 0x05)
  // Offset 0x4: 4 bytes button data
  // Offset 0xA: 3 bytes left stick
  // Offset 0xD: 3 bytes right stick
  
  uint32_t buttons = report[4] | (report[5] << 8) | (report[6] << 16) | (report[7] << 24);
  
  Serial.print("  Buttons: ");
  bool any = false;
  // Byte 0 (report[4])
  if (buttons & 0x00000001) { Serial.print("Y "); any = true; }
  if (buttons & 0x00000002) { Serial.print("X "); any = true; }
  if (buttons & 0x00000004) { Serial.print("B "); any = true; }
  if (buttons & 0x00000008) { Serial.print("A "); any = true; }
  if (buttons & 0x00000010) { Serial.print("SR-Right "); any = true; }
  if (buttons & 0x00000020) { Serial.print("SL-Right "); any = true; }
  if (buttons & 0x00000040) { Serial.print("R "); any = true; }
  if (buttons & 0x00000080) { Serial.print("ZR "); any = true; }
  // Byte 1 (report[5])
  if (buttons & 0x00000100) { Serial.print("Minus "); any = true; }
  if (buttons & 0x00000200) { Serial.print("Plus "); any = true; }
  if (buttons & 0x00000400) { Serial.print("R-Stick "); any = true; }
  if (buttons & 0x00000800) { Serial.print("L-Stick "); any = true; }
  if (buttons & 0x00001000) { Serial.print("Home "); any = true; }
  if (buttons & 0x00002000) { Serial.print("Capture "); any = true; }
  if (buttons & 0x00004000) { Serial.print("C "); any = true; }
  // Byte 2 (report[6])
  if (buttons & 0x00010000) { Serial.print("Down "); any = true; }
  if (buttons & 0x00020000) { Serial.print("Up "); any = true; }
  if (buttons & 0x00040000) { Serial.print("Right "); any = true; }
  if (buttons & 0x00080000) { Serial.print("Left "); any = true; }
  if (buttons & 0x00100000) { Serial.print("SR-Left "); any = true; }
  if (buttons & 0x00200000) { Serial.print("SL-Left "); any = true; }
  if (buttons & 0x00400000) { Serial.print("L "); any = true; }
  if (buttons & 0x00800000) { Serial.print("ZL "); any = true; }
  // Byte 3 (report[7])
  if (buttons & 0x10000000) { Serial.print("Headset "); any = true; }
  if (buttons & 0x02000000) { Serial.print("GL "); any = true; }
  if (buttons & 0x01000000) { Serial.print("GR "); any = true; }
  if (!any) Serial.print("None");
  Serial.println();
  
  // Analog sticks (12-bit values, 3 bytes per stick)
  uint16_t lx = report[10] | ((report[11] & 0x0F) << 8);
  uint16_t ly = (report[11] >> 4) | (report[12] << 4);
  uint16_t rx = report[13] | ((report[14] & 0x0F) << 8);
  uint16_t ry = (report[14] >> 4) | (report[15] << 4);
  Serial.printf("  Left Stick:  X=%4d Y=%4d\n", lx, ly);
  Serial.printf("  Right Stick: X=%4d Y=%4d\n", rx, ry);
  
  // Battery info (if available)
  if (len >= 34) {
    uint16_t battery_mv = report[31] | (report[32] << 8);
    Serial.printf("  Battery: %d mV\n", battery_mv);
  }
}

// Parse real Nintendo Switch Pro Controller report (standard input mode - Report 0x30)
inline void parseRealSwitchProReport(const uint8_t* report, uint16_t len) {
  if (len < 12) return;
  
  // Real Switch Pro Controller format (simplified - standard input report)
  // Byte 0: Report ID (usually 0x30 for standard input)
  // Bytes 1-2: Button data
  // Bytes 3-8: Analog stick data
  
  uint16_t buttons = report[1] | (report[2] << 8);
  
  Serial.print("  Buttons: ");
  bool any = false;
  if (buttons & 0x0001) { Serial.print("Y "); any = true; }
  if (buttons & 0x0002) { Serial.print("X "); any = true; }
  if (buttons & 0x0004) { Serial.print("B "); any = true; }
  if (buttons & 0x0008) { Serial.print("A "); any = true; }
  if (buttons & 0x0040) { Serial.print("R "); any = true; }
  if (buttons & 0x0080) { Serial.print("ZR "); any = true; }
  if (buttons & 0x0100) { Serial.print("Minus "); any = true; }
  if (buttons & 0x0200) { Serial.print("Plus "); any = true; }
  if (buttons & 0x0400) { Serial.print("R-Stick "); any = true; }
  if (buttons & 0x0800) { Serial.print("L-Stick "); any = true; }
  if (buttons & 0x1000) { Serial.print("Home "); any = true; }
  if (buttons & 0x2000) { Serial.print("Capture "); any = true; }
  if (buttons & 0x0010) { Serial.print("L "); any = true; }
  if (buttons & 0x0020) { Serial.print("ZL "); any = true; }
  if (!any) Serial.print("None");
  Serial.println();
  
  // D-Pad (in byte 3, lower nibble)
  uint8_t dpad = report[3] & 0x0F;
  if (dpad <= 7) {
    Serial.printf("  D-Pad: %s\n", NS_DPAD_NAMES[dpad]);
  } else if (dpad == 8) {
    Serial.println("  D-Pad: Center");
  }
  
  // Analog sticks (12-bit values, 3 bytes per stick)
  if (len >= 9) {
    uint16_t lx = report[4] | ((report[5] & 0x0F) << 8);
    uint16_t ly = (report[5] >> 4) | (report[6] << 4);
    uint16_t rx = report[7] | ((report[8] & 0x0F) << 8);
    uint16_t ry = (report[8] >> 4) | (report[9] << 4);
    Serial.printf("  Left Stick:  X=%4d Y=%4d\n", lx, ly);
    Serial.printf("  Right Stick: X=%4d Y=%4d\n", rx, ry);
  }
}

// Auto-detect and parse HID report based on protocol
inline void parseHIDReport(uint8_t protocol, const uint8_t* report, uint16_t len) {
  // Only parse gamepad/controller reports
  // Skip keyboard (protocol 1) and mouse (protocol 2)
  if (protocol == 1 || protocol == 2) {
    return;  // Skip non-gamepad devices
  }
  
  // Detect report type by report ID and size
  if (len >= 16 && report[0] == 0x05) {
    // Nintendo Switch Pro 2 Controller (Report ID 0x05)
    parseSwitchPro2Report(report, len);
  } else if (len >= 12 && report[0] == 0x30) {
    // Nintendo Switch Pro Controller (Report ID 0x30)
    parseRealSwitchProReport(report, len);
  } else if (len == sizeof(HID_NSGamepadReport_Data_t)) {
    // Emulated NS gamepad format
    parseNSGamepadReport(report, len);
  } else {
    // Generic gamepad
    parseGenericGamepadButtons(report, len);
  }
}
