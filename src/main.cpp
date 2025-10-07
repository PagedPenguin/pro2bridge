#include <Arduino.h>
#include "tusb.h"
#include <Adafruit_NeoPixel.h>
#include "pio_usb.h"
#include <pico/multicore.h>
#include "hid_report_parser.h"
#include "switch_controller_output.h"
#include "pro2_handshake.h"

// Debug output disabled (production mode - low latency)
#define DEBUG_SERIAL 0

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
  // CRITICAL: Initialize USB device FIRST before anything else
  initSwitchOutput();
  
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
  Serial.println("GPIO 12/13: Waiting for input controller...\n");
  Serial.println("Gamepad initialized. Waiting for input...");
#endif
  
  // Launch USB host task on core1
  multicore_launch_core1(core1_main);
  delay(200);
}

void loop() {
  // Service USB device stack
  tud_task();
  
  // Send periodic report to keep gamepad active
  static uint32_t last_report = 0;
  if (millis() - last_report >= 100) {  // Every 100ms
    sendSwitchReport();
    last_report = millis();
  }
  
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
  Serial.printf("HID device mounted: addr=%u, inst=%u, report_len=%u\n",
                dev_addr, instance, desc_len);
  
  // Check if it's a Nintendo controller
  uint16_t vid, pid;
  if (tuh_vid_pid_get(dev_addr, &vid, &pid)) {
    Serial.printf("  VID:0x%04X PID:0x%04X\n", vid, pid);
    if (vid == 0x057E) {
      Serial.println("  Nintendo controller detected!");
      if (pid == 0x2069) {
        Serial.println("  -> Pro 2 Controller - sending handshake...");
      }
    }
  }
#endif
  (void)desc_report;
  (void)desc_len;
  
  // Send Pro 2 handshake if needed
  sendPro2Handshake(dev_addr, instance);
  
  // Small delay to let handshake process
  delay(100);

  if (!tuh_hid_receive_report(dev_addr, instance)) {
#if DEBUG_SERIAL
    Serial.println("Failed to request initial report");
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

  // Forward input HID report to output gamepad (ALWAYS)
  forwardHIDReport(report, len);

  // Request next report
  if (!tuh_hid_receive_report(dev_addr, instance)) {
#if DEBUG_SERIAL
    Serial.println("Failed to request next report");
#endif
  }
}