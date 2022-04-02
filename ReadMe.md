# VSTGUI Scintilla View

This repository contains an adapter to use the free source code editing component Scintilla in VSTGUI.

To compile this example you need to have a copy of 
- VSTGUI (https://github.com/steinbergmedia/vstgui)
- scintilla (https://www.scintilla.org/index.html)
- lexilla (https://www.scintilla.org/Lexilla.html)

This adapter was tested with:

VSTGUI 4.10
scintilla 5.2.2
lexilla 5.1.6

Use cmake to build a project for macOS or Windows (Linux not supported).
Tell cmake where the 3 dependent projects live on your setup:

cmake -GXcode -DVSTGUI_PATH="../vstgui" -DSCINTILLA_PATH="../scintilla" -DLEXILLA_PATH="../lexilla"

