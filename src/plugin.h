#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#include "m64p_common.h"
#include "m64p_plugin.h"
#include "m64p_types.h"
#include "m64p_config.h"

# ifdef _WIN32
#define DLSYM(a, b) GetProcAddress(a, b)
#else
#include <dlfcn.h>
#define DLSYM(a, b) dlsym(a, b)
#endif

typedef struct
{
    CONTROL *control;	// pointer to CONTROL struct in Core library
    int serial;			// pointer to BUTTONS struct in Core library
} SController;

/* global function definitions */
extern void DebugMessage(int level, const char *message, ...);

#endif // __PLUGIN_H__
