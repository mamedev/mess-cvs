# only OS specific output files and rules
OSOBJS = \
        $(OBJ)/Win32/osdepend.o \
        $(OBJ)/Win32/dirent.o \
        $(OBJ)/Win32/MAME32.o \
        $(OBJ)/Win32/M32Util.o \
        $(OBJ)/Win32/DirectInput.o \
        $(OBJ)/Win32/DIKeyboard.o \
        $(OBJ)/Win32/DIJoystick.o \
        $(OBJ)/Win32/uclock.o \
        $(OBJ)/Win32/Display.o \
        $(OBJ)/Win32/GDIDisplay.o \
        $(OBJ)/Win32/DDrawWindow.o \
        $(OBJ)/Win32/DDrawDisplay.o \
        $(OBJ)/Win32/DirectDraw.o \
        $(OBJ)/Win32/RenderBitmap.o \
        $(OBJ)/Win32/Dirty.o \
        $(OBJ)/Win32/led.o \
        $(OBJ)/Win32/status.o \
        $(OBJ)/Win32/DirectSound.o \
        $(OBJ)/Win32/NullSound.o \
        $(OBJ)/Win32/Keyboard.o \
        $(OBJ)/Win32/Joystick.o \
        $(OBJ)/Win32/Trak.o \
        $(OBJ)/Win32/file.o \
        $(OBJ)/Win32/Directories.o \
        $(OBJ)/Win32/mzip.o \
        $(OBJ)/Win32/debug.o \
        $(OBJ)/Win32/fmsynth.o \
        $(OBJ)/Win32/NTFMSynth.o \
        $(OBJ)/Win32/audit32.o \
        $(OBJ)/Win32/ColumnEdit.o \
        $(OBJ)/Win32/Screenshot.o \
        $(OBJ)/Win32/TreeView.o \
        $(OBJ)/Win32/Splitters.o \
        $(OBJ)/Win32/Bitmask.o \
        $(OBJ)/Win32/DataMap.o \
        $(OBJ)/Win32/Avi.o \
        $(OBJ)/Win32/RenderMMX.o \
        $(OBJ)/Win32/dxdecode.o

ifdef MESS
RES = $(OBJ)/mess/Win32/mess32.res
else
RES = $(OBJ)/Win32/mame32.res
endif

# additional OS specific object files

ifdef MIDAS
OSOBJS +=  $(OBJ)/Win32/MidasSound.o
endif

ifdef MAME_NET
OSOBJS += \
		$(OBJ)/network.o \
        $(OBJ)/Win32/net32.o \
        $(OBJ)/Win32/netchat32.o
endif

ifdef HELP
HELPFILES += $(OBJ)/Win32/hlp/Mame32.hlp


$(HELPFILES): src/Win32/hlp/Mame32.hpj
        @MakeHelp.bat
endif

# check if this is a MESS build
ifdef MESS
OSOBJS += \
        $(OBJ)/mess/windows/fdc.o \
        $(OBJ)/mess/windows/dirio.o \
        $(OBJ)/mess/Win32/mess32ui.o \
        $(OBJ)/mess/Win32/MessProperties.o \
        $(OBJ)/mess/Win32/MessOptions.o \
        $(OBJ)/mess/Win32/fileio.o \
        $(OBJ)/mess/Win32/SmartListView.o
else
OSOBJS += \
        $(OBJ)/Win32/Win32ui.o \
        $(OBJ)/Win32/Properties.o \
        $(OBJ)/Win32/options.o
endif

