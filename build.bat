@echo off
setlocal

echo ==========================
echo  Graphgen -- Build Batch
echo ==========================
echo.

IF NOT EXIST build mkdir build
pushd build

IF ["%LIBPATH%"] == [""] goto :no_vcvars

set EXE_NAME=graphgen_win.exe
set DLL_NAME=graphgen.dll
set FILES= ../../code/graphgen.cpp ../../code/render.cpp ../../code/nfa_parse.cpp
set PLATFILES= ../../code/graphgen_win.cpp 
set CCFLAGS= /MTd /EHsc /O2 /Oi /WX /W4 /wd4201 /wd4505 /FC /Z7 /Fm
set LDFLAGS= /incremental:no /opt:ref
goto :win64

:win64
echo Building win64
IF NOT EXIST win64 mkdir win64
pushd win64
	del *.pdb > NUL 2> NUL
	cl /nologo %CCFLAGS% %FILES% /LD /link /DLL %LDFLAGS% /PDB:%BASE_NAME%%RANDOM%.pdb /EXPORT:UpdateAndRender /out:%DLL_NAME%
	cl /nologo %CCFLAGS% %PLATFILES% /link user32.lib Gdi32.lib winmm.lib %LDFLAGS% /OUT:%EXE_NAME% /subsystem:Windows,5.2 
    IF NOT EXIST data mkdir data
    del /Q data\*
    xcopy /Y /K /Q ..\..\data data\
	if ERRORLEVEL 1 goto :eof
popd
echo.
goto :done

:win32
echo Building win32
IF NOT EXIST win32 mkdir win32
pushd win32
	del *.pdb > NUL 2> NUL
	cl /nologo %CCFLAGS% %FILES% /LD /link /DLL %LDFLAGS% /PDB:%BASE_NAME%%RANDOM%.pdb /EXPORT:UpdateAndRender /out:%DLL_NAME%
	cl /nologo %CCFLAGS% %PLATFILES% /link user32.lib Gdi32.lib winmm.lib %LDFLAGS% /OUT:%EXE_NAME% /subsystem:Windows,5.1 
    IF NOT EXIST data mkdir data
    del /Q data\*
    xcopy /Y /K /Q ..\..\data data\
	if ERRORLEVEL 1 goto :eof
popd
echo.
goto :done

:no_vcvars
echo ERROR: Ensure you've called Visual Studio's vcvarsall.bat before building
echo        This is usually in C:\Program Files ^(x86^)\Visual Studio ^<X^>\VC\
echo.

:done
popd

