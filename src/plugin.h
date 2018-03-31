#ifndef __PLUGIN_H__
#define __PLUGIN_H__

#ifndef PROJECT_64
#include "m64p_common.h"
#include "m64p_plugin.h"
#include "m64p_types.h"
#include "m64p_config.h"

/* global function definitions */
extern void DebugMessage(int level, const char *message, ...);
#else
#include "pj64_types.h"

#if defined(_WIN32)
#define IMPORT extern "C" __declspec(dllimport)
#define EXPORT __declspec(dllexport)
#define CALL   __cdecl
#else
#define IMPORT extern "C"
#define EXPORT __attribute__((visibility("default")))
#define CALL
#endif
#endif

#ifdef _WIN32
#include <Windows.h>
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

#endif // __PLUGIN_H__
