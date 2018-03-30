#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <sys/socket.h>

#include "plugin.h"
#include "version.h"
#include "rs232.h"

/* global data definitions */
SController controller[4];  // 1 controller

/* static data definitions */
static void (*l_DebugCallback)(void *, int, const char *) = NULL;
static void *l_DebugCallContext = NULL;
static int l_PluginInit = 0;

static m64p_handle l_ConfigInput;

ptr_ConfigOpenSection      ConfigOpenSection = NULL;
ptr_ConfigSaveSection      ConfigSaveSection = NULL;
ptr_ConfigSetDefaultInt    ConfigSetDefaultInt = NULL;
ptr_ConfigSetDefaultString ConfigSetDefaultString = NULL;
ptr_ConfigGetParamInt      ConfigGetParamInt = NULL;
ptr_ConfigGetParamString   ConfigGetParamString = NULL;

/* Global functions */
void DebugMessage(int level, const char *message, ...)
{
	char msgbuf[1024];
	va_list args;

	if (l_DebugCallback == NULL)
		return;

	va_start(args, message);
	vsprintf(msgbuf, message, args);

	(*l_DebugCallback)(l_DebugCallContext, level, msgbuf);

	va_end(args);
}

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
	ConfigSetDefaultString = (ptr_ConfigSetDefaultString) DLSYM(CoreLibHandle, "ConfigSetDefaultString");
	ConfigGetParamInt = (ptr_ConfigGetParamInt) DLSYM(CoreLibHandle, "ConfigGetParamInt");
	ConfigGetParamString = (ptr_ConfigGetParamString) DLSYM(CoreLibHandle, "ConfigGetParamString");

	if (!ConfigOpenSection || !ConfigSaveSection || !ConfigSetDefaultInt || !ConfigSetDefaultString|| !ConfigGetParamInt || !ConfigGetParamString)
		return M64ERR_INCOMPATIBLE;

	if (ConfigOpenSection("Input-Serial", &l_ConfigInput) != M64ERR_SUCCESS)
	{
		DebugMessage(M64MSG_ERROR, "Couldn't open config section 'Input-Serial'");
		return M64ERR_INPUT_NOT_FOUND;
	}

	ConfigSetDefaultString(l_ConfigInput, "Serial1", "ttyACM0", "Serial device for controller");
	ConfigSetDefaultString(l_ConfigInput, "Serial2", "ttyACM1", "Serial device for controller");
	ConfigSetDefaultString(l_ConfigInput, "Serial3", "ttyACM2", "Serial device for controller");
	ConfigSetDefaultString(l_ConfigInput, "Serial4", "ttyACM3", "Serial device for controller");

	ConfigSetDefaultInt(l_ConfigInput, "Baud1", 115200, "Baud rate for controller 1");
	ConfigSetDefaultInt(l_ConfigInput, "Baud2", 115200, "Baud rate for controller 2");
	ConfigSetDefaultInt(l_ConfigInput, "Baud3", 115200, "Baud rate for controller 3");
	ConfigSetDefaultInt(l_ConfigInput, "Baud4", 115200, "Baud rate for controller 4");
	ConfigSaveSection("Input-Serial");

	int devices = comEnumerate();

	for (int i=0; i<comGetNoPorts(); i++)
	{
		printf("com[%i]: %s\n", i, comGetPortName(i));
	}

	printf("Found %i serial devices\n", devices);

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

		char serial_param_buf[8];
		sprintf(serial_param_buf, "Serial%d", i+1);
		char baud_param_buf[6];
		sprintf(baud_param_buf, "Baud%d", i+1);

		const char* serial	= ConfigGetParamString(l_ConfigInput, serial_param_buf);
		int baud	= ConfigGetParamInt(l_ConfigInput, baud_param_buf);

		if (serial && baud)
		{
			int port = comFindPort(serial);

			if (port >= 0 && comOpen(port, baud) != 0)
			{
				printf("Assigned controller %i to serial port %s\n", i+1, serial);

				// init controller
				controller[i].control->Present = 1;
				controller[i].control->RawData = 1;
				controller[i].control->Plugin = PLUGIN_NONE;
				controller[0].serial = port;
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
EXPORT void CALL ControllerCommand(int Control, unsigned char *Command)
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
EXPORT void CALL ReadController(int Control, unsigned char *Command)
{
	if (Command == NULL || Control != 0)
		return;

	unsigned char tx_len = Command[0] & 0x3F;
	unsigned char rx_len = Command[1] & 0x3F;

	unsigned char *rx_data = Command + 2 + tx_len;

	comWrite(controller[0].serial, (const char*) Command, 2 + tx_len);

	char buffer[rx_len];
	memset(buffer, 0, sizeof(buffer));

	comRead(controller[0].serial, buffer, rx_len);

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
EXPORT void CALL GetKeys(int Control, BUTTONS *Keys )
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
