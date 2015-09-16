#define DESCRIPTOR_DEF
#include "croskeyboard3.h"

static ULONG CrosKeyboardDebugLevel = 100;
static ULONG CrosKeyboardDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

NTSTATUS
DriverEntry(
	__in PDRIVER_OBJECT  DriverObject,
	__in PUNICODE_STRING RegistryPath
	)
{
	NTSTATUS               status = STATUS_SUCCESS;
	WDF_DRIVER_CONFIG      config;
	WDF_OBJECT_ATTRIBUTES  attributes;

	CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_INIT,
		"Driver Entry: Built %s %s\n", __DATE__, __TIME__);

	WDF_DRIVER_CONFIG_INIT(&config, CrosKeyboardEvtDeviceAdd);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

	//
	// Create a framework driver object to represent our driver.
	//

	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		&attributes,
		&config,
		WDF_NO_HANDLE
		);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
			"WdfDriverCreate failed with status 0x%x\n", status);
	}

	return status;
}

NTSTATUS
OnPrepareHardware(
	_In_  WDFDEVICE     FxDevice,
	_In_  WDFCMRESLIST  FxResourcesRaw,
	_In_  WDFCMRESLIST  FxResourcesTranslated
	)
	/*++

	Routine Description:

	This routine caches the SPB resource connection ID.

	Arguments:

	FxDevice - a handle to the framework device object
	FxResourcesRaw - list of translated hardware resources that
	the PnP manager has assigned to the device
	FxResourcesTranslated - list of raw hardware resources that
	the PnP manager has assigned to the device

	Return Value:

	Status

	--*/
{
	PCROSKEYBOARD_CONTEXT pDevice = GetDeviceContext(FxDevice);
	BOOLEAN fSpbResourceFound = FALSE;
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(FxResourcesRaw);

	return status;
}

NTSTATUS
OnReleaseHardware(
	_In_  WDFDEVICE     FxDevice,
	_In_  WDFCMRESLIST  FxResourcesTranslated
	)
	/*++

	Routine Description:

	Arguments:

	FxDevice - a handle to the framework device object
	FxResourcesTranslated - list of raw hardware resources that
	the PnP manager has assigned to the device

	Return Value:

	Status

	--*/
{
	PCROSKEYBOARD_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	UNREFERENCED_PARAMETER(FxResourcesTranslated);

	return status;
}

NTSTATUS
OnD0Entry(
	_In_  WDFDEVICE               FxDevice,
	_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
	)
	/*++

	Routine Description:

	This routine allocates objects needed by the driver.

	Arguments:

	FxDevice - a handle to the framework device object
	FxPreviousState - previous power state

	Return Value:

	Status

	--*/
{
	UNREFERENCED_PARAMETER(FxPreviousState);

	PCROSKEYBOARD_CONTEXT pDevice = GetDeviceContext(FxDevice);
	NTSTATUS status = STATUS_SUCCESS;

	char junk = __inbyte(0x60);

	WdfTimerStart(pDevice->Timer, WDF_REL_TIMEOUT_IN_MS(10));

	pDevice->ConnectInterrupt = true;

	return status;
}

NTSTATUS
OnD0Exit(
	_In_  WDFDEVICE               FxDevice,
	_In_  WDF_POWER_DEVICE_STATE  FxPreviousState
	)
	/*++

	Routine Description:

	This routine destroys objects needed by the driver.

	Arguments:

	FxDevice - a handle to the framework device object
	FxPreviousState - previous power state

	Return Value:

	Status

	--*/
{
	UNREFERENCED_PARAMETER(FxPreviousState);

	PCROSKEYBOARD_CONTEXT pDevice = GetDeviceContext(FxDevice);

	WdfTimerStop(pDevice->Timer, TRUE);

	pDevice->ConnectInterrupt = false;

	return STATUS_SUCCESS;
}

static void update_keyboard(PCROSKEYBOARD_CONTEXT pDevice, BYTE shiftKeys, BYTE keyCodes[KBD_KEY_CODES]) {
	_CROSKEYBOARD_KEYBOARD_REPORT report;
	report.ReportID = REPORTID_KEYBOARD;
	report.ShiftKeyFlags = shiftKeys;
	for (int i = 0; i < KBD_KEY_CODES; i++) {
		report.KeyCodes[i] = keyCodes[i];
	}

	size_t bytesWritten;
	CrosKeyboardProcessVendorReport(pDevice, &report, sizeof(report), &bytesWritten);
}

void updateSpecialKeys(PCROSKEYBOARD_CONTEXT pDevice, int ps2code) {
	bool PrepareForRight = pDevice->PrepareForRight;
	pDevice->PrepareForRight = false;
	switch (ps2code) {
		case 29:
			if (PrepareForRight)
				pDevice->RightCtrl = true;
			else
				pDevice->LeftCtrl = true;
			return; //ctrl
		case 157:
			if (PrepareForRight)
				pDevice->RightCtrl = false;
			else
				pDevice->LeftCtrl = false;
			return; //ctrl

		case 56:
			if (PrepareForRight)
				pDevice->RightAlt = true;
			else
				pDevice->LeftAlt = true;
			return; //alt
		case 184:
			if (PrepareForRight)
				pDevice->RightAlt = false;
			else
				pDevice->LeftAlt = false;
			return; //alt

		case 42:
			pDevice->LeftShift = true;
			return; //left shift
		case 170:
			pDevice->LeftShift = false;
			return; //left shift

		case 54:
			pDevice->RightShift = true;
			return; //right shift
		case 182:
			pDevice->RightShift = false;
			return; //right shift

		case 91:
			pDevice->LeftWin = true;
			return; //left win
		case 219:
			pDevice->LeftWin = false;
			return; //left win
	}
	pDevice->PrepareForRight = false;
	if (ps2code == 224)
		pDevice->PrepareForRight = true;
}

BYTE HIDCodeFromPS2Code(int ps2code, bool *remove) {
	*remove = false;
	switch (ps2code) {
		case 0x1e:
			return 0x04; //a
		case 158:
			*remove = true;
			return 0x04; //a

		case 48:
			return 0x05; //b
		case 176:
			*remove = true;
			return 0x05; //b

		case 46:
			return 0x06; //c
		case 174:
			*remove = true;
			return 0x06; //c

		case 32:
			return 0x07; //d
		case 160:
			*remove = true;
			return 0x07; //d

		case 18:
			return 0x08; //e
		case 146:
			*remove = true;
			return 0x08; //e

		case 33:
			return 0x09; //f
		case 161:
			*remove = true;
			return 0x09; //f

		case 34:
			return 0x0a; //g
		case 162:
			*remove = true;
			return 0x0a; //g

		case 35:
			return 0x0b; //h
		case 163:
			*remove = true;
			return 0x0b; //h

		case 23:
			return 0x0c; //i
		case 151:
			*remove = true;
			return 0x0c; //i

		case 36:
			return 0x0d; //j
		case 164:
			*remove = true;
			return 0x0d; //j

		case 37:
			return 0x0e; //k
		case 165:
			*remove = true;
			return 0x0e; //k

		case 38:
			return 0x0f; //l
		case 166:
			*remove = true;
			return 0x0f; //l

		case 50:
			return 0x10; //m
		case 178:
			*remove = true;
			return 0x10; //m

		case 49:
			return 0x11; //n
		case 177:
			*remove = true;
			return 0x11; //n

		case 24:
			return 0x12; //o
		case 152:
			*remove = true;
			return 0x12; //o

		case 25:
			return 0x13; //p
		case 153:
			*remove = true;
			return 0x13; //p

		case 16:
			return 0x14; //q
		case 144:
			*remove = true;
			return 0x14; //q

		case 19:
			return 0x15; //r
		case 147:
			*remove = true;
			return 0x15; //r

		case 31:
			return 0x16; //s
		case 159:
			*remove = true;
			return 0x16; //s

		case 20:
			return 0x17; //t
		case 148:
			*remove = true;
			return 0x17; //t

		case 22:
			return 0x18; //u
		case 150:
			*remove = true;
			return 0x18; //u

		case 47:
			return 0x19; //v
		case 175:
			*remove = true;
			return 0x19; //v

		case 17:
			return 0x1a; //w
		case 145:
			*remove = true;
			return 0x1a; //w

		case 45:
			return 0x1b; //x
		case 173:
			*remove = true;
			return 0x1b; //x

		case 21:
			return 0x1c; //y
		case 149:
			*remove = true;
			return 0x1c; //y

		case 44:
			return 0x1d; //z
		case 172:
			*remove = true;
			return 0x1d; //z

		case 2:
			return 0x1e; //1
		case 130:
			*remove = true;
			return 0x1e; //1

		case 3:
			return 0x1f; //2
		case 131:
			*remove = true;
			return 0x1f; //2

		case 4:
			return 0x20; //3
		case 132:
			*remove = true;
			return 0x20; //3

		case 5:
			return 0x21; //4
		case 133:
			*remove = true;
			return 0x21; //4

		case 6:
			return 0x22; //5
		case 134:
			*remove = true;
			return 0x22; //5

		case 7:
			return 0x23; //6
		case 135:
			*remove = true;
			return 0x23; //6

		case 8:
			return 0x24; //7
		case 136:
			*remove = true;
			return 0x24; //7

		case 9:
			return 0x25; //8
		case 137:
			*remove = true;
			return 0x25; //8

		case 10:
			return 0x26; //9
		case 138:
			*remove = true;
			return 0x26; //9

		case 11:
			return 0x27; //0
		case 139:
			*remove = true;
			return 0x27; //0

		case 28:
			return 0x28; //Enter
		case 156:
			*remove = true;
			return 0x28; //Enter

		case 1:
			return 0x29; //Escape
		case 129:
			*remove = true;
			return 0x29; //Escape

		case 14:
			return 0x2a; //Backspace
		case 142:
			*remove = true;
			return 0x2a; //Backspace

		case 15:
			return 0x2b; //Tab
		case 143:
			*remove = true;
			return 0x2b; //Tab

		case 57:
			return 0x2c; //Space
		case 185:
			*remove = true;
			return 0x2c; //Space

		case 12:
			return 0x2d; //-
		case 140:
			*remove = true;
			return 0x2d; //-

		case 13:
			return 0x2e; //=
		case 141:
			*remove = true;
			return 0x2e; //=

		case 26:
			return 0x2f; //[
		case 154:
			*remove = true;
			return 0x2f; //[

		case 27:
			return 0x30; //]
		case 155:
			*remove = true;
			return 0x30; //]

		case 43:
			return 0x31; //|
		case 171:
			*remove = true;
			return 0x31; //|

		case 39:
			return 0x33; //;
		case 167:
			*remove = true;
			return 0x33; //;

		case 40:
			return 0x34; //'
		case 168:
			*remove = true;
			return 0x34; //'

		case 41:
			return 0x35; //`
		case 169:
			*remove = true;
			return 0x35; //`

		case 51:
			return 0x36; //,
		case 179:
			*remove = true;
			return 0x36; //,

		case 52:
			return 0x37; //.
		case 180:
			*remove = true;
			return 0x37; //.

		case 53:
			return 0x38; // /
		case 181:
			*remove = true;
			return 0x38; // /

		case 59:
			return 0x3a; // F1
		case 187:
			*remove = true;
			return 0x3a; // F1

		case 60:
			return 0x3b; // F2
		case 188:
			*remove = true;
			return 0x3b; // F2

		case 61:
			return 0x3c; // F3
		case 189:
			*remove = true;
			return 0x3c; // F3

		case 62:
			return 0x3d; // F4
		case 190:
			*remove = true;
			return 0x3d; // F4

		case 63:
			return 0x3e; // F5
		case 191:
			*remove = true;
			return 0x3e; // F5

		case 64:
			return 0x3f; // F6
		case 192:
			*remove = true;
			return 0x3f; // F6

		case 65:
			return 0x40; // F7
		case 193:
			*remove = true;
			return 0x40; // F7

		case 66:
			return 0x41; // F8
		case 194:
			*remove = true;
			return 0x41; // F8

		case 67:
			return 0x42; // F9
		case 195:
			*remove = true;
			return 0x42; // F9

		case 68:
			return 0x43; // F10
		case 196:
			*remove = true;
			return 0x43; // F10

		case 77:
			return 0x4f; // right arrow
		case 205:
			*remove = true;
			return 0x4f; // right arrow

		case 75:
			return 0x50; // left arrow
		case 203:
			*remove = true;
			return 0x50; // left arrow

		case 80:
			return 0x51; // down arrow
		case 208:
			*remove = true;
			return 0x51; // down arrow

		case 72:
			return 0x52; // up arrow
		case 200:
			*remove = true;
			return 0x52; // up arrow
	}
	return 0x00;
}

void removeCode(PCROSKEYBOARD_CONTEXT pDevice, BYTE code) {
	for (int i = 0; i < KBD_KEY_CODES; i++) {
		if (pDevice->keyCodes[i] == code) {
			pDevice->keyCodes[i] = 0x00;
		}
	}
	BYTE keyCodes[KBD_KEY_CODES] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int j = 0;
	for (int i = 0; i < KBD_KEY_CODES; i++) {
		if (pDevice->keyCodes[i] != 0x00) {
			keyCodes[j] = pDevice->keyCodes[i];
			j++;
		}
	}
	for (int i = 0; i < KBD_KEY_CODES; i++) {
		pDevice->keyCodes[i] = keyCodes[i];
	}
}

void addCode(PCROSKEYBOARD_CONTEXT pDevice,BYTE code) {
	for (int i = 0; i < KBD_KEY_CODES; i++) {
		if (pDevice->keyCodes[i] == 0x00) {
			pDevice->keyCodes[i] = code;
			break;
		}
		else if (pDevice->keyCodes[i] == code) {
			return;
		}
	}
}

BOOLEAN OnInterruptIsr(
	WDFINTERRUPT Interrupt,
	ULONG MessageID) {
	UNREFERENCED_PARAMETER(MessageID);

	WDFDEVICE Device = WdfInterruptGetDevice(Interrupt);
	PCROSKEYBOARD_CONTEXT pDevice = GetDeviceContext(Device);

	if (!pDevice->ConnectInterrupt)
		return true;

	int ps2code = __inbyte(0x60);
	if (ps2code == pDevice->lastps2code)
		return true;
	pDevice->lastps2code = ps2code;
	bool remove = false;
	BYTE hidcode = HIDCodeFromPS2Code(ps2code, &remove);
	if (remove) {
		removeCode(pDevice, hidcode);
	}
	else {
		addCode(pDevice, hidcode);
	}

	updateSpecialKeys(pDevice, ps2code);

	bool overrideCtrl = false;
	bool overrideAlt = false;
	bool overrideAltGr = false;
	bool overrideWin = false;
	bool mediaKey = false;

	BYTE consumerKey = 0x00;

	BYTE keyCodes[KBD_KEY_CODES] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	for (int i = 0; i < KBD_KEY_CODES; i++) {
		keyCodes[i] = pDevice->keyCodes[i];
		BYTE keyCode = keyCodes[i];
		if (pDevice->LeftCtrl) {
			if (keyCode == 0x3a) {
				overrideCtrl = true;
				overrideAlt = true;
				keyCodes[i] = 0x50;
				//Alt+Back Arrow (F1)
			}
			else if (keyCode == 0x3b) {
				overrideCtrl = true;
				overrideAlt = true;
				keyCodes[i] = 0x4f;
				//Alt+Forward Arrow (F2)
			}
			else if (keyCode == 0x3c) {
				overrideCtrl = true;
				keyCodes[i] = 0x3e;
				//F5 (F3)
			}
			else if (keyCode == 0x3d) {
				overrideCtrl = true;
				keyCodes[i] = 0x44;
				//F11 (F4)
			}
			else if (keyCode == 0x3e) {
				overrideCtrl = true;
				overrideWin = true;
				keyCodes[i] = 0x2b;
				//win+tab (F5)
			}
			else if (keyCode == 0x3f) {
				mediaKey = true;
				consumerKey = 0x02;
				//brightness down (F6)
			}
			else if (keyCode == 0x40) {
				mediaKey = true;
				consumerKey = 0x01;
				//brightness up (F7)
			}
			else if (keyCode == 0x41) {
				mediaKey = true;
				consumerKey = 0x10; //mute (F8)
			}
			else if (keyCode == 0x42) {
				mediaKey = true;
				consumerKey = 0x40; //volume down (F9)
			}
			else if (keyCode == 0x43) {
				mediaKey = true;
				consumerKey = 0x20; //volume up (F10)
			}
			else if (keyCode == 0x2a) {
				if (!pDevice->LeftAlt)
					overrideCtrl = true;
				keyCodes[i] = 0x4c; //delete (backspace)
			}
			else if (keyCode == 0x52) {
				overrideCtrl = true;
				keyCodes[i] = 0x4b; //page up (up arrow)
			}
			else if (keyCode == 0x51) {
				overrideCtrl = true;
				keyCodes[i] = 0x4e; //page down (down arrow)
			}
		}
	}

	BYTE ShiftKeys = 0;
	if (pDevice->LeftCtrl != overrideCtrl)
		ShiftKeys |= KBD_LCONTROL_BIT;
	if (pDevice->LeftAlt != overrideAlt)
		ShiftKeys |= KBD_LALT_BIT;
	if (pDevice->LeftShift)
		ShiftKeys |= KBD_LSHIFT_BIT;
	if (pDevice->LeftWin != overrideWin)
		ShiftKeys |= KBD_RGUI_BIT;

	if (pDevice->RightCtrl)
		ShiftKeys |= KBD_RCONTROL_BIT;
	if (pDevice->RightAlt != overrideAltGr)
		ShiftKeys |= KBD_RALT_BIT;
	if (pDevice->RightShift)
		ShiftKeys |= KBD_RSHIFT_BIT;

	if (mediaKey) {
		_CROSKEYBOARD_MEDIA_REPORT report;
		report.ReportID = REPORTID_MEDIA;
		report.ControlCode = consumerKey;
		size_t bytesWritten;
		CrosKeyboardProcessVendorReport(pDevice, &report, sizeof(report), &bytesWritten);
	}
	else {
		_CROSKEYBOARD_MEDIA_REPORT report;
		report.ReportID = REPORTID_MEDIA;
		report.ControlCode = 0x00;
		size_t bytesWritten;
		CrosKeyboardProcessVendorReport(pDevice, &report, sizeof(report), &bytesWritten);
		update_keyboard(pDevice, ShiftKeys, keyCodes);
	}

	return true;
}


void CrosKeyboardTimerFunc(_In_ WDFTIMER hTimer) {
	WDFDEVICE Device = (WDFDEVICE)WdfTimerGetParentObject(hTimer);
	PCROSKEYBOARD_CONTEXT pDevice = GetDeviceContext(Device);

	if (!pDevice->ConnectInterrupt)
		return;

	char fixkeys = __inbyte(0x60);

	return;
}

NTSTATUS
CrosKeyboardEvtDeviceAdd(
	IN WDFDRIVER       Driver,
	IN PWDFDEVICE_INIT DeviceInit
	)
{
	NTSTATUS                      status = STATUS_SUCCESS;
	WDF_IO_QUEUE_CONFIG           queueConfig;
	WDF_OBJECT_ATTRIBUTES         attributes;
	WDFDEVICE                     device;
	WDF_INTERRUPT_CONFIG interruptConfig;
	WDFQUEUE                      queue;
	UCHAR                         minorFunction;
	PCROSKEYBOARD_CONTEXT               devContext;

	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_PNP,
		"CrosKeyboardEvtDeviceAdd called\n");

	//
	// Tell framework this is a filter driver. Filter drivers by default are  
	// not power policy owners. This works well for this driver because
	// HIDclass driver is the power policy owner for HID minidrivers.
	//

	WdfFdoInitSetFilter(DeviceInit);

	{
		WDF_PNPPOWER_EVENT_CALLBACKS pnpCallbacks;
		WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpCallbacks);

		pnpCallbacks.EvtDevicePrepareHardware = OnPrepareHardware;
		pnpCallbacks.EvtDeviceReleaseHardware = OnReleaseHardware;
		pnpCallbacks.EvtDeviceD0Entry = OnD0Entry;
		pnpCallbacks.EvtDeviceD0Exit = OnD0Exit;

		WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpCallbacks);
	}

	//
	// Because we are a virtual device the root enumerator would just put null values 
	// in response to IRP_MN_QUERY_ID. Lets override that.
	//

	minorFunction = IRP_MN_QUERY_ID;

	status = WdfDeviceInitAssignWdmIrpPreprocessCallback(
		DeviceInit,
		CrosKeyboardEvtWdmPreprocessMnQueryId,
		IRP_MJ_PNP,
		&minorFunction,
		1
		);
	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceInitAssignWdmIrpPreprocessCallback failed Status 0x%x\n", status);

		return status;
	}

	//
	// Setup the device context
	//

	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, CROSKEYBOARD_CONTEXT);

	//
	// Create a framework device object.This call will in turn create
	// a WDM device object, attach to the lower stack, and set the
	// appropriate flags and attributes.
	//

	status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfDeviceCreate failed with status code 0x%x\n", status);

		return status;
	}

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

	queueConfig.EvtIoInternalDeviceControl = CrosKeyboardEvtInternalDeviceControl;

	status = WdfIoQueueCreate(device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue
		);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfIoQueueCreate failed 0x%x\n", status);

		return status;
	}

	//
	// Create manual I/O queue to take care of hid report read requests
	//

	devContext = GetDeviceContext(device);

	WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

	queueConfig.PowerManaged = WdfFalse;

	status = WdfIoQueueCreate(device,
		&queueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->ReportQueue
		);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"WdfIoQueueCreate failed 0x%x\n", status);

		return status;
	}

	//
	// Create an interrupt object for hardware notifications
	//
	WDF_INTERRUPT_CONFIG_INIT(
		&interruptConfig,
		OnInterruptIsr,
		NULL);
	interruptConfig.PassiveHandling = TRUE;

	status = WdfInterruptCreate(
		device,
		&interruptConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&devContext->Interrupt);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
			"Error creating WDF interrupt object - %!STATUS!",
			status);

		return status;
	}

	WDF_TIMER_CONFIG              timerConfig;
	WDFTIMER                      hTimer;

	WDF_TIMER_CONFIG_INIT_PERIODIC(&timerConfig, CrosKeyboardTimerFunc, 10);

	WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
	attributes.ParentObject = device;
	status = WdfTimerCreate(&timerConfig, &attributes, &hTimer);
	devContext->Timer = hTimer;
	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_PNP, "(%!FUNC!) WdfTimerCreate failed status:%!STATUS!\n", status);
		return status;
	}

	//
	// Initialize DeviceMode
	//

	devContext->DeviceMode = DEVICE_MODE_MOUSE;

	return status;
}

NTSTATUS
CrosKeyboardEvtWdmPreprocessMnQueryId(
	WDFDEVICE Device,
	PIRP Irp
	)
{
	NTSTATUS            status;
	PIO_STACK_LOCATION  IrpStack, previousSp;
	PDEVICE_OBJECT      DeviceObject;
	PWCHAR              buffer;

	PAGED_CODE();

	//
	// Get a pointer to the current location in the Irp
	//

	IrpStack = IoGetCurrentIrpStackLocation(Irp);

	//
	// Get the device object
	//
	DeviceObject = WdfDeviceWdmGetDeviceObject(Device);


	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_PNP,
		"CrosKeyboardEvtWdmPreprocessMnQueryId Entry\n");

	//
	// This check is required to filter out QUERY_IDs forwarded
	// by the HIDCLASS for the parent FDO. These IDs are sent
	// by PNP manager for the parent FDO if you root-enumerate this driver.
	//
	previousSp = ((PIO_STACK_LOCATION)((UCHAR *)(IrpStack)+
		sizeof(IO_STACK_LOCATION)));

	if (previousSp->DeviceObject == DeviceObject)
	{
		//
		// Filtering out this basically prevents the Found New Hardware
		// popup for the root-enumerated CrosKeyboard on reboot.
		//
		status = Irp->IoStatus.Status;
	}
	else
	{
		switch (IrpStack->Parameters.QueryId.IdType)
		{
		case BusQueryDeviceID:
		case BusQueryHardwareIDs:
			//
			// HIDClass is asking for child deviceid & hardwareids.
			// Let us just make up some id for our child device.
			//
			buffer = (PWCHAR)ExAllocatePoolWithTag(
				NonPagedPool,
				CROSKEYBOARD_HARDWARE_IDS_LENGTH,
				CROSKEYBOARD_POOL_TAG
				);

			if (buffer)
			{
				//
				// Do the copy, store the buffer in the Irp
				//
				RtlCopyMemory(buffer,
					CROSKEYBOARD_HARDWARE_IDS,
					CROSKEYBOARD_HARDWARE_IDS_LENGTH
					);

				Irp->IoStatus.Information = (ULONG_PTR)buffer;
				status = STATUS_SUCCESS;
			}
			else
			{
				//
				//  No memory
				//
				status = STATUS_INSUFFICIENT_RESOURCES;
			}

			Irp->IoStatus.Status = status;
			//
			// We don't need to forward this to our bus. This query
			// is for our child so we should complete it right here.
			// fallthru.
			//
			IoCompleteRequest(Irp, IO_NO_INCREMENT);

			break;

		default:
			status = Irp->IoStatus.Status;
			IoCompleteRequest(Irp, IO_NO_INCREMENT);
			break;
		}
	}

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardEvtWdmPreprocessMnQueryId Exit = 0x%x\n", status);

	return status;
}

VOID
CrosKeyboardEvtInternalDeviceControl(
	IN WDFQUEUE     Queue,
	IN WDFREQUEST   Request,
	IN size_t       OutputBufferLength,
	IN size_t       InputBufferLength,
	IN ULONG        IoControlCode
	)
{
	NTSTATUS            status = STATUS_SUCCESS;
	WDFDEVICE           device;
	PCROSKEYBOARD_CONTEXT     devContext;
	BOOLEAN             completeRequest = TRUE;

	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);

	device = WdfIoQueueGetDevice(Queue);
	devContext = GetDeviceContext(device);

	CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
		"%s, Queue:0x%p, Request:0x%p\n",
		DbgHidInternalIoctlString(IoControlCode),
		Queue,
		Request
		);

	//
	// Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl. So depending on the ioctl code, we will either
	// use retreive function or escape to WDM to get the UserBuffer.
	//

	switch (IoControlCode)
	{

	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		//
		// Retrieves the device's HID descriptor.
		//
		status = CrosKeyboardGetHidDescriptor(device, Request);
		break;

	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		//
		//Retrieves a device's attributes in a HID_DEVICE_ATTRIBUTES structure.
		//
		status = CrosKeyboardGetDeviceAttributes(Request);
		break;

	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		//
		//Obtains the report descriptor for the HID device.
		//
		status = CrosKeyboardGetReportDescriptor(device, Request);
		break;

	case IOCTL_HID_GET_STRING:
		//
		// Requests that the HID minidriver retrieve a human-readable string
		// for either the manufacturer ID, the product ID, or the serial number
		// from the string descriptor of the device. The minidriver must send
		// a Get String Descriptor request to the device, in order to retrieve
		// the string descriptor, then it must extract the string at the
		// appropriate index from the string descriptor and return it in the
		// output buffer indicated by the IRP. Before sending the Get String
		// Descriptor request, the minidriver must retrieve the appropriate
		// index for the manufacturer ID, the product ID or the serial number
		// from the device extension of a top level collection associated with
		// the device.
		//
		status = CrosKeyboardGetString(Request);
		break;

	case IOCTL_HID_WRITE_REPORT:
	case IOCTL_HID_SET_OUTPUT_REPORT:
		//
		//Transmits a class driver-supplied report to the device.
		//
		status = CrosKeyboardWriteReport(devContext, Request);
		break;

	case IOCTL_HID_READ_REPORT:
	case IOCTL_HID_GET_INPUT_REPORT:
		//
		// Returns a report from the device into a class driver-supplied buffer.
		// 
		status = CrosKeyboardReadReport(devContext, Request, &completeRequest);
		break;

	case IOCTL_HID_SET_FEATURE:
		//
		// This sends a HID class feature report to a top-level collection of
		// a HID class device.
		//
		status = CrosKeyboardSetFeature(devContext, Request, &completeRequest);
		break;

	case IOCTL_HID_GET_FEATURE:
		//
		// returns a feature report associated with a top-level collection
		//
		status = CrosKeyboardGetFeature(devContext, Request, &completeRequest);
		break;

	case IOCTL_HID_ACTIVATE_DEVICE:
		//
		// Makes the device ready for I/O operations.
		//
	case IOCTL_HID_DEACTIVATE_DEVICE:
		//
		// Causes the device to cease operations and terminate all outstanding
		// I/O requests.
		//
	default:
		status = STATUS_NOT_SUPPORTED;
		break;
	}

	if (completeRequest)
	{
		WdfRequestComplete(Request, status);

		CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"%s completed, Queue:0x%p, Request:0x%p\n",
			DbgHidInternalIoctlString(IoControlCode),
			Queue,
			Request
			);
	}
	else
	{
		CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
			"%s deferred, Queue:0x%p, Request:0x%p\n",
			DbgHidInternalIoctlString(IoControlCode),
			Queue,
			Request
			);
	}

	return;
}

NTSTATUS
CrosKeyboardGetHidDescriptor(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request
	)
{
	NTSTATUS            status = STATUS_SUCCESS;
	size_t              bytesToCopy = 0;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetHidDescriptor Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestRetrieveOutputMemory failed 0x%x\n", status);

		return status;
	}

	//
	// Use hardcoded "HID Descriptor" 
	//
	bytesToCopy = DefaultHidDescriptor.bLength;

	if (bytesToCopy == 0)
	{
		status = STATUS_INVALID_DEVICE_STATE;

		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"DefaultHidDescriptor is zero, 0x%x\n", status);

		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0, // Offset
		(PVOID)&DefaultHidDescriptor,
		bytesToCopy);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfMemoryCopyFromBuffer failed 0x%x\n", status);

		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetHidDescriptor Exit = 0x%x\n", status);

	return status;
}

NTSTATUS
CrosKeyboardGetReportDescriptor(
	IN WDFDEVICE Device,
	IN WDFREQUEST Request
	)
{
	NTSTATUS            status = STATUS_SUCCESS;
	ULONG_PTR           bytesToCopy;
	WDFMEMORY           memory;

	UNREFERENCED_PARAMETER(Device);

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetReportDescriptor Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputMemory(Request, &memory);
	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestRetrieveOutputMemory failed 0x%x\n", status);

		return status;
	}

	//
	// Use hardcoded Report descriptor
	//
	bytesToCopy = DefaultHidDescriptor.DescriptorList[0].wReportLength;

	if (bytesToCopy == 0)
	{
		status = STATUS_INVALID_DEVICE_STATE;

		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"DefaultHidDescriptor's reportLength is zero, 0x%x\n", status);

		return status;
	}

	status = WdfMemoryCopyFromBuffer(memory,
		0,
		(PVOID)DefaultReportDescriptor,
		bytesToCopy);
	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfMemoryCopyFromBuffer failed 0x%x\n", status);

		return status;
	}

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, bytesToCopy);

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetReportDescriptor Exit = 0x%x\n", status);

	return status;
}


NTSTATUS
CrosKeyboardGetDeviceAttributes(
	IN WDFREQUEST Request
	)
{
	NTSTATUS                 status = STATUS_SUCCESS;
	PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetDeviceAttributes Entry\n");

	//
	// This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
	// will correctly retrieve buffer from Irp->UserBuffer. 
	// Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
	// field irrespective of the ioctl buffer type. However, framework is very
	// strict about type checking. You cannot get Irp->UserBuffer by using
	// WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
	// internal ioctl.
	//
	status = WdfRequestRetrieveOutputBuffer(Request,
		sizeof(HID_DEVICE_ATTRIBUTES),
		(PVOID *)&deviceAttributes,
		NULL);
	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestRetrieveOutputBuffer failed 0x%x\n", status);

		return status;
	}

	//
	// Set USB device descriptor
	//

	deviceAttributes->Size = sizeof(HID_DEVICE_ATTRIBUTES);
	deviceAttributes->VendorID = CROSKEYBOARD_VID;
	deviceAttributes->ProductID = CROSKEYBOARD_PID;
	deviceAttributes->VersionNumber = CROSKEYBOARD_VERSION;

	//
	// Report how many bytes were copied
	//
	WdfRequestSetInformation(Request, sizeof(HID_DEVICE_ATTRIBUTES));

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetDeviceAttributes Exit = 0x%x\n", status);

	return status;
}

NTSTATUS
CrosKeyboardGetString(
	IN WDFREQUEST Request
	)
{

	NTSTATUS status = STATUS_SUCCESS;
	PWSTR pwstrID;
	size_t lenID;
	WDF_REQUEST_PARAMETERS params;
	void *pStringBuffer = NULL;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetString Entry\n");

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	switch ((ULONG_PTR)params.Parameters.DeviceIoControl.Type3InputBuffer & 0xFFFF)
	{
	case HID_STRING_ID_IMANUFACTURER:
		pwstrID = L"CrosKeyboard.\0";
		break;

	case HID_STRING_ID_IPRODUCT:
		pwstrID = L"MaxTouch Touch Screen\0";
		break;

	case HID_STRING_ID_ISERIALNUMBER:
		pwstrID = L"123123123\0";
		break;

	default:
		pwstrID = NULL;
		break;
	}

	lenID = pwstrID ? wcslen(pwstrID)*sizeof(WCHAR) + sizeof(UNICODE_NULL) : 0;

	if (pwstrID == NULL)
	{

		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"CrosKeyboardGetString Invalid request type\n");

		status = STATUS_INVALID_PARAMETER;

		return status;
	}

	status = WdfRequestRetrieveOutputBuffer(Request,
		lenID,
		&pStringBuffer,
		&lenID);

	if (!NT_SUCCESS(status))
	{

		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"CrosKeyboardGetString WdfRequestRetrieveOutputBuffer failed Status 0x%x\n", status);

		return status;
	}

	RtlCopyMemory(pStringBuffer, pwstrID, lenID);

	WdfRequestSetInformation(Request, lenID);

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetString Exit = 0x%x\n", status);

	return status;
}

NTSTATUS
CrosKeyboardWriteReport(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_REQUEST_PARAMETERS params;
	PHID_XFER_PACKET transferPacket = NULL;
	size_t bytesWritten = 0;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardWriteReport Entry\n");

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"CrosKeyboardWriteReport Xfer packet too small\n");

		status = STATUS_BUFFER_TOO_SMALL;
	}
	else
	{

		transferPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;

		if (transferPacket == NULL)
		{
			CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"CrosKeyboardWriteReport No xfer packet\n");

			status = STATUS_INVALID_DEVICE_REQUEST;
		}
		else
		{
			//
			// switch on the report id
			//

			switch (transferPacket->reportId)
			{
			default:

				CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
					"CrosKeyboardWriteReport Unhandled report type %d\n", transferPacket->reportId);

				status = STATUS_INVALID_PARAMETER;

				break;
			}
		}
	}

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardWriteReport Exit = 0x%x\n", status);

	return status;

}

NTSTATUS
CrosKeyboardProcessVendorReport(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN PVOID ReportBuffer,
	IN ULONG ReportBufferLen,
	OUT size_t* BytesWritten
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDFREQUEST reqRead;
	PVOID pReadReport = NULL;
	size_t bytesReturned = 0;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardProcessVendorReport Entry\n");

	status = WdfIoQueueRetrieveNextRequest(DevContext->ReportQueue,
		&reqRead);

	if (NT_SUCCESS(status))
	{
		status = WdfRequestRetrieveOutputBuffer(reqRead,
			ReportBufferLen,
			&pReadReport,
			&bytesReturned);

		if (NT_SUCCESS(status))
		{
			//
			// Copy ReportBuffer into read request
			//

			if (bytesReturned > ReportBufferLen)
			{
				bytesReturned = ReportBufferLen;
			}

			RtlCopyMemory(pReadReport,
				ReportBuffer,
				bytesReturned);

			//
			// Complete read with the number of bytes returned as info
			//

			WdfRequestCompleteWithInformation(reqRead,
				status,
				bytesReturned);

			CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
				"CrosKeyboardProcessVendorReport %d bytes returned\n", bytesReturned);

			//
			// Return the number of bytes written for the write request completion
			//

			*BytesWritten = bytesReturned;

			CrosKeyboardPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
				"%s completed, Queue:0x%p, Request:0x%p\n",
				DbgHidInternalIoctlString(IOCTL_HID_READ_REPORT),
				DevContext->ReportQueue,
				reqRead);
		}
		else
		{
			CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed Status 0x%x\n", status);
		}
	}
	else
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfIoQueueRetrieveNextRequest failed Status 0x%x\n", status);
	}

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardProcessVendorReport Exit = 0x%x\n", status);

	return status;
}

NTSTATUS
CrosKeyboardReadReport(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
	)
{
	NTSTATUS status = STATUS_SUCCESS;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardReadReport Entry\n");

	//
	// Forward this read request to our manual queue
	// (in other words, we are going to defer this request
	// until we have a corresponding write request to
	// match it with)
	//

	status = WdfRequestForwardToIoQueue(Request, DevContext->ReportQueue);

	if (!NT_SUCCESS(status))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"WdfRequestForwardToIoQueue failed Status 0x%x\n", status);
	}
	else
	{
		*CompleteRequest = FALSE;
	}

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardReadReport Exit = 0x%x\n", status);

	return status;
}

NTSTATUS
CrosKeyboardSetFeature(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_REQUEST_PARAMETERS params;
	PHID_XFER_PACKET transferPacket = NULL;
	CrosKeyboardFeatureReport* pReport = NULL;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardSetFeature Entry\n");

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"CrosKeyboardSetFeature Xfer packet too small\n");

		status = STATUS_BUFFER_TOO_SMALL;
	}
	else
	{

		transferPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;

		if (transferPacket == NULL)
		{
			CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"CrosKeyboardWriteReport No xfer packet\n");

			status = STATUS_INVALID_DEVICE_REQUEST;
		}
		else
		{
			//
			// switch on the report id
			//

			switch (transferPacket->reportId)
			{
			default:

				CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
					"CrosKeyboardSetFeature Unhandled report type %d\n", transferPacket->reportId);

				status = STATUS_INVALID_PARAMETER;

				break;
			}
		}
	}

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardSetFeature Exit = 0x%x\n", status);

	return status;
}

NTSTATUS
CrosKeyboardGetFeature(
	IN PCROSKEYBOARD_CONTEXT DevContext,
	IN WDFREQUEST Request,
	OUT BOOLEAN* CompleteRequest
	)
{
	NTSTATUS status = STATUS_SUCCESS;
	WDF_REQUEST_PARAMETERS params;
	PHID_XFER_PACKET transferPacket = NULL;

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetFeature Entry\n");

	WDF_REQUEST_PARAMETERS_INIT(&params);
	WdfRequestGetParameters(Request, &params);

	if (params.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET))
	{
		CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
			"CrosKeyboardGetFeature Xfer packet too small\n");

		status = STATUS_BUFFER_TOO_SMALL;
	}
	else
	{

		transferPacket = (PHID_XFER_PACKET)WdfRequestWdmGetIrp(Request)->UserBuffer;

		if (transferPacket == NULL)
		{
			CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
				"CrosKeyboardGetFeature No xfer packet\n");

			status = STATUS_INVALID_DEVICE_REQUEST;
		}
		else
		{
			//
			// switch on the report id
			//

			switch (transferPacket->reportId)
			{
			default:

				CrosKeyboardPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
					"CrosKeyboardGetFeature Unhandled report type %d\n", transferPacket->reportId);

				status = STATUS_INVALID_PARAMETER;

				break;
			}
		}
	}

	CrosKeyboardPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
		"CrosKeyboardGetFeature Exit = 0x%x\n", status);

	return status;
}

PCHAR
DbgHidInternalIoctlString(
	IN ULONG IoControlCode
	)
{
	switch (IoControlCode)
	{
	case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
		return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
	case IOCTL_HID_GET_REPORT_DESCRIPTOR:
		return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
	case IOCTL_HID_READ_REPORT:
		return "IOCTL_HID_READ_REPORT";
	case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
		return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
	case IOCTL_HID_WRITE_REPORT:
		return "IOCTL_HID_WRITE_REPORT";
	case IOCTL_HID_SET_FEATURE:
		return "IOCTL_HID_SET_FEATURE";
	case IOCTL_HID_GET_FEATURE:
		return "IOCTL_HID_GET_FEATURE";
	case IOCTL_HID_GET_STRING:
		return "IOCTL_HID_GET_STRING";
	case IOCTL_HID_ACTIVATE_DEVICE:
		return "IOCTL_HID_ACTIVATE_DEVICE";
	case IOCTL_HID_DEACTIVATE_DEVICE:
		return "IOCTL_HID_DEACTIVATE_DEVICE";
	case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
		return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
	case IOCTL_HID_SET_OUTPUT_REPORT:
		return "IOCTL_HID_SET_OUTPUT_REPORT";
	case IOCTL_HID_GET_INPUT_REPORT:
		return "IOCTL_HID_GET_INPUT_REPORT";
	default:
		return "Unknown IOCTL";
	}
}
