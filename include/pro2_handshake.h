/************************************************************************
MIT License

Nintendo Switch Pro 2 Controller Handshake/Initialization
Based on USB HID output commands required to activate Pro 2 controller
Complete 17-command initialization sequence from index.html

*************************************************************************/

#pragma once
#include <Arduino.h>
#include "tusb.h"
#include "pro2_usb_driver.h"

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

// ========== Pro 2 Initialization Commands (17 total) ==========
// Based on working sequence from index.html

// 1. Initialization Command 0x03 - Starts HID output at 4ms intervals
static const uint8_t INIT_CMD_0x03[] = {
  0x03, 0x91, 0x00, 0x0d, 0x00, 0x08,
  0x00, 0x00, 0x01, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Console MAC Address (Little Endian)
};

// 2. Unknown Command 0x07
static const uint8_t INIT_CMD_0x07[] = {
  0x07, 0x91, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00
};

// 3. Unknown Command 0x16
static const uint8_t INIT_CMD_0x16[] = {
  0x16, 0x91, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00
};

// 4. Request Controller MAC Command 0x15 Arg 0x01
static const uint8_t INIT_CMD_0x15_ARG_0x01[] = {
  0x15, 0x91, 0x00, 0x01, 0x00, 0x0e,
  0x00, 0x00, 0x00, 0x02,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // Console MAC Address (Little Endian)
  0xFF,  // Byte 14 with bit 0 masked off
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF  // Remainder of Console MAC Address
};

// 5. LTK Request Command 0x15 Arg 0x02
static const uint8_t INIT_CMD_0x15_ARG_0x02[] = {
  0x15, 0x91, 0x00, 0x02, 0x00, 0x11,
  0x00, 0x00, 0x00,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,  // LTK - 16 byte key
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

// 6. Unknown Command 0x15 Arg 0x03
static const uint8_t INIT_CMD_0x15_ARG_0x03[] = {
  0x15, 0x91, 0x00, 0x03, 0x00, 0x01,
  0x00, 0x00, 0x00
};

// 7. Unknown Command 0x09
static const uint8_t INIT_CMD_0x09[] = {
  0x09, 0x91, 0x00, 0x07, 0x00, 0x08,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 8. IMU Command 0x0C Arg 0x02 - No ACK needed
static const uint8_t INIT_CMD_0x0C_ARG_0x02[] = {
  0x0c, 0x91, 0x00, 0x02, 0x00, 0x04,
  0x00, 0x00, 0x27,
  0x00, 0x00, 0x00
};

// 9. OUT Unknown Command 0x11
static const uint8_t INIT_CMD_0x11[] = {
  0x11, 0x91, 0x00, 0x03,
  0x00, 0x00, 0x00, 0x00
};

// 10. Unknown Command 0x0A
static const uint8_t INIT_CMD_0x0A[] = {
  0x0a, 0x91, 0x00, 0x08, 0x00, 0x14,
  0x00, 0x00, 0x01,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0x35, 0x00, 0x46,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 11. IMU Command 0x0C Arg 0x04
static const uint8_t INIT_CMD_0x0C_ARG_0x04[] = {
  0x0c, 0x91, 0x00, 0x04, 0x00, 0x04,
  0x00, 0x00, 0x27,
  0x00, 0x00, 0x00
};

// 12. Enable Haptics (Probably) 0x03
static const uint8_t INIT_CMD_ENABLE_HAPTICS[] = {
  0x03, 0x91, 0x00, 0x0a, 0x00, 0x04,
  0x00, 0x00, 0x09,
  0x00, 0x00, 0x00
};

// 13. OUT Unknown Command 0x10 - No ACK
static const uint8_t INIT_CMD_0x10[] = {
  0x10, 0x91, 0x00, 0x01,
  0x00, 0x00, 0x00, 0x00
};

// 14. OUT Unknown Command 0x01
static const uint8_t INIT_CMD_0x01[] = {
  0x01, 0x91, 0x00, 0x0c,
  0x00, 0x00, 0x00, 0x00
};

// 15. OUT Unknown Command 0x03 (different from init command)
static const uint8_t INIT_CMD_0x03_ALT[] = {
  0x03, 0x91, 0x00, 0x01,
  0x00, 0x00, 0x00
};

// 16. OUT Unknown Command 0x0A (different from earlier 0x0A)
static const uint8_t INIT_CMD_0x0A_ALT[] = {
  0x0a, 0x91, 0x00, 0x02, 0x00, 0x04,
  0x00, 0x00, 0x03,
  0x00, 0x00
};

// 17. Set Player LED 0x09 - LED value should be set as needed (0x0, 0x1, 0x3, 0x7, etc.)
static const uint8_t INIT_CMD_SET_PLAYER_LED[] = {
  0x09, 0x91, 0x00, 0x07, 0x00, 0x08,
  0x00, 0x00, 
  0x01,  // LED bitfield - replace 0x00 with desired LED pattern
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
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

// Send data via USB bulk endpoint (like WebUSB does)
// Uses custom Pro 2 USB driver for direct endpoint access
inline bool sendBulkCommand(uint8_t dev_addr, const uint8_t* data, uint16_t len) {
  (void)dev_addr;  // We use the driver's stored address
  
  // Send via custom Pro 2 bulk driver
  bool result = pro2_send_bulk(data, len);
  
  if (!result) {
#if DEBUG_SERIAL
    Serial.println("  ✗ Bulk transfer failed!");
#endif
  }
  
  delay(10);  // Small delay between commands
  return result;
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
  
#if DEBUG_SERIAL
  Serial.printf("Sending Pro 2 complete init sequence (17 commands) to addr=%u inst=%u\n", dev_addr, instance);
  Serial.println("NOTE: Using bulk endpoint 0x01 (Interface 1)");
#endif
  
  // Send all 17 initialization commands in sequence via BULK endpoint
  bool success = true;
  
  // 1. Initialization Command 0x03 - Starts HID output at 4ms intervals
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x03, sizeof(INIT_CMD_0x03));
  
  // 2. Unknown Command 0x07
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x07, sizeof(INIT_CMD_0x07));
  
  // 3. Unknown Command 0x16
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x16, sizeof(INIT_CMD_0x16));
  
  // 4. Request Controller MAC Command 0x15 Arg 0x01
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x15_ARG_0x01, sizeof(INIT_CMD_0x15_ARG_0x01));
  
  // 5. LTK Request Command 0x15 Arg 0x02
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x15_ARG_0x02, sizeof(INIT_CMD_0x15_ARG_0x02));
  
  // 6. Unknown Command 0x15 Arg 0x03
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x15_ARG_0x03, sizeof(INIT_CMD_0x15_ARG_0x03));
  
  // 7. Unknown Command 0x09
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x09, sizeof(INIT_CMD_0x09));
  
  // 8. IMU Command 0x0C Arg 0x02 - No ACK needed
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x0C_ARG_0x02, sizeof(INIT_CMD_0x0C_ARG_0x02));
  
  // 9. OUT Unknown Command 0x11
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x11, sizeof(INIT_CMD_0x11));
  
  // 10. Unknown Command 0x0A
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x0A, sizeof(INIT_CMD_0x0A));
  
  // 11. IMU Command 0x0C Arg 0x04
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x0C_ARG_0x04, sizeof(INIT_CMD_0x0C_ARG_0x04));
  
  // 12. Enable Haptics
  success &= sendBulkCommand(dev_addr, INIT_CMD_ENABLE_HAPTICS, sizeof(INIT_CMD_ENABLE_HAPTICS));
  
  // 13. OUT Unknown Command 0x10 - No ACK
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x10, sizeof(INIT_CMD_0x10));
  
  // 14. OUT Unknown Command 0x01
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x01, sizeof(INIT_CMD_0x01));
  
  // 15. OUT Unknown Command 0x03 (alternate)
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x03_ALT, sizeof(INIT_CMD_0x03_ALT));
  
  // 16. OUT Unknown Command 0x0A (alternate)
  success &= sendBulkCommand(dev_addr, INIT_CMD_0x0A_ALT, sizeof(INIT_CMD_0x0A_ALT));
  
  // 17. Set Player LED
  success &= sendBulkCommand(dev_addr, INIT_CMD_SET_PLAYER_LED, sizeof(INIT_CMD_SET_PLAYER_LED));
  
  if (success) {
    state->handshake_sent = true;
    state->handshake_time = millis();
    
#if DEBUG_SERIAL
    Serial.println("Pro 2 complete init sequence (17 commands) sent successfully!");
#endif
  } else {
#if DEBUG_SERIAL
    Serial.println("Pro 2 init sequence FAILED - one or more commands did not send");
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
