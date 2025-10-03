/************************************************************************
Pro Controller Output Implementation
Handles HID bridging from input controllers to output gamepad
*************************************************************************/

#include "pro_controller_output.h"

// Forward generic gamepad report (7+ bytes) to output
void forwardGenericGamepad(const uint8_t* report, uint16_t len, ProControllerOutput* output) {
  if (len >= 7 && output) {
    // Standard gamepad format: 2 bytes buttons, 1 byte hat, 4 bytes axes
    uint16_t buttons = report[0] | (report[1] << 8);
    uint8_t hat = report[2] & 0x0F;
    uint8_t lx = report[3];
    uint8_t ly = report[4];
    uint8_t rx = report[5];
    uint8_t ry = report[6];
    
    output->setButtons(buttons);
    output->setDPad(hat);
    output->setLeftStick(lx, ly);
    output->setRightStick(rx, ry);
    output->sendReport();
  }
}

// Forward Switch Pro Controller (Report 0x30) to output
void forwardSwitchPro(const uint8_t* report, uint16_t len, ProControllerOutput* output) {
  if (len >= 12 && report[0] == 0x30 && output) {
    // Switch Pro Controller standard input report
    uint16_t buttons = report[1] | (report[2] << 8);
    uint8_t dpad = report[3] & 0x0F;
    
    // Extract 12-bit stick values and convert to 8-bit (0-4095 -> 0-255)
    uint16_t lx = report[4] | ((report[5] & 0x0F) << 8);
    uint16_t ly = (report[5] >> 4) | (report[6] << 4);
    uint16_t rx = report[7] | ((report[8] & 0x0F) << 8);
    uint16_t ry = (report[8] >> 4) | (report[9] << 4);
    
    output->setButtons(buttons);
    output->setDPad(dpad);
    output->setLeftStick(lx >> 4, ly >> 4);   // Scale 12-bit to 8-bit
    output->setRightStick(rx >> 4, ry >> 4);
    output->sendReport();
  }
}

// Forward Switch Pro 2 Controller (Report 0x05) to output
void forwardSwitchPro2(const uint8_t* report, uint16_t len, ProControllerOutput* output) {
  if (len >= 16 && report[0] == 0x05 && output) {
    // Switch Pro 2 format - buttons at offset 4
    uint32_t buttons32 = report[4] | (report[5] << 8) | (report[6] << 16) | (report[7] << 24);
    
    // Map Pro 2 buttons to standard 16-bit button layout
    uint16_t buttons = 0;
    if (buttons32 & 0x00000001) buttons |= (1 << 0);  // Y
    if (buttons32 & 0x00000002) buttons |= (1 << 1);  // X
    if (buttons32 & 0x00000004) buttons |= (1 << 2);  // B
    if (buttons32 & 0x00000008) buttons |= (1 << 3);  // A
    if (buttons32 & 0x00000040) buttons |= (1 << 4);  // R
    if (buttons32 & 0x00000080) buttons |= (1 << 5);  // ZR
    if (buttons32 & 0x00000100) buttons |= (1 << 6);  // Minus
    if (buttons32 & 0x00000200) buttons |= (1 << 7);  // Plus
    if (buttons32 & 0x00000400) buttons |= (1 << 8);  // R-Stick
    if (buttons32 & 0x00000800) buttons |= (1 << 9);  // L-Stick
    if (buttons32 & 0x00001000) buttons |= (1 << 10); // Home
    if (buttons32 & 0x00002000) buttons |= (1 << 11); // Capture
    if (buttons32 & 0x00400000) buttons |= (1 << 12); // L
    if (buttons32 & 0x00800000) buttons |= (1 << 13); // ZL
    
    // D-pad from byte 2
    uint8_t dpad = 0x08; // Centered
    if (buttons32 & 0x00020000) dpad = 0; // Up
    if (buttons32 & 0x00010000) dpad = 4; // Down
    if (buttons32 & 0x00080000) dpad = 6; // Left
    if (buttons32 & 0x00040000) dpad = 2; // Right
    
    // Extract 12-bit stick values and convert to 8-bit
    uint16_t lx = report[10] | ((report[11] & 0x0F) << 8);
    uint16_t ly = (report[11] >> 4) | (report[12] << 4);
    uint16_t rx = report[13] | ((report[14] & 0x0F) << 8);
    uint16_t ry = (report[14] >> 4) | (report[15] << 4);
    
    output->setButtons(buttons);
    output->setDPad(dpad);
    output->setLeftStick(lx >> 4, ly >> 4);
    output->setRightStick(rx >> 4, ry >> 4);
    output->sendReport();
  }
}

// Auto-detect and forward any HID report
void forwardHIDReport(const uint8_t* report, uint16_t len, ProControllerOutput* output) {
  if (!report || !output || len == 0) return;
  
  // Detect report type by size and report ID
  if (len >= 16 && report[0] == 0x05) {
    // Switch Pro 2 Controller
    forwardSwitchPro2(report, len, output);
  } else if (len >= 12 && report[0] == 0x30) {
    // Switch Pro Controller
    forwardSwitchPro(report, len, output);
  } else if (len >= 7) {
    // Generic gamepad
    forwardGenericGamepad(report, len, output);
  }
}
