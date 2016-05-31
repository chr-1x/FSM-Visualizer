@echo off

:: NOTE: if you edit the path manually, remove this line
if ["%VC%"] == [""] goto :no_vc

call "%VC%\vcvarsall.bat" amd64
devenv build\win64\graphgen_win.exe
goto :eof

:no_vc
echo ERROR: Can't launch devenv.exe, you need to either set %%VC%% variable or edit in the path manually.
echo.
