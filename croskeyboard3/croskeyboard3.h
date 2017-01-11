#if !defined(_CROSKEYBOARD_H_)
#define _CROSKEYBOARD_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <wdm.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <hidport.h>

#include "hidcommon.h"
#include "spb.h"

extern "C"

NTSTATUS
DriverEntry(
	_In_  PDRIVER_OBJECT   pDriverObject,
	_In_  PUNICODE_STRING  pRegistryPath
	);

EVT_WDF_DRIVER_DEVICE_ADD       OnDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP  OnDriverCleanup;

//
// String definitions
//

#define DRIVERNAME                 "croskeyboard3.sys: "

#define CROSKEYBOARD_POOL_TAG            (ULONG) 'lmtA'
#define CROSKEYBOARD_HARDWARE_IDS        L"CoolStar\\CrosKeyboard\0\0"
#define CROSKEYBOARD_HARDWARE_IDS_LENGTH sizeof(CROSKEYBOARD_HARDWARE_IDS)

#define NTDEVICE_NAME_STRING       L"\\Device\\CrosKeyboard"
#define SYMBOLIC_NAME_STRING       L"\\DosDevices\\CrosKeyboard"
//
// This is the default report descriptor for the Hid device provided
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR.
// 

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

#ifdef DESCRIPTOR_DEF
HID_REPORT_DESCRIPTOR DefaultReportDescriptor[] = {
	//
	// Keyboard report starts here
	//    
	0x05, 0x01,                         // USAGE_PAGE (Generic Desktop)
	0x09, 0x06,                         // USAGE (Keyboard)
	0xa1, 0x01,                         // COLLECTION (Application)
	0x85, REPORTID_KEYBOARD,            //   REPORT_ID (Keyboard)    
	0x05, 0x07,                         //   USAGE_PAGE (Keyboard)
	0x19, 0xe0,                         //   USAGE_MINIMUM (Keyboard LeftControl)
	0x29, 0xe7,                         //   USAGE_MAXIMUM (Keyboard Right GUI)
	0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
	0x25, 0x01,                         //   LOGICAL_MAXIMUM (1)
	0x75, 0x01,                         //   REPORT_SIZE (1)
	0x95, 0x08,                         //   REPORT_COUNT (8)
	0x81, 0x02,                         //   INPUT (Data,Var,Abs)
	0x95, 0x01,                         //   REPORT_COUNT (1)
	0x75, 0x08,                         //   REPORT_SIZE (8)
	0x81, 0x03,                         //   INPUT (Cnst,Var,Abs)
	0x95, 0x05,                         //   REPORT_COUNT (5)
	0x75, 0x01,                         //   REPORT_SIZE (1)
	0x05, 0x08,                         //   USAGE_PAGE (LEDs)
	0x19, 0x01,                         //   USAGE_MINIMUM (Num Lock)
	0x29, 0x05,                         //   USAGE_MAXIMUM (Kana)
	0x91, 0x02,                         //   OUTPUT (Data,Var,Abs)
	0x95, 0x01,                         //   REPORT_COUNT (1)
	0x75, 0x03,                         //   REPORT_SIZE (3)
	0x91, 0x03,                         //   OUTPUT (Cnst,Var,Abs)
	0x95, 0x06,                         //   REPORT_COUNT (6)
	0x75, 0x08,                         //   REPORT_SIZE (8)
	0x15, 0x00,                         //   LOGICAL_MINIMUM (0)
	0x25, 0x65,                         //   LOGICAL_MAXIMUM (101)
	0x05, 0x07,                         //   USAGE_PAGE (Keyboard)
	0x19, 0x00,                         //   USAGE_MINIMUM (Reserved (no event indicated))
	0x29, 0x65,                         //   USAGE_MAXIMUM (Keyboard Application)
	0x81, 0x00,                         //   INPUT (Data,Ary,Abs)
	0xc0,                               // END_COLLECTION

	0x05, 0x0C, /*		Usage Page (Consumer Devices)		*/
	0x09, 0x01, /*		Usage (Consumer Control)			*/
	0xA1, 0x01, /*		Collection (Application)			*/
	0x85, REPORTID_MEDIA,	/*		Report ID=2							*/
	0x05, 0x0C, /*		Usage Page (Consumer Devices)		*/
	0x15, 0x00, /*		Logical Minimum (0)					*/
	0x25, 0x01, /*		Logical Maximum (1)					*/
	0x75, 0x01, /*		Report Size (1)						*/
	0x95, 0x07, /*		Report Count (7)					*/
	0x09, 0x6F, /*		Usage (Brightess Up)				*/
	0x09, 0x70, /*		Usage (Brightness Down)				*/
	0x09, 0xB7, /*		Usage (Stop)						*/
	0x09, 0xCD, /*		Usage (Play / Pause)				*/
	0x09, 0xE2, /*		Usage (Mute)						*/
	0x09, 0xE9, /*		Usage (Volume Up)					*/
	0x09, 0xEA, /*		Usage (Volume Down)					*/
	0x81, 0x02, /*		Input (Data, Variable, Absolute)	*/
	0x95, 0x01, /*		Report Count (1)					*/
	0x81, 0x01, /*		Input (Constant)					*/
	0xC0,        /*        End Collection                        */

	0x06, 0x00, 0xff,                    // USAGE_PAGE (Vendor Defined Page 1)
	0x09, 0x03,                          // USAGE (Vendor Usage 3)
	0xa1, 0x01,                          // COLLECTION (Application)
	0x85, REPORTID_SETTINGS,              //   REPORT_ID (Settings)
	0x15, 0x00,                          //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,                    //   LOGICAL_MAXIMUM (256)
	0x75, 0x08,                          //   REPORT_SIZE  (8)   - bits
	0x95, 0x01,                          //   REPORT_COUNT (1)  - Bytes
	0x09, 0x02,                          //   USAGE (Vendor Usage 1)
	0x91, 0x02,                          //   OUTPUT (Data,Var,Abs)
	0x09, 0x03,                          //   USAGE (Vendor Usage 2)
	0x91, 0x02,                          //   OUTPUT (Data,Var,Abs)
	0xc0,                                // END_COLLECTION

	0x06, 0x00, 0xff,                    // USAGE_PAGE (Vendor Defined Page 1)
	0x09, 0x04,                          // USAGE (Vendor Usage 4)
	0xa1, 0x01,                          // COLLECTION (Application)
	0x85, REPORTID_SPECKEYS,             //   REPORT_ID (Special Keys)
	0x15, 0x00,                          //   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,                    //   LOGICAL_MAXIMUM (256)
	0x75, 0x08,                          //   REPORT_SIZE  (8)   - bits
	0x95, 0x01,                          //   REPORT_COUNT (1)  - Bytes
	0x09, 0x02,                          //   USAGE (Vendor Usage 1)
	0x81, 0x02,                          //   INPUT (Data,Var,Abs)
	0x95, 0x01,                          //   REPORT_COUNT (1)  - Bytes
	0x81, 0x01,                          //   INPUT (Constant)
	0xc0,                                // END_COLLECTION
};


//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of DefaultReportDescriptor.
//

CONST HID_DESCRIPTOR DefaultHidDescriptor = {
	0x09,   // length of HID descriptor
	0x21,   // descriptor type == HID  0x21
	0x0100, // hid spec release
	0x00,   // country code == Not Specified
	0x01,   // number of HID class descriptors
	{ 0x22,   // descriptor type 
	sizeof(DefaultReportDescriptor) }  // total length of report descriptor
};
#endif

#define true 1
#define false 0

struct croskeyboardsettings {
	int keyboardMapping = 1;
	int mapSearchToCapsLock = 0;
	int powerKeyAsDelete = 0;
};

typedef struct _CROSKEYBOARD_CONTEXT
{

	WDFQUEUE ReportQueue;

	BYTE DeviceMode;

	WDFINTERRUPT Interrupt;

	BOOLEAN ConnectInterrupt;

	WDFTIMER Timer;

	bool PrepareForRight;

	bool LeftCtrl;

	bool RightCtrl;

	bool LeftAlt;

	bool RightAlt;

	bool LeftShift;

	bool RightShift;

	bool LeftWin;

	bool RightWin;

	unsigned char lastps2code = 0x00;

	int lastps2codeint = 0x00;

	LONGLONG lastRead;

	BYTE keyCodes[KBD_KEY_CODES];

	struct croskeyboardsettings settings;

} CROSKEYBOARD_CONTEXT, *PCROSKEYBOARD_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CROSKEYBOARD_CONTEXT, GetDeviceContext)

//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD CrosKeyboardDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD CrosKeyboardEvtDeviceAdd;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS CrosKeyboardEvtWdmPreprocessMnQueryId;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL CrosKeyboardEvtInternalDeviceControl;

NTSTATUS
CrosKeyboardGetHidDescriptor(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request
	);

NTSTATUS
CrosKeyboardGetReportDescriptor(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request
	);

NTSTATUS
CrosKeyboardGetDeviceAttributes(
	IN WDFREQUEST Request
	);

NTSTATUS
CrosKeyboardGetString(
	IN WDFREQUEST Request
	);

NTSTATUS
CrosKeyboardWriteReport(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request
	);

NTSTATUS
CrosKeyboardProcessVendorReport(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN PVOID ReportBuffer,
	IN ULONG ReportBufferLen,
	OUT size_t* BytesWritten
	);

NTSTATUS
CrosKeyboardReadReport(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
	);

NTSTATUS
CrosKeyboardSetFeature(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
	);

NTSTATUS
CrosKeyboardGetFeature(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
	);

PCHAR
DbgHidInternalIoctlString(
	IN ULONG        IoControlCode
	);

//
// Helper macros
//

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_INFO    2
#define DEBUG_LEVEL_VERBOSE 3

#define DBG_INIT  1
#define DBG_PNP   2
#define DBG_IOCTL 4

#if DBG
#define CrosKeyboardPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (CrosKeyboardDebugLevel >= dbglevel &&                         \
        (CrosKeyboardDebugCatagories && dbgcatagory))                 \
	    {                                                           \
        DbgPrint(DRIVERNAME);                                   \
        DbgPrint(fmt, __VA_ARGS__);                             \
	    }                                                           \
}
#else
#define CrosKeyboardPrint(dbglevel, fmt, ...) {                       \
}
#endif

#endif
#pragma once
