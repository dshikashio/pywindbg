//
// To Do List
//
// Nice to Haves
// - pydbgeng and pywindbg available by default
//
// Might be useful
// - Wrap the "old" apis as python calls

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <structmember.h>
#include "pyembed.h"

typedef struct _IoBridgeObject {
    PyObject_HEAD
    int width;
} IoBridgeObject;

#define CONSOLE_WIDTH 85

static int
find_line_width(char *hay, int maxwidth, char needle)
{
    int i;
    for (i = 0; i < maxwidth; i++) {
        if (hay[i] == needle)
            return i;
    }
    return i;
}

static PyObject *
IoBridge_write(IoBridgeObject *self, PyObject *args)
{
    char *data = NULL;

    if (!PyArg_ParseTuple(args, "s", &data))
        return NULL;

    if (data == NULL)
        return NULL;

    if (gPyDebugControl) {
        Py_BEGIN_ALLOW_THREADS
        char *p = data;
        // Windbg is 'dumb' and doesn't auto wrap lines so control 
        // that with width. But sometimes there is a newline already...
        if (self->width) {
            char tmp;
            while (lstrlenA(p) > self->width) {
                int width = find_line_width(p, self->width, '\n');
                if (width == 0)
                    width = 1;

                tmp = p[width];
                p[width] = '\0';
                gPyDebugControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "%s", p);
                p += width;
                *p = tmp;

                if (width >= self->width)
                    gPyDebugControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "\n");
            }
        }

        // Most of the time we don't need a newline here, but sometimes we do
        gPyDebugControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "%s", p);
        if (lstrcmpA(">>> ", p) != 0 && p[lstrlenA(p)-1] != '\n')
            gPyDebugControl->Output(DEBUG_OUTCTL_ALL_CLIENTS, "\n");
        Py_END_ALLOW_THREADS
    }
    Py_RETURN_NONE;
}

static PyObject *
IoBridge_readline(IoBridgeObject *self, PyObject *args)
{
    HRESULT hr;
    ULONG bufsize;
    PSTR buf = NULL;
    PyObject *ret = NULL;

#define BUF_SIZE 0x4000
    buf = (PSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, BUF_SIZE);
    if (buf != NULL) {
        if (gPyDebugControl) {
            Py_BEGIN_ALLOW_THREADS
            hr = gPyDebugControl->Input(buf, BUF_SIZE-1, &bufsize);
            Py_END_ALLOW_THREADS
            if (hr == S_OK) {
                if (lstrcmpiA(buf, "quit()") == 0 || 
                    lstrcmpiA(buf, "exit()") == 0)
                {
                    // EOF
                    ret = Py_BuildValue("s", "");
                }
                else if (lstrlenA(buf) == 0)
                {
                    // Empty line
                    ret = Py_BuildValue("s", "\n");
                }
                else
                {
                    // Normal case
                    ret = Py_BuildValue("s", buf);
                }
                goto done;
            }
        }
    }
    // Something went wrong, return EOF
    ret = Py_BuildValue("s", "");
done:
    if (buf) HeapFree(GetProcessHeap(), 0, buf);
    return ret;
}

static PyMethodDef IoBridge_methods[] = {
    {"write", (PyCFunction)IoBridge_write, METH_VARARGS,
     "Write to extensions output"
    },
    {"readline", (PyCFunction)IoBridge_readline, METH_VARARGS,
    "Read extension input"
    },
    {NULL}
};

static PyMemberDef IoBridge_members[] = {
    {"width", T_INT, offsetof(IoBridgeObject, width), 
     0, "Output width"},
    {NULL}
};

static PyTypeObject IoBridgeType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "extio.IoBridge",          /*tp_name*/
    sizeof(IoBridgeObject),    /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_reserved*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "IoBridge objects",        /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    IoBridge_methods,          /* tp_methods */
    IoBridge_members,          /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    0,                         /* tp_new */
};

static PyObject *
ExtIo_ExitNop(PyObject *ignore, PyObject *arg)
{
    Py_RETURN_NONE;
}

static PyMethodDef  ExtIoMethods[] = {
    {"exit_nop", (PyCFunction)ExtIo_ExitNop, METH_VARARGS, 
    "Type exit() or quit() to return back to Windbg."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef extiomodule =
{
    PyModuleDef_HEAD_INIT,
    "extio",
    "extio module",
    -1,
    ExtIoMethods,
    NULL, NULL, NULL, NULL
};

//HRESULT
PyMODINIT_FUNC PyInit_extio(void)
{
    PyObject *m;

    IoBridgeType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&IoBridgeType) < 0)
        return NULL;

    m = PyModule_Create(&extiomodule);
    if (m == NULL) {
        return NULL;
    }

    Py_INCREF(&IoBridgeType);
    if (PyModule_AddObject(m, "IoBridge", (PyObject*)&IoBridgeType) != 0) {
        Py_DECREF(&IoBridgeType);
        Py_DECREF(m);
        return NULL;
    }

    return m;
}

HRESULT python_init(void)
{
    HRESULT hr;
    PyObject *module = NULL;
    PyObject *IoBridge = NULL;
    IoBridgeObject *OBInst = NULL;
    PyObject *ExitNop = NULL;
    PyObject *BuiltIns = NULL;

    if (PyImport_AppendInittab("extio", PyInit_extio) == -1)
        return E_FAIL;

	Py_Initialize();
    //PyEval_InitThreads();

    hr = E_FAIL;

    if (PyRun_SimpleString("import sys") != 0)
        return E_FAIL;

    module = PyImport_ImportModule("extio");
    if (module == NULL)
        goto fail;

    ExitNop = PyObject_GetAttrString(module, "exit_nop");
    if (ExitNop == NULL)
        goto fail;

    IoBridge = PyObject_GetAttrString(module, "IoBridge");
    if (IoBridge == NULL)
        goto fail;

    OBInst = (IoBridgeObject *)PyObject_CallObject((PyObject *)&IoBridgeType, NULL);
    if (OBInst == NULL)
        goto fail;

    OBInst->width = CONSOLE_WIDTH;

    if (PySys_SetObject("stdout", (PyObject *)OBInst) != 0)
        goto fail;
    if (PySys_SetObject("stderr", (PyObject *)OBInst) != 0)
        goto fail;
    if (PySys_SetObject("stdin", (PyObject *)OBInst) != 0)
        goto fail;
    if (PySys_SetObject("exit", ExitNop) != 0)
        goto fail;

    // Replace __builtins__.exit and __builtins__.quit
    // If for some reason we fail, then ignore and go on
    BuiltIns = PyEval_GetBuiltins();
    if (BuiltIns != NULL) {
        PyDict_SetItemString(BuiltIns, "exit", ExitNop);
        PyDict_SetItemString(BuiltIns, "quit", ExitNop);
    }

    hr = S_OK;

fail:
    Py_XDECREF(OBInst);
    Py_XDECREF(IoBridge);
    Py_XDECREF(ExitNop);
    Py_XDECREF(module);
    return hr;
}

void python_fini(void)
{
    Py_Finalize();
}

HRESULT CALLBACK
pyeval(IN IDebugClient *Client, IN OPTIONAL PCSTR args)
{
    PSTR cmd = NULL;
    size_t cmdlen = 0;

    ENTER_CALLBACK(Client);

    cmdlen = lstrlenA(args + 2);
    cmd = (PSTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, cmdlen);
    if (cmd == NULL)
        goto done;

    lstrcpyA(cmd, args);
    lstrcatA(cmd, "\n");

    if (PyRun_SimpleString(cmd) == -1) {
        PyErr_Print();
        goto done;
    }

done:
    gPyDebugControl->Output(DEBUG_OUTPUT_NORMAL, "\n");
    if (cmd) HeapFree(GetProcessHeap(), 0, cmd);
    LEAVE_CALLBACK();
    return S_OK;
}

HRESULT CALLBACK
pyexec(IN IDebugClient *Client, IN OPTIONAL PCSTR args)
{
    FILE* file = NULL;
    PyObject *m;
    PyObject *d;
    PyObject* v = NULL;

    ENTER_CALLBACK(Client);
    Client->SetOutputWidth(100);

    file = fopen(args, "rb");
    if (file == NULL) {
        gDebugControl->Output(DEBUG_OUTPUT_ERROR, "Error opening '%s'", args);
        goto done;
    }

    if ((m = PyImport_AddModule("__main__")) == NULL)
        goto done;

    d = PyModule_GetDict(m);

    if ((v = PyRun_File(file, args, Py_file_input, d, d)) == NULL) {
        PyErr_Print();
        goto done;
    }

done:
    if (file != NULL)
    {
        fclose(file);
    }
    Py_XDECREF(v);
    // Do we need to decref d or m?
    LEAVE_CALLBACK();
    return S_OK;
}


HRESULT CALLBACK
python(IN IDebugClient *Client, IN OPTIONAL PCSTR args)
{
    PyObject *m;
    PyObject *d;
    PyObject *v1 = NULL;
    PyObject* v2 = NULL;
    ULONG Mask;

    const char *cmds[] = {
        "import code",

        "code.interact(banner=\""
        "\\nEntering Python Interpreter\\n"
        "type 'quit()' to quit\\n"
        "\", local=globals())"
    };

    ENTER_CALLBACK(Client);

    Client->GetOutputMask(&Mask);
    Mask &= ~DEBUG_OUTPUT_PROMPT;
    Client->SetOutputMask(Mask);

    if ((m = PyImport_AddModule("__main__")) == NULL)
        goto done;

    d = PyModule_GetDict(m);
    if ((v1 = PyRun_StringFlags(cmds[0], Py_single_input, d, d, NULL)) == NULL) {
        PyErr_Print();
        goto done;
    }
    if ((v2 = PyRun_StringFlags(cmds[1], Py_single_input, d, d, NULL)) == NULL) {
        PyErr_Print();
        goto done;
    }

done:
    Mask |= DEBUG_OUTPUT_PROMPT;
    Client->SetOutputMask(Mask);

    gDebugControl->Output(DEBUG_OUTPUT_NORMAL, "Leaving Python\n");
    Py_XDECREF(v1);
    Py_XDECREF(v2);
    LEAVE_CALLBACK();
    return S_OK;
}
