#ifndef _PYEXT_H
#define _PYEXT_H

#include <windows.h>
#define KDEXT_64BIT
#include <dbgeng.h>

extern IDebugControl *gPyDebugControl;
extern IDebugClient  *gDebugClient;
extern IDebugControl *gDebugControl;

#define ENTER_CALLBACK(Client)  \
        if (ExtQuery(Client) != S_OK) { return E_FAIL; }
#define LEAVE_CALLBACK()    ExtRelease()

HRESULT ExtQuery(IN IDebugClient *Client);
void ExtRelease(void);

#endif


