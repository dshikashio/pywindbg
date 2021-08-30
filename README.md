## Introduction

Pywindbg is a Windbg extension that embeds a Python interpreter into the Windbg console.

Currently does not interact with the debugging session.  Requires dbgeng bindings available separately (see Pybag).


## Requirements
* Visual Studio 2019
* Microsoft Windows 10 SDK
* Python 3.9 x86 and x64


## Usage

Build in Visual Studio.  Tweak props files if necessary.

From windbg
* Adjust extpath (.extpath+ c:\location-of-extenstion\)
* !pywindbg.help

