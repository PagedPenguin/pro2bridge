#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 16
#define NUM_LEDS 1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  // Test basic functionality first
  strip.begin();
  
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n\n=== BASIC TEST - RP2350 ===");
  Serial.println("If you see this, Serial works!");
  Serial.println("Watch for LED blinks...\n");
}

void loop() {
  static int count = 0;
  
  // Cycle through colors
  if (count % 3 == 0) {
    strip.setPixelColor(0, 0xFF0000);  // Red
    Serial.println("RED");
  } else if (count % 3 == 1) {
    strip.setPixelColor(0, 0x00FF00);  // Green
    Serial.println("GREEN");
  } else {
    strip.setPixelColor(0, 0x0000FF);  // Blue
    Serial.println("BLUE");
  }
  strip.show();
  
  count++;
  delay(1000);
}
