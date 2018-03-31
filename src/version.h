#ifndef __VERSION_H__
#define __VERSION_H__

#define PLUGIN_NAME				"Mupen64Plus Serial Input Plugin"
#define PLUGIN_VERSION			0x00002
#define INPUT_API_VERSION		0x20001
#define INPUT_API_VERSION_PJ64	0x0101
#define CONFIG_API_VERSION		0x20100

#define VERSION_PRINTF_SPLIT(x) (((x) >> 16) & 0xffff), (((x) >> 8) & 0xff), ((x) & 0xff)

#endif /* #define __VERSION_H__ */
