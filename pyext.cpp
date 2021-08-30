#include "pyext.h"
#include "pyembed.h"

#define VER_MAJ 1
#define VER_MIN 0

// For Callbacks
IDebugClient  *gDebugClient = NULL;
IDebugControl *gPyDebugControl = NULL;

// Current scope
IDebugControl *gDebugControl = NULL;

HRESULT ExtQuery(IN IDebugClient *Client)
{
    return Client->QueryInterface(__uuidof(IDebugControl),
                    (void **)&gDebugControl);
}

void ExtRelease(void)
{
    if (gDebugControl != NULL) {
        gDebugControl->Release();
        gDebugControl = NULL;
    }
}


HRESULT CALLBACK
DebugExtensionInitialize(OUT PULONG Version, OUT PULONG Flags)
{
    HRESULT hr = S_FALSE;

    *Version = DEBUG_EXTENSION_VERSION(VER_MAJ, VER_MIN);
    *Flags = 0;

    if ((hr = DebugCreate(__uuidof(IDebugClient), (void**)&gDebugClient)) != S_OK)
        return hr;

    if ((hr = gDebugClient->QueryInterface(__uuidof(IDebugControl),
        (void**)&gPyDebugControl)) != S_OK)
        goto fail;

    if (python_init() != S_OK)
    {
        hr = E_FAIL;
        goto fail;
    }
    return S_OK;

fail:
    if (gPyDebugControl) {
        gPyDebugControl->Release();
        gPyDebugControl = NULL;
    }
    if (gDebugClient) {
        gDebugClient->Release(); 
        gDebugClient = NULL;
    }
    return hr;
}

void CALLBACK
DebugExtensionUninitialize(void)
{
    python_fini();

    if (gPyDebugControl) {
        gPyDebugControl->Release();
        gPyDebugControl = NULL;
    }
    if (gDebugClient) {
        gDebugClient->Release();
        gDebugClient = NULL;
    }

    return;
}

HRESULT CALLBACK
help(IN IDebugClient *Client, IN OPTIONAL PCSTR args)
{
    char help_msg[] = 
        "\n PyExt - Windbg Python Extension\n"
        "\teval [expr]\n"
        "\t\t- Evaluate an expression\n"
        "\texec [filename]\n"
        "\t\t- Run a script\n"
        "\tpython\n"
        "\t\t- Interactive Python interpreter\n"
        "";
    UNREFERENCED_PARAMETER(args);
    ENTER_CALLBACK(Client);

    gDebugControl->Output(DEBUG_OUTPUT_NORMAL, "%s\n", help_msg);
    LEAVE_CALLBACK();
    return S_OK;
}

