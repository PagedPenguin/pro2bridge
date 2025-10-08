/************************************************************************
MIT License

Pro 2 Controller Custom USB Driver
Direct bulk endpoint access for initialization commands

This bypasses the HID class and talks directly to the bulk endpoints
like the WebUSB implementation does.

*************************************************************************/

#pragma once
#include <Arduino.h>
#include "tusb.h"

// Pro 2 USB endpoint addresses
#define PRO2_EP_OUT  0x01  // Bulk OUT endpoint
#define PRO2_EP_IN   0x81  // Bulk IN endpoint (0x01 | 0x80)

// Track if we have an open Pro 2 device
static struct {
  uint8_t dev_addr;
  uint8_t itf_num;
  bool is_mounted;
  uint8_t ep_out;
  uint8_t ep_in;
} pro2_device = {0};

// Called when a Pro 2 controller is detected and configured
// This is called from the main code after detecting VID/PID
inline void pro2_driver_init(uint8_t dev_addr, uint8_t itf_num) {
  pro2_device.dev_addr = dev_addr;
  pro2_device.itf_num = itf_num;
  pro2_device.is_mounted = true;
  pro2_device.ep_out = PRO2_EP_OUT;
  pro2_device.ep_in = PRO2_EP_IN;
  
#if DEBUG_SERIAL
  Serial.printf("Pro 2 driver initialized: addr=%u, itf=%u\n", dev_addr, itf_num);
  Serial.printf("  EP OUT: 0x%02X\n", pro2_device.ep_out);
  Serial.printf("  EP IN: 0x%02X\n", pro2_device.ep_in);
#endif
}

// Send raw data to Pro 2 bulk OUT endpoint
inline bool pro2_send_bulk(const uint8_t* data, uint16_t len) {
  if (!pro2_device.is_mounted) {
#if DEBUG_SERIAL
    Serial.println("Pro 2 device not mounted!");
#endif
    return false;
  }
  
#if DEBUG_SERIAL
  Serial.printf("Pro 2 bulk OUT: %u bytes to EP 0x%02X\n", len, pro2_device.ep_out);
  Serial.print("  Data: ");
  for (uint16_t i = 0; i < min(len, (uint16_t)16); i++) {
    Serial.printf("%02X ", data[i]);
  }
  if (len > 16) Serial.print("...");
  Serial.println();
#endif
  
  // TinyUSB host with PIO USB only supports HID class properly
  // We have to use HID set_report instead of raw bulk transfers
  // Send as HID Output Report (Report ID 0)
  return tuh_hid_set_report(pro2_device.dev_addr, 0, 0, HID_REPORT_TYPE_OUTPUT, (void*)data, len);
}

// Read response from Pro 2 bulk IN endpoint
inline bool pro2_read_bulk(uint8_t* buffer, uint16_t len) {
  if (!pro2_device.is_mounted) {
    return false;
  }
  
  // Use HID get_report instead
  return tuh_hid_get_report(pro2_device.dev_addr, 0, 0, HID_REPORT_TYPE_INPUT, buffer, len);
}

// Reset driver state when device disconnects
inline void pro2_driver_reset() {
  pro2_device.is_mounted = false;
  pro2_device.dev_addr = 0;
  pro2_device.itf_num = 0;
  
#if DEBUG_SERIAL
  Serial.println("Pro 2 driver reset");
#endif
}

// Check if Pro 2 is mounted
inline bool pro2_is_mounted() {
  return pro2_device.is_mounted;
}
