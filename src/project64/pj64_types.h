#ifndef PROJECT64_TYPES
#define PROJECT64_TYPES

typedef struct
{
	unsigned short Version;        /* Should be set to 0x0101 */
	unsigned short Type;           /* Set to PLUGIN_TYPE_RSP */
	char Name[100];      /* Name of the DLL */

						 /* If DLL supports memory these memory options then set them to TRUE or FALSE
						 if it does not support it */
	int NormalMemory;   /* a normal BYTE array */
	int MemoryBswaped;  /* a normal BYTE array where the memory has been pre
						bswap on a dword (32 bits) boundry */
} PLUGIN_INFO;

typedef enum {
	M64MSG_ERROR = 1,
	M64MSG_WARNING,
	M64MSG_INFO,
	M64MSG_STATUS,
	M64MSG_VERBOSE
} m64p_msg_level;

typedef struct
{
    int Present;
	int RawData;
	int Plugin;
} CONTROL;

typedef union {
	unsigned int Value;
	struct {
		unsigned R_DPAD : 1;
		unsigned L_DPAD : 1;
		unsigned D_DPAD : 1;
		unsigned U_DPAD : 1;
		unsigned START_BUTTON : 1;
		unsigned Z_TRIG : 1;
		unsigned B_BUTTON : 1;
		unsigned A_BUTTON : 1;

		unsigned R_CBUTTON : 1;
		unsigned L_CBUTTON : 1;
		unsigned D_CBUTTON : 1;
		unsigned U_CBUTTON : 1;
		unsigned R_TRIG : 1;
		unsigned L_TRIG : 1;
		unsigned Reserved1 : 1;
		unsigned Reserved2 : 1;

		signed   X_AXIS : 8;
		signed   Y_AXIS : 8;
	};
} BUTTONS;

typedef struct
{
    void * hMainWindow;
    void * hinst;

	int MemoryBswaped;  // memory in client- or server-native endian
    unsigned char * HEADER;   // the ROM header (first 40h bytes of the ROM)
    CONTROL * Controls; // pointer to array of 4 controllers, i.e.:  CONTROL Controls[4];
} CONTROL_INFO;

enum PluginType
{
    PLUGIN_NONE = 1,
    PLUGIN_MEMPAK = 2,
    PLUGIN_RUMBLE_PAK = 3,
    PLUGIN_TANSFER_PAK = 4, // not implemeted for non raw data
    PLUGIN_RAW = 5, // the controller plugin is passed in raw data
};

enum
{
	PLUGIN_TYPE_RSP = 1,
	PLUGIN_TYPE_GFX = 2,
	PLUGIN_TYPE_AUDIO = 3,
	PLUGIN_TYPE_CONTROLLER = 4,
};

#endif