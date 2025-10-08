#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// Enable device stack (HID + CDC for compatibility)
#define CFG_TUD_ENABLED     1

// Enable host stack with pio-usb
#define CFG_TUH_ENABLED     1
#define CFG_TUH_RPI_PIO_USB 1

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

// Disable CDC completely
#define CFG_TUD_CDC              0

// Enable HID only
#define CFG_TUD_HID              1

//--------------------------------------------------------------------
// HOST CONFIGURATION (for PIO USB HID)
//--------------------------------------------------------------------

#define CFG_TUH_ENUMERATION_BUFSIZE 512  // Increased for Pro 2 multiple interfaces
#define CFG_TUH_HUB                 1
#define CFG_TUH_DEVICE_MAX          (CFG_TUH_HUB ? 4 : 1)
#define CFG_TUH_HID                 4
#define CFG_TUH_HID_EPIN_BUFSIZE    64
#define CFG_TUH_HID_EPOUT_BUFSIZE   64

// Enable vendor-specific interface for Pro 2 bulk endpoint
#define CFG_TUH_VENDOR              1

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */
