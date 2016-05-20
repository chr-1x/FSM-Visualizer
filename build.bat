@echo off
setlocal

echo ==========================
echo  Dracogen -- Build Batch
echo ==========================
echo.

IF NOT EXIST build mkdir build
pushd build

set BASE_NAME=dracogen
set EXE_NAME=%BASE_NAME%_win.exe
set DLL_NAME=%BASE_NAME%.dll
set DEFINES= /DINTERNAL=1
set CCFLAGS= /MTd %DEFINES% /Gm- /GR- /EHa- /Od /Oi /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4706 /wd4996 /wd4127 /wd4244 /FC /Z7 /Fm /I%DEV%/lib/chr
set LDFLAGS= /incremental:no /opt:ref 
goto :win64

:win64
echo Building win64
IF NOT EXIST win64 mkdir win64
pushd win64
	del *.pdb > NUL 2> NUL
	cl /nologo %CCFLAGS% ../../code/dracogen.cpp /LD /link /DLL %LDFLAGS% /PDB:%BASE_NAME%%RANDOM%.pdb /EXPORT:GameUpdateAndRender /out:%DLL_NAME%
	cl /nologo %CCFLAGS% ../../code/windows_dracogen.cpp /link user32.lib Gdi32.lib %LDFLAGS% /OUT:%EXE_NAME% /subsystem:Windows,5.2 
	if ERRORLEVEL 1 goto :eof
popd
echo.
goto :done

:win32
echo Building win32
IF NOT EXIST win32 mkdir win32
pushd win32
	del *.pdb > NUL 2> NUL
	cl /nologo %CCFLAGS% ../../code/dracogen.cpp /LD /link /DLL %LDFLAGS% /PDB:%BASE_NAME%%RANDOM%.pdb /EXPORT:GameUpdateAndRender /out:%DLL_NAME%
	cl /nologo %CCFLAGS% ../../code/windows_dracogen.cpp /link user32.lib Gdi32.lib %LDFLAGS% /OUT:%EXE_NAME% /subsystem:Windows,5.1 
	if ERRORLEVEL 1 goto :eof
popd
echo.
goto :done

:done
popd
