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

// Pro 2 initialization commands (sent via USB bulk endpoint)
// Command 0x03 - Starts HID output at 4ms intervals
static const uint8_t INIT_CMD_0x03[] = {
  0x03,                           // Command ID
  0x91, 0x00, 0x0D, 0x00,        // Parameters
  0x08, 0x00, 0x00, 0x01, 0x00,  // Parameters
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // Console MAC Address (placeholder)
};

// Command 0x07 - Unknown but required
static const uint8_t INIT_CMD_0x07[] = {
  0x07,                           // Command ID
  0x91, 0x00, 0x01, 0x00,        // Parameters
  0x00, 0x00, 0x00               // Padding
};

// Command 0x15 Arg 0x01 - Request Controller MAC
static const uint8_t INIT_CMD_0x15_01[] = {
  0x15,                           // Command ID
  0x91, 0x00, 0x01, 0x00,        // Parameters (Arg 0x01)
  0x0E, 0x00, 0x00, 0x00, 0x02,  // Parameters
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // Console MAC
  0x00,                           // MAC byte with bit masked
  0x00, 0x00, 0x00, 0x00, 0x00   // Remainder of MAC
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

// Send handshake commands to Pro 2 controller via bulk endpoint
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
  
  // Pro 2 uses USB bulk OUT endpoint (EP 0x01) for commands
  // Send initialization sequence: 0x03, 0x07, 0x15
  
#if DEBUG_SERIAL
  Serial.printf("Sending Pro 2 init sequence to addr=%u inst=%u\n", dev_addr, instance);
#endif
  
  // Command 0x03 - Start HID output at 4ms intervals
  bool success = tuh_hid_send_report(dev_addr, instance, 0, 
                                      (void*)INIT_CMD_0x03, sizeof(INIT_CMD_0x03));
  
  if (success) {
    delay(10);  // Small delay between commands
    
    // Command 0x07 - Unknown but required
    success = tuh_hid_send_report(dev_addr, instance, 0,
                                   (void*)INIT_CMD_0x07, sizeof(INIT_CMD_0x07));
    delay(10);
    
    // Command 0x15 - Request controller MAC
    success = tuh_hid_send_report(dev_addr, instance, 0,
                                   (void*)INIT_CMD_0x15_01, sizeof(INIT_CMD_0x15_01));
  }
  
  if (success) {
    state->handshake_sent = true;
    state->handshake_time = millis();
    
#if DEBUG_SERIAL
    Serial.println("Pro 2 init sequence sent successfully");
#endif
  } else {
#if DEBUG_SERIAL
    Serial.println("Pro 2 init sequence FAILED");
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
