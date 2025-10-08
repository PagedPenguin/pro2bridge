#include <Arduino.h>
#include "tusb.h"
#include <Adafruit_NeoPixel.h>
#include "pio_usb.h"
#include <pico/multicore.h>
#include "hid_report_parser.h"
#include "switch_controller_output.h"
#include "pro2_handshake.h"

// Debug modes
#define DEBUG_SERIAL 1          // Enable serial output with button parsing
#define DEBUG_DISABLE_OUTPUT 1  // Disable Switch output (for testing Pro 2 handshake only)

#define LED_PIN 16
#define NUM_LEDS 1
#define BLINK_COLOR 0x00FF00  // Green
#define BLINK_MS 50

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// Store previous report to filter duplicates (support up to 64 byte reports)
static uint8_t prev_report[4][64] = {0};  // Support up to 4 HID instances
static uint16_t prev_report_len[4] = {0};

// Core1: USB Host task
void core1_main() {
  delay(100);  // Let core0 initialize serial first
  
  // Initialize Pico-PIO-USB for host mode on core1
  pio_usb_configuration_t pio_cfg = PIO_USB_DEFAULT_CONFIG;
  pio_cfg.pin_dp = 12;  // USB D+ pin (D- will be pin_dp + 1 = 13)
  tuh_configure(1, TUH_CFGID_RPI_PIO_USB_CONFIGURATION, &pio_cfg);
  
  // Initialize TinyUSB host stack on core1
  tuh_init(1);
  
  while (true) {
    tuh_task();  // Run USB host task continuously on core1
  }
}

void setup() {
#if !DEBUG_DISABLE_OUTPUT
  // CRITICAL: Initialize USB device FIRST before anything else
  initSwitchOutput();
#endif
  
  // Initialize Pro 2 handshake system
  initPro2Handshake();
  
  // Wait for USB enumeration to complete
  delay(1000);
  
  // Now initialize LED
  strip.begin();
  strip.setPixelColor(0, 0xFF0000);  // Red = starting
  strip.show();
  delay(500);
  strip.setPixelColor(0, 0);
  strip.show();

#if DEBUG_SERIAL
  // Initialize Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== RP2350 USB HID Bridge (Debug Mode) ===");
  Serial.println("Native USB: Emulating USB Gamepad + Serial");
  Serial.println("PIO USB Host: GPIO 12 (D+) / GPIO 13 (D-)");
  Serial.println("Pro 2 Handshake: 17-command sequence enabled");
  Serial.println("Waiting for input controller...\n");
#endif
  
  // Launch USB host task on core1
  multicore_launch_core1(core1_main);
  delay(200);
  
#if DEBUG_SERIAL
  Serial.println("✓ USB Host initialized on Core1 (PIO USB)");
  Serial.println("✓ Ready to detect Pro 2 Controller\n");
#endif

  // Wait for USB host to enumerate devices - poll for up to 5 seconds
#if DEBUG_SERIAL
  Serial.println("Waiting for Pro 2 controller enumeration...");
#endif
  
  bool controller_found = false;
  uint32_t start_time = millis();
  
  // Poll for up to 5 seconds, checking every 100ms
  while (!controller_found && (millis() - start_time) < 5000) {
    // Give USB host task time to process
    delay(100);
    
    // Check if a Pro 2 controller is connected
    // Try device addresses 1-4 (typical range for USB host)
    for (uint8_t addr = 1; addr <= 4; addr++) {
      if (tuh_mounted(addr)) {
        uint16_t vid, pid;
        if (tuh_vid_pid_get(addr, &vid, &pid)) {
#if DEBUG_SERIAL
          Serial.printf("Found device at addr %u: VID:0x%04X PID:0x%04X\n", addr, vid, pid);
#endif
          
          // Check if it's a Pro 2 controller
          if (vid == 0x057E && pid == 0x2069) {
#if DEBUG_SERIAL
            Serial.println("\n>>> PRO 2 CONTROLLER FOUND AT BOOT <<<");
            Serial.println("Initializing Pro 2 USB driver...");
#endif
            
            // Initialize Pro 2 custom USB driver
            pro2_driver_init(addr, USB_INTERFACE_NUMBER);
            
#if DEBUG_SERIAL
            Serial.println("Sending handshake immediately...\n");
#endif
            
            // Get HID instance count for this device
            uint8_t hid_count = tuh_hid_instance_count(addr);
            for (uint8_t inst = 0; inst < hid_count; inst++) {
              // Send handshake to each HID instance
              sendPro2Handshake(addr, inst);
              delay(100);
              
              // Start receiving reports
              tuh_hid_receive_report(addr, inst);
            }
            
#if DEBUG_SERIAL
            Serial.println("✓ Boot handshake complete!\n");
#endif
            controller_found = true;
            break;  // Found and initialized, exit loop
          }
        }
      }
    }
    
#if DEBUG_SERIAL
    if (!controller_found && (millis() - start_time) % 500 == 0) {
      Serial.print(".");  // Progress indicator
    }
#endif
  }
  
#if DEBUG_SERIAL
  if (!controller_found) {
    Serial.println("\nNo Pro 2 controller found via enumeration.");
    Serial.println("Attempting blind handshake to addr=1, inst=0...\n");
#endif
    
    // Try sending handshake blindly to address 1, instance 0
    // This assumes the controller is at the first device address
    delay(500);  // Extra delay for enumeration
    
#if DEBUG_SERIAL
    Serial.println("Sending blind Pro 2 handshake...");
#endif
    
    sendPro2Handshake(1, 0);  // Try address 1, instance 0
    delay(200);
    
    // Try to start receiving reports
    tuh_hid_receive_report(1, 0);
    
#if DEBUG_SERIAL
    Serial.println("✓ Blind handshake sent. Waiting for controller response...\n");
  }
#endif
}

void loop() {
#if !DEBUG_DISABLE_OUTPUT
  // Service USB device stack
  tud_task();
  
  // Send periodic report to keep gamepad active
  static uint32_t last_report = 0;
  if (millis() - last_report >= 100) {  // Every 100ms
    sendSwitchReport();
    last_report = millis();
  }
#endif
  
  delay(1);
}

// ----------------------------------------------------------------------
// TinyUSB Callbacks
// ----------------------------------------------------------------------

// Called when any device is mounted (not just HID)
void tuh_mount_cb(uint8_t dev_addr) {
#if DEBUG_SERIAL
  uint16_t vid, pid;
  tuh_vid_pid_get(dev_addr, &vid, &pid);
  Serial.printf("\n>>> Device Connected [VID:%04X PID:%04X]\n", vid, pid);
#endif
  
  // Check for HID interfaces and start receiving reports
  uint8_t hid_count = tuh_hid_instance_count(dev_addr);
  for (uint8_t idx = 0; idx < hid_count; idx++) {
#if DEBUG_SERIAL
    uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, idx);
    const char* protocol_str[] = { "Generic", "Keyboard", "Mouse" };
    if (itf_protocol <= 2) {
      Serial.printf("    HID %s detected\n", protocol_str[itf_protocol]);
    }
#endif
    tuh_hid_receive_report(dev_addr, idx);
  }
}

// Called when any device is unmounted
void tuh_umount_cb(uint8_t dev_addr) {
#if DEBUG_SERIAL
  Serial.printf("\n<<< Device Disconnected\n");
#endif
  (void)dev_addr;
}

// HID specific mount callback
void tuh_hid_mount_cb(uint8_t dev_addr, uint8_t instance,
                      uint8_t const* desc_report, uint16_t desc_len) {
#if DEBUG_SERIAL
  Serial.printf("\n=== HID Device Mounted ===\n");
  Serial.printf("  Device Address: %u\n", dev_addr);
  Serial.printf("  Instance: %u\n", instance);
  Serial.printf("  Report Descriptor Length: %u bytes\n", desc_len);
  
  // Get device info
  uint16_t vid, pid;
  if (tuh_vid_pid_get(dev_addr, &vid, &pid)) {
    Serial.printf("  VID: 0x%04X\n", vid);
    Serial.printf("  PID: 0x%04X\n", pid);
    
    // Check if it's a Nintendo controller
    if (vid == 0x057E) {
      Serial.println("  ✓ Nintendo Controller Detected!");
      
      // Identify specific controller type
      const char* controller_name = "Unknown";
      bool is_pro2 = false;
      
      switch (pid) {
        case 0x2066:
          controller_name = "Joy-Con (R) 2";
          break;
        case 0x2067:
          controller_name = "Joy-Con (L) 2";
          break;
        case 0x2069:
          controller_name = "Pro Controller 2";
          is_pro2 = true;
          break;
        case 0x2073:
          controller_name = "GameCube Controller (NSO)";
          break;
        default:
          controller_name = "Nintendo Device";
          break;
      }
      
      Serial.printf("  Controller Type: %s\n", controller_name);
      
      if (is_pro2) {
        Serial.println("\n  >>> PRO 2 CONTROLLER CONFIRMED <<<");
        Serial.println("  Initializing Pro 2 USB driver...");
        
        // Initialize Pro 2 custom USB driver for bulk endpoint access
        pro2_driver_init(dev_addr, USB_INTERFACE_NUMBER);
        
        Serial.println("  Initiating 17-command handshake sequence...");
        
        // Send Pro 2 handshake
        bool handshake_result = sendPro2Handshake(dev_addr, instance);
        
        if (handshake_result) {
          Serial.println("  ✓ Handshake sequence initiated successfully");
        } else {
          Serial.println("  ✗ Handshake sequence FAILED");
        }
        
        // Wait longer for Pro 2 to process all commands
        delay(200);
      } else {
        Serial.printf("  Not a Pro 2 controller - skipping handshake\n");
      }
    } else {
      Serial.printf("  Non-Nintendo device (VID: 0x%04X)\n", vid);
    }
  } else {
    Serial.println("  Warning: Could not retrieve VID/PID");
  }
  
  Serial.println("=========================\n");
#endif
  
  (void)desc_report;
  (void)desc_len;
  
  // Request first report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
#if DEBUG_SERIAL
    Serial.println("  ✗ Failed to request initial HID report");
#endif
  } else {
#if DEBUG_SERIAL
    Serial.println("  ✓ Initial HID report requested");
#endif
  }
}

void tuh_hid_umount_cb(uint8_t dev_addr, uint8_t instance) {
#if DEBUG_SERIAL
  Serial.printf("HID device unmounted: addr=%u, inst=%u\n", dev_addr, instance);
#endif
  
  // Reset handshake state for this device
  resetPro2Handshake(instance);
  
  (void)dev_addr;
}

void tuh_hid_report_received_cb(uint8_t dev_addr, uint8_t instance,
                                uint8_t const* report, uint16_t len) {
#if DEBUG_SERIAL
  // Check if this report is identical to the previous one (debug only)
  if (instance < 4) {  // Safety check
    if (len == prev_report_len[instance] && 
        memcmp(report, prev_report[instance], len) == 0) {
      // Duplicate report - skip processing but still request next
      if (!tuh_hid_receive_report(dev_addr, instance)) {
        Serial.println("Failed to request next report");
      }
      return;
    }
    
    // Store this report for next comparison
    prev_report_len[instance] = len;
    if (len <= 64) {
      memcpy(prev_report[instance], report, len);
    }
  }
  
  // Print the report header
  Serial.printf("\n--- Report (addr=%u inst=%u, %u bytes) ---\n", dev_addr, instance, len);
  
  // Get the interface protocol to determine device type
  uint8_t const itf_protocol = tuh_hid_interface_protocol(dev_addr, instance);
  
  // Parse and print human-readable format
  parseHIDReport(itf_protocol, report, len);
  
  // Also print raw hex data
  Serial.print("  Raw: ");
  for (uint16_t i = 0; i < len; i++) {
    if (report[i] < 16) Serial.print("0");
    Serial.print(report[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  
  // Blink WS2812B on HID report received (debug only)
  strip.setPixelColor(0, BLINK_COLOR);  // Turn LED on
  strip.show();
  delay(BLINK_MS);
  strip.setPixelColor(0, 0);           // Turn LED off
  strip.show();
#else
  // Production mode - no debug overhead
  (void)dev_addr;
  (void)instance;
#endif

  // Forward input HID report to output gamepad
#if !DEBUG_DISABLE_OUTPUT
  forwardHIDReport(report, len);
#else
  // Output disabled - just log in debug mode
  (void)report;
  (void)len;
#endif

  // Request next report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
#if DEBUG_SERIAL
    Serial.println("Failed to request next report");
#endif
  }
}