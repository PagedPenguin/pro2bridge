/************************************************************************
MIT License

Nintendo Switch Controller Output using switch_tinyusb library
Bridges HID input reports to Nintendo Switch gamepad format

*************************************************************************/

#pragma once
#include <Arduino.h>
#include "switch_tinyusb.h"

// Global USB HID device and Switch gamepad instances
extern Adafruit_USBD_HID G_usb_hid;
extern NSGamepad SwitchGamepad;

// Initialize the Switch gamepad output
inline void initSwitchOutput() {
  // Start TinyUSB device stack first
  TinyUSBDevice.begin(0);
  
  // Initialize gamepad with proper descriptors
  SwitchGamepad.begin();
  
  // Wait until device mounted (critical for Switch recognition)
  uint32_t start = millis();
  while (!TinyUSBDevice.mounted() && (millis() - start < 5000)) {
    delay(1);
  }
  
  // Give Switch time to enumerate
  delay(100);
  
  // Send initial neutral report (Switch expects this)
  SwitchGamepad.releaseAll();
  SwitchGamepad.leftXAxis(0x80);   // Center
  SwitchGamepad.leftYAxis(0x80);
  SwitchGamepad.rightXAxis(0x80);
  SwitchGamepad.rightYAxis(0x80);
  SwitchGamepad.dPad(NSGAMEPAD_DPAD_CENTERED);
  
  // Wait for ready and send
  for (int i = 0; i < 10 && !SwitchGamepad.ready(); i++) {
    delay(10);
  }
  if (SwitchGamepad.ready()) {
    SwitchGamepad.write();
  }
}

// Send current gamepad state
inline void sendSwitchReport() {
  if (SwitchGamepad.ready()) {
    SwitchGamepad.write();
  }
}

// ======================================================================
// HID Report Translation Functions
// ======================================================================

// Map Nintendo Switch Pro 2 Controller (Report 0x05) to NSGamepad
inline void forwardSwitchPro2Report(const uint8_t* report, uint16_t len) {
  if (len < 16) return;
  
  // Pro 2 format: 4 bytes buttons at offset 4, sticks at offset 10
  uint32_t buttons = report[4] | (report[5] << 8) | (report[6] << 16) | (report[7] << 24);
  
  // Clear all buttons first
  SwitchGamepad.releaseAll();
  
  // Map buttons (Pro 2 → NSGamepad)
  // Byte 0 (report[4])
  if (buttons & 0x00000001) SwitchGamepad.press(NSButton_Y);
  if (buttons & 0x00000002) SwitchGamepad.press(NSButton_X);
  if (buttons & 0x00000004) SwitchGamepad.press(NSButton_B);
  if (buttons & 0x00000008) SwitchGamepad.press(NSButton_A);
  if (buttons & 0x00000040) SwitchGamepad.press(NSButton_RightTrigger);      // R
  if (buttons & 0x00000080) SwitchGamepad.press(NSButton_RightThrottle);     // ZR
  
  // Byte 1 (report[5])
  if (buttons & 0x00000100) SwitchGamepad.press(NSButton_Minus);
  if (buttons & 0x00000200) SwitchGamepad.press(NSButton_Plus);
  if (buttons & 0x00000400) SwitchGamepad.press(NSButton_RightStick);
  if (buttons & 0x00000800) SwitchGamepad.press(NSButton_LeftStick);
  if (buttons & 0x00001000) SwitchGamepad.press(NSButton_Home);
  if (buttons & 0x00002000) SwitchGamepad.press(NSButton_Capture);
  
  // Byte 2 (report[6]) - D-Pad
  bool dpad_down  = (buttons & 0x00010000) != 0;
  bool dpad_up    = (buttons & 0x00020000) != 0;
  bool dpad_right = (buttons & 0x00040000) != 0;
  bool dpad_left  = (buttons & 0x00080000) != 0;
  if (buttons & 0x00400000) SwitchGamepad.press(NSButton_LeftTrigger);       // L
  if (buttons & 0x00800000) SwitchGamepad.press(NSButton_LeftThrottle);      // ZL
  
  // Set D-Pad
  SwitchGamepad.dPad(dpad_up, dpad_down, dpad_left, dpad_right);
  
  // Analog sticks (12-bit → 8-bit conversion)
  uint16_t lx = report[10] | ((report[11] & 0x0F) << 8);  // 0-4095
  uint16_t ly = (report[11] >> 4) | (report[12] << 4);
  uint16_t rx = report[13] | ((report[14] & 0x0F) << 8);
  uint16_t ry = (report[14] >> 4) | (report[15] << 4);
  
  // Scale 12-bit (0-4095) to 8-bit (0-255)
  SwitchGamepad.leftXAxis(lx >> 4);
  SwitchGamepad.leftYAxis(ly >> 4);
  SwitchGamepad.rightXAxis(rx >> 4);
  SwitchGamepad.rightYAxis(ry >> 4);
  
  // Send the report
  sendSwitchReport();
}

// Map standard Nintendo Switch Pro Controller (Report 0x30) to NSGamepad
inline void forwardSwitchProReport(const uint8_t* report, uint16_t len) {
  if (len < 12) return;
  
  uint16_t buttons = report[1] | (report[2] << 8);
  
  // Clear all buttons
  SwitchGamepad.releaseAll();
  
  // Map buttons
  if (buttons & 0x0001) SwitchGamepad.press(NSButton_Y);
  if (buttons & 0x0002) SwitchGamepad.press(NSButton_X);
  if (buttons & 0x0004) SwitchGamepad.press(NSButton_B);
  if (buttons & 0x0008) SwitchGamepad.press(NSButton_A);
  if (buttons & 0x0010) SwitchGamepad.press(NSButton_LeftTrigger);
  if (buttons & 0x0020) SwitchGamepad.press(NSButton_LeftThrottle);
  if (buttons & 0x0040) SwitchGamepad.press(NSButton_RightTrigger);
  if (buttons & 0x0080) SwitchGamepad.press(NSButton_RightThrottle);
  if (buttons & 0x0100) SwitchGamepad.press(NSButton_Minus);
  if (buttons & 0x0200) SwitchGamepad.press(NSButton_Plus);
  if (buttons & 0x0400) SwitchGamepad.press(NSButton_RightStick);
  if (buttons & 0x0800) SwitchGamepad.press(NSButton_LeftStick);
  if (buttons & 0x1000) SwitchGamepad.press(NSButton_Home);
  if (buttons & 0x2000) SwitchGamepad.press(NSButton_Capture);
  
  // D-Pad (in byte 3, lower nibble)
  uint8_t dpad = report[3] & 0x0F;
  if (dpad <= 7) {
    SwitchGamepad.dPad(dpad);
  } else {
    SwitchGamepad.dPad(NSGAMEPAD_DPAD_CENTERED);
  }
  
  // Analog sticks (12-bit → 8-bit)
  if (len >= 9) {
    uint16_t lx = report[4] | ((report[5] & 0x0F) << 8);
    uint16_t ly = (report[5] >> 4) | (report[6] << 4);
    uint16_t rx = report[7] | ((report[8] & 0x0F) << 8);
    uint16_t ry = (report[8] >> 4) | (report[9] << 4);
    
    SwitchGamepad.leftXAxis(lx >> 4);
    SwitchGamepad.leftYAxis(ly >> 4);
    SwitchGamepad.rightXAxis(rx >> 4);
    SwitchGamepad.rightYAxis(ry >> 4);
  }
  
  sendSwitchReport();
}

// Map generic gamepad (simple button layout) to NSGamepad
inline void forwardGenericGamepadReport(const uint8_t* report, uint16_t len) {
  if (len < 2) return;
  
  uint16_t buttons = report[0] | (report[1] << 8);
  
  // Clear all buttons
  SwitchGamepad.releaseAll();
  
  // Simple 1:1 button mapping (first 14 buttons)
  for (uint8_t i = 0; i < 14 && i < 16; i++) {
    if (buttons & (1 << i)) {
      SwitchGamepad.press(i);
    }
  }
  
  // If report has analog data (typical gamepad format)
  if (len >= 6) {
    SwitchGamepad.leftXAxis(report[2]);
    SwitchGamepad.leftYAxis(report[3]);
    SwitchGamepad.rightXAxis(report[4]);
    SwitchGamepad.rightYAxis(report[5]);
  } else {
    // Center sticks if no analog data
    SwitchGamepad.leftXAxis(0x80);
    SwitchGamepad.leftYAxis(0x80);
    SwitchGamepad.rightXAxis(0x80);
    SwitchGamepad.rightYAxis(0x80);
  }
  
  // D-Pad centered by default
  SwitchGamepad.dPad(NSGAMEPAD_DPAD_CENTERED);
  
  sendSwitchReport();
}

// Auto-detect report type and forward appropriately
inline void forwardHIDReport(const uint8_t* report, uint16_t len) {
  if (len == 0 || report == nullptr) return;
  
  // Detect report type by report ID and size
  if (len >= 16 && report[0] == 0x05) {
    // Nintendo Switch Pro 2 Controller (Report ID 0x05)
    forwardSwitchPro2Report(report, len);
  } else if (len >= 12 && report[0] == 0x30) {
    // Nintendo Switch Pro Controller (Report ID 0x30)
    forwardSwitchProReport(report, len);
  } else {
    // Generic gamepad - try to parse as standard HID
    forwardGenericGamepadReport(report, len);
  }
}
