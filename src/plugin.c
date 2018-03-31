#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "plugin.h"
#include "version.h"
#include "rs232.h"

#ifdef PROJECT_64
#include "configini.h"

#define CONFIG_FILE "Config\\Serial-Input.ini"

static Config *l_ConfigInput;
#endif

/* global data definitions */
SController controller[4];  // 4 controllers

#ifndef PROJECT_64
/* static data definitions */
static void (*l_DebugCallback)(void *, int, const char *) = NULL;
static void *l_DebugCallContext = NULL;
static int l_PluginInit = 0;

static m64p_handle l_ConfigInput;

ptr_ConfigOpenSection      ConfigOpenSection = NULL;
ptr_ConfigSaveSection      ConfigSaveSection = NULL;
ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
ptr_ConfigSetDefaultBool   ConfigSetDefaultBool = NULL;
ptr_ConfigSetDefaultString ConfigSetDefaultString = NULL;
ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
ptr_ConfigGetParamBool     ConfigGetParamBool = NULL;
ptr_ConfigGetParamString   ConfigGetParamString = NULL;
#endif

/* Global functions */
void DebugMessage(int level, const char *message, ...)
{
	char msgbuf[1024];
	va_list args;

#ifndef PROJECT_64
	if (l_DebugCallback == NULL)
		return;
#endif

	va_start(args, message);
	vsprintf(msgbuf, message, args);

#ifdef PROJECT_64
	printf("%s\n", msgbuf);
#else
	(*l_DebugCallback)(l_DebugCallContext, level, msgbuf);
#endif

	va_end(args);
}

void InitializeComPorts()
{
	int devices = comEnumerate();

	for (int i = 0; i < comGetNoPorts(); i++)
		DebugMessage(M64MSG_INFO, "com[%i]: %s", i, comGetPortName(i));

	DebugMessage(M64MSG_INFO, "Found %i serial devices", devices);
}

#ifdef PROJECT_64

EXPORT void PluginLoaded(void)
{
	InitializeComPorts();

	l_ConfigInput = ConfigNew();

	/* set settings */
	ConfigSetBoolString(l_ConfigInput, "true", "false");

	if (ConfigReadFile(CONFIG_FILE, &l_ConfigInput) != CONFIG_OK)
		printf("ConfigOpenFile failed for " CONFIG_FILE);

	if (!ConfigHasSection(l_ConfigInput, "Controller 1")) {
		ConfigAddBool(l_ConfigInput, "Controller 1", "Enabled", false);
		ConfigAddString(l_ConfigInput, "Controller 1", "Serial", "COM1");
		ConfigAddInt(l_ConfigInput, "Controller 1", "Baud", 115200);
	}

	if (!ConfigHasSection(l_ConfigInput, "Controller 2")) {
		ConfigAddBool(l_ConfigInput, "Controller 2", "Enabled", false);
		ConfigAddString(l_ConfigInput, "Controller 2", "Serial", "COM2");
		ConfigAddInt(l_ConfigInput, "Controller 2", "Baud", 115200);
	}

	if (!ConfigHasSection(l_ConfigInput, "Controller 3")) {
		ConfigAddBool(l_ConfigInput, "Controller 3", "Enabled", false);
		ConfigAddString(l_ConfigInput, "Controller 3", "Serial", "COM3");
		ConfigAddInt(l_ConfigInput, "Controller 3", "Baud", 115200);
	}

	if (!ConfigHasSection(l_ConfigInput, "Controller 4")) {
		ConfigAddBool(l_ConfigInput, "Controller 4", "Enabled", false);
		ConfigAddString(l_ConfigInput, "Controller 4", "Serial", "COM4");
		ConfigAddInt(l_ConfigInput, "Controller 4", "Baud", 115200);
	}

	ConfigPrintToFile(l_ConfigInput, CONFIG_FILE);
}

EXPORT void CloseDLL(void)
{
	comTerminate();
	ConfigFree(l_ConfigInput);
}

BOOL APIENTRY DllMain(HINSTANCE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:;
		break;

	case DLL_THREAD_ATTACH:
		break;

	case DLL_THREAD_DETACH:
		break;

	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

/******************************************************************
Function: GetDllInfo
Purpose:  This function allows the emulator to gather information
about the dll by filling in the PluginInfo structure.
input:    a pointer to a PLUGIN_INFO stucture that needs to be
filled by the function. (see def above)
output:   none
*******************************************************************/
EXPORT void CALL GetDllInfo(PLUGIN_INFO* PluginInfo)
{
#ifdef _DEBUG
	sprintf(PluginInfo->Name, "Direct Serial Input (Debug): %i.%i.%i", VERSION_PRINTF_SPLIT(PLUGIN_VERSION));
#else
	sprintf(PluginInfo->Name, "Direct Serial Input: %i.%i.%i", VERSION_PRINTF_SPLIT(PLUGIN_VERSION));
#endif
	PluginInfo->Type = PLUGIN_TYPE_CONTROLLER;
	PluginInfo->Version = INPUT_API_VERSION_PJ64;
}
#else

/* Mupen64Plus plugin functions */
EXPORT m64p_error CALL PluginStartup(m64p_dynlib_handle CoreLibHandle, void *Context, void (*DebugCallback)(void *, int, const char *))
{
	ptr_CoreGetAPIVersions CoreAPIVersionFunc;

	if (l_PluginInit)
		return M64ERR_ALREADY_INIT;

	/* first thing is to set the callback function for debug info */
	l_DebugCallback = DebugCallback;
	l_DebugCallContext = Context;

	/* attach and call the CoreGetAPIVersions function, check Config API version for compatibility */
	CoreAPIVersionFunc = (ptr_CoreGetAPIVersions) DLSYM(CoreLibHandle, "CoreGetAPIVersions");
	if (CoreAPIVersionFunc == NULL)
	{
		DebugMessage(M64MSG_ERROR, "Core emulator broken; no CoreAPIVersionFunc() function found.");
		return M64ERR_INCOMPATIBLE;
	}

	int ConfigAPIVersion, DebugAPIVersion, VidextAPIVersion;
	
	(*CoreAPIVersionFunc)(&ConfigAPIVersion, &DebugAPIVersion, &VidextAPIVersion, NULL);
	if ((ConfigAPIVersion & 0xffff0000) != (CONFIG_API_VERSION & 0xffff0000) || ConfigAPIVersion < CONFIG_API_VERSION)
	{
		DebugMessage(M64MSG_ERROR, "Emulator core Config API (v%i.%i.%i) incompatible with plugin (v%i.%i.%i)",
				VERSION_PRINTF_SPLIT(ConfigAPIVersion), VERSION_PRINTF_SPLIT(CONFIG_API_VERSION));
		return M64ERR_INCOMPATIBLE;
	}

	ConfigOpenSection = (ptr_ConfigOpenSection) DLSYM(CoreLibHandle, "ConfigOpenSection");
	ConfigSaveSection = (ptr_ConfigSaveSection) DLSYM(CoreLibHandle, "ConfigSaveSection");
	ConfigSetDefaultInt = (ptr_ConfigSetDefaultInt) DLSYM(CoreLibHandle, "ConfigSetDefaultInt");
	ConfigSetDefaultBool = (ptr_ConfigSetDefaultBool) DLSYM(CoreLibHandle, "ConfigSetDefaultBool");
	ConfigSetDefaultString = (ptr_ConfigSetDefaultString) DLSYM(CoreLibHandle, "ConfigSetDefaultString");
	ConfigGetParamInt = (ptr_ConfigGetParamInt)DLSYM(CoreLibHandle, "ConfigGetParamInt");
	ConfigGetParamBool = (ptr_ConfigGetParamBool)DLSYM(CoreLibHandle, "ConfigGetParamBool");
	ConfigGetParamString = (ptr_ConfigGetParamString) DLSYM(CoreLibHandle, "ConfigGetParamString");

	if (!ConfigOpenSection || !ConfigSaveSection || !ConfigSetDefaultInt || !ConfigSetDefaultString|| !ConfigGetParamInt || !ConfigGetParamBool || !ConfigGetParamString)
		return M64ERR_INCOMPATIBLE;

	if (ConfigOpenSection("Input-Serial", &l_ConfigInput) != M64ERR_SUCCESS)
	{
		DebugMessage(M64MSG_ERROR, "Couldn't open config section 'Input-Serial'");
		return M64ERR_INPUT_NOT_FOUND;
	}

	ConfigSetDefaultBool(l_ConfigInput, "Enabled1", 0, "Set controller 1 on or off");
	ConfigSetDefaultString(l_ConfigInput, "Serial1", "ttyACM0", "Serial device for controller");
	ConfigSetDefaultInt(l_ConfigInput, "Baud1", 115200, "Baud rate for controller 1");

	ConfigSetDefaultBool(l_ConfigInput, "Enabled2", 0, "Set controller 2 on or off");
	ConfigSetDefaultString(l_ConfigInput, "Serial2", "ttyACM1", "Serial device for controller");
	ConfigSetDefaultInt(l_ConfigInput, "Baud2", 115200, "Baud rate for controller 2");

	ConfigSetDefaultBool(l_ConfigInput, "Enabled3", 0, "Set controller 3 on or off");
	ConfigSetDefaultString(l_ConfigInput, "Serial3", "ttyACM2", "Serial device for controller");
	ConfigSetDefaultInt(l_ConfigInput, "Baud3", 115200, "Baud rate for controller 3");

	ConfigSetDefaultBool(l_ConfigInput, "Enabled4", 0, "Set controller 4 on or off");
	ConfigSetDefaultString(l_ConfigInput, "Serial4", "ttyACM3", "Serial device for controller");
	ConfigSetDefaultInt(l_ConfigInput, "Baud4", 115200, "Baud rate for controller 4");
	ConfigSaveSection("Input-Serial");

	InitializeComPorts();

	l_PluginInit = 1;
	return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginShutdown(void)
{
	if (!l_PluginInit)
		return M64ERR_NOT_INIT;

	comTerminate();

	l_PluginInit = 0;
	return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL PluginGetVersion(m64p_plugin_type *PluginType, int *PluginVersion, int *APIVersion, const char **PluginNamePtr, int *Capabilities)
{
	/* set version info */
	if (PluginType != NULL)
		*PluginType = M64PLUGIN_INPUT;

	if (PluginVersion != NULL)
		*PluginVersion = PLUGIN_VERSION;

	if (APIVersion != NULL)
		*APIVersion = INPUT_API_VERSION;

	if (PluginNamePtr != NULL)
		*PluginNamePtr = PLUGIN_NAME;

	if (Capabilities != NULL)
		*Capabilities = 0;

	return M64ERR_SUCCESS;
}

#endif

/******************************************************************
	Function: InitiateControllers
	Purpose:  This function initialises how each of the controllers
						should be handled.
	input:    - The handle to the main window.
						- A controller structure that needs to be filled for
							the emulator to know how to handle each controller.
	output:   none
*******************************************************************/
EXPORT void CALL InitiateControllers(CONTROL_INFO ControlInfo)
{
	// reset controllers
	memset(controller, 0, sizeof(SController));

	for (int i=0; i<4; i++)
	{
		// set our CONTROL struct pointers to the array that was passed in to this function from the core
		// this small struct tells the core whether each controller is plugged in, and what type of pak is connected
		controller[i].control = ControlInfo.Controls;

#ifdef PROJECT_64
		char serial_sec_buf[13];
		sprintf(serial_sec_buf, "Controller %d", i + 1);

		bool enabled;
		char serial[1024];
		int baud;

		ConfigReadBool(l_ConfigInput, serial_sec_buf, "Enabled", &enabled, false);
		ConfigReadString(l_ConfigInput, serial_sec_buf, "Serial", serial, sizeof(serial), "COM1");
		ConfigReadInt(l_ConfigInput, serial_sec_buf, "Baud", &baud, 115200);
#else
		char enabled_param_buf[9];
		sprintf(enabled_param_buf, "Enabled%d", i + 1);

		char serial_param_buf[8];
		sprintf(serial_param_buf, "Serial%d", i + 1);
		char baud_param_buf[6];
		sprintf(baud_param_buf, "Baud%d", i + 1);

		int enabled = ConfigGetParamBool(l_ConfigInput, enabled_param_buf);
		const char* serial	= ConfigGetParamString(l_ConfigInput, serial_param_buf);
		int baud	= ConfigGetParamInt(l_ConfigInput, baud_param_buf);
#endif

		if (enabled && serial && baud)
		{
			int port = comFindPort(serial);

			if (port >= 0 && comOpen(port, baud) != 0)
			{
				DebugMessage(M64MSG_INFO, "Assigned controller %i to serial port %s", i+1, serial);

				// init controller
				controller[i].control->Present = 1;
				controller[i].control->RawData = 1;
				controller[i].control->Plugin = PLUGIN_NONE;
				controller[i].serial = port;
			}
		}
	}

	DebugMessage(M64MSG_INFO, "%s version %i.%i.%i initialized.", PLUGIN_NAME, VERSION_PRINTF_SPLIT(PLUGIN_VERSION));
}

/******************************************************************
	Function: ControllerCommand
	Purpose:  To process the raw data that has just been sent to a
						specific controller.
	input:    - Controller Number (0 to 3) and -1 signalling end of
							processing the pif ram.
						- Pointer of data to be processed.
	output:   none

	note:     This function is only needed if the DLL is allowing raw
						data, or the plugin is set to raw

						the data that is being processed looks like this:
						initilize controller: 01 03 00 FF FF FF
						read controller:      01 04 01 FF FF FF FF
*******************************************************************/
EXPORT void CALL ControllerCommand(int index, unsigned char *cmd)
{
}

/******************************************************************
	Function: ReadController
	Purpose:  To process the raw data in the pif ram that is about to
						be read.
	input:    - Controller Number (0 to 3) and -1 signalling end of
							processing the pif ram.
						- Pointer of data to be processed.
	output:   none
	note:     This function is only needed if the DLL is allowing raw
						data.
*******************************************************************/
EXPORT void CALL ReadController(int index, unsigned char *cmd)
{
	if (cmd == NULL || !controller[index].control->Present)
		return;

	unsigned char tx_len = cmd[0] & 0x3F;
	const unsigned char rx_len = cmd[1] & 0x3F;

	unsigned char *rx_data = cmd + 2 + tx_len;

	comWrite(controller[index].serial, (const char*) cmd, 2 + tx_len);

	char buffer[33];
	memset(buffer, 0, sizeof(buffer));

	comRead(controller[index].serial, buffer, rx_len);

	memcpy(rx_data, buffer, rx_len);
}

/******************************************************************
	Function: RomOpen
	Purpose:  This function is called when a rom is open. (from the
						emulation thread)
	input:    none
	output:   none
*******************************************************************/
EXPORT int CALL RomOpen(void)
{
	return 1;
}

/******************************************************************
	Function: RomClosed
	Purpose:  This function is called when a rom is closed.
	input:    none
	output:   none
*******************************************************************/
EXPORT void CALL RomClosed(void)
{
}

/******************************************************************
	Function: GetKeys
	Purpose:  To get the current state of the controllers buttons.
	input:    - Controller Number (0 to 3)
						- A pointer to a BUTTONS structure to be filled with
						the controller state.
	output:   none
*******************************************************************/
EXPORT void CALL GetKeys(int index, BUTTONS *keys)
{
}

/******************************************************************
	Function: SDL_KeyDown
	Purpose:  To pass the SDL_KeyDown message from the emulator to the
						plugin.
	input:    keymod and keysym of the SDL_KEYDOWN message.
	output:   none
*******************************************************************/
EXPORT void CALL SDL_KeyDown(int keymod, int keysym)
{
}

/******************************************************************
	Function: SDL_KeyUp
	Purpose:  To pass the SDL_KeyUp message from the emulator to the
						plugin.
	input:    keymod and keysym of the SDL_KEYUP message.
	output:   none
*******************************************************************/
EXPORT void CALL SDL_KeyUp(int keymod, int keysym)
{
}
