/************************************************************************
Pro Controller Output - Exact Nintendo Switch Pro Controller USB Device
Mimics real Switch Pro Controller USB descriptors exactly
*************************************************************************/

#pragma once
#include <Arduino.h>
#include "Adafruit_TinyUSB.h"

// Switch-compatible Gamepad VID/PID (Hori is officially licensed by Nintendo)
#define GAMEPAD_VID  0x0F0D  // Hori Co., Ltd (Nintendo licensed)
#define GAMEPAD_PID  0x00C1  // HORIPAD for Nintendo Switch

// Simple Generic Gamepad HID Report Descriptor
uint8_t const desc_hid_report_pro_controller[] = {
  0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
  0x09, 0x05,        // Usage (Game Pad)
  0xA1, 0x01,        // Collection (Application)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x35, 0x00,        //   Physical Minimum (0)
  0x45, 0x01,        //   Physical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x10,        //   Report Count (16)
  0x05, 0x09,        //   Usage Page (Button)
  0x19, 0x01,        //   Usage Minimum (0x01)
  0x29, 0x10,        //   Usage Maximum (0x10)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0x05, 0x01,        //   Usage Page (Generic Desktop Ctrls)
  0x25, 0x07,        //   Logical Maximum (7)
  0x46, 0x3B, 0x01,  //   Physical Maximum (315)
  0x75, 0x04,        //   Report Size (4)
  0x95, 0x01,        //   Report Count (1)
  0x65, 0x14,        //   Unit (System: English Rotation, Length: Centimeter)
  0x09, 0x39,        //   Usage (Hat switch)
  0x81, 0x42,        //   Input (Data,Var,Abs,Null State)
  0x65, 0x00,        //   Unit (None)
  0x95, 0x01,        //   Report Count (1)
  0x81, 0x01,        //   Input (Const,Array,Abs)
  0x26, 0xFF, 0x00,  //   Logical Maximum (255)
  0x46, 0xFF, 0x00,  //   Physical Maximum (255)
  0x09, 0x30,        //   Usage (X)
  0x09, 0x31,        //   Usage (Y)
  0x09, 0x32,        //   Usage (Z)
  0x09, 0x35,        //   Usage (Rz)
  0x75, 0x08,        //   Report Size (8)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data,Var,Abs)
  0xC0,              // End Collection
};

// Gamepad Report Structure (7 bytes to match descriptor)
typedef struct __attribute__((packed)) {
  uint16_t buttons;    // 16 buttons (2 bytes)
  uint8_t hat;         // D-pad (hat switch) - 4 bits + 4 bits padding
  uint8_t lx;          // Left stick X (0-255)
  uint8_t ly;          // Left stick Y (0-255)
  uint8_t rx;          // Right stick X (0-255)
  uint8_t ry;          // Right stick Y (0-255)
} ProControllerReport_t;

class ProControllerOutput {
  private:
    Adafruit_USBD_HID usb_hid;
    ProControllerReport_t report;
    
  public:
    ProControllerOutput() : usb_hid() {
      // Initialize report to neutral state
      memset(&report, 0, sizeof(report));
      report.lx = 0x80;  // Center
      report.ly = 0x80;
      report.rx = 0x80;
      report.ry = 0x80;
      report.hat = 0x08; // Centered
    }
    
    void begin() {
      // Set Hori VID/PID (Nintendo Switch compatible)
      USBDevice.setID(GAMEPAD_VID, GAMEPAD_PID);
      USBDevice.setManufacturerDescriptor("HORI CO.,LTD.");
      USBDevice.setProductDescriptor("HORIPAD S");
      
      // Configure HID gamepad
      usb_hid.setPollInterval(4);
      usb_hid.setReportDescriptor(desc_hid_report_pro_controller, sizeof(desc_hid_report_pro_controller));
      usb_hid.begin();
      
      // Wait for USB to be ready
      while (!USBDevice.mounted()) delay(1);
    }
    
    // Set button state (bit mask)
    void setButtons(uint16_t buttons) {
      report.buttons = buttons;
    }
    
    // Set individual button
    void setButton(uint8_t button_num, bool pressed) {
      if (button_num < 16) {
        if (pressed) {
          report.buttons |= (1 << button_num);
        } else {
          report.buttons &= ~(1 << button_num);
        }
      }
    }
    
    // Set D-pad direction (0-7 = directions, 8 = center)
    void setDPad(uint8_t direction) {
      report.hat = direction;
    }
    
    // Set analog sticks (0-255, center = 128)
    void setLeftStick(uint8_t x, uint8_t y) {
      report.lx = x;
      report.ly = y;
    }
    
    void setRightStick(uint8_t x, uint8_t y) {
      report.rx = x;
      report.ry = y;
    }
    
    // Send the current report to the host
    bool sendReport() {
      return usb_hid.sendReport(0, &report, sizeof(report));
    }
    
    // Reset to neutral state
    void reset() {
      report.buttons = 0;
      report.hat = 0x08;
      report.lx = 0x80;
      report.ly = 0x80;
      report.rx = 0x80;
      report.ry = 0x80;
    }
    
    // Get current report (for debugging)
    ProControllerReport_t* getReport() {
      return &report;
    }
};

// HID Bridging Functions
void forwardGenericGamepad(const uint8_t* report, uint16_t len, ProControllerOutput* output);
void forwardSwitchPro(const uint8_t* report, uint16_t len, ProControllerOutput* output);
void forwardSwitchPro2(const uint8_t* report, uint16_t len, ProControllerOutput* output);
void forwardHIDReport(const uint8_t* report, uint16_t len, ProControllerOutput* output);
