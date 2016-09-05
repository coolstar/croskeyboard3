#if !defined(_CROSKEYBOARD_COMMON_H_)
#define _CROSKEYBOARD_COMMON_H_

//
//These are the device attributes returned by vmulti in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//

#define CROSKEYBOARD_PID              0x0303
#define CROSKEYBOARD_VID              0x18D1
#define CROSKEYBOARD_VERSION          0x0001

//
// These are the report ids
//

#define REPORTID_KEYBOARD       0x07
#define REPORTID_MEDIA          0x08
#define REPORTID_SETTINGS		0x09
#define REPORTID_SPECKEYS		0x0A

//
// Keyboard specific report infomation
//

#define KBD_LCONTROL_BIT     1
#define KBD_LSHIFT_BIT       2
#define KBD_LALT_BIT         4
#define KBD_LGUI_BIT         8
#define KBD_RCONTROL_BIT     16
#define KBD_RSHIFT_BIT       32
#define KBD_RALT_BIT         64
#define KBD_RGUI_BIT         128

#define KBD_KEY_CODES        6

#pragma pack(1)
typedef struct _CROSKEYBOARD_KEYBOARD_REPORT
{

	BYTE      ReportID;

	// Left Control, Left Shift, Left Alt, Left GUI
	// Right Control, Right Shift, Right Alt, Right GUI
	BYTE      ShiftKeyFlags;

	BYTE      Reserved;

	// See http://www.usb.org/developers/devclass_docs/Hut1_11.pdf
	// for a list of key codes
	BYTE      KeyCodes[KBD_KEY_CODES];

} CrosKeyboardKeyboardReport;

#pragma pack()

#pragma pack(1)
typedef struct _CROSKEYBOARD_MEDIA_REPORT
{

	BYTE      ReportID;

	BYTE	  ControlCode;

	BYTE	  Reserved;

} CrosKeyboardMediaReport;

#pragma pack()

#pragma pack(1)
typedef struct _CROSKEYBOARD_SPECKEY_REPORT
{

	BYTE      ReportID;

	BYTE	  ControlCode;

	BYTE	  Reserved;

} CrosKeyboardSpecialKeyReport;

#pragma pack()

//
// Feature report infomation
//

#define DEVICE_MODE_MOUSE        0x00
#define DEVICE_MODE_SINGLE_INPUT 0x01
#define DEVICE_MODE_MULTI_INPUT  0x02

#pragma pack(1)
typedef struct _CROSKEYBOARD_FEATURE_REPORT
{

	BYTE      ReportID;

	BYTE      DeviceMode;

	BYTE      DeviceIdentifier;

} CrosKeyboardFeatureReport;

typedef struct _CROSKEYBOARD_MAXCOUNT_REPORT
{

	BYTE         ReportID;

	BYTE         MaximumCount;

} CrosKeyboardMaxCountReport;
#pragma pack()

#pragma pack(1)
typedef struct _CROSKEYBOARD_SETTINGS_REPORT
{

	BYTE        ReportID;

	BYTE		SettingsRegister;

	BYTE		SettingsValue;

} CrosKeyboardSettingsReport;
#pragma pack()

#endif
#pragma once
