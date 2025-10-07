/************************************************************************
MIT License

Nintendo Switch Pro 2 Controller Handshake/Initialization
Based on USB HID output commands required to activate Pro 2 controller

*************************************************************************/

#pragma once
#include <Arduino.h>
#include "tusb.h"

// Nintendo Switch Controller USB IDs
#define VENDOR_ID_NINTENDO      0x057E
#define PRODUCT_ID_JOYCON2_R    0x2066
#define PRODUCT_ID_JOYCON2_L    0x2067
#define PRODUCT_ID_PROCON2      0x2069
#define PRODUCT_ID_GCNSO        0x2073

// USB Interface number for Nintendo controllers
#define USB_INTERFACE_NUMBER    1

// Track handshake state per device
struct Pro2HandshakeState {
  bool is_pro2_controller;
  bool handshake_sent;
  bool handshake_complete;
  uint32_t handshake_time;
};

// Global handshake state (support up to 4 devices)
static Pro2HandshakeState handshake_state[4] = {0};

// Initialization Command 0x03 - Starts HID output at 4ms intervals
// Note: MAC address bytes (0xFF) should be replaced with actual console MAC address
static const uint8_t INIT_COMMAND_0x03[] = {
  0x03,                           // Command ID
  0x91, 0x00, 0x0d, 0x00,        // Unknown parameters
  0x08, 0x00, 0x00, 0x01, 0x00,  // Unknown parameters
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Console MAC Address (Little Endian)
};

// Check if device is a Pro 2 controller
inline bool isPro2Controller(uint8_t dev_addr) {
  uint16_t vid, pid;
  if (tuh_vid_pid_get(dev_addr, &vid, &pid)) {
    return (vid == VENDOR_ID_NINTENDO && pid == PRODUCT_ID_PROCON2);
  }
  return false;
}

// Check if device is any Nintendo Switch controller that needs handshake
inline bool isNintendoController(uint8_t dev_addr) {
  uint16_t vid, pid;
  if (tuh_vid_pid_get(dev_addr, &vid, &pid)) {
    if (vid == VENDOR_ID_NINTENDO) {
      return (pid == PRODUCT_ID_PROCON2 || 
              pid == PRODUCT_ID_JOYCON2_R || 
              pid == PRODUCT_ID_JOYCON2_L ||
              pid == PRODUCT_ID_GCNSO);
    }
  }
  return false;
}

// Send handshake command to Pro 2 controller
inline bool sendPro2Handshake(uint8_t dev_addr, uint8_t instance) {
  if (instance >= 4) return false;
  
  Pro2HandshakeState* state = &handshake_state[instance];
  
  // Check if this is a Pro 2 controller
  if (!state->is_pro2_controller) {
    state->is_pro2_controller = isPro2Controller(dev_addr);
    if (!state->is_pro2_controller) {
      // Not a Pro 2, mark as complete so we don't check again
      state->handshake_complete = true;
      return true;
    }
  }
  
  // Already sent handshake
  if (state->handshake_sent) {
    // Check if enough time has passed (wait 500ms for controller to respond)
    if (!state->handshake_complete && 
        (millis() - state->handshake_time) > 500) {
      state->handshake_complete = true;
    }
    return state->handshake_complete;
  }
  
  // Send initialization command via HID Set_Report
  // This uses the control endpoint to send the command
  bool success = tuh_hid_set_report(
    dev_addr, 
    instance,
    0,  // Report ID (0 for no ID)
    HID_REPORT_TYPE_OUTPUT,
    (void*)INIT_COMMAND_0x03,
    sizeof(INIT_COMMAND_0x03)
  );
  
  if (success) {
    state->handshake_sent = true;
    state->handshake_time = millis();
    
#if DEBUG_SERIAL
    Serial.printf("Pro 2 handshake sent to addr=%u inst=%u\n", dev_addr, instance);
#endif
  }
  
  return success;
}

// Check if handshake is complete for this device
inline bool isPro2HandshakeComplete(uint8_t instance) {
  if (instance >= 4) return true;
  return handshake_state[instance].handshake_complete;
}

// Reset handshake state when device disconnects
inline void resetPro2Handshake(uint8_t instance) {
  if (instance < 4) {
    memset(&handshake_state[instance], 0, sizeof(Pro2HandshakeState));
  }
}

// Initialize handshake system
inline void initPro2Handshake() {
  memset(handshake_state, 0, sizeof(handshake_state));
}
