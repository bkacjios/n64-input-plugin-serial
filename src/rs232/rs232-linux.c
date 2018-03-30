/*
	Cross-platform serial / RS232 library
	Version 0.21, 11/10/2015
	-> LINUX and MacOS implementation
	-> rs232-linux.c

	The MIT License (MIT)

	Copyright (c) 2013-2015 Frédéric Meslin, Florent Touchard
	Email: fredericmeslin@hotmail.com
	Website: www.fredslab.net
	Twitter: @marzacdev

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all
	copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
	SOFTWARE.
  
*/

#if defined(__unix__) || defined(__unix) || \
	defined(__APPLE__) && defined(__MACH__)

#define _DARWIN_C_SOURCE

#include "rs232.h"

#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <dirent.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*****************************************************************************/
/** Base name for COM devices */
#if defined(__APPLE__) && defined(__MACH__)
	static const char * devBases[] = {
		"tty."
	};
	static int noBases = 1;
#else
	static const char * devBases[] = {
		"ttyACM", "ttyUSB", "rfcomm", "ttyS"
	};
	static int noBases = 4;
#endif

/*****************************************************************************/
typedef struct {
	char * port;
	int handle;
} COMDevice;

#define COM_MAXDEVICES        64
static COMDevice comDevices[COM_MAXDEVICES];
static int noDevices = 0;

/*****************************************************************************/
/** Private functions */
void _AppendDevices(const char * base);
int _BaudFlag(int BaudRate);

/*****************************************************************************/
int comEnumerate()
{
	for (int i = 0; i < noDevices; i++) {
		if (comDevices[i].port) free(comDevices[i].port);
		comDevices[i].port = NULL;
	}
	noDevices = 0;
	for (int i = 0; i < noBases; i++)
		_AppendDevices(devBases[i]);
	return noDevices;
}

void comTerminate()
{
	comCloseAll();
	for (int i = 0; i < noDevices; i++) {
		if (comDevices[i].port) free(comDevices[i].port);
		comDevices[i].port = NULL;
	}
}

int comGetNoPorts()
{
	return noDevices;
}

/*****************************************************************************/
int comFindPort(const char * name)
{
	int p;
	for (p = 0; p < noDevices; p++)
		if (strcmp(name, comDevices[p].port) == 0)
			return p;
	return -1;
}

const char * comGetInternalName(int index)
{
	#define COM_MAXNAME    128
	static char name[COM_MAXNAME];
	if (index >= noDevices || index < 0)
		return NULL;
	sprintf(name, "/dev/%s", comDevices[index].port);
	return name;
}

const char * comGetPortName(int index) {
	if (index >= noDevices || index < 0)
		return NULL;
	return comDevices[index].port;
}

/*****************************************************************************/
int comOpen(int index, int baudrate)
{
	if (index >= noDevices || index < 0)
		return 0;
	// Close if already open
	COMDevice * com = &comDevices[index];
	if (com->handle >= 0) comClose(index);
	// Open port
	printf("Try %s \n", comGetInternalName(index));
	int handle = open(comGetInternalName(index), O_RDWR | O_NOCTTY);
	if (handle < 0)
		return 0;
	printf("Open %s \n", comGetInternalName(index));
	// General configuration
	struct termios config;
	memset(&config, 0, sizeof(config));

	config.c_iflag |= IGNBRK;
	config.c_oflag = 0;
	config.c_lflag = 0;
	config.c_cflag = CREAD | CLOCAL | CS8;

	int flag = _BaudFlag(baudrate);
	cfsetospeed(&config, flag);
	cfsetispeed(&config, flag);

	// Validate configuration
	if (tcsetattr(handle, TCSANOW, &config) < 0) {
		close(handle);
		return 0;
	}
	com->handle = handle;
	return 1;
}

void comClose(int index)
{
	if (index >= noDevices || index < 0)
		return;
	COMDevice * com = &comDevices[index];
	if (com->handle < 0) 
		return;
	tcdrain(com->handle);
	close(com->handle);
	com->handle = -1;
}

void comCloseAll()
{
	for (int i = 0; i < noDevices; i++)
		comClose(i);
}

/*****************************************************************************/
int comWrite(int index, const char * buffer, size_t len)
{
	if (index >= noDevices || index < 0)
		return 0;
	if (comDevices[index].handle <= 0)
		return 0;
	int res = write(comDevices[index].handle, buffer, len);
	return res;
}

int comRead(int index, char * buffer, size_t len)
{
	if (index >= noDevices || index < 0)
		return 0;
	if (comDevices[index].handle <= 0)
		return 0;

	size_t bytes_left, bytes_read;
	ssize_t res;

	bytes_left = len;
	bytes_read = 0;

	do {
		if ((res = read(comDevices[index].handle, buffer + bytes_read, bytes_left)) < 0)
			return -1;

		bytes_read += res;
		bytes_left -= res;
	} while (bytes_left > 0);

	return bytes_read;
}

/*****************************************************************************/
int _BaudFlag(int BaudRate)
{
	switch(BaudRate)
	{
		case 50: return B50;
		case 75: return B75;
		case 110: return B110;
		case 134: return B134;
		case 150: return B150;
		case 200: return B200;
		case 300: return B300;
		case 600: return B600;
		case 1200: return B1200;
		case 1800: return B1800;
		case 2400: return B2400;
		case 4800: return B4800;
		case 9600: return B9600;
		case 19200: return B19200;
		case 38400: return B38400;
		case 57600: return B57600;
		case 115200: return B115200;
		case 230400: return B230400;
		case 460800: return B460800;
		case 500000: return B500000;
		case 576000: return B576000;
		case 921600: return B921600;
		case 1000000: return B1000000;
		case 1152000: return B1152000;
		case 1500000: return B1500000;
		case 2000000: return B2000000;
#ifdef B2500000
		case 2500000: return B2500000;
#endif
#ifdef B3000000
		case 3000000: return B3000000;
#endif
#ifdef B3500000
		case 3500000: return B3500000;
#endif
#ifdef B4000000
		case 4000000: return B4000000;
#endif
		default: return -1;
	}
}

void _AppendDevices(const char * base)
{
	int baseLen = strlen(base);
	struct dirent * dp;
	// Enumerate devices
	DIR * dirp = opendir("/dev");
	while ((dp = readdir(dirp)) && noDevices < COM_MAXDEVICES) {
		if (strlen(dp->d_name) >= baseLen) {
			if (memcmp(base, dp->d_name, baseLen) == 0) {
				COMDevice * com = &comDevices[noDevices ++];
				com->port = (char *) strdup(dp->d_name);
				com->handle = -1;
			}
		}
	}
	closedir(dirp);
}

#endif // unix
