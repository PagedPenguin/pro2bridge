# Switch TinyUSB Integration Notes

## What Changed

### Files Created
1. **`include/switch_controller_output.h`**
   - Wrapper around `switch_tinyusb` library
   - Provides `initSwitchOutput()`, `sendSwitchReport()`, `forwardHIDReport()`
   - Auto-detects and translates Pro 2, Pro 1, and generic gamepad reports

2. **`src/switch_controller_output.cpp`**
   - Defines global instances: `G_usb_hid` and `SwitchGamepad`

### Files Modified
1. **`src/main.cpp`**
   - Changed include from `"pro_controller_output.h"` → `"switch_controller_output.h"`
   - Replaced `ProControllerOutput proController` with library-based approach
   - Updated function calls:
     - `proController.begin()` → `initSwitchOutput()`
     - `proController.sendReport()` → `sendSwitchReport()`
     - `forwardHIDReport(report, len, &proController)` → `forwardHIDReport(report, len)`

2. **`include/hid_report_parser.h`**
   - Removed duplicate definitions of `enum NSButtons`, `NSDirection_t`, `HID_NSGamepadReport_Data_t`
   - Now includes `"switch_tinyusb.h"` to use library definitions
   - Kept debug parsing functions and button name arrays

3. **`platformio.ini`**
   - Added `-I./lib/switch_tinyusb` to build flags

## How It Works

### USB Output (switch_tinyusb library)
- **Library**: `lib/switch_tinyusb/switch_tinyusb.h`
- **Class**: `NSGamepad` - Nintendo Switch compatible gamepad
- **Device ID**: VID:0x0f0d PID:0x00c1 (HORIPAD S)
- **Report Format**: 7-byte HID report (14 buttons, 4 axes, 1 D-pad)

### Report Translation Pipeline
```
Input Controller → PIO-USB (GPIO 12/13) → TinyUSB Host (Core1)
                                              ↓
                                    tuh_hid_report_received_cb()
                                              ↓
                                    forwardHIDReport() [auto-detect]
                                              ↓
                        ┌─────────────────────┼─────────────────────┐
                        ↓                     ↓                     ↓
            forwardSwitchPro2Report   forwardSwitchProReport   forwardGenericGamepadReport
                   (Report 0x05)          (Report 0x30)           (fallback)
                        ↓                     ↓                     ↓
                        └─────────────────────┼─────────────────────┘
                                              ↓
                                      NSGamepad.write()
                                              ↓
                                    TinyUSB Device (Core0)
                                              ↓
                                    Native USB → Switch/PC
```

### Button Mapping (Pro 2 → NSGamepad)
| Pro 2 Button | Byte | Bit | NSGamepad Enum |
|--------------|------|-----|----------------|
| Y            | 4    | 0   | NSButton_Y     |
| X            | 4    | 1   | NSButton_X     |
| B            | 4    | 2   | NSButton_B     |
| A            | 4    | 3   | NSButton_A     |
| R            | 4    | 6   | NSButton_RightTrigger |
| ZR           | 4    | 7   | NSButton_RightThrottle |
| Minus        | 5    | 0   | NSButton_Minus |
| Plus         | 5    | 1   | NSButton_Plus  |
| R-Stick      | 5    | 2   | NSButton_RightStick |
| L-Stick      | 5    | 3   | NSButton_LeftStick |
| Home         | 5    | 4   | NSButton_Home  |
| Capture      | 5    | 5   | NSButton_Capture |
| L            | 6    | 6   | NSButton_LeftTrigger |
| ZL           | 6    | 7   | NSButton_LeftThrottle |
| D-Pad        | 6    | 0-3 | dPad(up,down,left,right) |

### Analog Stick Conversion
- **Input**: 12-bit (0-4095) from Pro 2 controller
- **Output**: 8-bit (0-255) for NSGamepad
- **Method**: Right shift by 4 bits (`value >> 4`)
- **Center**: 0x80 (128)

## Testing

### Build
```bash
pio run
```

### Upload
```bash
pio run --target upload
```

### Debug Mode
Set `DEBUG_SERIAL 1` in `src/main.cpp` to see:
- Button names when pressed
- Analog stick values
- Report type detection
- LED blink on input

## Next Steps

If you need the old `ProControllerOutput` class behavior:
1. The new implementation uses `NSGamepad` from `switch_tinyusb`
2. All translation happens in `switch_controller_output.h`
3. The output format is identical (HORIPAD S compatible)

If you want to customize button mapping:
- Edit `forwardSwitchPro2Report()` in `switch_controller_output.h`
- Change which buttons map to which `NSButton_*` enums
