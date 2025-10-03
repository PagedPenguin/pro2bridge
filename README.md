# Nintendo Switch Pro 2 Controller USB Bridge

A USB HID bridge specifically designed for the **Nintendo Switch Pro 2 Controller**, enabling it to work on the **Nintendo Switch 1**. Built on the RP2350 microcontroller with low latency.

## Features

- ✅ **Pro 2 Controller Support**: Support for Nintendo Switch Pro 2 (Report 0x05) **_(Probably)_**
- ✅ **Dual-Core Architecture**: Core0 handles output, Core1 handles input
- ✅ **Auto-Detection**: Also supports original Pro Controller and generic gamepads
- ✅ **Debug Mode**: Optional serial output with button name parsing and LED feedback

## Hardware Requirements

### Components
- **Waveshare RP2350-Zero** (or compatible RP2350 board)
- **2x 22Ω resistors** (for USB D+/D- lines)
- **USB cable** (for native USB connection to PC/Switch)
- **Input controller** (any USB gamepad)

### Wiring

```
Input Controller USB → RP2350 PIO USB (GPIO 12/13)
├─ D+ → GPIO 12 (via 22Ω resistor)
├─ D- → GPIO 13 (via 22Ω resistor)
├─ GND → GND
└─ VBUS → 5V

RP2350 Native USB → PC/Switch
└─ USB-C connector (built-in)

Optional: WS2812B LED → GPIO 16 (debug indicator)
```

## Software Setup

### Prerequisites
- [PlatformIO](https://platformio.org/) (VSCode extension recommended)
- USB drivers for RP2350

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/pro2-2-ns1.git
   cd pro2-2-ns1
   ```

2. **Open in VScode**
   - Open the project folder in VSCode with PlatformIO extension
   - PlatformIO will automatically install dependencies

3. **Build and Upload**
   - In the PlatformIO extention tab click build


## Configuration

### Debug Mode

Edit `src/main.cpp` to enable/disable debug features:

```cpp
// Debug output disabled (production mode - low latency)
#define DEBUG_SERIAL 0  // Set to 1 for debug mode
```

**Debug Mode OFF (0)** - Production:
- ✅ Low latency (~6-12ms)
- ✅ No serial output
- ✅ No LED blink
- ✅ No duplicate filtering

**Debug Mode ON (1)** - Development:
- 🐛 Serial output with button names
- 🐛 LED blink on input
- 🐛 Duplicate report filtering
- ⚠️ Higher latency (~56-66ms due to LED delay)

### Supported Controllers

**Primary Target:**
- **Nintendo Switch Pro 2 Controller** (Report 0x05)

**Also Compatible:**
- Nintendo Switch Pro Controller (Report 0x30)
- Generic USB Gamepads (standard HID format)

## Performance

| Metric | Value |
|--------|-------|
| **Average Latency** | 6-12ms |
| **Best Case** | 2-5ms |
| **Worst Case** | 15-20ms |
| **USB Polling** | 4ms (output) |
| **Processing Overhead** | <1ms |

**Comparison:**
- Wired controller: 1-8ms
- This bridge: 6-12ms ✅
- Bluetooth: 10-30ms
- Wireless 2.4GHz: 5-15ms

## Project Structure

```
pro2-2-ns1/
├── include/
│   ├── pro_controller_output.h    # Output gamepad class & bridge functions
│   ├── hid_report_parser.h        # Input report parsing (debug)
│   └── tusb_config.h               # TinyUSB configuration
├── src/
│   ├── main.cpp                    # Main program & USB callbacks
│   └── pro_controller_output.cpp  # HID bridging implementation
├── platformio.ini                  # PlatformIO configuration
└── README.md
```

## How It Works

1. **Input**: Pro 2 Controller connects to GPIO 12/13 via PIO USB (Core1)
2. **Detection**: Recognizes Pro 2 by Report ID 0x05 and 16+ byte report size
3. **Translation**: Maps Pro 2's 32-bit button layout to standard 16-bit format
4. **Stick Conversion**: Scales 12-bit analog values (0-4095) to 8-bit (0-255)
5. **Output**: Sends as HORIPAD S gamepad via native USB (Core0)

### Architecture

```
┌──────────────────────┐
│  Switch Pro 2        │
│  Controller          │
│  (Report ID 0x05)    │
└──────────┬───────────┘
           │ USB
           │ PIO USB Host
           │ GPIO 12/13
           │
    ┌──────▼──────────────────────┐
    │    RP2350 (Core1)            │
    │  • USB Host Stack            │
    │  • Report ID Detection       │
    │  • Pro 2 Format Parser       │
    └──────┬──────────────────────┘
           │ Translation
           │ • 32-bit → 16-bit buttons
           │ • 12-bit → 8-bit sticks
           │ • D-pad extraction
    ┌──────▼──────────────────────┐
    │    RP2350 (Core0)            │
    │  • USB Device Stack          │
    │  • HID Gamepad (HORIPAD S)   │
    │  • 7-byte report format      │
    └──────┬──────────────────────┘
           │ Native USB
           │ VID:0F0D PID:00C1
           │
┌──────────▼───────────┐
│   PC / Switch 1      │
│   (HORIPAD S)        │
│   All buttons work!  │
└──────────────────────┘
```

<!-- ## Troubleshooting

### Pro 2 Controller not detected
- Verify Pro 2 is sending Report ID 0x05 (enable DEBUG_SERIAL)
- Check wiring (22Ω resistors required on D+/D-)
- Verify GPIO 12/13 connections
- Try a different USB cable

### Buttons not working correctly
- Enable DEBUG_SERIAL to see button mapping
- Check if controller is in correct mode
- Verify all buttons appear in debug output

### No output on PC/Switch
- Device should appear as "HORIPAD S" in device manager
- Check native USB cable connection
- Verify controller is sending reports (debug mode)
- Try unplugging and replugging

### High latency
- Ensure DEBUG_SERIAL is set to 0
- LED blink adds 50ms delay in debug mode
- Check USB cable quality on both sides

### Not recognized as controller
- Unplug and replug native USB cable
- Check Device Manager for "HID-compliant game controller"
- May also show "RP2350 Zero" serial port (normal with CDC enabled)

### GL/GR or special buttons not working
- These are mapped to extended buttons (12-15)
- Some games may not recognize all 16 buttons
- Check game's controller configuration -->

## License

MIT License - See LICENSE file for details

## Credits

- Uses [Pico-PIO-USB](https://github.com/sekigon-gonnoc/Pico-PIO-USB) and [Adafruit TinyUSB Library](https://github.com/adafruit/Adafruit_TinyUSB_Arduino)
- HID report parsing inspired by [touchgadgetdev](https://github.com/touchgadget/switch_tinyusb)
- Switch Pro 2 research from [ndeadly](https://github.com/ndeadly/switch2_controller_research)

## Contributing

Contributions welcome! Please open an issue or pull request.

### TODO
- [ ] Custom button remapping for GL/GR/C/Headset buttons
- [ ] Make HID perfectly mimic Pro (1) controller
- [ ] Support for Gyro/accelerometer
- [ ] Battery level indicator on output device
- [ ] Rumble/haptic feedback support

## Support

For issues, questions, or feature requests, please open an issue on GitHub.
